#pragma once

#ifndef __SVCMODEMMANAGER__
#define __SVCMODEMMANAGER__

#include <unordered_map>

#include "libs/folly/folly/concurrency/UnboundedQueue.h"

#include "Framework.hpp"
#include "Machine.hpp"
#include "MiscUtils.hpp"
#include "Process.hpp"

#define MODEM_MANAGER "./3rd-party/ModemManager/runtime/sbin/ModemManager"
#define MMCLI "./3rd-party/ModemManager/runtime/bin/mmcli"
#define QMICLI "./3rd-party/ModemManager/runtime/bin/qmicli"
#define ADB_CLI "./3rd-party/adb/adb"

enum mm_events {
    MM_EVT_MODEM_INITIALIZED = 0,
    MM_EVT_MODEM_GOT_PATH,
    MM_EVT_MODEM_READY,
    MM_EVT_MODEM_REMOVED,
    MM_EVT_MODEM_SURPRISE_REMOVED,
    MM_EVT_MODEM_INIT_FAILED,
    MM_EVT_MODEM_RESET_FAILED,
    MM_EVT_MODEM_DISCONNECTING,
    MM_EVT_MODEM_CONNECTING,
    MM_EVT_MODEM_RESET_REQUESTED,
    MM_EVT_MM_RESTART_REQUESTED,
    MM_EVT_MM_STOPPED,
    MM_EVT_MM_STARTED,
    MM_EVT_CLEAN_CONN_START,
    MM_EVT_CLEAN_CONN_END,
};

class SvcModemManager {

private:
    ProcessRunner ModemManagerProc;

    unique_ptr<thread> event_thread;
    MainLoop mm_loop;
    bool request_modem_reset = false;
    bool already_autoconnected = false;

    shared_ptr<React::TimeoutWatcher> reconnection_timer = nullptr;

    function<void(string &)> callback_log = nullptr;
    function<void(mm_events)> callback_events = nullptr;
    function<void(string, bool)> callback_msg_logger_cmd = nullptr;
    vector<function<void(mm_events)>> callback_events_specific;

    unordered_map<mm_events, bool> DisabledEvents;

    pthread_mutex_t mutex_restart = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_task = PTHREAD_MUTEX_INITIALIZER;

    bool adb_waiting_device = false;
    bool adb_wait_coldboot = true;
    bool adb_monitor_running = false;

    void log_modem_manager(const char *bytes, size_t n)
    {
        if ((n < 1) || bytes[0] == '\n')
            return;

        string output = string(bytes, n);

        if (!ue_modem_initialized && string_contains(output, "modem initialized")) {
            ue_modem_initialized = true;
            ue_dev_attached = true;
            request_modem_reset = false;
            already_autoconnected = false;
            GL1G(TAG, "Modem Initialized");
            mm_loop.RunTask([&] {
                callback_events(MM_EVT_MODEM_INITIALIZED);
                ConfigureModemSync();
            });
        }
        else if (!ue_modem_initialized && string_contains(output, "No card found")) {
            GL1R(TAG, "SIM Card not found!");
        }
        else if (!ue_modem_initialized && string_contains(output, "modem couldn't be initialized")) {
            GL1R(TAG, "Error: Modem can't be initialized");
            callback_events(MM_EVT_MODEM_INIT_FAILED);
        }
        else if (ue_dev_attached && string_contains(output, "removing empty device")) {
            ue_dev_attached = false;
            ue_modem_initialized = false;
            mm_loop.RunTask([&] {
                if (request_modem_reset) {
                    // Indicate modem removal upon request
                    GL1C(TAG, "Controlled Modem Removal");
                    callback_events(MM_EVT_MODEM_REMOVED);
                }
                else {
                    // Indicate non-solicited modem removal
                    GL1R(TAG, "Modem Removed!");
                    callback_events(MM_EVT_MODEM_SURPRISE_REMOVED);
                }
            });
        }

        vector<string> lines;
        strtk::parse(output, "\n", lines);

        for (string &line : lines) {

            if (G_UNLIKELY(!line.size()))
                continue;

            if ((!ue_dev_attached) &&
                StateMachine.config.services.ue_modem_manager.auto_search_modem_interface_path &&
                string_contains(line, "/dev/cdc-wdm")) {
                ue_dev_attached = true;
                ue_dev_path = "/dev/" + string_split(string_split(line, "/").back(), "]")[0];
                GL1G(TAG, "Got Modem at ", ue_dev_path);
                mm_loop.RunTask([&] {
                    callback_events(MM_EVT_MODEM_GOT_PATH);
                });
            }

            if (callback_log)
                callback_log(line);
        }
    }

    void LogCommand(string cmd, bool error = false)
    {
        if (!error)
            GL1C(TAG, "UE Command: ", cmd);
        else
            GL1R(TAG, "UE Command: ", cmd);

        if (callback_msg_logger_cmd)
            callback_msg_logger_cmd(cmd, error);
    }

    int RunADBCmd(string cmd, bool log_cmd = false)
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (log_cmd)
            LogCommand(cmd);

        if (MM.adb_device.size())
            return ProcessExec(format("{} -s {} shell \"{}\"", ADB_CLI, MM.adb_device, cmd));
        else
            return ProcessExec(format("{} shell \"{}\"", ADB_CLI, cmd));
    }

    string RunADBCmdGetResult(string cmd, bool log_cmd = false)
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (log_cmd)
            LogCommand(cmd);

        if (MM.adb_device.size())
            return ProcessExecGetResult(format("{} -s {} shell \"{}\"", ADB_CLI, MM.adb_device, cmd));
        else
            return ProcessExecGetResult(format("{} shell \"{}\"", ADB_CLI, cmd));
    }

    int RunADBReboot(bool log_cmd = false)
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (log_cmd)
            LogCommand("adb reboot");

        if (MM.adb_device.size())
            return ProcessExec(format("{} -s {} reboot", ADB_CLI, MM.adb_device));
        else
            return ProcessExec(format("{} reboot", ADB_CLI));
    }

    int RunADBDevices(bool log_cmd = false)
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (log_cmd)
            LogCommand("list adb devices");

        string res = ProcessExecGetResult(format("{} devices", ADB_CLI));

        if (MM.adb_device.size() && string_contains(res, MM.adb_device)) {
            return 0;
        }

        return -1;
    }

public:
    const char *TAG = "[ModemManager] ";

    bool initialized = false;
    bool ue_modem_initialized = false;
    bool ue_connected = false; // TODO
    bool ue_dev_attached = false;
    bool stop_request = false;
    string ue_dev_path;

    bool init()
    {
        if (initialized)
            return true;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!MM.enable) {
            GL1M(TAG, "ModemManager not started!");
            return true;
        }

        // Configure default event callback
        callback_events = [this](mm_events evt) {
            if (stop_request)
                return;

            // Check if event is not disabled
            if (DisabledEvents.find(evt) != DisabledEvents.end()) {
                if (DisabledEvents[evt] == true)
                    return;
            }

            // Execute user events callback only if event is not disabled
            for (auto &s_fcn : callback_events_specific)
                s_fcn(evt);
        };

        ue_dev_path = MM.default_modem_interface_path;

        ModemManagerProc.SetStartCallback([&]() {
            system("pkill -f -3 ModemManager");

            callback_events(MM_EVT_MM_STARTED);
            stop_request = false;
        });
        ModemManagerProc.SetStopCallback([&]() {
            ue_modem_initialized = false;
            ue_connected = false;
            ue_dev_attached = false;

            GL1R(TAG, "Process stopped");

            if (stop_request)
                return;

            callback_events(MM_EVT_MM_STOPPED);
        });

        // Event Loop
        folly::UMPSCQueue<int, true> wait_thread;
        event_thread = make_unique<thread>([&]() {
            pthread_setname_np(pthread_self(), "modem_manager_event_loop");
            enable_idle_scheduler();
            mm_loop.RunTask([&]() {
                wait_thread.enqueue(1);
            });
            mm_loop.run(true);
        });
        event_thread->detach();
        int res;
        wait_thread.dequeue(res);

        if (MM.enable_adb) {
            GL1G(TAG, "Configuring ADB Device");
            ConfigureADB();
            return true;
        }

        return ModemManagerProc.init(
            MODEM_MANAGER,
            "--debug",
            [&](const char *bytes, size_t n) {
                if (G_UNLIKELY(stop_request))
                    return;

                log_modem_manager(bytes, n);
            },
            true,
            true,
            1000);
    }

    void SetLogCallback(function<void(string &line)> fcn)
    {
        callback_log = fcn;
    }

    void SetEventsCallback(function<void(mm_events)> fcn)
    {
        callback_events_specific.push_back([this, fcn](mm_events evt) {
            fcn(evt);
        });
    }

    void SetEventsCallback(mm_events requested_evt, function<void(void)> fcn)
    {
        callback_events_specific.push_back([this, requested_evt, fcn](mm_events evt) {
            if (evt == requested_evt)
                fcn();
        });
    }

    void Restart()
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (MM.enable_adb)
            return;

        ue_dev_attached = false;
        ue_modem_initialized = false;
        ue_connected = false;

        callback_events(MM_EVT_MM_RESTART_REQUESTED);
        ModemManagerProc.restart();
    }

    void ConfigureModem(bool wait_cmd_to_finish = false)
    {
        if (wait_cmd_to_finish)
            ConfigureModemSync();
        else
            mm_loop.RunTask([&]() {
                ConfigureModemSync();
            });
    }

    void ConfigureADB(bool wait_cmd_to_finish = false)
    {
        if (wait_cmd_to_finish)
            ConfigureADBSync();
        else
            mm_loop.RunTask([&]() {
                ConfigureADBSync();
            });
    }

    void ConfigureADBSync()
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!MM.enable)
            return;

        int total_tasks = 11;
        int current_task = 0;
        bool device_found = !RunADBDevices(true);

        if (!device_found) {
            GL1R(TAG, "ADB Device ", MM.adb_device, " not found!");
            return;
        }

        RunADBCmd("input keyevent 82"); // Wakeup Screen
        this_thread::sleep_for(500ms);

        GL1G(TAG, format("[{}/{}] Checking if device is locked", ++current_task, total_tasks));
        bool is_locked = string_contains(RunADBCmdGetResult("dumpsys deviceidle | grep mScreenLocked"),
                                         "mScreenLocked=true");

        if (is_locked && MM.adbpin.size()) {

            GL1G(TAG, format("[{}/{}] Unlocking device via ADB PIN", ++current_task, total_tasks));
            RunADBCmd(format("input text {}", MM.adbpin)); // Wakeup Screen
            this_thread::sleep_for(200ms);
            RunADBCmd("input keyevent 62");
            this_thread::sleep_for(100ms);
        }
        else
            GL1G(TAG, format("[{}/{}] Device already unlocked", ++current_task, total_tasks));

        if (adb_wait_coldboot) {
            GL1G(TAG, format("[{}/{}] Waiting Device startup... (10s)", ++current_task, total_tasks));
            adb_wait_coldboot = false;
            this_thread::sleep_for(10s);
        }

        GL1G(TAG, format("[{}/{}] Setting display to keep turned ON", ++current_task, total_tasks));
        RunADBCmd("svc power stayon usb"); // Keep display ON

        GL1G(TAG, format("[{}/{}] Go to data roaming settings", ++current_task, total_tasks));
        RunADBCmd("am start -a android.settings.DATA_ROAMING_SETTINGS"); // Goto Roaming options

        GL1G(TAG, format("[{}/{}] Disabling Bluetooth", ++current_task, total_tasks));
        RunADBCmd("svc bluetooth disable"); // Disable Bluetooth
        GL1G(TAG, format("[{}/{}] Disabling Wi-Fi", ++current_task, total_tasks));
        RunADBCmd("svc wifi disable"); // Disable Wi-Fi

        this_thread::sleep_for(1s);

        if (MM.auto_connect_to_apn) {
            GL1G(TAG, format("[{}/{}] Enabling SIM data", ++current_task, total_tasks));
            RunADBCmd("svc data enable");
        }
        else {
            GL1G(TAG, format("[{}/{}] Disabling SIM data", ++current_task, total_tasks));
            RunADBCmd("svc data disable");
        }

        GL1G(TAG, format("[{}/{}] Enabling SIM data roaming", ++current_task, total_tasks));
        for (int i = 1; i <= 6; i++) {
            RunADBCmd(format("settings put global data_roaming{} 1", i)); // Enable Roaming on SIM1-6
        }

        GL1G(TAG, format("[{}/{}] Enable Airplane mode", ++current_task, total_tasks));
        RunADBCmd("cmd connectivity airplane-mode enable"); // Enable Airplane mode
        GL1G(TAG, format("[{}/{}] Clearing logs (logcat)", ++current_task, total_tasks));
        RunADBCmd("logcat -c"); // Clean logcat logs
        // adb logcat -c
        this_thread::sleep_for(1s);

        GL1G(TAG, "Modem Configured");
        callback_events(MM_EVT_MODEM_INITIALIZED);

        ue_modem_initialized = true;

        if (MM.auto_connect_modem) {
            AutoConnectModem();
        }
        else {
            callback_events(MM_EVT_MODEM_READY);
            GL1G(TAG, "Modem configured and ready to accept commands!");
        }

        if (!adb_monitor_running) {
            adb_monitor_running = true;
            // Enable ADB monitoring runner
            mm_loop.onInterval(1, [&]() {
                ADBCheckConnection();
                return true;
            });
        }
    }

    void ConfigureModemSync()
    {
        int total_tasks = 7;
        int current_task = 0;
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!MM.enable)
            return;

        GL1Y(TAG, "Configuring Modem...");

        GL1(TAG, format("[{}/{}] Disconnecting Modem from any base-station", ++current_task, total_tasks));
        ATDisconnectBaseStation(true);

        GL1(TAG, format("[{}/{}] Enabling Modem", ++current_task, total_tasks));
        ProcessExec(MMCLI " -m any --enable", false, 10.0);

        this_thread::sleep_for(chrono::seconds(2));

        GL1(TAG, format("[{}/{}] --set--allowed-modes=\"{}\" --set-preferred-mode=\"{}\"", ++current_task, total_tasks, MM.allowed_modes, MM.preferred_mode));
        ProcessExec(format(MMCLI " -m any --set-allowed-modes \"{}\"  --set-preferred-mode \"{}\"", MM.allowed_modes, MM.preferred_mode));
        ProcessExec(format(MMCLI " -m any --set-current-bands=\"{}\"", MM.bands));

        GL1(TAG, format("[{}/{}] Deleting unused APNs", ++current_task, total_tasks));
        // ----- QMI only -----
        // Delete extra APN profiles
        for (size_t i = 2; i < 10; i++) {
            ProcessExec(format(QMICLI " -d {} -p --wds-delete-profile=\"3gpp,{}\" > /dev/null 2>&1", ue_dev_path, i), false, 2.0);
        }

        // Configure first APN profile
        ProcessExec(format(QMICLI " -d {} -p --wds-modify-profile=\"3gpp,1,apn={},pdp-type=ipv4\"", ue_dev_path, MM.apn), false, 2.0);

        // ----- AT only -----
        // Delete extra APN profiles
        for (size_t i = 2; i < 10; i++) {
            ProcessExec(format(MMCLI " -m any --3gpp-profile-manager-delete=\"profile-id={}\" > /dev/null 2>&1", i), false, 2.0);
        }

        GL1(TAG, format("[{}/{}] Setting APN to {}", ++current_task, total_tasks, MM.apn));
        // Configure first APN profile
        ProcessExec(format(MMCLI " -m any --3gpp-profile-manager-set=\"profile-id=1,apn={},ip-type=ipv4,profile-enabled=true\"", MM.apn));

        if (MM.auto_connect_to_apn && !MM.manual_apn_connection) {
            // Enable APN autoconnection
            GL1(TAG, format("[{}/{}] Enabling APN autoreconnection", ++current_task, total_tasks));
            ProcessExec((QMICLI " -d " + ue_dev_path + " -p --wds-set-autoconnect-settings=enabled,roaming-allowed > /dev/null 2>&1"));
        }
        else {
            GL1(TAG, format("[{}/{}] Disabling APN autoreconnection", ++current_task, total_tasks));
            ProcessExec((QMICLI " -d " + ue_dev_path + " -p --wds-set-autoconnect-settings=disabled,roaming-allowed > /dev/null 2>&1"));
        }

        GL1(TAG, format("[{}/{}] Releasing ModemManager control (--disable)", ++current_task, total_tasks));
        ProcessExec(MMCLI " -m any --disable");

        if (MM.auto_connect_modem) {
            AutoConnectModem();
        }
        else {
            callback_events(MM_EVT_MODEM_READY);
            GL1G(TAG, "Modem configured and ready to accept commands!");
        }
    }

    void AutoConnectModem()
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!MM.enable)
            return;

        if (!already_autoconnected) {
            already_autoconnected = true;
            GL1G(TAG, "Starting Automatic Modem Connection/Reconnection");
            if (MM.disable_fuzzing_on_first_connection) {
                // Disable fuzzing parameters
                GL1C(TAG, "Disabling Fuzzing Parameters for first connection");
                callback_events(MM_EVT_CLEAN_CONN_START);
                Fuzzing &fuzz = StateMachine.config.fuzzing;
                bool o_dup = fuzz.enable_duplication;
                bool o_mut = fuzz.enable_mutation;
                bool o_opt = fuzz.enable_optimization;
                fuzz.enable_duplication = false;
                fuzz.enable_mutation = false;
                fuzz.enable_optimization = false;

                ATConnectBaseStation(true);

                this_thread::sleep_for(chrono::milliseconds(MM.connection_timeout_ms));
                // Recover fuzzing parameters
                GL1C(TAG, "Fuzzing Parameters Recovered");
                fuzz.enable_duplication = o_dup;
                fuzz.enable_mutation = o_mut;
                fuzz.enable_optimization = o_opt;
                GL1G(TAG, "Modem configured and ready to accept commands!");
                callback_events(MM_EVT_CLEAN_CONN_END);
                callback_events(MM_EVT_MODEM_READY);
                StateMachine.ResetStats();
            }

            StartModemReconnectionTimeout(true);
        }
        else
            RestartModemConnection();
    }

    void DisableEvent(mm_events evt)
    {
        DisabledEvents[evt] = true;
    }

    void EnableEvent(mm_events evt)
    {
        DisabledEvents[evt] = false;
    }

    void ADBConnectBaseStation(bool log_cmd = false)
    {
        RunADBCmd("cmd connectivity airplane-mode disable", log_cmd); // Disable Airplane-mode
    }

    void ADBDisconnectBaseStation(bool log_cmd = false)
    {
        RunADBCmd("cmd connectivity airplane-mode enable", log_cmd); // Enable Airplane-mode
        this_thread::sleep_for(2s);
    }

    void ADBEnableModemAPN(bool log_cmd = false)
    {
        RunADBCmd("svc data enable", log_cmd); // Enable Data
    }

    void ADBDisableModemAPN(bool log_cmd = false)
    {
        RunADBCmd("svc data disable", log_cmd); // Disable Data
    }

    bool ADBResetModem(bool log_cmd = false)
    {
        return !RunADBReboot(log_cmd); // Reboot Device
    }

    bool ADBCheckConnection(bool log_cmd = false)
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!MM.enable_adb)
            return false;

        bool is_device_connected = !RunADBDevices(log_cmd);

        if (!is_device_connected && request_modem_reset) {
            // Device disconnected upon request
            ue_modem_initialized = false;
            adb_waiting_device = true;
            return false;
        }
        else if (is_device_connected && adb_waiting_device) {
            // Device reconnected
            request_modem_reset = false;
            adb_waiting_device = false;
            adb_wait_coldboot = true;
            GL1G(TAG, "Device Reconnected");
            ConfigureADBSync();
            return true;
        }
        else if (is_device_connected)
            return true;

        if (adb_waiting_device)
            return false;

        // Device is not connected not was requested a reset
        GL1R(TAG, "Modem Removed!");
        adb_waiting_device = true;
        ue_modem_initialized = false;
        callback_events(MM_EVT_MODEM_SURPRISE_REMOVED);
        return false;
    }

    void StartModemReconnectionTimeout(bool verbose = false)
    {
        if (stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;
        if (!MM.enable)
            return;

        StartModemReconnectionTimeout(MM.connection_timeout_ms, verbose);
    }

    void StartModemReconnectionTimeout(int64_t connection_timeout_ms, bool verbose = false)
    {
        if (stop_request)
            return;

        if (verbose)
            GL1(TAG, "Reconnection Timeout (", connection_timeout_ms, " ms)");

        if (!reconnection_timer)
            reconnection_timer = mm_loop.onTimeoutMS(connection_timeout_ms, [&]() {
                RestartModemConnection();
            });
        else {
            reconnection_timer->setMS(connection_timeout_ms);
        }
    }

    void StopModemReconnectionTimeout()
    {
        if (stop_request)
            return;

        if (reconnection_timer)
            reconnection_timer->cancel();
    }

    void RestartModemConnection()
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;
        if (!MM.enable)
            return;

        if (MM.auto_reconnect_modem)
            StopModemReconnectionTimeout();

        mm_loop.RunTaskSync(mutex_task, [&]() {
            StopModemConnection();
            StartModemConnection();
        });
    }

    void StartModemConnectionSync()
    {

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;
        if (!MM.enable)
            return;

        if (MM.auto_reconnect_modem)
            StopModemReconnectionTimeout();

        if (ue_modem_initialized && !stop_request) {
            if ((!MM.use_only_at_connections) && ue_modem_initialized) {
                LogCommand("Connect");
                callback_events(MM_EVT_MODEM_CONNECTING);
                if (MM.enable_adb) {
                    // ADB Enabled
                    ADBConnectBaseStation();
                }
                else {
                    ProcessExec(MMCLI " -m any --set-power-state-on");
                }
            }
            else if (MM.use_only_at_connections)
                ATConnectBaseStation();
        }
        else if (!ue_modem_initialized)
            GL1(TAG, "StartModemConnection: Modem not initialized");

        if (MM.auto_reconnect_modem)
            StartModemReconnectionTimeout(MM.connection_timeout_ms, true);
    }

    void StartModemConnection()
    {
        if (stop_request)
            return;

        mm_loop.RunTaskSync(mutex_task, [&]() {
            StartModemConnectionSync();
        });
    }

    void StopModemConnectionSync()
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;
        if (!MM.enable)
            return;

        if (MM.auto_reconnect_modem)
            StopModemReconnectionTimeout();

        if (!ue_modem_initialized || stop_request)
            return;

        if ((!MM.use_only_at_connections) && ue_modem_initialized) {
            LogCommand("Disconnect");
            callback_events(MM_EVT_MODEM_DISCONNECTING);
            if (MM.enable_adb) {
                // ADB Enabled
                ADBDisconnectBaseStation();
            }
            else {
                ProcessExec(MMCLI " -m any --disable");
                // Put in low power mode
                ProcessExec(MMCLI " -m any --set-power-state-low");
            }
        }
        else if (MM.use_only_at_connections)
            ATDisconnectBaseStation();

        if (MM.manual_apn_connection)
            DisableModemAPN(true);
    }

    void StopModemConnection()
    {
        if (stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;
        if (!MM.enable)
            return;

        if (MM.auto_reconnect_modem)
            StopModemReconnectionTimeout();

        mm_loop.RunTaskSync(mutex_task, [&]() {
            StopModemConnectionSync();
            if (MM.auto_reconnect_modem)
                StartModemReconnectionTimeout();
        });
    }

    void EnableModemAPN(bool wait_cmd = false, int delay_ms = 0)
    {
        if (!ue_modem_initialized || stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!wait_cmd) {
            mm_loop.RunTask([this, delay_ms]() {
                if (delay_ms)
                    this_thread::sleep_for(chrono::milliseconds(delay_ms));
                LogCommand("Enable APN Autoconnection");
                if (MM.enable_adb) {
                    // ADB Enabled
                    ADBEnableModemAPN();
                }
                else
                    ProcessExec((QMICLI " -d " + ue_dev_path + " -p --wds-set-autoconnect-settings=enabled,roaming-allowed > /dev/null 2>&1"));
            });
        }
        else {
            if (!ue_modem_initialized)
                return;
            if (delay_ms)
                this_thread::sleep_for(chrono::milliseconds(delay_ms));

            LogCommand("Disable APN Autoconnection");
            if (MM.enable_adb) {
                // ADB Enabled
                ADBEnableModemAPN();
            }
            else
                ProcessExec((QMICLI " -d " + ue_dev_path + " -p --wds-set-autoconnect-settings=enabled,roaming-allowed > /dev/null 2>&1"));
        }
    }

    void DisableModemAPN(bool wait_cmd = false)
    {
        if (!ue_modem_initialized || stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!wait_cmd) {
            mm_loop.RunTask([&]() {
                LogCommand("Disable APN Autoconnection");
                if (MM.enable_adb) {
                    // ADB Enabled
                    ADBDisableModemAPN();
                }
                else
                    ProcessExec((QMICLI " -d " + ue_dev_path + " -p --wds-set-autoconnect-settings=disabled,roaming-allowed > /dev/null 2>&1"));
            });
        }
        else {
            LogCommand("Disable APN Autoconnection");
            if (MM.enable_adb) {
                // ADB Enabled
                ADBDisableModemAPN();
            }
            else
                ProcessExec((QMICLI " -d " + ue_dev_path + " -p --wds-set-autoconnect-settings=disabled,roaming-allowed > /dev/null 2>&1"));
        }
    }

    void ResetModemSync()
    {
        int res;

        if (!ue_modem_initialized || stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        request_modem_reset = true;

        LogCommand("Reset Modem");

        callback_events(MM_EVT_MODEM_RESET_REQUESTED);

        if (MM.enable_adb)
            res = ADBResetModem(true); // Via ADB
        else
            res = ProcessExec(MMCLI " -m any --reset"); // Via modem manager

        if (res) {
            GL1R(TAG, "UE reset failed");
            request_modem_reset = false;
            callback_events(MM_EVT_MODEM_RESET_FAILED);
        }
    }

    void ResetModem()
    {
        if (!ue_modem_initialized || stop_request)
            return;

        mm_loop.RunTask([&]() {
            ResetModemSync();
        });
    }

    template <class T>
    void SetCommandsLogger(T &CmdLoggerInstance)
    {
        callback_msg_logger_cmd = [&](string msg, bool error = false) {
            CmdLoggerInstance.writeLog(msg, error);
        };
    }

    void IndicateConnection()
    {
        if (stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (!MM.enable)
            return;

        if (!MM.auto_connect_to_apn && MM.manual_apn_connection)
            EnableModemAPN(false, MM.manual_apn_connection_delay_ms);
    }

    // Indicate that target has been reset by external means
    void IndicateTargetReset()
    {
        if (stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        GL1C(TAG, "Reset Indicated");

        request_modem_reset = true;

        if (MM.enable_adb)
            return;

        if (MM.auto_reconnect_modem)
            StopModemReconnectionTimeout();

        Restart();
    }

    // API used by WDGlobalTimer
    bool IndicateTimeout(WDGlobalTimeout &global_timeout, int ext_timeouts_count)
    {
        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;

        if (G_UNLIKELY(!MM.enable || stop_request))
            return false;

        if (ext_timeouts_count >= MM.global_timeouts_count && MM.reset_modem_on_global_timeout)
            ResetModemSync();
        else
            ATReconnectBaseStation();

        if (MM.auto_reconnect_modem)
            StartModemReconnectionTimeout(MM.connection_timeout_ms, true);

        if (ext_timeouts_count >= MM.global_timeouts_count) {
            // Restart ModemManager Process
            GL1C(TAG, "Max Timeout Indicated");

            if (!MM.enable_adb) {
                request_modem_reset = true;
                if (MM.auto_reconnect_modem)
                    StopModemReconnectionTimeout();

                Restart();
            }

            // Reset timeout count if USB Hub is disabled
            if (!StateMachine.config.services.usb_hub_control.enable)
                return true;
        }

        return false;
    }

    void ATConnectBaseStation(bool wait_cmd_to_finish = false)
    {
        if (!ue_modem_initialized || stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;
        Nr5G &nr5g = StateMachine.config.nr5_g;

        if (MM.enable_adb) {
            // ADB Enabled
            // TODO: move this outside AT specific functions
            callback_events(MM_EVT_MODEM_CONNECTING);
            ADBConnectBaseStation(true);
            return;
        }

        string at_cmd = format("AT+COPS=1,2,{}{},12", nr5g.mcc, nr5g.mnc);
        LogCommand(format("Connect ({})", at_cmd));
        callback_events(MM_EVT_MODEM_CONNECTING);
        SendATCommand(at_cmd, wait_cmd_to_finish);
    }

    void ATDisconnectBaseStation(bool wait_cmd_to_finish = false)
    {
        if (!ue_modem_initialized || stop_request)
            return;

        UeModemManager &MM = StateMachine.config.services.ue_modem_manager;
        Nr5G &nr5g = StateMachine.config.nr5_g;

        if (MM.enable_adb) {
            // ADB Enabled
            // TODO: move this outside AT specific functions
            callback_events(MM_EVT_MODEM_DISCONNECTING);
            ADBDisconnectBaseStation(true);
            return;
        }

        LogCommand("Disconnect (AT+COPS=2)");
        callback_events(MM_EVT_MODEM_DISCONNECTING);
        SendATCommand("AT+COPS=2", wait_cmd_to_finish);
    }

    void ATReconnectBaseStation(bool wait_cmd_to_finish = false)
    {
        ATDisconnectBaseStation(wait_cmd_to_finish);
        ATConnectBaseStation(wait_cmd_to_finish);
    }

    void SendATCommand(string cmd, bool wait_cmd_to_finish = false)
    {
        if (!ue_modem_initialized || stop_request)
            return;

        if (!wait_cmd_to_finish) {
            mm_loop.RunTask([this, cmd]() {
                ProcessExec(format(MMCLI " -m any --command=\"{}\" > /dev/null 2>&1", cmd));
            });
        }
        else {
            ProcessExec(format(MMCLI " -m any --command=\"{}\" > /dev/null 2>&1", cmd));
        }
    }

    bool ModemInitialized()
    {
        return ue_modem_initialized;
    }

    string SendATCommandGetResponse(string cmd)
    {
        if (!ue_modem_initialized || stop_request)
            return "";

        return ProcessExecGetResult(format(MMCLI " -m any --command=\"{}\"", cmd));
    }

    void stop()
    {
        stop_request = true;
        ModemManagerProc.stop(true, true);
    }
};

#endif