#pragma once

#ifndef __WDPARSEARGS__
#define __WDPARSEARGS__

#include <tuple>
#include <unordered_map>

#include "Machine.hpp"
#include "MiscUtils.hpp"
#include "libs/cxxopts/cxxopts.hpp"

class WDParseArgs {
private:
    unique_ptr<cxxopts::Options> options = nullptr;
    std::unordered_map<std::string, pair<uint8_t, ordered_json *>> args_json_refs;
    ordered_json ref_global_config;
    std::unordered_map<std::string, std::function<void(std::string &, json &, json &)>> ArgsCallbacks;
    std::vector<tuple<std::function<void(std::string &, json &, json &)> &, string, json, json>> ArgsCallbacksToCall;

    unordered_map<string, shared_ptr<cxxopts::OptionAdder>> ExtraArgsParams;
    std::unordered_map<std::string, pair<std::function<void(std::string &, json &)>, json::value_t>> ExtraArgsCallbacks;
    std::vector<tuple<std::function<void(std::string &, json &)> &, string, json>> ExtraArgsCallbacksToCall;

    string app_name = "";
    string app_description = "";

    cxxopts::OptionAdder *add_params(cxxopts::OptionAdder *param, ordered_json &j, vector<string> &blocked_params)
    {
        for (auto &[key, value] : j.items()) {

            if (std::find(blocked_params.begin(), blocked_params.end(), key) != blocked_params.end())
                continue;

            switch (value.type()) {
            case json::value_t::string:
                args_json_refs[key] = make_pair((uint8_t)value.type(), &value);
                param = &(*param)(key, "", cxxopts::value<string>()->default_value(value));
                break;

            case json::value_t::boolean:
                args_json_refs[key] = make_pair((uint8_t)value.type(), &value);
                param = &(*param)(key, "", cxxopts::value<bool>()->default_value(to_string(value.get<bool>())));
                break;

            case json::value_t::number_integer:
            case json::value_t::number_unsigned:
                args_json_refs[key] = make_pair((uint8_t)value.type(), &value);
                param = &(*param)(key, "", cxxopts::value<int64_t>()->default_value(to_string(value.get<int64_t>())));
                break;

            case json::value_t::number_float:
                args_json_refs[key] = make_pair((uint8_t)value.type(), &value);
                param = &(*param)(key, "", cxxopts::value<double>()->default_value(to_string(value.get<double>())));
                break;

            default:
                break;
            }
        }

        return param;
    }

    void add_groups(cxxopts::Options &options, vector<string> &groups_name)
    {
        vector<string> v;
        add_groups(options, groups_name, v);
    }

    bool add_groups(cxxopts::Options &options, vector<string> &group_path, vector<string> &blocked_params)
    {
        bool ret = true;
        for (auto &g : group_path) {
            if (g.size()) {
                ret &= add_group(options, g, blocked_params);
            }
        }

        return ret;
    }

    bool add_group(cxxopts::Options &options, string group_path, vector<string> &blocked_params)
    {
        // ignore empty group
        if (!group_path.size())
            return true;

        group_path = "/config/" + group_path;

        if (!ref_global_config.contains(json::json_pointer(group_path)))
            return false;

        ordered_json &j = ref_global_config[json::json_pointer(group_path)];

        if (!j.size())
            return false;

        std::vector<std::string> group_nodes = std::move(string_split(group_path, "/"));

        if (!group_nodes.size())
            return false;

        cxxopts::OptionAdder param = options.add_options(group_nodes.back());
        add_params(&param, j, blocked_params);

        return true;
    }

    bool CreateOptions()
    {
        if (options)
            return true;

        options = make_unique<cxxopts::Options>(cxxopts::Options(app_name, app_description));

        if (!options)
            return false;

        // clang-format off
        options->add_options()
        ("h,help", "Print help")
        ("default-config", "Start with default config", cxxopts::value<bool>())
        ("g,gui", "Open Graphical User Interface (GUI)", cxxopts::value<bool>())
        ;
        // clang-format on

        // Copy reference global json config
        ref_global_config = StateMachine.ref_global_config;

        return true;
    }

    void UpdateRefValue(ordered_json &dst_val, const cxxopts::KeyValue &src_val, json::value_t val_type)
    {
        switch (val_type) {
        case json::value_t::string:
            dst_val = src_val.as<string>();
            break;

        case json::value_t::boolean:
            dst_val = src_val.as<bool>();
            break;

        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
            dst_val = src_val.as<int64_t>();
            break;

        case json::value_t::number_float:
            dst_val = src_val.as<double>();
            break;

        default:
            break;
        }
    }

public:
    const char *TAG = "[ParseArgs] ";
    cxxopts::ParseResult args;
    int args_count;

    void SetProgramDescription(string app_name_, string app_description_)
    {
        app_name = app_name_;
        app_description = app_description_;
        CreateOptions();
    }

    void PrintHelp()
    {
        if (!options)
            return;

        LOG1(options->help());
    }

    bool Parse(int argc, char **argv, vector<string> extra_groups = {}, bool run_args_callback = false)
    {
        if (!options)
            return false;

        std::vector<std::string> groups_path = {"", "Options", "Fuzzing"};
        std::vector<std::string> block_params = {"Enable", "Enabled"};

        for (auto &n : extra_groups) {
            groups_path.emplace_back(n);
        }

        bool ret = add_groups(*options, groups_path, block_params);

        // Parse arguments
        args = options->parse(argc, argv);

        // Check for args that exit the program
        if (args.count("help")) {
            PrintHelp();
            exit(0);
        }

        // Check arguments provided by the user and update reference global json config
        args_count = 0;
        int vals_changed = 0;
        for (auto &a : args) {
            if (!args.count(a.key()))
                continue;

            ++args_count;

            if (ExtraArgsCallbacks.contains(a.key())) {
                ordered_json val;
                UpdateRefValue(val, a, ExtraArgsCallbacks[a.key()].second);
                ExtraArgsCallbacksToCall.push_back({ExtraArgsCallbacks[a.key()].first, a.key(), val});
                continue;
            }

            if (!args_json_refs.contains(a.key()))
                continue;

            auto &cfg_ref = args_json_refs[a.key()];

            json o_val = *cfg_ref.second;
            UpdateRefValue(*cfg_ref.second, a, (json::value_t)cfg_ref.first);
            json n_val = *cfg_ref.second;

            if (o_val != n_val) {
                ++vals_changed;
                LOG2Y(TAG, fmt::format("Changed \"{}\" from \"{}\" to \"{}\"", a.key(), o_val.dump(), n_val.dump()));
            }
            else
                GL1(TAG, "\"", a.key(), "\" unchanged (", n_val.dump(), ")");

            // Register args callbacks to call later
            if (ArgsCallbacks.contains(a.key()))
                ArgsCallbacksToCall.push_back({ArgsCallbacks[a.key()], a.key(), o_val, n_val});
        }

        // Update global config
        if (vals_changed)
            StateMachine.SetConfig(ref_global_config);

        if (!run_args_callback)
            return ret;

        for (auto &[fcn, arg_name, o_val, n_val] : ArgsCallbacksToCall) {
            fcn(arg_name, o_val, n_val);
        }

        for (auto &[fcn, arg_name, val] : ExtraArgsCallbacksToCall) {
            fcn(arg_name, val);
        }

        return ret;
    }

    void RunArgsCallback()
    {
        for (auto &[fcn, arg_name, o_val, n_val] : ArgsCallbacksToCall) {
            fcn(arg_name, o_val, n_val);
        }

        for (auto &[fcn, arg_name, val] : ExtraArgsCallbacksToCall) {
            fcn(arg_name, val);
        }
    }

    // Add callback to new parameter not in configuration file
    bool AddArgCallback(string arg_path, string arg_description, json::value_t value_type, function<void(string, json &)> fcn)
    {
        if (!options)
            return false;

        string arg_name = arg_path;
        string arg_full_path = "/config/" + arg_path;

        if (ref_global_config.contains(json::json_pointer(arg_full_path)))
            return false;

        vector<string> arg_nodes = string_split(arg_path, "/");
        arg_name = arg_nodes.back();

        string arg_group = "";
        if (arg_nodes.size() > 1)
            arg_group = arg_nodes[arg_nodes.size() - 2];

        if (!ExtraArgsParams.contains(arg_group)) {
            ExtraArgsParams[arg_group] = make_shared<cxxopts::OptionAdder>(options->add_options(arg_group));
        }

        switch (value_type) {
        case json::value_t::string: {
            auto &param = *ExtraArgsParams[arg_group];
            ExtraArgsParams[arg_group] = make_shared<cxxopts::OptionAdder>(param(arg_name, arg_description, cxxopts::value<std::string>()));
            break;
        }

        case json::value_t::boolean: {
            auto &param = *ExtraArgsParams[arg_group];
            ExtraArgsParams[arg_group] = make_shared<cxxopts::OptionAdder>(param(arg_name, arg_description, cxxopts::value<bool>()));
            break;
        }

        case json::value_t::number_integer:
        case json::value_t::number_unsigned: {
            auto &param = *ExtraArgsParams[arg_group];
            ExtraArgsParams[arg_group] = make_shared<cxxopts::OptionAdder>(param(arg_name, arg_description, cxxopts::value<int64_t>()));
            break;
        }

        case json::value_t::number_float: {
            auto &param = *ExtraArgsParams[arg_group];
            ExtraArgsParams[arg_group] = make_shared<cxxopts::OptionAdder>(param(arg_name, arg_description, cxxopts::value<double>()));
            break;
        }

        default:
            break;
        }

        ExtraArgsCallbacks[arg_name] = make_pair(fcn, value_type);

        return true;
    }

    // Add Callback to existing parameter in configuration file
    bool AddArgCallback(string arg_path, string arg_description, function<void(string, json &, json &)> fcn)
    {
        if (!options)
            return false;

        string arg_name = arg_path;
        string arg_full_path = "/config/" + arg_path;

        if (!ref_global_config.contains(json::json_pointer(arg_full_path)))
            return false;

        ArgsCallbacks[arg_name] = fcn;

        return true;
    }

    template <class T>
    void AddArgs(T &Instance)
    {
        Instance.AddArgs(*this);
    }
};

#endif