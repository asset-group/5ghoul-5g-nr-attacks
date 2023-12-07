#include "WDMapper.h"

using namespace std;
using namespace misc_utils;

typedef struct
{
    uint8_t *buf;
    unsigned int buf_len;
} graphviz_output_t;

uint16_t opt_udp_dst_port = 0;

Machine DiffMachine;

void ConfigurePseudoHeader(Machine &SM)
{
    string default_pcap_protocol = SM.config.options.default_protocol_encap_name;
    string default_protocol = SM.config.options.default_protocol_name;

    string_to_lowercase(default_pcap_protocol);
    string_to_lowercase(default_protocol);

    // Process pseudoheader when loading pcap files (set direction)
    if (string_contains(default_pcap_protocol, "_h4") || string_contains(default_protocol, "_h4")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            if (pkt_len >= 4) {
                // Set packet direction
                wd_set_packet_direction(SM.wd, (int)pkt[3]);
                // Return pseudoheader offset
                offset = 4;
            }
        });
    }
    else if (string_contains(default_pcap_protocol, "sysdig")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) { offset = 0; });
    }
    else if (string_contains(default_protocol, "mac-lte-framed")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            wd_set_packet_direction(SM.wd, !(int)pkt[49 + 1]);
        });
    }
    else if (string_contains(default_protocol, "mac-nr-framed")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            wd_set_packet_direction(SM.wd, !(int)pkt[48 + 1]);
        });
    }
    else if (string_contains(default_pcap_protocol, "ieee802_11_radiotap")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            wd_set_packet_direction(SM.wd, (int)pkt[8]);
            offset = 9;
        });
    }
    else if (string_contains(default_pcap_protocol, "usb_linux_mmapped") || string_contains(default_pcap_protocol, "220")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            wd_set_packet_direction(SM.wd, (int)((pkt[10] & 0x80) > 0));
        });
    }
    else if (string_contains(default_pcap_protocol, "nordic_ble") || string_contains(default_pcap_protocol, "272")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            wd_set_packet_direction(SM.wd, (int)(!((pkt[8] & 0x02) > 0)));
        });
    }
    else if (string_contains(default_pcap_protocol, "sll_v1")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            wd_set_packet_direction(SM.wd, (int)(!(pkt[1] & 0x04)));
        });
    }
    // TODO: detect named encap and protocol name
    else if (string_contains(default_pcap_protocol, "encap:1")) {
        SM.OnProcessPseudoHeader([&SM](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
            if (opt_udp_dst_port && pkt_len > 37) {
                uint16_t udp_dst_port = htons(*((uint16_t *)&pkt[36]));
                wd_set_packet_direction(SM.wd, (udp_dst_port == opt_udp_dst_port ? WD_DIR_TX : WD_DIR_RX));
            }
        });
    }
}

graphviz_output_t graphviz_dot_to_format(const char *dot_graph, const char *output_format)
{
    static GVC_t *gvc = gvContext();
    static std::mutex dot_mutex;
    graphviz_output_t output;
    lock_guard<mutex> m(dot_mutex);
    Agraph_t *g;
    char *buf;
    unsigned int buf_len;

    output.buf = NULL;
    output.buf_len = 0;

    g = agmemread(dot_graph);

    if (g == NULL) {
        return output;
    }

    gvLayout(gvc, g, "dot");
    // agset(g, "splines", "line");
    gvRenderData(gvc, g, output_format, &buf, &buf_len);
    gvFreeLayout(gvc, g);
    agclose(g);

    if (buf) {
        output.buf = (uint8_t *)buf;
        output.buf_len = buf_len;
    }

    return output;
}

void graphviz_free_data(graphviz_output_t &graphviz_output)
{
    if (graphviz_output.buf) {
        gvFreeRenderData((char *)graphviz_output.buf);
        graphviz_output.buf_len = 0;
    }
}

int main(int argc, char **argv)
{
    bool file_saved = false;
    bool model_loaded = false;

    // clang-format off
    cxxopts::Options options("WDMapper", "Creates a State Machine from wireshark protocol fields especification");
    options.add_options()
    ("help", "Print help")
    ("c,config", "Configuration File", cxxopts::value<string>()->default_value("configs/generic_config.json"))
    ("i,input", "Input File (*.json,*.pcap,*.pcapng)", cxxopts::value<vector<std::string>>())
    ("d,diff", "Diff Input File (*.json,*.pcap,*.pcapng)", cxxopts::value<std::string>())
    ("o,output", "Output File (*.dot,.svg,*.png,*.json)", cxxopts::value<string>()->default_value("wdmapper.svg"))
    ("fast", "Generate low quality graph (faster)")
    ("ignore_tx", "Ignores TX Mapping")
    ("v,verbose", "Enable debug mode (verbose output)")
    ("udp-dst-port", "Tell which UDP port to set as TX", cxxopts::value<std::uint16_t>());
    // clang-format on

    try {
        auto result = options.parse(argc, argv);

        if (!result.arguments().size() || result.count("help")) {
            cout << options.help({"", "Group"}) << endl;
            exit(0);
        }

        if (!result.count("input")) {
            LOGR("Please provide configuration file \"bin/wdmapper -c <config file> ...\"");
            exit(1);
        }

        if (result.count("udp-dst-port"))
            opt_udp_dst_port = result["udp-dst-port"].as<uint16_t>();

        string config_file_path = result["config"].as<string>();
        vector<string> model_paths = result["input"].as<vector<string>>();
        int models_number = model_paths.size();

        // Set wireshark log level
        if (!result.count("verbose"))
            wdissector_set_log_level(WD_LOG_LEVEL_CRITICAL);
        else
            wdissector_set_log_level(WD_LOG_LEVEL_DEBUG);

        // Initialize Generic Model Configuration
        if (!StateMachine.init(config_file_path.c_str())) {
            LOGR("Configuration file could not be open.");
            exit(1);
        }

        wd_set_dissection_mode(StateMachine.wd, WD_MODE_FAST);

        // Configure dissection depending on proto type
        ConfigurePseudoHeader(StateMachine);

        // Load input files
        if (models_number == 1)
            LOGY("Loading Model...");
        else
            LOGY("Merging Models...");

        for (size_t i = 0; i < models_number; i++) {
            model_loaded = StateMachine.LoadModel(model_paths[i].c_str(),
                                                  false,
                                                  (models_number > 1) || (result.count("diff") > 1),
                                                  result["ignore_tx"].as<bool>());

            if (!model_loaded) {
                LOGR("Error loading " + model_paths[i]);
                exit(1);
            }

            LOGG("Loaded " + model_paths[i]);
            // Print total states and transitions per layer
            for (auto &layer : StateMachine.GetStateMap()) {
                LOG3Y("Layer:\"", layer.layer_name, "\"");
                LOG4C("--> States:", layer.total_mapped_states, ", Transitions:", layer.total_mapped_transitions);
            }
            LOG2Y("Total States: ", StateMachine.TotalStates());
            LOG2Y("Total Transitions: ", StateMachine.TotalTransitions());
            LOG1("------------------------------------------------------");
        }

        if (result.count("diff")) {
            // Apply diff to state machine
            string diff_path = result["diff"].as<string>();

            LOG2G("Diffing with ", diff_path);
            // Initialize Diff Model Configuration
            if (!DiffMachine.init(config_file_path.c_str())) {
                LOGR("Configuration file could not be open.");
                exit(1);
            }

            // Configure dissection depending on proto type
            ConfigurePseudoHeader(DiffMachine);

            GL1Y("Loading Diff Model...");
            model_loaded = DiffMachine.LoadModel(diff_path.c_str(),
                                                 false,
                                                 false,
                                                 result["ignore_tx"].as<bool>());

            if (!model_loaded) {
                LOGR("Error loading " + diff_path);
                exit(1);
            }

            LOGG("Loaded " + diff_path);
            // Print total states and transitions per layer
            for (auto &layer : DiffMachine.GetStateMap()) {
                LOG3Y("Layer:\"", layer.layer_name, "\"");
                LOG4C("--> States:", layer.total_mapped_states, ", Transitions:", layer.total_mapped_transitions);
            }
            LOG2Y("Total States: ", DiffMachine.TotalStates());
            LOG2Y("Total Transitions: ", DiffMachine.TotalTransitions());

            // Discover new nodes in the diff model
            vector<string> AddedNodes;
            vector<string> RemovedNodes;
            auto &RefStates = StateMachine.GetStates();
            auto &DiffStates = DiffMachine.GetStates();
            // Search for new nodes
            for (auto &DiffNode : DiffStates) {
                if (!RefStates.contains(DiffNode.first))
                    AddedNodes.emplace_back(DiffNode.first);
            }
            // Search for removed nodes in the diff model
            for (auto &RefNode : RefStates) {
                if (!DiffStates.contains(RefNode.first))
                    RemovedNodes.emplace_back(RefNode.first);
            }

            // Sort nodes
            sort(AddedNodes.begin(), AddedNodes.end());
            sort(RemovedNodes.begin(), RemovedNodes.end());

            LOG2Y("New Nodes:", AddedNodes.size());
            LOG2Y("Removed Nodes:", RemovedNodes.size());

            for (auto &NodeName : AddedNodes)
                LOG2G("(+) ", NodeName);

            for (auto &NodeName : RemovedNodes)
                LOG2R("(-) ", NodeName);

            LOG3Y("Merging ref model with \"", diff_path, "\"...");
            // TODO: Use json models for merging for now. Merging pcap directly may miss some states
            string tmp_model_path = diff_path + ".json";
            DiffMachine.SaveModel(tmp_model_path.c_str());
            // Merge with original model
            StateMachine.LoadModel(tmp_model_path.c_str(),
                                   false,
                                   true,
                                   result["ignore_tx"].as<bool>());
            unlink(tmp_model_path.c_str());

            // Color ADDED nodes with the merged model
            auto &MergedNodes = StateMachine.GetStates();
            auto &RefTransitions = StateMachine.GetTransitions();
            for (auto &AddedNode : AddedNodes) {
                // Get node state ref
                if (!MergedNodes.contains(AddedNode)) {
                    LOG3R("State \"", AddedNode, "\" skipped");
                    continue;
                }
                auto &node_state = MergedNodes[AddedNode];
                // Apply bold style
                node_state.node->set("label", format("<<B>(+) {}</B>>", AddedNode));
                // Set font color to darkgreen
                node_state.node->set("fontcolor", "darkgreen");
                // Set background to transparent green
                node_state.node->set("style", "filled");
                node_state.node->set("fillcolor", "\"#5BAE3240\"");

                // Color to/from edges with merged model
                auto &node_transition_table = RefTransitions[&node_state];
                for (auto &to_state : node_transition_table.to_state) {
                    to_state.second.edge->set("color", "darkgreen");
                    to_state.second.edge->set("penwidth", "2");
                }
                for (auto &from_state : node_transition_table.from_state) {
                    from_state.second->edge->set("color", "darkgreen");
                    from_state.second->edge->set("penwidth", "2");
                }
            }

            // Color REMOVED nodes with the merged model
            for (auto &RemovedNode : RemovedNodes) {
                // Get node state ref
                auto &node_state = MergedNodes[RemovedNode];
                // Apply bold style
                node_state.node->set("label", format("<<B>(-) {}</B>>", RemovedNode));
                // Set font color to darkgreen
                node_state.node->set("fontcolor", "firebrick");
                // Set background to transparent green
                node_state.node->set("style", "filled");
                node_state.node->set("fillcolor", "\"#E60A0A40\"");

                // Color to/from edges with merged model
                auto &node_transition_table = RefTransitions[&node_state];
                for (auto &to_state : node_transition_table.to_state) {
                    to_state.second.edge->set("color", "firebrick");
                    to_state.second.edge->set("penwidth", "2");
                }
                for (auto &from_state : node_transition_table.from_state) {
                    from_state.second->edge->set("color", "firebrick");
                    from_state.second->edge->set("penwidth", "2");
                }
            }

            // Save diff json file
            string o_file = result["output"].as<string>();
            string d_name = string_split(string_split(o_file, "/").back(), ".").front() + ".diff.json";
            std::ofstream diff_json_file(d_name, std::ios::trunc);

            if (!diff_json_file.good()) {
                LOG2R("Error saving ", d_name);
                exit(1);
            }

            json diff_json;
            diff_json["Added"] = AddedNodes;
            diff_json["Removed"] = RemovedNodes;
            diff_json_file << diff_json.dump(4);
            diff_json_file.close();
            LOG3G("Done! Saved json diff file to \"", d_name, "\"");

            LOG1("------------------------------------------------------");
        }

        // Save State Machine graph (*.png,*.dot,*.svg)
        string original_output_file = result["output"].as<string>();
        string output_file = original_output_file;
        string file_extension = string_file_extension(output_file);
        int save_format;

        if (file_extension == "dot") {
            // dot/json
            auto o_list = string_split(output_file, ".");
            output_file = o_list[o_list.size() - 2] + ".json";
            save_format = 0;
        }
        else if (file_extension == "svg") {
            // svg
            save_format = 1;
        }
        else if (file_extension == "png") {
            // png
            save_format = 2;
        }
        else if (file_extension == "json") {
            // json
            save_format = 0;
        }
        else if (file_extension == "") {
            // dot/json
            output_file += ".json";
            original_output_file = output_file;
            save_format = 0;
        }
        else {
            LOGR("Save format not recognized. Valid extensions are *.dot,*.svg,*.png,*.json");
        }

        // Run save format
        LOGY("Saving file...");

        if (result["fast"].as<bool>()) {
            // Set Faster Rendering for graphviz
            // StateMachine.MachineGraph.set(gvpp::AttrType::GRAPH, "splines", "line");
            StateMachine.MachineGraph.set(gvpp::AttrType::GRAPH, "maxiter", "1");
            StateMachine.MachineGraph.set(gvpp::AttrType::GRAPH, "mclimit", "0.01");
        }

        // Force showing all states
        StateMachine.config.state_mapper.show_all_states = true;

        switch (save_format) {
        case 0:
            file_saved = StateMachine.SaveModel(output_file.c_str());
            break;
        case 1:
        case 2:
            graphviz_output_t graphviz_data = graphviz_dot_to_format(StateMachine.get_graph().c_str(), file_extension.c_str());
            if (!graphviz_data.buf || !graphviz_data.buf_len)
                break;

            ofstream of(output_file, ios::out | ios::binary | std::ios::trunc);
            if (!of.good())
                break;

            of.write((char *)graphviz_data.buf, graphviz_data.buf_len);
            of.close();
            graphviz_free_data(graphviz_data);
            file_saved = true;
            break;
        }

        // Save Success
        if (file_saved) {
            if (result.count("diff"))
                LOGG("Saved diffed \"" + original_output_file + "\"");
            else
                LOGG("Saved \"" + original_output_file + "\"");
        }
        else
            LOGR("Error saving " + output_file);
    }
    catch (const cxxopts::exceptions::exception &e) {
        cout << "error parsing options: " << e.what() << endl;
        exit(1);
    }
}