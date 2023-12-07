#ifndef __PCAPDRIVER__
#define __PCAPDRIVER__

#include "Machine.hpp"
#include "MiscUtils.hpp"
#include "libs/zmq.hpp"
#include "libs/shared_memory.h"
#include <PcapFileDevice.h>
#include <RawPacket.h>
#include <map>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define W_GNB_PHY_MIB (200)
#define W_GNB_MAC_UE_DL_SIB (201)
#define W_GNB_PHY_INITIATE_RA_PROCEDURE (202)
#define W_GNB_MAC_UE_DL_RAR_PDU_WITH_DATA (203)
#define W_GNB_MAC_UE_DL_PDU_WITH_DATA (204)
#define W_GNB_MAC_UE_UL_PDU_WITH_DATA (205)
#define W_UE_UL_PDU_WITH_DATA (210)
#define W_GNB_PDCP_PLAIN_DL (206)
#define W_GNB_PDCP_ENC_DL (207)
#define W_GNB_PDCP_PLAIN_UL (208)
#define W_GNB_PDCP_ENC_UL (209) // Not used

using namespace nlohmann;
using namespace std;
using namespace std::chrono_literals;
using namespace pcpp;

class PcapDriver {
private:
    const char *TAG = "[PcapDriver] ";
    RawPacket _rawPacket;
    PcapFileReaderDevice *pcapReader = nullptr;
    wd_filter_t _filter_downlink;
    wd_filter_t _filter_uplink;
    wd_filter_t _filter_downlink_rar;
    uint32_t _pdu_size;
    uint8_t _pdu_type;
    uint64_t _packet_count;
    uint64_t _packet_count_fuzzed;

public:
    map<string, int> _map_layers_frequency;
    map<string, int> _map_fuzzed_layers_frequency;
    map<string, double> _map_fuzzed_layers_weight;
    uint32_t _layers_weigth_pkt_threshold;

    void init(const char *pcap_file_path = NULL)
    {
        // Create a pcap file reader
        if (pcapReader) {
            delete pcapReader;
            pcapReader = nullptr;
        }
        pcapReader = new PcapFileReaderDevice(pcap_file_path);
        pcapReader->open();

        _filter_downlink = wd_filter("mac-nr.direction == 1");
        _filter_uplink = wd_filter("mac-nr.direction == 0");
        _filter_downlink_rar = wd_filter("nr-rrc.rrcSetup_element");
        _packet_count = 0;

        StateMachine.config.fuzzing.enable_mutation = false;
        _layers_weigth_pkt_threshold = 12759;
    }

    uint16_t pdu_size()
    {
        return _pdu_size;
    }

    uint8_t pdu_type()
    {
        return _pdu_type;
    }

    void send(uint8_t *pdu, uint16_t pdu_size, uint16_t mutex_num)
    {
        // Don't do anything
    }

    void notify()
    {
    }

    uint8_t isValid()
    {
        return 1;
    }

    void PacketWasFuzzed(const char *dissector_str)
    {
        
        string real_dissector_str = dissector_str;

        if (string_contains(dissector_str, "MAC-NR")){
            real_dissector_str = "mac-nr";
        }
        else if (string_contains(dissector_str, "RRC")){
            real_dissector_str = "nr-rrc";
        }
        else if (string_contains(dissector_str, "NAS")){
            real_dissector_str = "nas-5gs";          
        }
        else if (string_contains(dissector_str, "PDCP-NR")){
            real_dissector_str = "pdcp-nr";       
        }
        else if (string_contains(dissector_str, "RLC-NR")){
            real_dissector_str = "rlc-nr";       
        }

        vector<string> pkt_dissectors_list;
        if (real_dissector_str.size() && find(pkt_dissectors_list.begin(), pkt_dissectors_list.end(), real_dissector_str) == pkt_dissectors_list.end()) {
            auto it = _map_fuzzed_layers_frequency.find(real_dissector_str);
            if (it == _map_fuzzed_layers_frequency.end())
                _map_fuzzed_layers_frequency[real_dissector_str] = 1;
            else
                it->second += 1;
            pkt_dissectors_list.push_back(real_dissector_str);
        }

        ++_packet_count_fuzzed;
    }

    double GetLayerWeigth(const char *dissector_str){
        string real_dissector_str;

        if (string_contains(dissector_str, "MAC-NR")){
            real_dissector_str = "mac-nr";
        }
        else if (string_contains(dissector_str, "RRC")){
            real_dissector_str = "nr-rrc";
        }
        else if (string_contains(dissector_str, "NAS")){
            real_dissector_str = "nas-5gs";          
        }
        else if (string_contains(dissector_str, "PDCP-NR")){
            real_dissector_str = "pdcp-nr";       
        }
        else if (string_contains(dissector_str, "RLC-NR")){
            real_dissector_str = "rlc-nr";       
        }

        
        if (_map_fuzzed_layers_weight.contains(real_dissector_str))
        {
            return _map_fuzzed_layers_weight[real_dissector_str];
        }

        return 1.0;
    }

    uint8_t *receive()
    {
        uint8_t return_packet = 0;
        while (pcapReader->getNextPacket(_rawPacket)) {

            _pdu_size = _rawPacket.getRawDataLen() - 48;

            packet_set_filter(_filter_downlink);
            packet_set_filter(_filter_uplink);

            packet_dissect((uint8_t *)_rawPacket.getRawData() + 48, _rawPacket.getRawDataLen() - 48);

            if (packet_read_filter(_filter_downlink_rar))
                _pdu_type = T_ENB_MAC_UE_DL_RAR_PDU_WITH_DATA, return_packet = 1;
            else if (packet_read_filter(_filter_downlink)) {
                _pdu_type = T_ENB_MAC_UE_DL_PDU_WITH_DATA, return_packet = 1;
            }
            else if (packet_read_filter(_filter_uplink)) {
                _pdu_type = T_ENB_MAC_UE_UL_PDU_WITH_DATA, return_packet = 1;
            }

            if (_pdu_type == T_ENB_MAC_UE_DL_RAR_PDU_WITH_DATA ||
                _pdu_type == T_ENB_MAC_UE_DL_PDU_WITH_DATA) {
                ++_packet_count;
                vector<string> pkt_dissectors_list;
                for (size_t i = 1; i < packet_dissectors_count(); i++) {
                    const char *dissector_str = packet_dissector(i);
                    if (dissector_str && find(pkt_dissectors_list.begin(), pkt_dissectors_list.end(), dissector_str) == pkt_dissectors_list.end()) {
                        auto it = _map_layers_frequency.find(dissector_str);
                        if (it == _map_layers_frequency.end())
                            _map_layers_frequency[dissector_str] = 1;
                        else
                            it->second += 1;
                        pkt_dissectors_list.push_back(dissector_str);
                    }
                }

                if (_packet_count > _layers_weigth_pkt_threshold &&
                    (!StateMachine.config.fuzzing.enable_mutation)) {
                    StateMachine.config.fuzzing.enable_mutation = true;

                    double layers_count = 0;
                    for (const auto &[key, value] : _map_layers_frequency)
                        layers_count += (double)value;

                    for (const auto &[key, value] : _map_layers_frequency)
                        _map_fuzzed_layers_weight[key] = (1.0 - ((double)value / layers_count));

                    double min_layer_prob = 0.0;
                    double max_layer_prob = 1.0;
                    uint32_t idx = 0;
                    for (const auto &[key, value] : _map_fuzzed_layers_weight) {
                        if (value > min_layer_prob)
                            min_layer_prob = value;
                        if (value < max_layer_prob)
                            max_layer_prob = value;
                        ++idx;
                    }

                    for (const auto &[key, value] : _map_fuzzed_layers_weight){
                        if (value == max_layer_prob)
                        _map_fuzzed_layers_weight[key] = (value/min_layer_prob)/2.0; // 2.0 to adjust automatically later
                        else
                        _map_fuzzed_layers_weight[key] = value/min_layer_prob;
                    }
                }
            }

            if (return_packet) {

                return (uint8_t *)_rawPacket.getRawData() + 48;
            }
        }

        cout << "Number of packets: " << _packet_count << endl;

        cout << "All Packets layers:" << endl;
        for (const auto &[key, value] : _map_layers_frequency)
            std::cout << '[' << key << "] = " << value << "; " << endl;

        cout << endl
             << "Fuzzed Packets: " << _packet_count_fuzzed << endl;
        for (const auto &[key, value] : _map_fuzzed_layers_frequency)
            std::cout << '[' << key << "] = " << value << "; " << endl;

        cout << endl
             << "Weights: " << endl;
        for (const auto &[key, value] : _map_fuzzed_layers_weight)
            std::cout << '[' << key << "] = " << value << "; " << endl;

        exit(0);
    }
};

#endif