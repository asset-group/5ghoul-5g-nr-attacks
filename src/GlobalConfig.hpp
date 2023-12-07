//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     GlobalConfig data = nlohmann::ordered_json::parse(jsonString);

#pragma once

#include <variant>
#include "json.hpp"

#ifndef NLOHMANN_OPT_HELPER
#define NLOHMANN_OPT_HELPER
namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::shared_ptr<T>> {
        static void to_json(ordered_json & j, const std::shared_ptr<T> & opt) {
            if (!opt) j = nullptr; else j = *opt;
        }

        static std::shared_ptr<T> from_json(const ordered_json & j) {
            if (j.is_null()) return std::unique_ptr<T>(); else return std::unique_ptr<T>(new T(j.get<T>()));
        }
    };
}
#endif

namespace quicktype {
    using nlohmann::json;
    using nlohmann::ordered_json;

    inline json get_untyped(const ordered_json & j, const char * property) {
        if (j.find(property) != j.end()) {
            return j.at(property).get<json>();
        }
        return json();
    }

    inline json get_untyped(const ordered_json & j, std::string property) {
        return get_untyped(j, property.data());
    }

    template <typename T>
    inline std::shared_ptr<T> get_optional(const ordered_json & j, const char * property) {
        if (j.find(property) != j.end()) {
            return j.at(property).get<std::shared_ptr<T>>();
        }
        return std::shared_ptr<T>();
    }

    template <typename T>
    inline std::shared_ptr<T> get_optional(const ordered_json & j, std::string property) {
        return get_optional<T>(j, property.data());
    }

    struct Bluetooth {
        int64_t auth_req;
        bool bridge_hci;
        bool disable_role_switch;
        bool enable_bounding;
        int64_t io_cap;
        bool intercept_tx;
        bool lmp_sniffing;
        std::string own_bd_address;
        std::string pin;
        bool rx_bypass;
        bool rx_bypass_on_demand;
        bool randomize_own_bt_address;
        bool save_hci_packets;
        bool serial_auto_discovery;
        int64_t serial_baud_rate;
        bool serial_enable_debug;
        bool serial_enable_debug_hci;
        std::string serial_port;
        bool show_null_poll_packets;
        std::string target_bd_address;
        std::vector<std::string> target_bd_address_list;
    };

    struct Exclude {
        std::string apply_to;
        std::string description;
        std::string filter;
    };

    struct StopCondition {
        int64_t max_iterations;
        int64_t max_time_minutes;
        bool stop_on_max_iterations;
        bool stop_on_max_time_minutes;
    };

    struct Fuzzing {
        bool enable_duplication;
        bool enable_mutation;
        bool enable_optimization;
        double default_duplication_probability;
        double default_mutation_probability;
        double default_mutation_field_probability;
        double field_mutation_backoff_multipler;
        int64_t max_fields_mutation;
        bool normalize_protocol_layers_mutation;
        int64_t max_duplication_time_ms;
        bool packet_retry;
        int64_t packet_retry_timeout_ms;
        bool global_timeout;
        int64_t global_timeout_seconds;
        bool state_loop_detection;
        int64_t state_loop_detection_threshold;
        int64_t random_seed;
        bool seed_increment_every_iteration;
        bool restore_session_on_startup;
        bool save_session_on_exit;
        StopCondition stop_condition;
        int64_t selector;
        int64_t mutator;
        std::vector<std::string> default_mutators;
        std::vector<std::string> default_selectors;
        std::vector<Exclude> excludes;
    };

    struct Adb {
        std::string adb_device;
        std::string adb_filter;
        std::vector<std::string> adb_magic_words;
        std::string adb_program;
        std::string adb_sub_system;
    };

    struct Microphone {
        double microphone_detection_sensitivity;
        int64_t microphone_device_id;
    };

    struct Qcdm {
        bool qcdmadb;
        std::string qcdm_device;
        std::vector<std::string> qcdm_magic_words;
    };

    struct SerialUart {
        int64_t serial_baud_rate;
        std::vector<std::string> serial_magic_words;
        std::string serial_port_name;
    };

    struct Ssh {
        std::string ssh_command;
        bool ssh_enable_pre_commands;
        std::string ssh_host_address;
        std::vector<std::string> ssh_magic_words;
        std::string ssh_password;
        int64_t ssh_port;
        std::vector<std::string> ssh_pre_commands;
        std::string ssh_username;
    };

    struct Monitor {
        bool enable;
        bool print_to_stdout;
        bool wait_external_event;
        std::vector<int64_t> active_monitor_types;
        std::vector<std::string> monitors_type_list;
        SerialUart serial_uart;
        Adb adb;
        Microphone microphone;
        Ssh ssh;
        Qcdm qcdm;
    };

    struct Subscriber {
        std::string imsi;
        std::string k;
        std::shared_ptr<std::string> opc;
        std::string apn;
        std::shared_ptr<std::string> op;
    };

    struct Nr5G {
        std::string mcc;
        std::string mnc;
        bool auto_start_base_station;
        bool auto_start_core_network;
        std::string base_station_config_file;
        std::string base_station_arguments;
        std::string core_network_config_file;
        bool enable_simulator;
        int64_t simulator_delay_us;
        int64_t simulation_connection_timeout_ms;
        std::string simulator_ue_arguments;
        std::vector<Subscriber> subscribers;
    };

    struct Options {
        std::string default_protocol_name;
        std::string default_protocol_encap_name;
        bool save_protocol_capture;
        bool live_protocol_capture;
        bool save_logs_to_file;
        bool save_latency_metrics;
        bool skip_packet_processing;
        int64_t program;
        bool auto_start_program;
        bool auto_restart_program;
        bool launch_program_with_gdb;
        std::vector<std::string> programs_list;
        int64_t main_thread_core;
        bool save_core_dump;
    };

    struct PythonApiServer {
        bool enable;
        std::string listen_address;
        int64_t port;
        std::string api_namespace;
        bool enable_events;
        bool logging;
        int64_t server_module;
        std::vector<std::string> server_modules_list;
    };

    struct ReportModule {
        bool enabled;
        std::string type;
        std::string script;
        std::string credentials_file;
        bool only_errors;
        std::vector<std::string> to;
        std::vector<std::string> magic_words;
    };

    struct ReportsSender {
        bool enable;
        std::vector<ReportModule> report_modules;
    };

    struct TShark {
        bool enable;
        std::vector<std::string> interfaces_name;
        bool enable_display_filter;
        std::string display_filter;
        bool use_tcp_dump_for_capture;
    };

    struct UeModemManager {
        bool enable;
        std::string apn;
        std::string allowed_modes;
        std::string preferred_mode;
        std::string bands;
        bool disable_fuzzing_on_first_connection;
        bool auto_connect_modem;
        int64_t connection_timeout_ms;
        bool use_only_at_connections;
        bool auto_connect_to_apn;
        bool manual_apn_connection;
        int64_t manual_apn_connection_delay_ms;
        bool auto_reconnect_modem;
        bool reset_modem_on_global_timeout;
        int64_t global_timeouts_count;
        std::string default_modem_interface_path;
        bool auto_search_modem_interface_path;
        bool use_in_usbip_host_ssh;
        bool enable_adb;
        std::string adb_device;
        std::string adbpin;
    };

    struct UsbDevice {
        bool enabled;
        std::string name;
        std::string vidpid;
        std::shared_ptr<bool> reset_on_program_startup;
    };

    struct UsbHubControl {
        bool enable;
        std::vector<UsbDevice> usb_devices;
        int64_t global_timeouts_count;
        int64_t toggle_power_delay_ms;
        bool use_in_usbip_host_ssh;
    };

    struct HostSsh {
        bool enable;
        std::string username;
        std::string password;
        int64_t port;
        bool unbind_on_exit;
        std::string upload_folder;
    };

    struct Usbip {
        bool enable;
        std::string server_address;
        int64_t port;
        std::vector<UsbDevice> usb_devices;
        HostSsh host_ssh;
    };

    struct Services {
        PythonApiServer python_api_server;
        ReportsSender reports_sender;
        TShark t_shark;
        UeModemManager ue_modem_manager;
        UsbHubControl usb_hub_control;
        Usbip usbip;
    };

    using StateNameField = std::variant<std::vector<std::string>, std::string>;

    struct Mapping {
        bool append_summary;
        std::string filter;
        std::string layer_name;
        StateNameField state_name_field;
    };

    struct Overrides {
    };

    struct StateMapper {
        bool enable;
        int64_t packet_layer_offset;
        std::string save_folder;
        bool show_all_states;
        int64_t pcap_pseudo_header_direction_offset;
        bool ignore_malformed_packets;
        std::vector<Mapping> mapping;
        Overrides overrides;
    };

    struct CommonRejection {
        std::string description;
        std::string filter;
    };

    struct Validation {
        std::vector<CommonRejection> common_rejections;
        std::string default_fragments_layer;
        std::string default_packet_layer;
        std::string initial_state;
    };

    struct Wifi {
        std::string wifi_interface;
        std::string wifi_ssid;
        std::string wifi_password;
        int64_t wifi_channel;
        int64_t wifi_key_auth;
        std::vector<std::string> wifi_key_auth_list;
        int64_t wifi_rsn_crypto;
        std::vector<std::string> wifi_rsn_crypto_list;
        std::string wifi_username;
        int64_t wifi_eap_method;
        std::vector<std::string> wifi_eap_method_list;
        bool wifi_allow_internet;
        bool wifi_dhcp;
        std::string wifi_dhcp_gateway_address;
        bool wifi802_11_w;
        std::string wifi_country_code;
    };

    struct Config {
        std::string name;
        Options options;
        Bluetooth bluetooth;
        Wifi wifi;
        Nr5G nr5_g;
        Fuzzing fuzzing;
        StateMapper state_mapper;
        Validation validation;
        Monitor monitor;
        Services services;
    };

    struct GlobalConfig {
        Config config;
    };
}

namespace nlohmann {
    void from_json(const ordered_json & j, quicktype::Bluetooth & x);
    void to_json(ordered_json & j, const quicktype::Bluetooth & x);

    void from_json(const ordered_json & j, quicktype::Exclude & x);
    void to_json(ordered_json & j, const quicktype::Exclude & x);

    void from_json(const ordered_json & j, quicktype::StopCondition & x);
    void to_json(ordered_json & j, const quicktype::StopCondition & x);

    void from_json(const ordered_json & j, quicktype::Fuzzing & x);
    void to_json(ordered_json & j, const quicktype::Fuzzing & x);

    void from_json(const ordered_json & j, quicktype::Adb & x);
    void to_json(ordered_json & j, const quicktype::Adb & x);

    void from_json(const ordered_json & j, quicktype::Microphone & x);
    void to_json(ordered_json & j, const quicktype::Microphone & x);

    void from_json(const ordered_json & j, quicktype::Qcdm & x);
    void to_json(ordered_json & j, const quicktype::Qcdm & x);

    void from_json(const ordered_json & j, quicktype::SerialUart & x);
    void to_json(ordered_json & j, const quicktype::SerialUart & x);

    void from_json(const ordered_json & j, quicktype::Ssh & x);
    void to_json(ordered_json & j, const quicktype::Ssh & x);

    void from_json(const ordered_json & j, quicktype::Monitor & x);
    void to_json(ordered_json & j, const quicktype::Monitor & x);

    void from_json(const ordered_json & j, quicktype::Subscriber & x);
    void to_json(ordered_json & j, const quicktype::Subscriber & x);

    void from_json(const ordered_json & j, quicktype::Nr5G & x);
    void to_json(ordered_json & j, const quicktype::Nr5G & x);

    void from_json(const ordered_json & j, quicktype::Options & x);
    void to_json(ordered_json & j, const quicktype::Options & x);

    void from_json(const ordered_json & j, quicktype::PythonApiServer & x);
    void to_json(ordered_json & j, const quicktype::PythonApiServer & x);

    void from_json(const ordered_json & j, quicktype::ReportModule & x);
    void to_json(ordered_json & j, const quicktype::ReportModule & x);

    void from_json(const ordered_json & j, quicktype::ReportsSender & x);
    void to_json(ordered_json & j, const quicktype::ReportsSender & x);

    void from_json(const ordered_json & j, quicktype::TShark & x);
    void to_json(ordered_json & j, const quicktype::TShark & x);

    void from_json(const ordered_json & j, quicktype::UeModemManager & x);
    void to_json(ordered_json & j, const quicktype::UeModemManager & x);

    void from_json(const ordered_json & j, quicktype::UsbDevice & x);
    void to_json(ordered_json & j, const quicktype::UsbDevice & x);

    void from_json(const ordered_json & j, quicktype::UsbHubControl & x);
    void to_json(ordered_json & j, const quicktype::UsbHubControl & x);

    void from_json(const ordered_json & j, quicktype::HostSsh & x);
    void to_json(ordered_json & j, const quicktype::HostSsh & x);

    void from_json(const ordered_json & j, quicktype::Usbip & x);
    void to_json(ordered_json & j, const quicktype::Usbip & x);

    void from_json(const ordered_json & j, quicktype::Services & x);
    void to_json(ordered_json & j, const quicktype::Services & x);

    void from_json(const ordered_json & j, quicktype::Mapping & x);
    void to_json(ordered_json & j, const quicktype::Mapping & x);

    void from_json(const ordered_json & j, quicktype::Overrides & x);
    void to_json(ordered_json & j, const quicktype::Overrides & x);

    void from_json(const ordered_json & j, quicktype::StateMapper & x);
    void to_json(ordered_json & j, const quicktype::StateMapper & x);

    void from_json(const ordered_json & j, quicktype::CommonRejection & x);
    void to_json(ordered_json & j, const quicktype::CommonRejection & x);

    void from_json(const ordered_json & j, quicktype::Validation & x);
    void to_json(ordered_json & j, const quicktype::Validation & x);

    void from_json(const ordered_json & j, quicktype::Wifi & x);
    void to_json(ordered_json & j, const quicktype::Wifi & x);

    void from_json(const ordered_json & j, quicktype::Config & x);
    void to_json(ordered_json & j, const quicktype::Config & x);

    void from_json(const ordered_json & j, quicktype::GlobalConfig & x);
    void to_json(ordered_json & j, const quicktype::GlobalConfig & x);

    void from_json(const ordered_json & j, std::variant<std::vector<std::string>, std::string> & x);
    void to_json(ordered_json & j, const std::variant<std::vector<std::string>, std::string> & x);

    inline void from_json(const ordered_json & j, quicktype::Bluetooth& x) {
        x.auth_req = j.at("AuthReq").get<int64_t>();
        x.bridge_hci = j.at("BridgeHCI").get<bool>();
        x.disable_role_switch = j.at("DisableRoleSwitch").get<bool>();
        x.enable_bounding = j.at("EnableBounding").get<bool>();
        x.io_cap = j.at("IOCap").get<int64_t>();
        x.intercept_tx = j.at("InterceptTX").get<bool>();
        x.lmp_sniffing = j.at("LMPSniffing").get<bool>();
        x.own_bd_address = j.at("OwnBDAddress").get<std::string>();
        x.pin = j.at("Pin").get<std::string>();
        x.rx_bypass = j.at("RXBypass").get<bool>();
        x.rx_bypass_on_demand = j.at("RXBypassOnDemand").get<bool>();
        x.randomize_own_bt_address = j.at("RandomizeOwnBTAddress").get<bool>();
        x.save_hci_packets = j.at("SaveHCIPackets").get<bool>();
        x.serial_auto_discovery = j.at("SerialAutoDiscovery").get<bool>();
        x.serial_baud_rate = j.at("SerialBaudRate").get<int64_t>();
        x.serial_enable_debug = j.at("SerialEnableDebug").get<bool>();
        x.serial_enable_debug_hci = j.at("SerialEnableDebugHCI").get<bool>();
        x.serial_port = j.at("SerialPort").get<std::string>();
        x.show_null_poll_packets = j.at("ShowNullPollPackets").get<bool>();
        x.target_bd_address = j.at("TargetBDAddress").get<std::string>();
        x.target_bd_address_list = j.at("TargetBDAddressList").get<std::vector<std::string>>();
    }

    inline void to_json(ordered_json & j, const quicktype::Bluetooth & x) {
        j = ordered_json::object();
        j["AuthReq"] = x.auth_req;
        j["BridgeHCI"] = x.bridge_hci;
        j["DisableRoleSwitch"] = x.disable_role_switch;
        j["EnableBounding"] = x.enable_bounding;
        j["IOCap"] = x.io_cap;
        j["InterceptTX"] = x.intercept_tx;
        j["LMPSniffing"] = x.lmp_sniffing;
        j["OwnBDAddress"] = x.own_bd_address;
        j["Pin"] = x.pin;
        j["RXBypass"] = x.rx_bypass;
        j["RXBypassOnDemand"] = x.rx_bypass_on_demand;
        j["RandomizeOwnBTAddress"] = x.randomize_own_bt_address;
        j["SaveHCIPackets"] = x.save_hci_packets;
        j["SerialAutoDiscovery"] = x.serial_auto_discovery;
        j["SerialBaudRate"] = x.serial_baud_rate;
        j["SerialEnableDebug"] = x.serial_enable_debug;
        j["SerialEnableDebugHCI"] = x.serial_enable_debug_hci;
        j["SerialPort"] = x.serial_port;
        j["ShowNullPollPackets"] = x.show_null_poll_packets;
        j["TargetBDAddress"] = x.target_bd_address;
        j["TargetBDAddressList"] = x.target_bd_address_list;
    }

    inline void from_json(const ordered_json & j, quicktype::Exclude& x) {
        x.apply_to = j.at("ApplyTo").get<std::string>();
        x.description = j.at("Description").get<std::string>();
        x.filter = j.at("Filter").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::Exclude & x) {
        j = ordered_json::object();
        j["ApplyTo"] = x.apply_to;
        j["Description"] = x.description;
        j["Filter"] = x.filter;
    }

    inline void from_json(const ordered_json & j, quicktype::StopCondition& x) {
        x.max_iterations = j.at("MaxIterations").get<int64_t>();
        x.max_time_minutes = j.at("MaxTimeMinutes").get<int64_t>();
        x.stop_on_max_iterations = j.at("StopOnMaxIterations").get<bool>();
        x.stop_on_max_time_minutes = j.at("StopOnMaxTimeMinutes").get<bool>();
    }

    inline void to_json(ordered_json & j, const quicktype::StopCondition & x) {
        j = ordered_json::object();
        j["MaxIterations"] = x.max_iterations;
        j["MaxTimeMinutes"] = x.max_time_minutes;
        j["StopOnMaxIterations"] = x.stop_on_max_iterations;
        j["StopOnMaxTimeMinutes"] = x.stop_on_max_time_minutes;
    }

    inline void from_json(const ordered_json & j, quicktype::Fuzzing& x) {
        x.enable_duplication = j.at("EnableDuplication").get<bool>();
        x.enable_mutation = j.at("EnableMutation").get<bool>();
        x.enable_optimization = j.at("EnableOptimization").get<bool>();
        x.default_duplication_probability = j.at("DefaultDuplicationProbability").get<double>();
        x.default_mutation_probability = j.at("DefaultMutationProbability").get<double>();
        x.default_mutation_field_probability = j.at("DefaultMutationFieldProbability").get<double>();
        x.field_mutation_backoff_multipler = j.at("FieldMutationBackoffMultipler").get<double>();
        x.max_fields_mutation = j.at("MaxFieldsMutation").get<int64_t>();
        x.normalize_protocol_layers_mutation = j.at("NormalizeProtocolLayersMutation").get<bool>();
        x.max_duplication_time_ms = j.at("MaxDuplicationTimeMS").get<int64_t>();
        x.packet_retry = j.at("PacketRetry").get<bool>();
        x.packet_retry_timeout_ms = j.at("PacketRetryTimeoutMS").get<int64_t>();
        x.global_timeout = j.at("GlobalTimeout").get<bool>();
        x.global_timeout_seconds = j.at("GlobalTimeoutSeconds").get<int64_t>();
        x.state_loop_detection = j.at("StateLoopDetection").get<bool>();
        x.state_loop_detection_threshold = j.at("StateLoopDetectionThreshold").get<int64_t>();
        x.random_seed = j.at("RandomSeed").get<int64_t>();
        x.seed_increment_every_iteration = j.at("SeedIncrementEveryIteration").get<bool>();
        x.restore_session_on_startup = j.at("RestoreSessionOnStartup").get<bool>();
        x.save_session_on_exit = j.at("SaveSessionOnExit").get<bool>();
        x.stop_condition = j.at("StopCondition").get<quicktype::StopCondition>();
        x.selector = j.at("Selector").get<int64_t>();
        x.mutator = j.at("Mutator").get<int64_t>();
        x.default_mutators = j.at("DefaultMutators").get<std::vector<std::string>>();
        x.default_selectors = j.at("DefaultSelectors").get<std::vector<std::string>>();
        x.excludes = j.at("Excludes").get<std::vector<quicktype::Exclude>>();
    }

    inline void to_json(ordered_json & j, const quicktype::Fuzzing & x) {
        j = ordered_json::object();
        j["EnableDuplication"] = x.enable_duplication;
        j["EnableMutation"] = x.enable_mutation;
        j["EnableOptimization"] = x.enable_optimization;
        j["DefaultDuplicationProbability"] = x.default_duplication_probability;
        j["DefaultMutationProbability"] = x.default_mutation_probability;
        j["DefaultMutationFieldProbability"] = x.default_mutation_field_probability;
        j["FieldMutationBackoffMultipler"] = x.field_mutation_backoff_multipler;
        j["MaxFieldsMutation"] = x.max_fields_mutation;
        j["NormalizeProtocolLayersMutation"] = x.normalize_protocol_layers_mutation;
        j["MaxDuplicationTimeMS"] = x.max_duplication_time_ms;
        j["PacketRetry"] = x.packet_retry;
        j["PacketRetryTimeoutMS"] = x.packet_retry_timeout_ms;
        j["GlobalTimeout"] = x.global_timeout;
        j["GlobalTimeoutSeconds"] = x.global_timeout_seconds;
        j["StateLoopDetection"] = x.state_loop_detection;
        j["StateLoopDetectionThreshold"] = x.state_loop_detection_threshold;
        j["RandomSeed"] = x.random_seed;
        j["SeedIncrementEveryIteration"] = x.seed_increment_every_iteration;
        j["RestoreSessionOnStartup"] = x.restore_session_on_startup;
        j["SaveSessionOnExit"] = x.save_session_on_exit;
        j["StopCondition"] = x.stop_condition;
        j["Selector"] = x.selector;
        j["Mutator"] = x.mutator;
        j["DefaultMutators"] = x.default_mutators;
        j["DefaultSelectors"] = x.default_selectors;
        j["Excludes"] = x.excludes;
    }

    inline void from_json(const ordered_json & j, quicktype::Adb& x) {
        x.adb_device = j.at("ADBDevice").get<std::string>();
        x.adb_filter = j.at("ADBFilter").get<std::string>();
        x.adb_magic_words = j.at("ADBMagicWords").get<std::vector<std::string>>();
        x.adb_program = j.at("ADBProgram").get<std::string>();
        x.adb_sub_system = j.at("ADBSubSystem").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::Adb & x) {
        j = ordered_json::object();
        j["ADBDevice"] = x.adb_device;
        j["ADBFilter"] = x.adb_filter;
        j["ADBMagicWords"] = x.adb_magic_words;
        j["ADBProgram"] = x.adb_program;
        j["ADBSubSystem"] = x.adb_sub_system;
    }

    inline void from_json(const ordered_json & j, quicktype::Microphone& x) {
        x.microphone_detection_sensitivity = j.at("MicrophoneDetectionSensitivity").get<double>();
        x.microphone_device_id = j.at("MicrophoneDeviceId").get<int64_t>();
    }

    inline void to_json(ordered_json & j, const quicktype::Microphone & x) {
        j = ordered_json::object();
        j["MicrophoneDetectionSensitivity"] = x.microphone_detection_sensitivity;
        j["MicrophoneDeviceId"] = x.microphone_device_id;
    }

    inline void from_json(const ordered_json & j, quicktype::Qcdm& x) {
        x.qcdmadb = j.at("QCDMADB").get<bool>();
        x.qcdm_device = j.at("QCDMDevice").get<std::string>();
        x.qcdm_magic_words = j.at("QCDMMagicWords").get<std::vector<std::string>>();
    }

    inline void to_json(ordered_json & j, const quicktype::Qcdm & x) {
        j = ordered_json::object();
        j["QCDMADB"] = x.qcdmadb;
        j["QCDMDevice"] = x.qcdm_device;
        j["QCDMMagicWords"] = x.qcdm_magic_words;
    }

    inline void from_json(const ordered_json & j, quicktype::SerialUart& x) {
        x.serial_baud_rate = j.at("SerialBaudRate").get<int64_t>();
        x.serial_magic_words = j.at("SerialMagicWords").get<std::vector<std::string>>();
        x.serial_port_name = j.at("SerialPortName").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::SerialUart & x) {
        j = ordered_json::object();
        j["SerialBaudRate"] = x.serial_baud_rate;
        j["SerialMagicWords"] = x.serial_magic_words;
        j["SerialPortName"] = x.serial_port_name;
    }

    inline void from_json(const ordered_json & j, quicktype::Ssh& x) {
        x.ssh_command = j.at("SSHCommand").get<std::string>();
        x.ssh_enable_pre_commands = j.at("SSHEnablePreCommands").get<bool>();
        x.ssh_host_address = j.at("SSHHostAddress").get<std::string>();
        x.ssh_magic_words = j.at("SSHMagicWords").get<std::vector<std::string>>();
        x.ssh_password = j.at("SSHPassword").get<std::string>();
        x.ssh_port = j.at("SSHPort").get<int64_t>();
        x.ssh_pre_commands = j.at("SSHPreCommands").get<std::vector<std::string>>();
        x.ssh_username = j.at("SSHUsername").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::Ssh & x) {
        j = ordered_json::object();
        j["SSHCommand"] = x.ssh_command;
        j["SSHEnablePreCommands"] = x.ssh_enable_pre_commands;
        j["SSHHostAddress"] = x.ssh_host_address;
        j["SSHMagicWords"] = x.ssh_magic_words;
        j["SSHPassword"] = x.ssh_password;
        j["SSHPort"] = x.ssh_port;
        j["SSHPreCommands"] = x.ssh_pre_commands;
        j["SSHUsername"] = x.ssh_username;
    }

    inline void from_json(const ordered_json & j, quicktype::Monitor& x) {
        x.enable = j.at("Enable").get<bool>();
        x.print_to_stdout = j.at("PrintToStdout").get<bool>();
        x.wait_external_event = j.at("WaitExternalEvent").get<bool>();
        x.active_monitor_types = j.at("ActiveMonitorTypes").get<std::vector<int64_t>>();
        x.monitors_type_list = j.at("MonitorsTypeList").get<std::vector<std::string>>();
        x.serial_uart = j.at("SerialUART").get<quicktype::SerialUart>();
        x.adb = j.at("ADB").get<quicktype::Adb>();
        x.microphone = j.at("Microphone").get<quicktype::Microphone>();
        x.ssh = j.at("SSH").get<quicktype::Ssh>();
        x.qcdm = j.at("QCDM").get<quicktype::Qcdm>();
    }

    inline void to_json(ordered_json & j, const quicktype::Monitor & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["PrintToStdout"] = x.print_to_stdout;
        j["WaitExternalEvent"] = x.wait_external_event;
        j["ActiveMonitorTypes"] = x.active_monitor_types;
        j["MonitorsTypeList"] = x.monitors_type_list;
        j["SerialUART"] = x.serial_uart;
        j["ADB"] = x.adb;
        j["Microphone"] = x.microphone;
        j["SSH"] = x.ssh;
        j["QCDM"] = x.qcdm;
    }

    inline void from_json(const ordered_json & j, quicktype::Subscriber& x) {
        x.imsi = j.at("IMSI").get<std::string>();
        x.k = j.at("K").get<std::string>();
        x.opc = quicktype::get_optional<std::string>(j, "OPC");
        x.apn = j.at("APN").get<std::string>();
        x.op = quicktype::get_optional<std::string>(j, "OP");
    }

    inline void to_json(ordered_json & j, const quicktype::Subscriber & x) {
        j = ordered_json::object();
        j["IMSI"] = x.imsi;
        j["K"] = x.k;
        if (x.opc) {
            j["OPC"] = x.opc;
        }
        j["APN"] = x.apn;
        if (x.op) {
            j["OP"] = x.op;
        }
    }

    inline void from_json(const ordered_json & j, quicktype::Nr5G& x) {
        x.mcc = j.at("MCC").get<std::string>();
        x.mnc = j.at("MNC").get<std::string>();
        x.auto_start_base_station = j.at("AutoStartBaseStation").get<bool>();
        x.auto_start_core_network = j.at("AutoStartCoreNetwork").get<bool>();
        x.base_station_config_file = j.at("BaseStationConfigFile").get<std::string>();
        x.base_station_arguments = j.at("BaseStationArguments").get<std::string>();
        x.core_network_config_file = j.at("CoreNetworkConfigFile").get<std::string>();
        x.enable_simulator = j.at("EnableSimulator").get<bool>();
        x.simulator_delay_us = j.at("SimulatorDelayUS").get<int64_t>();
        x.simulation_connection_timeout_ms = j.at("SimulationConnectionTimeoutMS").get<int64_t>();
        x.simulator_ue_arguments = j.at("SimulatorUEArguments").get<std::string>();
        x.subscribers = j.at("Subscribers").get<std::vector<quicktype::Subscriber>>();
    }

    inline void to_json(ordered_json & j, const quicktype::Nr5G & x) {
        j = ordered_json::object();
        j["MCC"] = x.mcc;
        j["MNC"] = x.mnc;
        j["AutoStartBaseStation"] = x.auto_start_base_station;
        j["AutoStartCoreNetwork"] = x.auto_start_core_network;
        j["BaseStationConfigFile"] = x.base_station_config_file;
        j["BaseStationArguments"] = x.base_station_arguments;
        j["CoreNetworkConfigFile"] = x.core_network_config_file;
        j["EnableSimulator"] = x.enable_simulator;
        j["SimulatorDelayUS"] = x.simulator_delay_us;
        j["SimulationConnectionTimeoutMS"] = x.simulation_connection_timeout_ms;
        j["SimulatorUEArguments"] = x.simulator_ue_arguments;
        j["Subscribers"] = x.subscribers;
    }

    inline void from_json(const ordered_json & j, quicktype::Options& x) {
        x.default_protocol_name = j.at("DefaultProtocolName").get<std::string>();
        x.default_protocol_encap_name = j.at("DefaultProtocolEncapName").get<std::string>();
        x.save_protocol_capture = j.at("SaveProtocolCapture").get<bool>();
        x.live_protocol_capture = j.at("LiveProtocolCapture").get<bool>();
        x.save_logs_to_file = j.at("SaveLogsToFile").get<bool>();
        x.save_latency_metrics = j.at("SaveLatencyMetrics").get<bool>();
        x.skip_packet_processing = j.at("SkipPacketProcessing").get<bool>();
        x.program = j.at("Program").get<int64_t>();
        x.auto_start_program = j.at("AutoStartProgram").get<bool>();
        x.auto_restart_program = j.at("AutoRestartProgram").get<bool>();
        x.launch_program_with_gdb = j.at("LaunchProgramWithGDB").get<bool>();
        x.programs_list = j.at("ProgramsList").get<std::vector<std::string>>();
        x.main_thread_core = j.at("MainThreadCore").get<int64_t>();
        x.save_core_dump = j.at("SaveCoreDump").get<bool>();
    }

    inline void to_json(ordered_json & j, const quicktype::Options & x) {
        j = ordered_json::object();
        j["DefaultProtocolName"] = x.default_protocol_name;
        j["DefaultProtocolEncapName"] = x.default_protocol_encap_name;
        j["SaveProtocolCapture"] = x.save_protocol_capture;
        j["LiveProtocolCapture"] = x.live_protocol_capture;
        j["SaveLogsToFile"] = x.save_logs_to_file;
        j["SaveLatencyMetrics"] = x.save_latency_metrics;
        j["SkipPacketProcessing"] = x.skip_packet_processing;
        j["Program"] = x.program;
        j["AutoStartProgram"] = x.auto_start_program;
        j["AutoRestartProgram"] = x.auto_restart_program;
        j["LaunchProgramWithGDB"] = x.launch_program_with_gdb;
        j["ProgramsList"] = x.programs_list;
        j["MainThreadCore"] = x.main_thread_core;
        j["SaveCoreDump"] = x.save_core_dump;
    }

    inline void from_json(const ordered_json & j, quicktype::PythonApiServer& x) {
        x.enable = j.at("Enable").get<bool>();
        x.listen_address = j.at("ListenAddress").get<std::string>();
        x.port = j.at("Port").get<int64_t>();
        x.api_namespace = j.at("APINamespace").get<std::string>();
        x.enable_events = j.at("EnableEvents").get<bool>();
        x.logging = j.at("Logging").get<bool>();
        x.server_module = j.at("ServerModule").get<int64_t>();
        x.server_modules_list = j.at("ServerModulesList").get<std::vector<std::string>>();
    }

    inline void to_json(ordered_json & j, const quicktype::PythonApiServer & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["ListenAddress"] = x.listen_address;
        j["Port"] = x.port;
        j["APINamespace"] = x.api_namespace;
        j["EnableEvents"] = x.enable_events;
        j["Logging"] = x.logging;
        j["ServerModule"] = x.server_module;
        j["ServerModulesList"] = x.server_modules_list;
    }

    inline void from_json(const ordered_json & j, quicktype::ReportModule& x) {
        x.enabled = j.at("Enabled").get<bool>();
        x.type = j.at("Type").get<std::string>();
        x.script = j.at("Script").get<std::string>();
        x.credentials_file = j.at("CredentialsFile").get<std::string>();
        x.only_errors = j.at("OnlyErrors").get<bool>();
        x.to = j.at("To").get<std::vector<std::string>>();
        x.magic_words = j.at("MagicWords").get<std::vector<std::string>>();
    }

    inline void to_json(ordered_json & j, const quicktype::ReportModule & x) {
        j = ordered_json::object();
        j["Enabled"] = x.enabled;
        j["Type"] = x.type;
        j["Script"] = x.script;
        j["CredentialsFile"] = x.credentials_file;
        j["OnlyErrors"] = x.only_errors;
        j["To"] = x.to;
        j["MagicWords"] = x.magic_words;
    }

    inline void from_json(const ordered_json & j, quicktype::ReportsSender& x) {
        x.enable = j.at("Enable").get<bool>();
        x.report_modules = j.at("ReportModules").get<std::vector<quicktype::ReportModule>>();
    }

    inline void to_json(ordered_json & j, const quicktype::ReportsSender & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["ReportModules"] = x.report_modules;
    }

    inline void from_json(const ordered_json & j, quicktype::TShark& x) {
        x.enable = j.at("Enable").get<bool>();
        x.interfaces_name = j.at("InterfacesName").get<std::vector<std::string>>();
        x.enable_display_filter = j.at("EnableDisplayFilter").get<bool>();
        x.display_filter = j.at("DisplayFilter").get<std::string>();
        x.use_tcp_dump_for_capture = j.at("UseTCPDumpForCapture").get<bool>();
    }

    inline void to_json(ordered_json & j, const quicktype::TShark & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["InterfacesName"] = x.interfaces_name;
        j["EnableDisplayFilter"] = x.enable_display_filter;
        j["DisplayFilter"] = x.display_filter;
        j["UseTCPDumpForCapture"] = x.use_tcp_dump_for_capture;
    }

    inline void from_json(const ordered_json & j, quicktype::UeModemManager& x) {
        x.enable = j.at("Enable").get<bool>();
        x.apn = j.at("APN").get<std::string>();
        x.allowed_modes = j.at("AllowedModes").get<std::string>();
        x.preferred_mode = j.at("PreferredMode").get<std::string>();
        x.bands = j.at("Bands").get<std::string>();
        x.disable_fuzzing_on_first_connection = j.at("DisableFuzzingOnFirstConnection").get<bool>();
        x.auto_connect_modem = j.at("AutoConnectModem").get<bool>();
        x.connection_timeout_ms = j.at("ConnectionTimeoutMS").get<int64_t>();
        x.use_only_at_connections = j.at("UseOnlyATConnections").get<bool>();
        x.auto_connect_to_apn = j.at("AutoConnectToAPN").get<bool>();
        x.manual_apn_connection = j.at("ManualAPNConnection").get<bool>();
        x.manual_apn_connection_delay_ms = j.at("ManualAPNConnectionDelayMS").get<int64_t>();
        x.auto_reconnect_modem = j.at("AutoReconnectModem").get<bool>();
        x.reset_modem_on_global_timeout = j.at("ResetModemOnGlobalTimeout").get<bool>();
        x.global_timeouts_count = j.at("GlobalTimeoutsCount").get<int64_t>();
        x.default_modem_interface_path = j.at("DefaultModemInterfacePath").get<std::string>();
        x.auto_search_modem_interface_path = j.at("AutoSearchModemInterfacePath").get<bool>();
        x.use_in_usbip_host_ssh = j.at("UseInUSBIPHostSSH").get<bool>();
        x.enable_adb = j.at("EnableADB").get<bool>();
        x.adb_device = j.at("ADBDevice").get<std::string>();
        x.adbpin = j.at("ADBPIN").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::UeModemManager & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["APN"] = x.apn;
        j["AllowedModes"] = x.allowed_modes;
        j["PreferredMode"] = x.preferred_mode;
        j["Bands"] = x.bands;
        j["DisableFuzzingOnFirstConnection"] = x.disable_fuzzing_on_first_connection;
        j["AutoConnectModem"] = x.auto_connect_modem;
        j["ConnectionTimeoutMS"] = x.connection_timeout_ms;
        j["UseOnlyATConnections"] = x.use_only_at_connections;
        j["AutoConnectToAPN"] = x.auto_connect_to_apn;
        j["ManualAPNConnection"] = x.manual_apn_connection;
        j["ManualAPNConnectionDelayMS"] = x.manual_apn_connection_delay_ms;
        j["AutoReconnectModem"] = x.auto_reconnect_modem;
        j["ResetModemOnGlobalTimeout"] = x.reset_modem_on_global_timeout;
        j["GlobalTimeoutsCount"] = x.global_timeouts_count;
        j["DefaultModemInterfacePath"] = x.default_modem_interface_path;
        j["AutoSearchModemInterfacePath"] = x.auto_search_modem_interface_path;
        j["UseInUSBIPHostSSH"] = x.use_in_usbip_host_ssh;
        j["EnableADB"] = x.enable_adb;
        j["ADBDevice"] = x.adb_device;
        j["ADBPIN"] = x.adbpin;
    }

    inline void from_json(const ordered_json & j, quicktype::UsbDevice& x) {
        x.enabled = j.at("Enabled").get<bool>();
        x.name = j.at("Name").get<std::string>();
        x.vidpid = j.at("VIDPID").get<std::string>();
        x.reset_on_program_startup = quicktype::get_optional<bool>(j, "ResetOnProgramStartup");
    }

    inline void to_json(ordered_json & j, const quicktype::UsbDevice & x) {
        j = ordered_json::object();
        j["Enabled"] = x.enabled;
        j["Name"] = x.name;
        j["VIDPID"] = x.vidpid;
        if (x.reset_on_program_startup) {
            j["ResetOnProgramStartup"] = x.reset_on_program_startup;
        }
    }

    inline void from_json(const ordered_json & j, quicktype::UsbHubControl& x) {
        x.enable = j.at("Enable").get<bool>();
        x.usb_devices = j.at("USBDevices").get<std::vector<quicktype::UsbDevice>>();
        x.global_timeouts_count = j.at("GlobalTimeoutsCount").get<int64_t>();
        x.toggle_power_delay_ms = j.at("TogglePowerDelayMS").get<int64_t>();
        x.use_in_usbip_host_ssh = j.at("UseInUSBIPHostSSH").get<bool>();
    }

    inline void to_json(ordered_json & j, const quicktype::UsbHubControl & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["USBDevices"] = x.usb_devices;
        j["GlobalTimeoutsCount"] = x.global_timeouts_count;
        j["TogglePowerDelayMS"] = x.toggle_power_delay_ms;
        j["UseInUSBIPHostSSH"] = x.use_in_usbip_host_ssh;
    }

    inline void from_json(const ordered_json & j, quicktype::HostSsh& x) {
        x.enable = j.at("Enable").get<bool>();
        x.username = j.at("Username").get<std::string>();
        x.password = j.at("Password").get<std::string>();
        x.port = j.at("Port").get<int64_t>();
        x.unbind_on_exit = j.at("UnbindOnExit").get<bool>();
        x.upload_folder = j.at("UploadFolder").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::HostSsh & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["Username"] = x.username;
        j["Password"] = x.password;
        j["Port"] = x.port;
        j["UnbindOnExit"] = x.unbind_on_exit;
        j["UploadFolder"] = x.upload_folder;
    }

    inline void from_json(const ordered_json & j, quicktype::Usbip& x) {
        x.enable = j.at("Enable").get<bool>();
        x.server_address = j.at("ServerAddress").get<std::string>();
        x.port = j.at("Port").get<int64_t>();
        x.usb_devices = j.at("USBDevices").get<std::vector<quicktype::UsbDevice>>();
        x.host_ssh = j.at("HostSSH").get<quicktype::HostSsh>();
    }

    inline void to_json(ordered_json & j, const quicktype::Usbip & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["ServerAddress"] = x.server_address;
        j["Port"] = x.port;
        j["USBDevices"] = x.usb_devices;
        j["HostSSH"] = x.host_ssh;
    }

    inline void from_json(const ordered_json & j, quicktype::Services& x) {
        x.python_api_server = j.at("PythonAPIServer").get<quicktype::PythonApiServer>();
        x.reports_sender = j.at("ReportsSender").get<quicktype::ReportsSender>();
        x.t_shark = j.at("TShark").get<quicktype::TShark>();
        x.ue_modem_manager = j.at("UEModemManager").get<quicktype::UeModemManager>();
        x.usb_hub_control = j.at("USBHubControl").get<quicktype::UsbHubControl>();
        x.usbip = j.at("USBIP").get<quicktype::Usbip>();
    }

    inline void to_json(ordered_json & j, const quicktype::Services & x) {
        j = ordered_json::object();
        j["PythonAPIServer"] = x.python_api_server;
        j["ReportsSender"] = x.reports_sender;
        j["TShark"] = x.t_shark;
        j["UEModemManager"] = x.ue_modem_manager;
        j["USBHubControl"] = x.usb_hub_control;
        j["USBIP"] = x.usbip;
    }

    inline void from_json(const ordered_json & j, quicktype::Mapping& x) {
        x.append_summary = j.at("AppendSummary").get<bool>();
        x.filter = j.at("Filter").get<std::string>();
        x.layer_name = j.at("LayerName").get<std::string>();
        x.state_name_field = j.at("StateNameField").get<quicktype::StateNameField>();
    }

    inline void to_json(ordered_json & j, const quicktype::Mapping & x) {
        j = ordered_json::object();
        j["AppendSummary"] = x.append_summary;
        j["Filter"] = x.filter;
        j["LayerName"] = x.layer_name;
        j["StateNameField"] = x.state_name_field;
    }

    inline void from_json(const ordered_json & j, quicktype::Overrides& x) {
    }

    inline void to_json(ordered_json & j, const quicktype::Overrides & x) {
        j = ordered_json::object();
    }

    inline void from_json(const ordered_json & j, quicktype::StateMapper& x) {
        x.enable = j.at("Enable").get<bool>();
        x.packet_layer_offset = j.at("PacketLayerOffset").get<int64_t>();
        x.save_folder = j.at("SaveFolder").get<std::string>();
        x.show_all_states = j.at("ShowAllStates").get<bool>();
        x.pcap_pseudo_header_direction_offset = j.at("PcapPseudoHeaderDirectionOffset").get<int64_t>();
        x.ignore_malformed_packets = j.at("IgnoreMalformedPackets").get<bool>();
        x.mapping = j.at("Mapping").get<std::vector<quicktype::Mapping>>();
        x.overrides = j.at("Overrides").get<quicktype::Overrides>();
    }

    inline void to_json(ordered_json & j, const quicktype::StateMapper & x) {
        j = ordered_json::object();
        j["Enable"] = x.enable;
        j["PacketLayerOffset"] = x.packet_layer_offset;
        j["SaveFolder"] = x.save_folder;
        j["ShowAllStates"] = x.show_all_states;
        j["PcapPseudoHeaderDirectionOffset"] = x.pcap_pseudo_header_direction_offset;
        j["IgnoreMalformedPackets"] = x.ignore_malformed_packets;
        j["Mapping"] = x.mapping;
        j["Overrides"] = x.overrides;
    }

    inline void from_json(const ordered_json & j, quicktype::CommonRejection& x) {
        x.description = j.at("Description").get<std::string>();
        x.filter = j.at("Filter").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::CommonRejection & x) {
        j = ordered_json::object();
        j["Description"] = x.description;
        j["Filter"] = x.filter;
    }

    inline void from_json(const ordered_json & j, quicktype::Validation& x) {
        x.common_rejections = j.at("CommonRejections").get<std::vector<quicktype::CommonRejection>>();
        x.default_fragments_layer = j.at("DefaultFragmentsLayer").get<std::string>();
        x.default_packet_layer = j.at("DefaultPacketLayer").get<std::string>();
        x.initial_state = j.at("InitialState").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::Validation & x) {
        j = ordered_json::object();
        j["CommonRejections"] = x.common_rejections;
        j["DefaultFragmentsLayer"] = x.default_fragments_layer;
        j["DefaultPacketLayer"] = x.default_packet_layer;
        j["InitialState"] = x.initial_state;
    }

    inline void from_json(const ordered_json & j, quicktype::Wifi& x) {
        x.wifi_interface = j.at("WifiInterface").get<std::string>();
        x.wifi_ssid = j.at("WifiSSID").get<std::string>();
        x.wifi_password = j.at("WifiPassword").get<std::string>();
        x.wifi_channel = j.at("WifiChannel").get<int64_t>();
        x.wifi_key_auth = j.at("WifiKeyAuth").get<int64_t>();
        x.wifi_key_auth_list = j.at("WifiKeyAuthList").get<std::vector<std::string>>();
        x.wifi_rsn_crypto = j.at("WifiRSNCrypto").get<int64_t>();
        x.wifi_rsn_crypto_list = j.at("WifiRSNCryptoList").get<std::vector<std::string>>();
        x.wifi_username = j.at("WifiUsername").get<std::string>();
        x.wifi_eap_method = j.at("WifiEAPMethod").get<int64_t>();
        x.wifi_eap_method_list = j.at("WifiEAPMethodList").get<std::vector<std::string>>();
        x.wifi_allow_internet = j.at("WifiAllowInternet").get<bool>();
        x.wifi_dhcp = j.at("WifiDHCP").get<bool>();
        x.wifi_dhcp_gateway_address = j.at("WifiDHCPGatewayAddress").get<std::string>();
        x.wifi802_11_w = j.at("Wifi802.11w").get<bool>();
        x.wifi_country_code = j.at("WifiCountryCode").get<std::string>();
    }

    inline void to_json(ordered_json & j, const quicktype::Wifi & x) {
        j = ordered_json::object();
        j["WifiInterface"] = x.wifi_interface;
        j["WifiSSID"] = x.wifi_ssid;
        j["WifiPassword"] = x.wifi_password;
        j["WifiChannel"] = x.wifi_channel;
        j["WifiKeyAuth"] = x.wifi_key_auth;
        j["WifiKeyAuthList"] = x.wifi_key_auth_list;
        j["WifiRSNCrypto"] = x.wifi_rsn_crypto;
        j["WifiRSNCryptoList"] = x.wifi_rsn_crypto_list;
        j["WifiUsername"] = x.wifi_username;
        j["WifiEAPMethod"] = x.wifi_eap_method;
        j["WifiEAPMethodList"] = x.wifi_eap_method_list;
        j["WifiAllowInternet"] = x.wifi_allow_internet;
        j["WifiDHCP"] = x.wifi_dhcp;
        j["WifiDHCPGatewayAddress"] = x.wifi_dhcp_gateway_address;
        j["Wifi802.11w"] = x.wifi802_11_w;
        j["WifiCountryCode"] = x.wifi_country_code;
    }

    inline void from_json(const ordered_json & j, quicktype::Config& x) {
        x.name = j.at("Name").get<std::string>();
        x.options = j.at("Options").get<quicktype::Options>();
        x.bluetooth = j.at("Bluetooth").get<quicktype::Bluetooth>();
        x.wifi = j.at("Wifi").get<quicktype::Wifi>();
        x.nr5_g = j.at("NR5G").get<quicktype::Nr5G>();
        x.fuzzing = j.at("Fuzzing").get<quicktype::Fuzzing>();
        x.state_mapper = j.at("StateMapper").get<quicktype::StateMapper>();
        x.validation = j.at("Validation").get<quicktype::Validation>();
        x.monitor = j.at("Monitor").get<quicktype::Monitor>();
        x.services = j.at("Services").get<quicktype::Services>();
    }

    inline void to_json(ordered_json & j, const quicktype::Config & x) {
        j = ordered_json::object();
        j["Name"] = x.name;
        j["Options"] = x.options;
        j["Bluetooth"] = x.bluetooth;
        j["Wifi"] = x.wifi;
        j["NR5G"] = x.nr5_g;
        j["Fuzzing"] = x.fuzzing;
        j["StateMapper"] = x.state_mapper;
        j["Validation"] = x.validation;
        j["Monitor"] = x.monitor;
        j["Services"] = x.services;
    }

    inline void from_json(const ordered_json & j, quicktype::GlobalConfig& x) {
        x.config = j.at("config").get<quicktype::Config>();
    }

    inline void to_json(ordered_json & j, const quicktype::GlobalConfig & x) {
        j = ordered_json::object();
        j["config"] = x.config;
    }
    inline void from_json(const ordered_json & j, std::variant<std::vector<std::string>, std::string> & x) {
        if (j.is_string())
            x = j.get<std::string>();
        else if (j.is_array())
            x = j.get<std::vector<std::string>>();
        else throw "Could not deserialize";
    }

    inline void to_json(ordered_json & j, const std::variant<std::vector<std::string>, std::string> & x) {
        switch (x.index()) {
            case 0:
                j = std::get<std::vector<std::string>>(x);
                break;
            case 1:
                j = std::get<std::string>(x);
                break;
            default: throw "Input JSON does not conform to schema";
        }
    }
}


const char *default_config = R"/({
    "config": {
        "Name": "Bluetooth",
        "Options": {
            "DefaultProtocolName": "encap:1",
            "DefaultProtocolEncapName": "encap:1",
            "SaveProtocolCapture": true,
            "LiveProtocolCapture": false,
            "SaveLogsToFile": true,
            "SaveLatencyMetrics": false,
            "SkipPacketProcessing": false,
            "Program": 1,
            "AutoStartProgram": true,
            "AutoRestartProgram": false,
            "LaunchProgramWithGDB": false,
            "ProgramsList": [
                ""
            ],
            "MainThreadCore": -1,
            "SaveCoreDump": false
        },
        "Bluetooth": {
            "AuthReq": 3,
            "BridgeHCI": true,
            "DisableRoleSwitch": false,
            "EnableBounding": true,
            "IOCap": 3,
            "InterceptTX": true,
            "LMPSniffing": true,
            "OwnBDAddress": "bc:bb:b1:8c:dd:4e",
            "Pin": "0000",
            "RXBypass": false,
            "RXBypassOnDemand": false,
            "RandomizeOwnBTAddress": true,
            "SaveHCIPackets": false,
            "SerialAutoDiscovery": false,
            "SerialBaudRate": 4000000,
            "SerialEnableDebug": false,
            "SerialEnableDebugHCI": false,
            "SerialPort": "/dev/ttyUSB1",
            "ShowNullPollPackets": false,
            "TargetBDAddress": "3c:61:05:4c:34:56",
            "TargetBDAddressList": [
                ""
            ]
        },
        "Wifi": {
            "WifiInterface": "wlan1",
            "WifiSSID": "TEST_KRA",
            "WifiPassword": "testtest",
            "WifiChannel": 9,
            "WifiKeyAuth": 0,
            "WifiKeyAuthList": [
                "WPA-EAP",
                "WPA-PSK",
                "SAE"
            ],
            "WifiRSNCrypto": 0,
            "WifiRSNCryptoList": [
                "CCMP",
                "TKIP"
            ],
            "WifiUsername": "matheus_garbelini",
            "WifiEAPMethod": 0,
            "WifiEAPMethodList": [
                "PEAP",
                "PWD",
                "TTLS",
                "TLS"
            ],
            "WifiAllowInternet": true,
            "WifiDHCP": true,
            "WifiDHCPGatewayAddress": "192.172.42.1",
            "Wifi802.11w": false,
            "WifiCountryCode": "US"
        },
        "NR5G": {
            "MCC": "208",
            "MNC": "95",
            "AutoStartBaseStation": false,
            "AutoStartCoreNetwork": false,
            "BaseStationConfigFile": "n78.106.conf",
            "BaseStationArguments": "--sa --continuous-tx -E",
            "CoreNetworkConfigFile": "open5gs.yaml",
            "EnableSimulator": false,
            "SimulatorDelayUS": 2000,
            "SimulationConnectionTimeoutMS": 4000,
            "SimulatorUEArguments": "-r 25 --ue-rxgain 140 --basicsim",
            "Subscribers": [
                {
                    "IMSI": "001010000000001",
                    "K": "00112233445566778899AABBCCDDEEFF",
                    "OPC": "00112233445566778899AABBCCDDEEFF",
                    "APN": "default"
                },
                {
                    "IMSI": "001010000000001",
                    "K": "00112233445566778899AABBCCDDEEFF",
                    "OP": "00112233445566778899AABBCCDDEEFF",
                    "APN": "default"
                }
            ]
        },
        "Fuzzing": {
            "EnableDuplication": false,
            "EnableMutation": false,
            "EnableOptimization": false,
            "DefaultDuplicationProbability": 0.1,
            "DefaultMutationProbability": 0.1,
            "DefaultMutationFieldProbability": 0.1,
            "FieldMutationBackoffMultipler": 0.5,
            "MaxFieldsMutation": 6,
            "NormalizeProtocolLayersMutation": false,
            "MaxDuplicationTimeMS": 6000,
            "PacketRetry": true,
            "PacketRetryTimeoutMS": 2000,
            "GlobalTimeout": true,
            "GlobalTimeoutSeconds": 45,
            "StateLoopDetection": false,
            "StateLoopDetectionThreshold": 5,
            "RandomSeed": 123456789,
            "SeedIncrementEveryIteration": false,
            "RestoreSessionOnStartup": false,
            "SaveSessionOnExit": false,
            "StopCondition": {
                "MaxIterations": 1000,
                "MaxTimeMinutes": 240,
                "StopOnMaxIterations": true,
                "StopOnMaxTimeMinutes": true
            },
            "Selector": 0,
            "Mutator": 1,
            "DefaultMutators": [
                "Random",
                "RandomBit",
                "RandomZeroByte",
                "RandomFullByte",
                "ToggleBit"
            ],
            "DefaultSelectors": [
                "Random",
                "Sequential",
                "Overlap"
            ],
            "Excludes": [
                {
                    "ApplyTo": "A",
                    "Description": "LMP_set_AFH",
                    "Filter": "btbrlmp.op == 60"
                },
                {
                    "ApplyTo": "A",
                    "Description": "LMP_detach",
                    "Filter": "btbrlmp.op == 7"
                },
                {
                    "ApplyTo": "A",
                    "Description": "FHS",
                    "Filter": "fhs"
                },
                {
                    "ApplyTo": "D",
                    "Description": "L2CAP",
                    "Filter": "btl2cap"
                }
            ]
        },
        "StateMapper": {
            "Enable": false,
            "PacketLayerOffset": 1,
            "SaveFolder": "configs/models/",
            "ShowAllStates": false,
            "PcapPseudoHeaderDirectionOffset": 50,
            "IgnoreMalformedPackets": true,
            "Mapping": [
                {
                    "AppendSummary": false,
                    "Filter": "",
                    "LayerName": "",
                    "StateNameField": ""
                },
                {
                    "AppendSummary": false,
                    "Filter": "",
                    "LayerName": "",
                    "StateNameField": [
                        ""
                    ]
                }
            ],
            "Overrides": {}
        },
        "Validation": {
            "CommonRejections": [
                {
                    "Description": "",
                    "Filter": ""
                }
            ],
            "DefaultFragmentsLayer": "",
            "DefaultPacketLayer": "",
            "InitialState": "IDLE"
        },
        "Monitor": {
            "Enable": true,
            "PrintToStdout": true,
            "WaitExternalEvent": false,
            "ActiveMonitorTypes": [
                0
            ],
            "MonitorsTypeList": [
                "Serial",
                "SSH",
                "Microphone",
                "ADB",
                "QCDM"
            ],
            "SerialUART": {
                "SerialBaudRate": 115200,
                "SerialMagicWords": [
                    "DBFW_ASSERT_TYPE_FATAL!!!",
                    "WRAP THOR AI",
                    "BT Started!",
                    "Guru Meditation Error",
                    "abort()"
                ],
                "SerialPortName": "/dev/ttyUSB3"
            },
            "ADB": {
                "ADBDevice": "",
                "ADBFilter": "",
                "ADBMagicWords": [
                    "ModemEvent: modem_failure",
                    "ModemRestartStats",
                    "SOC crashed",
                    "Unable to wake SOC"
                ],
                "ADBProgram": "logcat",
                "ADBSubSystem": "radio,crash,system,kernel"
            },
            "Microphone": {
                "MicrophoneDetectionSensitivity": 0.7,
                "MicrophoneDeviceId": -1
            },
            "SSH": {
                "SSHCommand": "sudo dmesg -C && sudo dmesg -e -w",
                "SSHEnablePreCommands": true,
                "SSHHostAddress": "127.0.0.1",
                "SSHMagicWords": [
                    "Backtrace:",
                    "Oops:",
                    "BUG:",
                    "RIP:",
                    "Call Trace:"
                ],
                "SSHPassword": "",
                "SSHPort": 22,
                "SSHPreCommands": [
                    "sudo sh -c \"echo 'module btusb +p' > /sys/kernel/debug/dynamic_debug/control\"",
                    "sudo sh -c \"echo 'module btintel +p' > /sys/kernel/debug/dynamic_debug/control\"",
                    "sudo sh -c \"echo 'module bluetooth +p' > /sys/kernel/debug/dynamic_debug/control\"",
                    "sudo sh -c \"echo 'module rfcomm +p' > /sys/kernel/debug/dynamic_debug/control\"",
                    "sudo hciconfig hci0 inqparms 18:18",
                    "sudo hciconfig hci0 pageparms 18:18"
                ],
                "SSHUsername": "matheus"
            },
            "QCDM": {
                "QCDMADB": false,
                "QCDMDevice": "/dev/ttyUSB0",
                "QCDMMagicWords": [
                    "testtest"
                ]
            }
        },
        "Services": {
            "PythonAPIServer": {
                "Enable": true,
                "ListenAddress": "127.0.0.1",
                "Port": 3000,
                "APINamespace": "/",
                "EnableEvents": false,
                "Logging": false,
                "ServerModule": 1,
                "ServerModulesList": [
                    "SocketIOServer",
                    "RESTServer"
                ]
            },
            "ReportsSender": {
                "Enable": true,
                "ReportModules": [
                    {
                        "Enabled": true,
                        "Type": "Email",
                        "Script": "gmail.py",
                        "CredentialsFile": "modules/reportsender/credentials.json",
                        "OnlyErrors": true,
                        "To": [
                            "mgarbelix@gmail.com"
                        ],
                        "MagicWords": [
                            "[Crash]",
                            "[Hang]",
                            "[Reset]"
                        ]
                    }
                ]
            },
            "TShark": {
                "Enable": false,
                "InterfacesName": [
                    "usbmon2"
                ],
                "EnableDisplayFilter": true,
                "DisplayFilter": "usb.transfer_type==0x02",
                "UseTCPDumpForCapture": false
            },
            "UEModemManager": {
                "Enable": false,
                "APN": "internet",
                "AllowedModes": "4g|5g",
                "PreferredMode": "5g",
                "Bands": "ngran-78",
                "DisableFuzzingOnFirstConnection": true,
                "AutoConnectModem": true,
                "ConnectionTimeoutMS": 4000,
                "UseOnlyATConnections": false,
                "AutoConnectToAPN": true,
                "ManualAPNConnection": false,
                "ManualAPNConnectionDelayMS": 0,
                "AutoReconnectModem": true,
                "ResetModemOnGlobalTimeout": false,
                "GlobalTimeoutsCount": 3,
                "DefaultModemInterfacePath": "/dev/cdc-wdm1",
                "AutoSearchModemInterfacePath": true,
                "UseInUSBIPHostSSH": false,
                "EnableADB": true,
                "ADBDevice": "UWEUW4XG8XCA8PWS",
                "ADBPIN": "110125"
            },
            "USBHubControl": {
                "Enable": false,
                "USBDevices": [
                    {
                        "Enabled": false,
                        "Name": "Fibocom",
                        "VIDPID": "2cb7:0104",
                        "ResetOnProgramStartup": false
                    },
                    {
                        "Enabled": false,
                        "Name": "SIMCOM SIM8202G",
                        "VIDPID": "1e0e:9001",
                        "ResetOnProgramStartup": false
                    },
                    {
                        "Enabled": true,
                        "Name": "Quectel RM500Q-GL",
                        "VIDPID": "2c7c:0800",
                        "ResetOnProgramStartup": false
                    },
                    {
                        "Enabled": false,
                        "Name": "Telit FT980",
                        "VIDPID": "1bc7:1050",
                        "ResetOnProgramStartup": false
                    },
                    {
                        "Enabled": false,
                        "Name": "GOSN GM800",
                        "VIDPID": "305a:1406",
                        "ResetOnProgramStartup": false
                    }
                ],
                "GlobalTimeoutsCount": 4,
                "TogglePowerDelayMS": 2000,
                "UseInUSBIPHostSSH": false
            },
            "USBIP": {
                "Enable": false,
                "ServerAddress": "172.18.37.220",
                "Port": 3240,
                "USBDevices": [
                    {
                        "Enabled": false,
                        "Name": "Fibocom FM150-AE",
                        "VIDPID": "2cb7:0104"
                    },
                    {
                        "Enabled": false,
                        "Name": "SIMCOM SIM8202G",
                        "VIDPID": "1e0e:9001"
                    },
                    {
                        "Enabled": false,
                        "Name": "Quectel RM500Q-GL",
                        "VIDPID": "2c7c:0800"
                    },
                    {
                        "Enabled": false,
                        "Name": "GOSN GM800",
                        "VIDPID": "305a:1406",
                        "ResetOnProgramStartup": false
                    }
                ],
                "HostSSH": {
                    "Enable": true,
                    "Username": "deck",
                    "Password": "deck",
                    "Port": 22,
                    "UnbindOnExit": true,
                    "UploadFolder": "/tmp/wd_tools"
                }
            }
        }
    }
})/";
