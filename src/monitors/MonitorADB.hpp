#pragma once

#ifndef __MONITORADB__
#define __MONITORADB__

#include <functional>
#include <pthread.h>
#include <src/Machine.hpp>
#include <src/Process.hpp>
#include <thread>

using namespace std;

class MonitorADB {
private:
    const char *TAG = "[Monitor] ";
    string *Device = nullptr;
    string *Program = nullptr;
    string *Filter = nullptr;
    string *SubSystem = nullptr;
    std::vector<std::string> device_list;

    bool *enable = nullptr;
    std::string adb_buffer = "";
    bool running = false;
    vector<string> *MagicWords;
    ProcessRunner ADBProcess;

    thread *monitor_ssh_thread = nullptr;
    function<void(string)> UserCallback = nullptr;
    function<void(bool)> UserCrashCallback = nullptr;

    inline void ADBCallback(const char *bytes, size_t n)
    {
        static int last_size = 0;
        if (n > 0) {
            adb_buffer.clear();
            adb_buffer = string(bytes, n);

            if (UserCrashCallback != nullptr && MagicWords != nullptr) {
                for (string &word : *MagicWords) {
                    if (string_contains(adb_buffer, word)) {
                        // Crash found
                        if (enable != nullptr && *enable)
                            UserCrashCallback(false);
                        else if (enable == nullptr)
                            UserCrashCallback(false);
                        break;
                    }
                }
            }

            vector<string> lines;
            strtk::parse(adb_buffer, "\n", lines);

            for (string &line : lines) {
                if (line.size() && line[0] != '\n') {
                    if (UserCallback != nullptr) {
                        if (enable != nullptr && *enable) {
                            UserCallback(line);
                        }
                        else if (enable == nullptr) {
                            UserCallback(line);
                        }
                    }
                }
            }
        }
    }

    string gen_adb_command_string(string cmd)
    {
        if (Device != nullptr && Program != nullptr && Filter != nullptr && SubSystem != nullptr) {
            string cmd_str = "";

            if (Device->size())
                cmd_str += "-s " + *Device + " ";
            cmd_str += "shell \"" + *Program;

            // cmd_str += " -v brief " + *SubSystem;

            if (SubSystem->size())
               cmd_str += " -b " + *SubSystem;

            if (Filter->size())
                cmd_str += " | grep -i -E '" + *Filter + "'";

            cmd_str += "\"";
            return cmd_str;
        }
        else
            return "";
    }

    bool run_adb_command(string cmd)
    {
        string cmd_str = gen_adb_command_string(cmd);

        if (cmd_str.size()) {
            int res = system(("3rd-party/adb/adb " + cmd_str).c_str());

            return (res == 0);
        }
        else
            return false;
    }

public:
    void printStatus()
    {
        if (ADBProcess.isRunning()) {
            if (Device != nullptr && Device->size())
                GL1G(TAG, "ADB Connected to device: ", *Device);
            else
                GL1G(TAG, "ADB Connected to default device");
        }
        else {
            if (Device != nullptr && Device->size())
                GL1R(TAG, "ERROR: ADB Could not connect to device ", *Device);
            else if (this->enable && *this->enable)
                GL1R(TAG, "ERROR: ADB Could not connect to default device");
            else
                GL1Y(TAG, "Disabled");
        }
    }

    bool IsOpen()
    {
        return ADBProcess.isRunning();
    }

    ~MonitorADB()
    {
        running = false;
    }

    bool init()
    {
        if (enable && *enable && Device != nullptr && Program != nullptr && Filter != nullptr && SubSystem != nullptr)
            return init(*Device, *Program, *Filter, *SubSystem);

        return false;
    }

    bool init(Config &conf)
    {
        Device = &conf.monitor.adb.adb_device;
        Program = &conf.monitor.adb.adb_program;
        Filter = &conf.monitor.adb.adb_filter;
        SubSystem = &conf.monitor.adb.adb_sub_system;

        enable = &conf.monitor.enable;
        MagicWords = &conf.monitor.adb.adb_magic_words;

        return init();
    }

    bool init(string &device, string &program, string &filter, string &subsystem)
    {
        Device = &device;
        Program = &program;
        Filter = &filter;
        SubSystem = &subsystem;

        string adb_args = gen_adb_command_string(*Program);

        GL1Y(TAG, "Connection string: adb ", adb_args);

        ADBProcess.setup("3rd-party/adb/adb", adb_args,
                         [&](const char *bytes, size_t n) {
                             ADBCallback(bytes, n);
                         },false, true, 1000);

        return ADBProcess.init();
    }

    void
    SetMagicWords(vector<string> &magicwords)
    {
        MagicWords = &magicwords;
    }

    void SetCallback(function<void(string)> callback)
    {
        if (callback != nullptr)
            UserCallback = callback;
    }

    void SetCrashCallback(function<void(bool)> callback)
    {
        if (callback != nullptr)
            UserCrashCallback = callback;
    }
};

#endif
