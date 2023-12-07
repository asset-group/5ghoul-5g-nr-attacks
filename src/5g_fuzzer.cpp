#include <algorithm>
#include <chrono>
#include <csignal>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <sched.h>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "libs/folly/folly/FBVector.h"
#include "libs/folly/folly/concurrency/UnboundedQueue.h"
#include "libs/folly/folly/stats/TimeseriesHistogram.h"
#include "libs/log_misc_utils.hpp"
#include "libs/oai_tracing/T_IDs.h"

#include "Fitness.hpp"
#include "Framework.hpp"
#include "Fuzzing.hpp"
#include "Machine.hpp"
#include "Modules.hpp"
#include "PacketLogger.hpp"
#include "ParseArgs.hpp"
#include "Process.hpp"
#include "drivers/SHMDriver.hpp"
#include "gui/GUI_Common.hpp"
#include "monitors/Monitors.hpp"

// Services
#include "services/ModemManager.hpp"
#include "services/ReportSender.hpp"
#include "services/TShark.hpp"
#include "services/USBHubControl.hpp"
#include "services/USBIP.hpp"

#define CONFIG_FILE_PATH "configs/5gnr_gnb_config.json"
#define CAPTURE_FILE "logs/5gnr_gnb/capture_nr5g_gnb.pcapng"
#define CAPTURE_LINKTYPE pcpp::LinkLayerType::LINKTYPE_ETHERNET
#define SUBSCRIBERS_SCRIPT "./scripts/open5gs-dbctl.sh"

// Summary
uint32_t counter_crashes = 0;
uint32_t counter_rx_mac = 0;
uint32_t counter_tx_mac = 0;
uint32_t counter_tx_mac_fuzzed = 0;
uint32_t counter_tx_mac_duplicated = 0;
uint32_t counter_tx_sib = 0;
uint32_t counter_tx_mib = 0;
uint32_t counter_tx_pdcp = 0;
uint32_t counter_rx_pdcp = 0;
uint32_t counter_tx_nas = 0;
uint32_t counter_rx_nas = 0;
uint32_t time_processing_dlsch = 0;
uint32_t time_processing_sib = 0;
uint32_t time_processing_mib = 0;
uint32_t time_processing_uplink = 0;

ScrollingBuffer graph_latency_downlink(200);
ScrollingBuffer graph_latency_uplink(200);
ScrollingBuffer graph_latency_sib(200);
ScrollingBuffer graph_latency_pdcp_downlink(200);
ScrollingBuffer graph_latency_pdcp_uplink(200);
folly::TimeseriesHistogram<int64_t> histogram_latencies_downlink(10, 0, 300, folly::MultiLevelTimeSeries<int64_t>(200, {200s}));
folly::TimeseriesHistogram<int64_t> histogram_latencies_uplink(10, 0, 300, folly::MultiLevelTimeSeries<int64_t>(200, {200s}));
folly::TimeseriesHistogram<int64_t> histogram_latencies_sib(10, 0, 300, folly::MultiLevelTimeSeries<int64_t>(200, {200s}));
folly::TimeseriesHistogram<int64_t> histogram_latencies_pdcp_downlink(10, 0, 300, folly::MultiLevelTimeSeries<int64_t>(200, {200s}));
folly::TimeseriesHistogram<int64_t> histogram_latencies_pdcp_uplink(10, 0, 300, folly::MultiLevelTimeSeries<int64_t>(200, {200s}));
float buckets_list_downlink[32];
float buckets_list_uplink[32];
float buckets_list_sib[32];
float buckets_list_pdcp_downlink[32];
float buckets_list_pdcp_uplink[32];
float bucket_x[32];

// Variables
folly::UMPSCQueue<pkt_evt_t, true> queue_pkt_evt;
// list_data<uint8_t> stored_packets;
shared_ptr<React::IntervalWatcher> timer_global = nullptr;
unordered_map<int, shared_ptr<TimeoutWatcher>> dup_timeout_list;
shared_ptr<IntervalWatcher> timer_6_seconds;
std::mutex mutex_pdcp;
uint8_t recv_summary[2048];
uint8_t enable_logger = 0;
bool mutate_downlink = false;
lni::fast_vector<uint8_t> pcap_buffer;

// UE simulation variables
bool ue_sim_request_restart = false;
std::shared_ptr<React::TimeoutWatcher> ue_sim_conn_timeout = nullptr;

// UE common variables
bool gnb_ready = false;
bool ue_task_started = false;
bool ue_modem_initialized = false;
bool ue_dev_attached = false;
string ue_dev_path;
bool ue_connected = false;

// Reference pcap buffers
uint8_t pcap_mac_nr_udp_hdr[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                 0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
                                 0x9b, 0x2d, 0x73, 0x40, 0x0, 0x40, 0x11, 0x00,
                                 0x00, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
                                 0x1, 0xe7, 0xa6, 0x27, 0xf, 0x0, 0x87, 0xfe,
                                 0x9a, 0x6d, 0x61, 0x63, 0x2d, 0x6e, 0x72};

uint8_t pcap_mac_lte_udp_hdr[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                  0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
                                  0x3b, 0x30, 0x81, 0x40, 0x0, 0x40, 0x11, 0xc,
                                  0x2f, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
                                  0x1, 0xd5, 0xc4, 0x27, 0xf, 0x0, 0x27, 0xfe,
                                  0x3a, 0x6d, 0x61, 0x63, 0x2d, 0x6c, 0x74, 0x65};

// --------- Instances ---------
Fitness WDFitness;
WDSignalHandler SignalHandler;
WDPacketHandler PacketHandler;
WDGlobalTimeout GlobalTimeout;
WDAnomalyReport AnomalyReport;
WDModules Modules;
Monitors MonitorTarget;
WDParseArgs ParseArgs;
SvcReportSender ReportSender;
SvcModemManager ModemManager;
SvcUSBHubControl USBHubControl;
SvcTShark Tshark;
ProcessRunner Open5GSProcess;
ProcessRunner OpenAirInterface;
ProcessRunner QCMonitor;
ProcessRunner OpenAirInterfaceUE;
ProcessRunner WSProcess;
PacketLogger WSPacketLogger;
PacketLogger NRPacketLogger;
SHMDriver OAICommMAC_DL;
SHMDriver OAICommMAC_DL_BCCH;
SHMDriver OAICommMAC_UL;
SHMDriver OAICommPDCP_DL;
SHMDriver OAICommPDCP_UL;
SHMDriver Open5GSNAS_DL;
SHMDriver Open5GSNAS_UL;

// Module handling
wd_module_request_t module_request = {0};

void start_live_capture()
{
    GL1Y("Opening Wireshark...");

    if (WSProcess.isRunning())
        WSProcess.stop(true, true);

    WSPacketLogger.SetPreInitCallback([&](const string file_path) {
        WSProcess.setDetached(true);
        WSProcess.init("bin/wireshark", "-k -i "s + file_path + " >/dev/null 2>&1", nullptr);
    });
    WSPacketLogger.init(CAPTURE_FILE, CAPTURE_LINKTYPE, false, true, &StateMachine.config.options.live_protocol_capture);
}

// Update summary each 100ms
inline void log_summary()
{
    if (!gui_enabled)
        return;
    // clang-format off
    GL5G(ICON_FA_ARROW_RIGHT  " Transmitted (TX): ", counter_tx_mac);
    GL5G(ICON_FA_ARROW_LEFT   " Received (RX):    ", counter_rx_mac);
    GL5G(ICON_FA_CLONE        " Broadcasts (SIB): ", counter_tx_sib);
    GL5G(ICON_FA_CLONE        " Broadcasts (MIB): ", counter_tx_mib);
    GL5G(ICON_FA_CLOCK        " DL-SCH Time (us): ", time_processing_dlsch);
    GL5G(ICON_FA_CLOCK        " UL Time (us):     ", time_processing_uplink);
    GL5G(ICON_FA_ARROW_RIGHT  " PDCP packets (TX):", counter_tx_pdcp);
    GL5G(ICON_FA_SITEMAP      " Total States:    ",  StateMachine.TotalStates());
    GL5G(ICON_FA_ARROWS_ALT   " Total Transitions:", StateMachine.TotalTransitions());
    GL5Y(ICON_FA_COMPRESS_ALT " Current State:    ", StateMachine.GetCurrentStateName());
    GL5Y(ICON_FA_REPLY        " Previous State:   ", StateMachine.GetPreviousStateName());
    // clang-format on
}

/**
@brief This function displays a series of plots showing various statistics about the LTE network.
It initializes the graphs if they have not been already and then updates the data for each of the five plots.
The plots include: DLSCH Latency, UL-SCH Latency, SIB Latency, PDCP DL Latency, and PDCP UL Latency.
The x-axis for each plot represents the latency (in microseconds) and the y-axis represents the relative frequency of that latency.
The data for the plots is obtained from the histogram_latencies_downlink, histogram_latencies_uplink, histogram_latencies_sib,
histogram_latencies_pdcp_downlink, and histogram_latencies_pdcp_uplink histograms, respectively.
The plots are displayed using the ImPlot library.
*/
void UserGraphs()
{
    static bool initialized = false;

    if (!initialized) {
        uint64_t step_size = histogram_latencies_sib.getBucketSize();
        uint64_t max_samples = (histogram_latencies_sib.getMax() / step_size) + 1; // include > max
        for (size_t i = 0; i < max_samples; i++) {
            bucket_x[i] = (float)step_size * i;
        }
        initialized = true;
    }

    uint64_t tot_count;
    if (tot_count = histogram_latencies_downlink.count(0)) {
        for (size_t n = 1; n < histogram_latencies_downlink.getNumBuckets(); ++n) {
            buckets_list_downlink[n - 1] = (histogram_latencies_downlink.getBucket(n).count(0) / (float)tot_count);
        }
    }

    if (tot_count = histogram_latencies_uplink.count(0)) {
        for (unsigned int n = 1; n < histogram_latencies_uplink.getNumBuckets(); ++n) {
            buckets_list_uplink[n - 1] = (histogram_latencies_uplink.getBucket(n).count(0) / (float)tot_count);
        }
    }

    if (tot_count = histogram_latencies_sib.count(0)) {
        for (unsigned int n = 1; n < histogram_latencies_sib.getNumBuckets(); ++n) {
            buckets_list_sib[n - 1] = (histogram_latencies_sib.getBucket(n).count(0) / (float)tot_count);
        }
    }

    if (tot_count = histogram_latencies_pdcp_downlink.count(0)) {
        for (unsigned int n = 1; n < histogram_latencies_pdcp_downlink.getNumBuckets(); ++n) {
            buckets_list_pdcp_downlink[n - 1] = (histogram_latencies_pdcp_downlink.getBucket(n).count(0) / (float)tot_count);
        }
    }

    if (tot_count = histogram_latencies_pdcp_uplink.count(0)) {
        for (unsigned int n = 1; n < histogram_latencies_pdcp_uplink.getNumBuckets(); ++n) {
            buckets_list_pdcp_uplink[n - 1] = (histogram_latencies_pdcp_uplink.getBucket(n).count(0) / (float)tot_count);
        }
    }

    ImGui::Begin("Graphs");
    ImVec2 graph_size = ImGui::GetContentRegionAvail();
    // float ratios[4] = {0.3,0.3,0.3,0.3};
    // if (ImPlot::BeginSubplots("UserSubPlots", 3, 2, ImGui::GetContentRegionAvail(), ImPlotSubplotFlags_NoTitle, &ratios[0], &ratios[2])) {
    if (ImPlot::BeginSubplots("UserSubPlots", 5, 2, graph_size, ImPlotSubplotFlags_NoTitle)) {
        double hist_y_max = 0.25;
        double hist_bar_width = 5.0;
        int hist_bars = (histogram_latencies_downlink.getMax() / histogram_latencies_downlink.getBucketSize()) + 1; // add > max bar

        // Downlink
        if (ImPlot::BeginPlot("DLSCH Latency", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            double x_max = (double)(histogram_latencies_downlink.getMax() + histogram_latencies_downlink.getBucketSize());
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, x_max);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, hist_y_max);
            ImPlot::PlotBars("", bucket_x, buckets_list_downlink, hist_bars, hist_bar_width);
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("DLSCH Time (us)", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, graph_latency_downlink.MaxSize);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, histogram_latencies_downlink.getMax());
            ImPlot::PlotLine("", &graph_latency_downlink.DataX[0], &graph_latency_downlink.DataY[0], graph_latency_downlink.Idx);
            ImPlot::EndPlot();
        }

        // Uplink
        if (ImPlot::BeginPlot("Uplink Latency", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            double x_max = (double)(histogram_latencies_uplink.getMax() + histogram_latencies_uplink.getBucketSize());
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, x_max);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, hist_y_max);
            ImPlot::PlotBars("", bucket_x, buckets_list_uplink, hist_bars, hist_bar_width);
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("Uplink Time (us)", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, graph_latency_uplink.MaxSize);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, histogram_latencies_uplink.getMax());
            ImPlot::PlotLine("", &graph_latency_uplink.DataX[0], &graph_latency_uplink.DataY[0], graph_latency_uplink.Idx);
            ImPlot::EndPlot();
        }

        // SIB
        if (ImPlot::BeginPlot("SIB Latency", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            double x_max = (double)(histogram_latencies_sib.getMax() + histogram_latencies_sib.getBucketSize());
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, x_max);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, hist_y_max);
            ImPlot::PlotBars("", bucket_x, buckets_list_sib, hist_bars, hist_bar_width);
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("SIB Time (us)", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, graph_latency_sib.MaxSize);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, histogram_latencies_sib.getMax());
            ImPlot::PlotLine("", &graph_latency_sib.DataX[0], &graph_latency_sib.DataY[0], graph_latency_sib.Idx);
            ImPlot::EndPlot();
        }

        // PDCP DL
        if (ImPlot::BeginPlot("PDCP DL Latency", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            double x_max = (double)(histogram_latencies_pdcp_downlink.getMax() + histogram_latencies_pdcp_downlink.getBucketSize());
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, x_max);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, hist_y_max);
            ImPlot::PlotBars("", bucket_x, buckets_list_pdcp_downlink, hist_bars, hist_bar_width);
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("PDCP DL Time (us)", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, graph_latency_pdcp_downlink.MaxSize);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, histogram_latencies_pdcp_downlink.getMax());
            ImPlot::PlotLine("", &graph_latency_pdcp_downlink.DataX[0], &graph_latency_pdcp_downlink.DataY[0], graph_latency_pdcp_downlink.Idx);
            ImPlot::EndPlot();
        }

        // PDCP UL
        if (ImPlot::BeginPlot("PDCP UL Latency", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            double x_max = (double)(histogram_latencies_pdcp_uplink.getMax() + histogram_latencies_pdcp_uplink.getBucketSize());
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, x_max);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, hist_y_max);
            ImPlot::PlotBars("", bucket_x, buckets_list_pdcp_uplink, hist_bars, hist_bar_width);
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("PDCP UL Time (us)", graph_size, ImPlotFlags_NoLegend | ImPlotFlags_NoInputs)) {
            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, graph_latency_pdcp_uplink.MaxSize);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, histogram_latencies_pdcp_uplink.getMax());
            ImPlot::PlotLine("", &graph_latency_pdcp_uplink.DataX[0], &graph_latency_pdcp_uplink.DataY[0], graph_latency_pdcp_uplink.Idx);
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
    ImGui::End();
}

void log_open5gs_program(const char *bytes, size_t n)
{
    if ((n < 1) || bytes[0] == '\n')
        return;

    string output = string(bytes, n);
    vector<string> lines;

    if (string_contains(output, "Configuration update command")) {
        GL1G("[!] 1/2 UE connected to eNB/gNB");
        ModemManager.IndicateConnection();
    }

    if (string_contains(output, "F-SEID")) {
        GL1G(format("[!] 2/2 UE connected to PDN \"{}\"", StateMachine.config.services.ue_modem_manager.apn));
        if (StateMachine.config.services.ue_modem_manager.auto_reconnect_modem)
            ModemManager.RestartModemConnection();
    }

    if (string_contains(output, "MAC failure")) {
        GL1R("[!] UE MAC Authentication Failure");
        if (StateMachine.config.nr5_g.enable_simulator) {
            GL1M("[!] Restarting UE process...");
            ue_sim_conn_timeout->cancel();
            ue_sim_request_restart = true;
            OpenAirInterfaceUE.restart();
        }
    }

    strtk::parse(output, "\n", lines);

    for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i].size()) {
            gui_log7.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, lines[i]);
        }
    }
}

void log_oai_enb_program(const char *bytes, size_t n)
{
    static bool restart_requested = false;

    if ((n < 1) || bytes[0] == '\n' || bytes[0] == NULL)
        return;

    string output = string(bytes, n);

    if ((!restart_requested) &&
        // (string_contains(output, "unknown RNTI 0000"))) {
        (string_contains(output, "unknown RNTI 0000") ||
         string_contains(output, "Timeout waiting for sync"))) {
        restart_requested = true;
        gnb_ready = false;

        loop.onTimeoutMS(2000, [&]() {
            restart_requested = false;
        });

        GL1R("[!] gNB in bad state. Restarting Base Station...");
        OpenAirInterface.restart(true);
    }
    // else if (!gnb_ready && string_contains(output, "Number of bad PUCCH received: 0")) {
    else if (!gnb_ready && (string_contains(output, "tx_reorder_thread started") ||
                            string_contains(output, "Number of bad PUCCH received: 0"))) {
        gnb_ready = true;

        NRPacketLogger.writeLog("------------------- Base Station Process Started -------------------");
        GL1G("[Main] eNB/gNB started!");
        if (!ModemManager.ModemInitialized())
            GL1Y("[!] Waiting UE task to start...");

        ModemManager.RestartModemConnection();
    }

    vector<string> lines;
    strtk::parse(output, "\n", lines);

    for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i].size()) {
            gui_log8.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, lines[i]);
        }
    }
}

/**
 * @brief Parses and logs the output from the UE process
 *
 * @param bytes The raw output from the UE process
 * @param n The size of the raw output
 */
void log_oai_ue_program(const char *bytes, size_t n)
{
    if ((n < 1) || bytes[0] == '\n')
        return;

    string output = string(bytes, n);

    auto &config_5g = StateMachine.config.nr5_g;

    if (config_5g.enable_simulator) {
        if (string_contains(output, "Found RAR")) {
            GL1G("[UE] Found RAR. Connection Timeout: ", config_5g.simulation_connection_timeout_ms, " MS");
            if (!ue_sim_conn_timeout)
                ue_sim_conn_timeout = loop.onTimeoutMS(config_5g.simulation_connection_timeout_ms,
                                                       [&]() {
                                                           GL1Y("[UE] Restarting connection...");
                                                           ue_sim_request_restart = true;
                                                           OpenAirInterfaceUE.restart();
                                                       });

            ue_sim_conn_timeout->setMS(config_5g.simulation_connection_timeout_ms);
        }
    }

    vector<string> lines;
    strtk::parse(output, "\n", lines);

    for (string &line : lines) {
        if (line.size()) {
            gui_log9.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, line);
        }
    }
}

void log_modem_manager_program(string &output)
{
    gui_log11.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, output);
}

void log_monitor_program(const char *bytes, size_t n)
{
    if ((n < 1) || bytes[0] == '\n')
        return;

    string output = string(bytes, n);
    vector<string> lines;
    strtk::parse(output, "\n", lines);

    for (string &line : lines) {
        if (line.size()) {
            gui_log10.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, line);
        }
    }
}

void log_monitor_program_str(string line)
{
    gui_log10.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, line);
}

/**
 * @brief Fuzzes a packet by modifying a randomly chosen field in the packet
 *
 * This function randomly chooses a field in the packet specified by the `buf_data` parameter,
 * and mutates the field based on its type. The probability of mutation is determined by
 * the `default_mutation_probability` field in the `fuzzing` configuration object.
 *
 * @param buf_data A pointer to the packet data to be fuzzed
 * @param buf_size The size of the packet data
 * @return `true` if the packet was fuzzed, `false` otherwise
 */
static inline bool packet_fuzzing(wd_t *wd, uint8_t *pkt, int offset = 0, int *sent_dup_num = NULL)
{
    bool fuzz_layer = false;
    uint8_t fuzzed_fields = 0;
    uint8_t layer_number = 0;
    Fuzzing &fuzz = StateMachine.config.fuzzing;
    // const char *current_layer;

    // If mutation not enabled, return
    if ((!fuzz.enable_mutation) ||
        (StateMachine.CurrentExclude & Machine::EXCLUDE_MUTATION))
        return false;

    float r = ((float)g_random_int()) / ((float)UINT32_MAX);
    uint32_t decision_offset = StateMachine.GetCurrentStateGlobalOffset();
    bool unknown_state = (decision_offset >= WDFitness.x.size());

    if ((!fuzz.enable_optimization || unknown_state) &&
        (r > fuzz.default_mutation_probability))
        // For unknown states or disabled optimization
        return false;
    else if (fuzz.enable_optimization && !unknown_state &&
             (r > WDFitness.x[decision_offset]))
        // For known states
        return false;

    // struct timespec start_time;
    // struct timespec end_time;
    // clock_gettime(CLOCK_MONOTONIC, &start_time);
    packet_navigate_cpp(wd, 1, 1, [&](proto_tree *subnode, uint8_t field_type, uint8_t *pkt_buf) -> uint8_t {
        switch (field_type) {
        case WD_TYPE_FIELD: {
            // uint64_t mask = packet_read_field_bitmask(subnode->finfo);
            // printf("     Field: %s, Size=%d, Type=%s, Offset=%d, Mask=%02X, Bit=%d\n",
            //    subnode->finfo->hfinfo->name, subnode->finfo->length,
            //    subnode->finfo->value.ftype->name, packet_read_field_offset(subnode->finfo),
            //    mask, packet_read_field_bitmask_offset(mask));

            float r = ((float)rand()) / ((float)RAND_MAX);

            //  fuzz.default_mutation_field_probability * OAICommMAC.GetLayerWeigth(current_layer)
            // If optimization not enabled or unknown State, use default_mutation_field_probability
            if ((!fuzz.enable_optimization || unknown_state) &&
                (r > fuzz.default_mutation_field_probability))
                // For unknown states or disabled optimization
                break;

            else if (fuzz.enable_optimization &&
                     !fuzz_layer &&
                     (r > WDFitness.x[decision_offset + 1]))
                // For known states
                break;

            uint8_t r_value = (uint8_t)g_random_int_range(0, 255);
            pkt[packet_read_field_offset(subnode->finfo)] = r_value;
            fuzzed_fields++;
            // OAICommMAC.PacketWasFuzzed(current_layer);
            break;
        }
        case WD_TYPE_GROUP: {
            // printf("\033[36m"
            //        "---> Group: %s, Type=%s, Size=%d\n"
            //        "\033[00m",
            //        subnode->finfo->hfinfo->name, subnode->finfo->value.ftype->name,
            //        subnode->finfo->length);
            break;
        }
        case WD_TYPE_LAYER: {
            // current_layer = subnode->finfo->hfinfo->name;
            // printf("\033[33m"
            //        "==== Layer: %s, Type=%s, Size=%d\n"
            //        "\033[00m",
            //        subnode->finfo->hfinfo->name,
            //        subnode->finfo->value.ftype->name, subnode->finfo->length);

            if (!fuzz.enable_optimization || unknown_state)
                break;

            float r = ((float)rand()) / ((float)RAND_MAX);
            if (r <= WDFitness.x[StateMachine.GetCurrentStateGlobalOffset() + layer_number + 2])
                fuzz_layer = true;
            else
                fuzz_layer = false;

            ++layer_number;

            break;
        }
        default:
            break;
        }

        if (fuzzed_fields >= fuzz.max_fields_mutation)
            return 0;
        else
            return 1;
    });
    // clock_gettime(CLOCK_MONOTONIC, &end_time);
    // long measured_latency_us = ((end_time.tv_sec - start_time.tv_sec) * 1000000000UL) + (end_time.tv_nsec - start_time.tv_nsec) / 1000;
    // LOG2("time=", measured_latency_us);

    return (fuzzed_fields > 0);
}

template <class T>
static inline bool packet_duplication(T &Driver, uint8_t *pkt_buf, uint16_t pkt_len, int offset = 0, uint64_t *sent_dup_num = NULL)
{
    static int id = 0;
    bool ret = false;

    if (G_UNLIKELY(!StateMachine.config.fuzzing.enable_duplication))
        return false;

    if ((StateMachine.CurrentExclude & Machine::EXCLUDE_DUPLICATION))
        return false;

    double r = ((float)g_random_int()) / ((float)UINT32_MAX);

    if (r <= StateMachine.config.fuzzing.default_duplication_probability) {
        if (G_UNLIKELY(pkt_len < offset))
            return false;

        if (G_UNLIKELY((pkt_len - offset) > WD_SHM_MAX_BUFFER_SIZE)) {
            GL1R(format("[!] packet_duplication(len={} > {})", (pkt_len - offset), WD_SHM_MAX_BUFFER_SIZE));
            return false;
        }

        vector<uint8_t> pkt(pkt_buf + offset, pkt_buf + pkt_len - offset);

        if (dup_timeout_list.size() > 30) // Limit duplication
            return false;
        ret = true;
        // string dup_summary = wd_packet_summary(StateMachine.wd);
        int dup_time = g_random_int_range(1, StateMachine.config.fuzzing.max_duplication_time_ms);
        int dup_id = id;
        int pkt_n = counter_tx_mac +
                    counter_rx_mac +
                    counter_tx_mac_fuzzed +
                    NRPacketLogger.logs_count + 1;
        dup_timeout_list[dup_id] = loop.onTimeoutMS(dup_time,
                                                    [&Driver, dup_time, pkt, dup_id, pkt_n, sent_dup_num]() {
                                                        if (sent_dup_num)
                                                            *sent_dup_num = pkt_n;
                                                        Driver.send((uint8_t *)&pkt[0], pkt.size(), 0, pkt_n);
                                                        dup_timeout_list.erase(dup_id);
                                                    });
        id++;
        // LOG3Y("Dup in ", dup_time, " MS");
    }

    return ret;
}

void save_packet(uint8_t *pkt_buf, uint16_t pkt_len, bool fuzzed = false, bool duplicated = false, uint64_t duplication_id = 0)
{
    if (G_UNLIKELY((!StateMachine.config.options.save_protocol_capture) &&
                   (!StateMachine.config.options.live_protocol_capture)))
        return;

    memcpy(pcap_buffer.data(), pkt_buf, pkt_len);
    pcap_buffer.resize(pkt_len);

    // Update UDP header
    uint16_t udp_len = 8 + (sizeof("mac-nr") - 1) + pkt_len;
    // uint16_t udp_len = 8 + (sizeof("mac-lte") - 1) + pkt_len;
    uint16_t *udp_len_field = (uint16_t *)(pcap_buffer.header() + 38);
    *udp_len_field = htons(udp_len);
    // Update IP header
    uint16_t *ip_len = (uint16_t *)(pcap_buffer.header() + 16);
    *ip_len = htons(20 + udp_len);

    // Save packet to pcap file
    if (StateMachine.config.options.save_protocol_capture) {
        if (fuzzed)
            NRPacketLogger.write(pcap_buffer.header(), pcap_buffer.header_size() + pkt_len, "Fuzzed from previous");
        else if (duplicated)
            NRPacketLogger.write(pcap_buffer.header(), pcap_buffer.header_size() + pkt_len, format("Duplicated from {}", duplication_id).c_str());
        else
            NRPacketLogger.write(pcap_buffer.header(), pcap_buffer.header_size() + pkt_len);
    }

    // Save packet to fifo pcap file
    if (StateMachine.config.options.live_protocol_capture) {
        if (fuzzed)
            WSPacketLogger.write(pcap_buffer.header(), pcap_buffer.header_size() + pkt_len, "Fuzzed from previous");
        else if (duplicated)
            WSPacketLogger.write(pcap_buffer.header(), pcap_buffer.header_size() + pkt_len, format("Duplicated from {}", duplication_id).c_str());
        else
            WSPacketLogger.write(pcap_buffer.header(), pcap_buffer.header_size() + pkt_len);
    }
}
bool check_msg_list(vector<string> nextStateLst, string summary)
{

    for (string ele : nextStateLst) {
        if (ele.find(summary) != string::npos) {
            return true;
        }
    }
    return false;
}
// check each element in the list appears how many times
string check_attack_type(vector<string> msg_lst_check, uint32_t limit)
{
    bool tag = false;
    auto dic = std::map<string, int>();
    for (string ele : msg_lst_check) {
        if (dic.find(ele) == dic.end()) {
            dic[ele] = 1;
        }
        else {
            dic[ele]++;
        }
    }
    for (auto ele : dic) {
        if (ele.second > limit) {
            puts(ele.first.c_str());
            tag = true;
        }
    }
    if (tag) {

        return "Flooding Attack";
    }
    // else {
    //     return "Out of order Attack";
    // }
    return "";
}

void mutate_field(uint8_t *pkt_buffer, string field_name, uint64_t value)
{
    if (packet_has_condition(field_name.c_str())) {

        field_info *field_match = packet_get_field(field_name.c_str());
        if (field_match) {
            int offset = packet_read_field_offset(field_match);
            int field_size = packet_read_field_size(field_match);

            printf("Got offset %d, len=%d\n", offset, field_size);

            memcpy(pkt_buffer + offset, &value, field_size);
        }
    }
}

void handler_packet_events(pkt_evt_t &pkt_evt)
{
    static auto n_sample = folly::LegacyStatsClock<>::time_point(0s); // Indicate current sample number
    static bool allow_iteration = true;                               // Indicate that iteration started

    if (pkt_evt.pkt_save && pkt_evt.pkt_buf) {
        if (pkt_evt.pkt_fuzzed && pkt_evt.o_pkt_len)
            save_packet(pkt_evt.o_pkt_buf, pkt_evt.o_pkt_len);
        save_packet(pkt_evt.pkt_buf, pkt_evt.pkt_len,
                    pkt_evt.pkt_fuzzed, pkt_evt.pkt_duplicated, pkt_evt.pkt_duplicated_id);
    }

    Modules.RunRequestsHandler(pkt_evt.evt);

    // Process packet events
    switch (pkt_evt.evt) {
    // MAC DL
    case T_ENB_MAC_UE_DL_RAR_PDU_WITH_DATA:
    case T_ENB_MAC_UE_DL_PDU_WITH_DATA:
    case W_GNB_MAC_UE_DL_RAR_PDU_WITH_DATA:
    case W_GNB_MAC_UE_DL_PDU_WITH_DATA: {

        if (pkt_evt.pkt_fuzzed) {
            GL1M("[M] TX --> ", pkt_evt.pkt_summary);
            GL2M("TX --> ", pkt_evt.pkt_summary);
        }
        else if (pkt_evt.pkt_duplicated) {
            GL1Y(format("[D:{}] TX --> {}", pkt_evt.pkt_duplicated_id, pkt_evt.pkt_summary));
            GL2Y("TX --> ", pkt_evt.pkt_summary);
        }
        else
            GL2C("TX --> ", pkt_evt.pkt_summary);

        // Allow new Iteration
        if (!allow_iteration && string_contains(pkt_evt.pkt_summary, "RRC Setup"))
            allow_iteration = true;

        if (StateMachine.config.options.save_latency_metrics)
            gui_log0.add_msg(format("[MAC DL] {}", pkt_evt.time_latency));

        if (!gui_enabled)
            break;

        histogram_latencies_downlink.addValue(n_sample, pkt_evt.time_latency);
        graph_latency_downlink.AddPoint(pkt_evt.time_latency);
        break;
    }

    // MAC UL
    case T_ENB_PHY_INITIATE_RA_PROCEDURE:
    case T_ENB_MAC_UE_UL_PDU_WITH_DATA:
    case W_GNB_PHY_INITIATE_RA_PROCEDURE:
    case W_GNB_MAC_UE_UL_PDU_WITH_DATA: {

        GL2G("RX <-- ", pkt_evt.pkt_summary);

        GlobalTimeout.RestartTimeout(true);

        if (string_contains(pkt_evt.pkt_summary, "RRC Release")) {
            GL1Y("Session Ended (RRC Release Received)");
            ModemManager.StartModemConnection();
        }

        // Indicate new Iteration
        if (allow_iteration && string_contains(pkt_evt.pkt_summary, "RRC Setup Request")) {
            allow_iteration = false;
            WDFitness.Iteration(StateMachine.stats_transitions);
        }

        if (StateMachine.config.options.save_latency_metrics)
            gui_log0.add_msg(format("[MAC UL] {}", pkt_evt.time_latency));

        if (!gui_enabled)
            break;

        histogram_latencies_uplink.addValue(n_sample, pkt_evt.time_latency);
        graph_latency_uplink.AddPoint(pkt_evt.time_latency);
        break;
    }

    // MAC DL BCCH (Broadcast)
    case T_ENB_MAC_UE_DL_SIB:
    case W_GNB_MAC_UE_DL_SIB: {

        GL3C("TX --> ", pkt_evt.pkt_summary);

        if (!gui_enabled)
            break;

        histogram_latencies_sib.addValue(n_sample, pkt_evt.time_latency);
        graph_latency_sib.AddPoint(pkt_evt.time_latency);
        break;
    }

    case T_ENB_PHY_MIB:
    case W_GNB_PHY_MIB: {

        GL4C("TX --> ", pkt_evt.pkt_summary);
        break;
    }

    // PDCP DL
    case T_ENB_PDCP_PLAIN:
    case T_ENB_PDCP_ENC:
    case W_GNB_PDCP_PLAIN_DL:
    case W_GNB_PDCP_ENC_DL: {

        const char *plane_type_str = (pkt_evt.pkt_buf[1] == 2 ? "[ USER ] " : "[SIGNAL] ");
        const char *enc_plain_str = (pkt_evt.evt == W_GNB_PDCP_PLAIN_DL ? "PLAIN --> " : "ENC  --> ");

        if (!pkt_evt.pkt_fuzzed) {
            if (string_contains(pkt_evt.pkt_summary, "Failure"))
                GL6R(plane_type_str, enc_plain_str, pkt_evt.pkt_summary);
            else
                GL6C(plane_type_str, enc_plain_str, pkt_evt.pkt_summary);
        }
        else
            GL6M(plane_type_str, enc_plain_str, pkt_evt.pkt_summary);

        if (StateMachine.config.options.save_latency_metrics)
            gui_log0.add_msg(format("[PDCP DL] {}", pkt_evt.time_latency));

        if (!gui_enabled)
            break;

        histogram_latencies_pdcp_downlink.addValue(n_sample, pkt_evt.time_latency);
        graph_latency_pdcp_downlink.AddPoint(pkt_evt.time_latency);
        break;
    }

    // PDCP UL
    case W_GNB_PDCP_PLAIN_UL: {

        const char *plane_type_str = (pkt_evt.pkt_buf[1] == 2 ? "[ USER ] " : "[SIGNAL] ");

        if (string_contains(pkt_evt.pkt_summary, "Failure"))
            GL6R(plane_type_str, "PLAIN <-- ", pkt_evt.pkt_summary);
        else
            GL6G(plane_type_str, "PLAIN <-- ", pkt_evt.pkt_summary);

        if (StateMachine.config.options.save_latency_metrics)
            gui_log0.add_msg(format("[PDCP UL] {}", pkt_evt.time_latency));

        if (!gui_enabled)
            break;

        histogram_latencies_pdcp_uplink.addValue(n_sample, pkt_evt.time_latency);
        graph_latency_pdcp_uplink.AddPoint(pkt_evt.time_latency);
        break;
    }

    // NAS DL
    case WD_SHM_EVT_NAS_5GS_DL_ENC:
    case WD_SHM_EVT_NAS_5GS_DL_PLAIN: {
        if (string_contains(pkt_evt.pkt_summary, "Failure") ||
            string_contains(pkt_evt.pkt_summary, "reject")) {
            GL12R("TX --> ", pkt_evt.pkt_summary);
            GL1R("TX --> ", pkt_evt.pkt_summary);
        }
        else
            GL12C("TX --> ", pkt_evt.pkt_summary);

        if (StateMachine.config.options.save_latency_metrics)
            gui_log0.add_msg(format("[NAS DL] {}", pkt_evt.time_latency));

        break;
    }

    // NAS UL
    case WD_SHM_EVT_NAS_5GS_UL_ENC:
    case WD_SHM_EVT_NAS_5GS_UL_PLAIN: {
        if (string_contains(pkt_evt.pkt_summary, "Failure") ||
            string_contains(pkt_evt.pkt_summary, "reject"))
            GL12R("RX <-- ", pkt_evt.pkt_summary);
        else
            GL12G("RX <-- ", pkt_evt.pkt_summary);

        if (StateMachine.config.options.save_latency_metrics)
            gui_log0.add_msg(format("[NAS UL] {}", pkt_evt.time_latency));

        break;
    }

    default:
        break;
    }

    // Increment n_sample by 1 second
    n_sample += 1s;
    // Update GUI summary
    log_summary();
}

void handler_nas(wd_t *wd, WDEventQueue<pkt_evt_t> &PacketQueue, SHMDriver &OAICommSHM)
{
    while (true) {
        bool pkt_fuzzed = false;
        bool pkt_duplicated = false;
        pkt_evt_t pkt_evt = {0};
        // fmt::print("NAS recv 1: [{}] c_sync={}, s_sync={}\n", OAICommSHM._shm_channel, local_sync.client_mutex[OAICommSHM._shm_channel]->count, local_sync.server_mutex[OAICommSHM._shm_channel]->count);
        driver_event_shm_t evt = OAICommSHM.receive();
        // fmt::print("NAS recv 2: [{}] c_sync={}, s_sync={}\n", OAICommSHM._shm_channel, local_sync.client_mutex[OAICommSHM._shm_channel]->count, local_sync.server_mutex[OAICommSHM._shm_channel]->count);

        if (evt.data_size) {
            switch (evt.type) {
            case WD_SHM_EVT_NAS_5GS_DL_PLAIN: {

                profiling_timer_start(4);

                if (StateMachine.config.options.skip_packet_processing)
                    goto NAS_DL_INTERCEPT_COMPLETE;

                Modules.run_tx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);
                Modules.run_tx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

            NAS_DL_INTERCEPT_COMPLETE:
                OAICommSHM.intercept_tx();

                uint64_t processing_time = profiling_timer_end(4) / 1000;

                counter_tx_nas += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_TX;
                pkt_evt.time_latency = processing_time;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;
            }
            case WD_SHM_EVT_NAS_5GS_UL_PLAIN: {

                profiling_timer_start(5);

                if (StateMachine.config.options.skip_packet_processing)
                    goto NAS_UL_INTERCEPT_COMPLETE;

                Modules.run_rx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);
                Modules.run_rx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

            NAS_UL_INTERCEPT_COMPLETE:
                uint64_t processing_time = profiling_timer_end(5) / 1000;

                counter_rx_nas += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_RX;
                pkt_evt.time_latency = processing_time;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;
            }
            }
        }
    }
}

void handler_pdcp(wd_t *wd, WDEventQueue<pkt_evt_t> &PacketQueue, SHMDriver &OAICommSHM)
{
    while (true) {
        bool pkt_fuzzed = false;
        bool pkt_duplicated = false;
        pkt_evt_t pkt_evt = {0};
        // fmt::print("PDCP recv 1: [{}] c_sync={}, s_sync={}\n", OAICommSHM._shm_channel, local_sync.client_mutex[OAICommSHM._shm_channel]->count, local_sync.server_mutex[OAICommSHM._shm_channel]->count);
        driver_event_shm_t evt = OAICommSHM.receive();
        // fmt::print("PDCP recv 2: [{}] c_sync={}, s_sync={}\n", OAICommSHM._shm_channel, local_sync.client_mutex[OAICommSHM._shm_channel]->count, local_sync.server_mutex[OAICommSHM._shm_channel]->count);

        if (evt.data_size) {
            switch (evt.type) {
            // RX - Receive encrypted/plain text PDCP DL from base-station (Mutex 2)
            case W_GNB_PDCP_ENC_DL:
            case W_GNB_PDCP_PLAIN_DL: {

                profiling_timer_start(2);

                if (StateMachine.config.options.skip_packet_processing)
                    goto PDCP_DL_INTERCEPT_COMPLETE;

                Modules.run_tx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);
                Modules.run_tx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

            PDCP_DL_INTERCEPT_COMPLETE:
                OAICommSHM.intercept_tx();

                uint64_t processing_time = profiling_timer_end(2) / 1000;

                counter_tx_pdcp += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_TX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.time_latency = processing_time;
                pkt_evt.pkt_save = 0;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;
            }

            // RX - Receive plain text PDCP UL from UE (Mutex 3)
            case W_GNB_PDCP_PLAIN_UL: {

                profiling_timer_start(3);

                if (StateMachine.config.options.skip_packet_processing)
                    goto PDCP_UL_INTERCEPT_COMPLETE;

                Modules.run_rx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);
                Modules.run_rx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

            PDCP_UL_INTERCEPT_COMPLETE:
                uint64_t processing_time = profiling_timer_end(3) / 1000;

                counter_rx_pdcp += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_RX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.time_latency = processing_time;
                pkt_evt.pkt_save = 0;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;
            }

            default:
                break;
            }
        }
    }
}

void handler_mac(wd_t *wd, WDEventQueue<pkt_evt_t> &PacketQueue, SHMDriver &OAICommSHM)
{
    uint64_t dup_pkt_id = 0;

    while (true) {
        bool pkt_fuzzed = false;
        bool pkt_duplicated = false;
        pkt_evt_t pkt_evt = {0};
        driver_event_shm_t evt = OAICommSHM.receive();
        pkt_duplicated = evt.flags.duplicated;

        if (evt.data_size) {
            switch (evt.type) {
            case T_ENB_MAC_UE_DL_PDU_WITH_DATA:
            case W_GNB_MAC_UE_DL_PDU_WITH_DATA: {

                profiling_timer_start(0);

                if (StateMachine.config.options.skip_packet_processing)
                    goto MAC_DL_INTERCEPT_COMPLETE;

                if (!pkt_duplicated)
                    StateMachine.PrepareStateMapper(wd);
                StateMachine.PrepareExcludes(wd);

                Modules.run_tx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

                wd_set_packet_direction(wd, WD_DIR_TX);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);

                StateMachine.RunExcludes(wd);
                if (!pkt_duplicated)
                    StateMachine.RunStateMapper(wd, true);

                // Save original packet before any fuzzing procedure
                pkt_evt.save_packet(evt.data_buffer, evt.data_size);

                pkt_fuzzed = Modules.run_tx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                if (!pkt_duplicated)
                    pkt_fuzzed |= packet_fuzzing(wd, evt.data_buffer);

            MAC_DL_INTERCEPT_COMPLETE:
                OAICommSHM.intercept_tx(); // Notify end of pdu processing to gNB

                time_processing_dlsch = profiling_timer_end(0) / 1000;

                if (!pkt_duplicated && !pkt_fuzzed)
                    packet_duplication(OAICommSHM, evt.data_buffer, evt.data_size, 12);

                counter_tx_mac += 1;
                if (pkt_fuzzed)
                    counter_tx_mac_fuzzed += 1;
                if (pkt_duplicated)
                    counter_tx_mac_duplicated += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_TX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.pkt_duplicated = pkt_duplicated;
                pkt_evt.pkt_duplicated_id = evt.data_id;
                pkt_evt.time_latency = time_processing_dlsch;
                pkt_evt.pkt_save = 1;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;
            }
            case T_ENB_PHY_INITIATE_RA_PROCEDURE:
            case W_GNB_PHY_INITIATE_RA_PROCEDURE:

                profiling_timer_start(1);

                if (StateMachine.config.options.skip_packet_processing)
                    goto MAC_RA_INTERCEPT_COMPLETE;

                StateMachine.PrepareStateMapper(wd);

                wd_set_packet_direction(wd, WD_DIR_RX);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);

                StateMachine.RunStateMapper(wd, StateMachine.config.state_mapper.enable);

            MAC_RA_INTERCEPT_COMPLETE:
                time_processing_uplink = profiling_timer_end(1) / 1000;

                counter_rx_mac += 1;
                // TODO: RA handled on validation
                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_RX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.time_latency = time_processing_uplink;
                pkt_evt.pkt_save = 1;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;

            case T_ENB_MAC_UE_DL_RAR_PDU_WITH_DATA:
            case W_GNB_MAC_UE_DL_RAR_PDU_WITH_DATA:

                profiling_timer_start(0);

                if (StateMachine.config.options.skip_packet_processing)
                    goto MAC_DL_RAR_INTERCEPT_COMPLETE;

                if (!pkt_duplicated)
                    StateMachine.PrepareStateMapper(wd);
                StateMachine.PrepareExcludes(wd);

                Modules.run_tx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

                wd_set_packet_direction(wd, WD_DIR_TX);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);

                StateMachine.RunExcludes(wd);
                if (!pkt_duplicated)
                    StateMachine.RunStateMapper(wd, true);

                // Save original packet before any fuzzing procedure
                pkt_evt.save_packet(evt.data_buffer, evt.data_size);

                pkt_fuzzed = Modules.run_tx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

                if (!pkt_duplicated)
                    pkt_fuzzed |= packet_fuzzing(wd, evt.data_buffer);

            MAC_DL_RAR_INTERCEPT_COMPLETE:
                OAICommSHM.intercept_tx(); // Notify end of pdu processing

                time_processing_dlsch = profiling_timer_end(0) / 1000;

                counter_tx_mac += 1;
                if (pkt_fuzzed)
                    counter_tx_mac_fuzzed += 1;
                if (pkt_duplicated)
                    counter_tx_mac_duplicated += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_TX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.time_latency = time_processing_dlsch;
                pkt_evt.pkt_save = 1;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;

            case W_UE_UL_PDU_WITH_DATA: {

                profiling_timer_start(0);

                if (StateMachine.config.options.skip_packet_processing)
                    goto MAC_UE_UL_INTERCEPT_COMPLETE;

                wd_set_packet_direction(wd, WD_DIR_RX);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);

            MAC_UE_UL_INTERCEPT_COMPLETE:
                OAICommSHM.intercept_tx(); // Notify end of pdu processing
                break;
            }

            case T_ENB_MAC_UE_UL_PDU_WITH_DATA:
            case W_GNB_MAC_UE_UL_PDU_WITH_DATA: {

                profiling_timer_start(1);

                if (StateMachine.config.options.skip_packet_processing)
                    goto MAC_UL_INTERCEPT_COMPLETE;

                StateMachine.PrepareStateMapper();
                StateMachine.PrepareExcludes();

                wd_set_packet_direction(wd, WD_DIR_RX);

                Modules.run_rx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);
                Modules.run_rx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

                // Run excludes filter
                StateMachine.RunExcludes(wd);
                StateMachine.RunStateMapper(wd);

            MAC_UL_INTERCEPT_COMPLETE:
                time_processing_uplink = profiling_timer_end(1) / 1000;

                counter_rx_mac += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_RX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.pkt_duplicated = pkt_duplicated;
                pkt_evt.time_latency = time_processing_uplink;
                pkt_evt.pkt_save = 1;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;
            }
            case T_ENB_MAC_UE_DL_SIB:
            case W_GNB_MAC_UE_DL_SIB:

                profiling_timer_start(0);

                if (StateMachine.config.options.skip_packet_processing)
                    goto MAC_DL_SIB_INTERCEPT_COMPLETE;

                Modules.run_tx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);
                Modules.run_tx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

            MAC_DL_SIB_INTERCEPT_COMPLETE:
                OAICommSHM.intercept_tx(); // Notify end of pdu processing
                time_processing_sib = profiling_timer_end(0) / 1000;

                counter_tx_sib += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_TX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.time_latency = time_processing_sib;
                pkt_evt.pkt_save = 0;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;

            case T_ENB_PHY_MIB:
            case W_GNB_PHY_MIB:

                profiling_timer_start(0);

                if (StateMachine.config.options.skip_packet_processing)
                    goto MAC_DL_MIB_INTERCEPT_COMPLETE;

                Modules.run_tx_pre_dissection(evt.type, evt.data_buffer, evt.data_size, wd);
                wd_packet_dissect(wd, evt.data_buffer, evt.data_size);
                Modules.run_tx_post_dissection(evt.type, evt.data_buffer, evt.data_size, wd);

            MAC_DL_MIB_INTERCEPT_COMPLETE:
                OAICommSHM.intercept_tx(); // Notify end of pdu processing

                time_processing_mib = profiling_timer_end(0) / 1000;

                counter_tx_mib += 1;

                pkt_evt.evt = evt.type;
                pkt_evt.pkt_buf = evt.data_buffer;
                pkt_evt.pkt_len = evt.data_size;
                pkt_evt.pkt_dir = WD_DIR_TX;
                pkt_evt.pkt_fuzzed = pkt_fuzzed;
                pkt_evt.pkt_duplicated = pkt_duplicated;
                pkt_evt.time_latency = time_processing_mib;
                pkt_evt.pkt_save = 0;
                pkt_evt.set_summary(wd_packet_summary(wd));
                PacketQueue.PushEvent(pkt_evt);
                break;
            }
        }
    }
}

void ue_events_handler(mm_events evt)
{
    switch (evt) {
    case MM_EVT_MODEM_INITIALIZED:
    case MM_EVT_CLEAN_CONN_START:
    case MM_EVT_CLEAN_CONN_END:
    case MM_EVT_MODEM_REMOVED:
        GlobalTimeout.RestartTimeout();
        break;

    case MM_EVT_MODEM_READY:
        ModemManager.DisableEvent(MM_EVT_MODEM_READY);
        // Monitor external event
        // if (!QCMonitor.process_started)
        //     QCMonitor.init();
        break;

    case MM_EVT_MODEM_CONNECTING:
        break;

    case MM_EVT_MODEM_SURPRISE_REMOVED:
        GlobalTimeout.RestartTimeout();
        GL1R("[!] Hardware reset detected.");
        counter_crashes++;
        AnomalyReport.IndicateCrash(format("[Crash] Device Removed at state \"{}\"", StateMachine.GetCurrentStateName()));
        break;

    case MM_EVT_MODEM_INIT_FAILED:
        GL1R("[!] Modem Couldn't Initialize");
        AnomalyReport.IndicateTimeout("[Hang] Interface Error");
        break;

    case MM_EVT_MM_STARTED:
        // Log message to capture file
        NRPacketLogger.writeLog("ModemManager process started");
        break;

    case MM_EVT_MM_STOPPED:
        // Log message to capture file
        NRPacketLogger.writeLog("ModemManager process stopped");
        break;

    default:
        break;
    }
}

void process_module_requests(wd_modules_ctx_t *ctx, wd_module_request_t *req, int evt)
{
    switch (evt) {
    case W_GNB_MAC_UE_DL_RAR_PDU_WITH_DATA:
    case W_GNB_MAC_UE_DL_PDU_WITH_DATA:
    case W_GNB_MAC_UE_UL_PDU_WITH_DATA:
    case W_GNB_MAC_UE_DL_SIB:
    case W_GNB_PHY_MIB: {
        // Process modules request for MAC layer
        while (req->tx_count) {
            --req->tx_count;
            OAICommMAC_DL.send(req->pkt_buf, req->pkt_len);
        }

        if (req->stop) {
            kill(0, SIGUSR2);
        }
        else if (req->disconnect) {
            ModemManager.StopModemConnection();
            GL1Y(Modules.TAG, "Disconnection requested");
        }
        break;
    }

    // TODO: Handle PDCP and NAS module requests
    case W_GNB_PDCP_ENC_DL:
    case W_GNB_PDCP_PLAIN_DL:
    case W_GNB_PDCP_ENC_UL:
    case W_GNB_PDCP_PLAIN_UL: {
        /* code */
        break;
    }

    case WD_SHM_EVT_NAS_5GS_DL_PLAIN:
    case WD_SHM_EVT_NAS_5GS_DL_ENC:
    case WD_SHM_EVT_NAS_5GS_UL_PLAIN:
    case WD_SHM_EVT_NAS_5GS_UL_ENC: {
        /* code */
        break;
    }

    default:
        break;
    }
}

void monitor_crash_callback(bool is_timeout)
{
    GL1R("[!] UE crashed");
    counter_crashes += 1;
    AnomalyReport.IndicateCrash(format("[Crash] Crash detected at state \"{}\"", StateMachine.GetCurrentStateName()));
}

int main(int argc, char **argv)
{
    // Init GUI
    GUI_Init(argc, argv, false);
    gui_add_user_fcn(UserGraphs);
    log_summary();

    set_affinity_no_hyperthreading(true);

    // Initialize Global StateMachine
    if (!StateMachine.init(CONFIG_FILE_PATH))
        exit(-1);

    Config &conf = StateMachine.config;
    Options &options = conf.options;
    Fuzzing &fuzzing = conf.fuzzing;

    // Configure logs
    if (options.save_logs_to_file) {
        gui_log0.enableLogToFile(true, "logs/" + conf.name, "latency", options.main_thread_core);
        gui_log1.disableGUI(!gui_enabled);
        gui_log1.enableLogToFile(true, "logs/" + conf.name, "events", options.main_thread_core);
        // gui_log2.disableGUI(true);
        // gui_log2.enableLogToFile(true, "logs/" + conf.name, "session", options.main_thread_core);
        // gui_log7.enableLogToFile(true, "logs/" + conf.name, "open5gs", options.main_thread_core);
        // gui_log8.enableLogToFile(true, "logs/" + conf.name, "oai_gnb", options.main_thread_core);
        // gui_log9.enableLogToFile(true, "logs/" + conf.name, "oai_ue", options.main_thread_core);
        gui_log10.enableLogToFile(true, "logs/" + conf.name, "monitor", options.main_thread_core);
        gui_log11.enableLogToFile(true, "logs/" + conf.name, "modem_manager", options.main_thread_core);
    }

    set_wd_log_g([](const char *msg) {
        GL1G("[Modules] ", msg);
        NRPacketLogger.writeLog(msg);
    });

    set_wd_log_y([](const char *msg) {
        GL1Y("[Modules] ", msg);
        NRPacketLogger.writeLog(msg);
    });

    set_wd_log_r([](const char *msg) {
        GL1R("[Modules] ", msg);
        NRPacketLogger.writeLog(msg, true);
    });

    ParseArgs.SetProgramDescription("lte_fuzzer", "Fuzzer for 5G NR User Equipment (UE) - MAC-NR, PDCP-NR, RLC-NR, NAS-5GS");
    ParseArgs.AddArgs(Modules);
    // ParseArgs.AddArgs(OAICommMAC_DL);
    ParseArgs.Parse(argc, argv, {"NR5G", "Services/UEModemManager"});

    // ----- Configure extern modules prefix -----
    // Register MAC DL/UL exploits prefix
    Modules.RegisterGroupPrefix(W_GNB_MAC_UE_DL_RAR_PDU_WITH_DATA, "mac_sch_");
    Modules.RegisterGroupPrefix(W_GNB_MAC_UE_DL_PDU_WITH_DATA, "mac_sch_");
    Modules.RegisterGroupPrefix(W_GNB_MAC_UE_UL_PDU_WITH_DATA, "mac_sch_");
    Modules.RegisterGroupPrefix(W_GNB_MAC_UE_DL_SIB, "mac_bcch_");
    Modules.RegisterGroupPrefix(W_GNB_PHY_MIB, "mac_bcch_");
    // Register PDCP DL/UL exploits prefix
    Modules.RegisterGroupPrefix(W_GNB_PDCP_ENC_DL, "pdcp_");
    Modules.RegisterGroupPrefix(W_GNB_PDCP_PLAIN_DL, "pdcp_");
    Modules.RegisterGroupPrefix(W_GNB_PDCP_ENC_UL, "pdcp_");
    Modules.RegisterGroupPrefix(W_GNB_PDCP_PLAIN_UL, "pdcp_");
    // Register NAS DL/UL exploits prefix
    Modules.RegisterGroupPrefix(WD_SHM_EVT_NAS_5GS_DL_PLAIN, "nas_");
    Modules.RegisterGroupPrefix(WD_SHM_EVT_NAS_5GS_DL_ENC, "nas_");
    Modules.RegisterGroupPrefix(WD_SHM_EVT_NAS_5GS_UL_PLAIN, "nas_");
    Modules.RegisterGroupPrefix(WD_SHM_EVT_NAS_5GS_UL_ENC, "nas_");
    Modules.SetRequestsHandler(process_module_requests);
    // Compile / Load modules
    if (!Modules.init())
        exit(1);

    // Check if all modules use prefix
    if (!Modules.CheckAllModulesPrefix())
        exit(1);

    // Handle extras arguments
    ParseArgs.RunArgsCallback();

    GL1G("----------LTE Fuzzer----------");
    // Try to load program model
    GL1Y("Loading Model...");
    // Process pseudoheader when loading pcap files (set direction)
    StateMachine.OnProcessPseudoHeader([](uint8_t *pkt, uint32_t pkt_len, uint32_t &offset) {
        if (pkt_len >= 49 + 4) {
            // in mac-lte-framed, direction is on second byte of pseusoheader (+ 49 bytes of udp)
            // Set packet direction
            packet_set_direction(!(int)pkt[49 + 1]);
            // Return pseudoheader offset
            offset = 0;
        }
    });
    // Load .json model according to default program
    StateMachine.SetStateGlobalOffsetPadding(2);
    if (StateMachine.LoadModel("nr-softmodem.json", true)) {
        GL1G("Model Loaded!");
        StateMachine.PrintSummary();
    }
    else
        GL1R("Failed to load Model!");

    // Initialize Monitor
    if (MonitorTarget.setup(conf)) {
        MonitorTarget.SetCallback(log_monitor_program_str);
        MonitorTarget.SetCrashCallback(monitor_crash_callback);
        MonitorTarget.init(conf);
        MonitorTarget.printStatus();
    }

    // Start SHM Interface Driver
    if (!OAICommMAC_DL.init(WD_SHM_DEFAULT_PATH, WD_SHM_MUTEX_0, WD_SHM_SERVER, true))
        LOGR("SHM Driver OAICommMAC_DL failed to load"), exit(1);
    if (!OAICommMAC_UL.init(WD_SHM_DEFAULT_PATH, WD_SHM_MUTEX_1, WD_SHM_SERVER))
        LOGR("SHM Driver OAICommMAC_UL failed to load"), exit(1);
    if (!OAICommMAC_DL_BCCH.init(WD_SHM_DEFAULT_PATH, WD_SHM_MUTEX_2, WD_SHM_SERVER))
        LOGR("SHM Driver OAICommMAC_DL_BCCH failed to load"), exit(1);
    if (!OAICommPDCP_DL.init(WD_SHM_DEFAULT_PATH, WD_SHM_MUTEX_3, WD_SHM_SERVER))
        LOGR("SHM Driver OAICommPDCP_DL failed to load"), exit(1);
    if (!OAICommPDCP_UL.init(WD_SHM_DEFAULT_PATH, WD_SHM_MUTEX_4, WD_SHM_SERVER))
        LOGR("SHM Driver OAICommPDCP_UL failed to load"), exit(1);
    if (!Open5GSNAS_DL.init(WD_SHM_DEFAULT_PATH, WD_SHM_MUTEX_5, WD_SHM_SERVER))
        LOGR("SHM Driver Open5GSNAS_DL failed to load"), exit(1);
    if (!Open5GSNAS_UL.init(WD_SHM_DEFAULT_PATH, WD_SHM_MUTEX_6, WD_SHM_SERVER))
        LOGR("SHM Driver Open5GSNAS_UL failed to load"), exit(1);

    // Start Core Network
    if (conf.nr5_g.auto_start_core_network) {
        // Register subscribers to core network
        int ret = 0;
        for (auto &subscriber : conf.nr5_g.subscribers) {
            string add_subscriber_cmd;

            if (subscriber.opc != nullptr) {
                GL1(format("[Open5GS] Adding IMSI {} with K={}, OPC={}, APN={}",
                           subscriber.imsi,
                           subscriber.k,
                           *subscriber.opc,
                           subscriber.apn));

                add_subscriber_cmd = format("{} add_ue_with_apn {} {} {} {} > /dev/null 2>&1",
                                            SUBSCRIBERS_SCRIPT,
                                            subscriber.imsi,
                                            subscriber.k,
                                            *subscriber.opc,
                                            subscriber.apn);
            }
            else {
                GL1(format("[Open5GS] Adding IMSI {} with K={}, OP={}, APN={}",
                           subscriber.imsi,
                           subscriber.k,
                           *subscriber.op,
                           subscriber.apn));

                add_subscriber_cmd = format("{} add_ue_with_apn_op {} {} {} {} > /dev/null 2>&1",
                                            SUBSCRIBERS_SCRIPT,
                                            subscriber.imsi,
                                            subscriber.k,
                                            *subscriber.op,
                                            subscriber.apn);
            }

            string remove_subscriber_cmd = format("{} remove {} > /dev/null 2>&1",
                                                  SUBSCRIBERS_SCRIPT,
                                                  subscriber.imsi);
            ProcessExec(remove_subscriber_cmd.c_str());     // Remove imsi
            ret |= ProcessExec(add_subscriber_cmd.c_str()); // Add imsi
        }

        if (!ret)
            GL1G("[Open5GS] Subscribers registered to core network: ", conf.nr5_g.subscribers.size());
        else
            GL1R("[Open5GS] Error while registering subscribers");

        // Update MCC and MNC in the core network config files
        ProcessExec(format("sed -i \"s/mcc:.*/mcc: {}/g\" ./configs/5gnr_gnb/{}",
                           conf.nr5_g.mcc, conf.nr5_g.core_network_config_file)
                        .c_str());
        ProcessExec(format("sed -i \"s/mnc:.*/mnc: {}/g\" ./configs/5gnr_gnb/{}",
                           conf.nr5_g.mnc, conf.nr5_g.core_network_config_file)
                        .c_str());

        ret = ProcessExec("sysctl -w net.ipv4.ip_forward=1"); // Enable IPV4 forwarding
        if (ret)
            GL1R("Error: sysctl -w net.ipv4.ip_forward=1 failed");
        // Uptable iptables rules
        // 1. iptables need to accept interface input
        ret |= ProcessExec(IPTABLES " -A INPUT -i ogstun -j ACCEPT", true);
        // 2. iptables need to accept forwarding packet to and from interface respectivelly
        ret |= ProcessExec(IPTABLES " -P FORWARD ACCEPT");
        ret |= ProcessExec(IPTABLES " -A FORWARD ! -i ogstun -o ogstun -j ACCEPT", true);
        ret |= ProcessExec(IPTABLES " -A FORWARD -i ogstun ! -o ogstun -j ACCEPT", true);
        // 3. iptables need to masquerade packets going from interface to external addresses
        ret |= ProcessExec(IPTABLES " -t nat -A POSTROUTING -s 45.45.0.0/16 ! -o ogstun -j MASQUERADE", true);
        if (ret)
            GL1R("iptables failed");

        Open5GSProcess.SetStartCallback([]() {
            ProcessExec("pkill -f -9 open5gs");
            usleep(100000UL);                                        // Sleep for 100ms
            ProcessExec("./3rd-party/open5gs-core/misc/netconf.sh"); // Configure oaitun interface
        });
        Open5GSProcess.SetStopCallback([]() {
            GL1R("[!] Open5GS stopped");
            // Log message to capture file
            NRPacketLogger.writeLog("Open5GS process stopped");
            if (!SignalHandler.Exiting) {
                Open5GSNAS_DL.Resync();
                Open5GSNAS_UL.Resync();
                OpenAirInterface.restart();
            }
        });
        Open5GSProcess.init(conf.options.programs_list[0],
                            "-c configs/5gnr_gnb/" +
                                conf.nr5_g.core_network_config_file, // Argument
                            log_open5gs_program,                     // Stdout/err callback
                            false,                                   // Change working dir
                            true);                                   // Auto restart upon exit
    }
    else {
        GL1R("Open5GS (Core Network) not autostarted!");
    }

    // Configure OAI (eNB/gNB)
    string config_file = (conf.nr5_g.enable_simulator ? "rfsim.n78.106.conf" : conf.nr5_g.base_station_config_file);
    string prg_name = (conf.options.launch_program_with_gdb ? "/usr/bin/gdb" : conf.options.programs_list[1]);
    string prg_args = "";

    char *current_dir = g_get_current_dir();

    if (conf.options.launch_program_with_gdb)
        prg_args += "-q -ex='set cwd " + GetPathDirName(conf.options.programs_list[1]) + "' -ex=r -ex=bt --args " + current_dir + conf.options.programs_list[1] + " ";

    // Main OAI arguments
    // prg_args += "--sa --continuous-tx -E"
    prg_args += conf.nr5_g.base_station_arguments;
    prg_args += " -O "s + current_dir + "/configs/5gnr_gnb/" + config_file;

    if (conf.nr5_g.enable_simulator) {
        g_setenv("BASICSIM_DELAY_US", std::to_string(conf.nr5_g.simulator_delay_us).c_str(), NULL);
        prg_args += " --rfsim";
    }

    // Update MCC and MNC in the base-station config files
    ProcessExec(format("sed -i \"s/mcc.*/mcc = {}/g\" ./configs/5gnr_gnb/{}",
                       conf.nr5_g.mcc, config_file)
                    .c_str());
    ProcessExec(format("sed -i \"s/mnc[^_].*/mnc = {}/g\" ./configs/5gnr_gnb/{}",
                       conf.nr5_g.mnc, config_file)
                    .c_str());

    // Update MCC and MNC in the core-network config files
    ProcessExec(format("sed -i \"s/mnc_length.*/mnc_length = {}/g\" ./configs/5gnr_gnb/{}",
                       conf.nr5_g.mnc.size(), conf.nr5_g.base_station_config_file)
                    .c_str());

    if (conf.nr5_g.auto_start_base_station) {
        OpenAirInterface.SetStartCallback([&]() {
            g_setenv("WD_SHM", "1", NULL);
        });
        OpenAirInterface.SetStopCallback([]() {
            gnb_ready = false;

            GL1R("[!] Base-Station process stopped");

            if (!SignalHandler.Exiting) {
                ModemManager.StopModemConnection();
                NRPacketLogger.writeLog("OAI process stopped");
                OAICommMAC_DL.Resync();
                OAICommMAC_DL_BCCH.Resync();
                OAICommMAC_UL.Resync();
                OAICommPDCP_DL.Resync();
                OAICommPDCP_UL.Resync();
            }
        });
        ProcessExec("pkill -3 nr-softmodem"); // TODO: improve
        OpenAirInterface.init(prg_name, prg_args, log_oai_enb_program, true, true, 1000);
    }
    else {
        GL1R("OpenAirInterface (eNB/gNB) not autostarted!");
    }

    // Start OAI UE Simulator
    if (conf.nr5_g.enable_simulator) {
        GL1M("[!] Simulation Enabled, disabling ModemManager and HubCtrl. Remember to enabled them later!");
        usleep(1 * 1e6);
        conf.services.ue_modem_manager.enable = false;
        conf.services.usb_hub_control.enable = false;

        GL1Y("Starting OAI UE Simulator (RFSIM)");

        string prg_name = (conf.options.launch_program_with_gdb ? "/usr/bin/gdb" : conf.options.programs_list[2]);
        string prg_args = (conf.options.launch_program_with_gdb ? "-q -ex='set cwd " + GetPathDirName(conf.options.programs_list[2]) +
                                                                      "' -ex=r -ex=bt --args " + conf.options.programs_list[2] + " "
                                                                : "");
        prg_args += conf.nr5_g.simulator_ue_arguments +
                    " -O " + current_dir + "/configs/5gnr_gnb/ue.conf";

        OpenAirInterfaceUE.SetStartCallback([&]() {
            g_unsetenv("WD_SHM");
            NRPacketLogger.writeLog("UE process started");
            GL1G("[!] UE process started");
        });
        OpenAirInterfaceUE.SetStopCallback([]() {
            GL1R("[!] UE process stopped");

            if (!ue_sim_request_restart) {
                // Process stopped without request
                GL1R("[!] UE process crashed");
                counter_crashes++;
                AnomalyReport.IndicateCrash(format("[Crash] Service stopped at state \"{}\"", StateMachine.GetCurrentStateName()));
            }
            ue_sim_request_restart = false;
        });

        ProcessExec("pkill -3 nr-uesoftmodem"); // TODO: improve
        OpenAirInterfaceUE.init(prg_name, prg_args, log_oai_ue_program, true, true, 100);
    }

    g_free(current_dir);

    // TODO: Move QCMonitor to monitors folder
    QCMonitor.setup("./modules/python/install/bin/python3",
                    "modules/QCSuper/qcsuper.py --usb-modem " +
                        conf.monitor.qcdm.qcdm_device +
                        " --diag-messages --pcap-dump logs/5gnr_gnb/ue_capture.pcap",
                    log_monitor_program,
                    false, true, 1000);

    // Initialize PCAP buffer for packet logging
    pcap_buffer.reserve(16535);
    memcpy(pcap_buffer.data(), pcap_mac_nr_udp_hdr, sizeof(pcap_mac_nr_udp_hdr));
    // memcpy(pcap_buffer.data(), pcap_mac_lte_udp_hdr, sizeof(pcap_mac_lte_udp_hdr));
    pcap_buffer.apply_offset(sizeof(pcap_mac_nr_udp_hdr));
    // pcap_buffer.apply_offset(sizeof(pcap_mac_lte_udp_hdr));

    NRPacketLogger.init(CAPTURE_FILE, CAPTURE_LINKTYPE, false, false, &StateMachine.config.options.save_protocol_capture);
    NRPacketLogger.SetPostLogCallback([&](const string msg, bool error) {
        if (conf.options.live_protocol_capture)
            WSPacketLogger.writeLog(msg, error);
    });

    if (conf.options.live_protocol_capture) {
        start_live_capture();
    }

    // Configure signal handler for this process
    SignalHandler.CallStop(OpenAirInterface);
    SignalHandler.CallStop(Open5GSProcess);
    SignalHandler.CallStop(QCMonitor);
    SignalHandler.CallStop(ModemManager);
    SignalHandler.CallStop(NRPacketLogger);
    SignalHandler.CallStop(WSPacketLogger);
    SignalHandler.CallStop(StateMachine);
    SignalHandler.CallStop(PacketHandler);
    SignalHandler.init();

    // Configure GlobalTimeout
    GlobalTimeout.CallIndicateTimeout(AnomalyReport);
    GlobalTimeout.CallRestart(OpenAirInterface);
    if (conf.nr5_g.enable_simulator)
        GlobalTimeout.CallRestart(OpenAirInterfaceUE);
    GlobalTimeout.CallRestart(Open5GSProcess);
    GlobalTimeout.CallIndicateTimeout(USBHubControl);
    GlobalTimeout.CallIndicateTimeout(ModemManager);
    GlobalTimeout.AddTimeoutCallback([]() {
        WDFitness.Iteration(StateMachine.stats_transitions);
    });
    GlobalTimeout.init();

    // Configure logger for anomaly report
    AnomalyReport.AddLogSink(NRPacketLogger);
    AnomalyReport.AddLogSink(ReportSender, true);

    // Configure USBHubControl
    USBHubControl.CallIndicateTargetReset(ModemManager);
    USBHubControl.CallIndicateTargetReset(AnomalyReport);
    USBHubControl.init();

    // Configure TShark
    Tshark.init();

    // Setup Modem Manager program
    ModemManager.SetEventsCallback(ue_events_handler);
    ModemManager.SetLogCallback(log_modem_manager_program);
    ModemManager.SetCommandsLogger(NRPacketLogger);
    ModemManager.init();

    // Init fitness engine and set default iteration callback
    WDFitness.SetDefaultCallbacks(NRPacketLogger);
    WDFitness.init(StateMachine.TotalStatesLayers(), 5);

    // Configure Main Packet Handlers
    PacketHandler.AddPacketHandlerWithDriver<SHMDriver>(Open5GSNAS_DL, handler_nas, WD_DIR_DL, WD_MODE_FAST, false, "proto:nas-5gs");
    PacketHandler.AddPacketHandlerWithDriver<SHMDriver>(Open5GSNAS_UL, handler_nas, WD_DIR_UL, WD_MODE_FAST, false, "proto:nas-5gs");
    PacketHandler.AddPacketHandlerWithDriver<SHMDriver>(OAICommPDCP_DL, handler_pdcp, WD_DIR_DL, WD_MODE_FAST, true, "proto:pdcp-nr-framed");
    PacketHandler.AddPacketHandlerWithDriver<SHMDriver>(OAICommPDCP_UL, handler_pdcp, WD_DIR_UL, WD_MODE_FAST, true, "proto:pdcp-nr-framed");
    PacketHandler.AddPacketHandlerWithDriver<SHMDriver>(OAICommMAC_DL, handler_mac, WD_DIR_DL, WD_MODE_NORMAL, true);
    PacketHandler.AddPacketHandlerWithDriver<SHMDriver>(OAICommMAC_DL_BCCH, handler_mac, WD_DIR_DL, WD_MODE_FAST, true);
    PacketHandler.AddPacketHandlerWithDriver<SHMDriver>(OAICommMAC_UL, handler_mac, WD_DIR_UL, WD_MODE_FAST, false);

    // Set Packet Events Handler
    PacketHandler.SetPacketEventsHandler(handler_packet_events);

    if (!conf.fuzzing.enable_mutation && !conf.fuzzing.enable_duplication)
        GL1R("[Main] Fuzzing not enabled! Running only target reconnection");

    // Run main fuzzing packet threads
    PacketHandler.Run();

    return 0;
}