#pragma once

#ifndef __SENDGRID__
#define __SENDGRID__

#include "Framework.hpp"
#include "Machine.hpp"
#include "MiscUtils.hpp"

#define WD_REPORT_SENDER_PATH "modules/reportsender/"

class SvcReportSender {
private:
    React::MainLoop r_loop;
    unordered_map<string, function<void(ReportModule &, string, string, bool)>> ReportTypeFcns;

    thread report_thread;
    bool initialized = false;

    void r_log(string &output)
    {
        vector<string> lines;
        strtk::parse(output, "\n", lines);
        strtk::remove_empty_strings(lines);
        for (size_t i = 0; i < lines.size(); i++) {
            GL1(lines[i]);
        }
    }

    string ExecScript(string &script, string cmds, double timeout = 3.0, bool verbose = false)
    {
        string res = ProcessExecGetResult(format("{} {}{} {}", WD_PYTHON, WD_REPORT_SENDER_PATH, script, cmds), verbose, timeout);
        r_log(res);
        return res;
    }

    string GenReportName(bool error_msg = false)
    {
        string priority, extras;
        Services &svcs = StateMachine.config.services;

        // Generate priority of report
        if (error_msg)
            priority = "Alert Report";
        else
            priority = "Report";

        // Generate info for usb hub service
        if (svcs.usb_hub_control.enable) {
            vector<string> usb_devices;
            for (auto &dev : svcs.usb_hub_control.usb_devices) {
                if (dev.enabled)
                    usb_devices.push_back(dev.name);
            }

            if (usb_devices.size()) {
                extras = ", USBHubCtrl Devices: {" + strtk::join(",", usb_devices) + "}";
            }
        }

        if (StateMachine.config.nr5_g.enable_simulator)
            extras += ", NR UE-Softmodem (RFSIM)";

        // chrono::duration<int64_t, centi>;
        auto c_time = chrono::system_clock::now();
        auto c_seconds = floor<chrono::duration<int64_t, centi>>(c_time);
        return format("{}{} from {} at {:%d/%m/%Y %H:%M}:{:%S}{}",
                      TAG, priority, g_get_host_name(), c_time, c_seconds, extras);
    }

public:
    const char *TAG = "[ReportSender] ";

    bool init()
    {
        if (initialized) {
            return false;
        }

        if (!StateMachine.config.services.reports_sender.enable) {
            GL1R(TAG, "Disabled in config file");
            return false;
        }

        report_thread = thread([&]() {
            pthread_setname_np(pthread_self(), "report_sender_loop");
            enable_idle_scheduler();
            r_loop.run();
        });
        report_thread.detach();

        r_loop.RunTask([&]() {
            ReportsSender &reportsender = StateMachine.config.services.reports_sender;

            for (auto &report_cfg : reportsender.report_modules) {
                if (!report_cfg.enabled)
                    continue;

                ifstream cred_file(report_cfg.credentials_file);

                if (!cred_file.good()) {
                    GL1R(TAG, "Credentials file not found: ", report_cfg.credentials_file);
                    continue;
                }

                cred_file.close();

                // Init all modules with credential file
                if (report_cfg.type == "Email") {
                    string res = ExecScript(report_cfg.script, "-c " + report_cfg.credentials_file, true);
                    if (string_contains(res, "Error:"))
                        GL1R(TAG, report_cfg.script, ": Error checking credentials");
                    else if (string_contains(res, "Token is valid"))
                        GL1G(TAG, report_cfg.script, ": Init OK");
                }
            }

            ReportTypeFcns["Email"] = [&](ReportModule &cfg, string name, string msg, bool wait) {
                SendEmail(cfg, name, msg, wait);
            };

            initialized = true;
            GL1G(TAG, "Ready");
        });

        return true;
    }

    void writeLog(string msg, bool is_error)
    {
        string report_name = GenReportName(is_error);

        SendReport(report_name, msg, is_error);
    }

    void SendReport(string report_name, string msg, bool is_error = false, bool wait = false)
    {
        if (!initialized)
            return;

        ReportsSender &reportsender = StateMachine.config.services.reports_sender;

        for (auto &report_cfg : reportsender.report_modules) {

            if ((!report_cfg.enabled) || (!ReportTypeFcns.contains(report_cfg.type)))
                continue;

            // Only allow errors if only_errors is enabled
            if (report_cfg.only_errors && !is_error)
                continue;

            bool string_matched = false;

            // Check if msg contain any magic words to proceed with sending the report
            for (auto &filter_str : report_cfg.magic_words)
                if (string_contains(msg, filter_str)) {
                    string_matched = true;
                    break;
                }

            if (string_matched)
                ReportTypeFcns[report_cfg.type](report_cfg, report_name, msg, wait);
        }
    }

    void SendEmail(ReportModule &cfg, string report_name, string msg, bool wait = false)
    {
        if (!initialized)
            return;

        if (!wait) {
            r_loop.RunTask([this, &cfg, report_name, msg]() {
                SendEmailSync(cfg, report_name, msg);
            });
        }
        else
            SendEmailSync(cfg, report_name, msg);
    }

    void SendEmailSync(ReportModule &cfg, string msg_name, string msg)
    {
        if (!initialized)
            return;

        for (string &to : cfg.to)
            ExecScript(cfg.script, format("send -t \"{}\" -s \"{}\" -b '{}'", to, msg_name, msg));
    }
};

#endif