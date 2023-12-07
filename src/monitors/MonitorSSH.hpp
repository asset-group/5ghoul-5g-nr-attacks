#pragma once

#ifndef __MONITORSSH__
#define __MONITORSSH__

#include <functional>
#include <pthread.h>
#include <src/Machine.hpp>
#include <src/Process.hpp>
#include <thread>

using namespace std;

class MonitorSSH {
private:
    const char *TAG = "[Monitor] ";
    string *HostAddress = nullptr;
    string *UserName = nullptr;
    string *Password = nullptr;
    int64_t *PortNumber = nullptr;
    string *SSHCommand = nullptr;
    bool *SSHEnablePreCommands = nullptr;
    vector<string> *SSHPreCommands = nullptr;

    bool *enable = nullptr;
    std::string ssh_buffer = "";
    bool running = false;
    vector<string> *MagicWords;
    ProcessRunner SSHProcess;

    thread *monitor_ssh_thread = nullptr;
    function<void(string)> UserCallback = nullptr;
    function<void(bool)> UserCrashCallback = nullptr;

    inline void SSHCallback(const char *bytes, size_t n)
    {
        static int last_size = 0;
        if (n > 0) {
            ssh_buffer.clear();
            ssh_buffer = string(bytes, n);

            if (UserCrashCallback != nullptr && MagicWords != nullptr) {
                for (string &word : *MagicWords) {
                    if (string_contains(ssh_buffer, word)) {
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
            strtk::parse(ssh_buffer, "\n", lines);

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

    string gen_ssh_command_string(string cmd, bool use_sshpass = false)
    {
        if (Password != nullptr && UserName != nullptr && HostAddress != nullptr && PortNumber != nullptr) {
            string cmd_str = "";
            if (use_sshpass)
                cmd_str += "sshpass -p " + *Password + " ";

            cmd_str += "ssh -T -F /dev/null -i /dev/null "s +
                       "-o NumberOfPasswordPrompts=1 " +
                       "-o PreferredAuthentications=password " +
                       "-o PubkeyAuthentication=no " +
                       "-o StrictHostKeyChecking=no " +
                       "-o UserKnownHostsFile=/dev/null " +
                       *UserName + "@" + *HostAddress +
                       " -p " + to_string(*PortNumber) +
                       " '" + cmd + "'";
            return cmd_str;
        }
        else
            return "";
    }

    bool run_ssh_command(string cmd)
    {
        string cmd_str = gen_ssh_command_string(cmd, true);
        if (cmd_str.size()) {
            int res = system(cmd_str.c_str());

            return (res == 0);
        }
        else
            return false;
    }

public:
    void printStatus()
    {
        if (SSHProcess.isRunning())
            GL1G(TAG, "SSH Connected to ", *UserName, "@", *HostAddress, ":", *PortNumber);
        else if (this->enable && *this->enable)
            GL1R(TAG, "ERROR: Could not connect to ", *UserName, "@", *HostAddress, ":", *PortNumber, ". Check your configuration file");
        else
            GL1Y(TAG, "Disabled");
    }

    bool IsOpen()
    {
        return SSHProcess.isRunning();
    }

    ~MonitorSSH()
    {
        running = false;
    }

    bool init()
    {
        if (enable && *enable && HostAddress != nullptr && UserName != nullptr && PortNumber != nullptr && Password != nullptr && SSHCommand != nullptr)
            return init(*HostAddress, *PortNumber, *UserName, *Password, *SSHCommand);

        return false;
    }

    bool init(Config &conf)
    {
        if (conf.monitor.ssh.ssh_command.capacity() < 128)
            conf.monitor.ssh.ssh_command.reserve(128);
        HostAddress = &conf.monitor.ssh.ssh_host_address;
        PortNumber = &conf.monitor.ssh.ssh_port;
        UserName = &conf.monitor.ssh.ssh_username;
        Password = &conf.monitor.ssh.ssh_password;
        SSHCommand = &conf.monitor.ssh.ssh_command;
        enable = &conf.monitor.enable;
        MagicWords = &conf.monitor.ssh.ssh_magic_words;
        if (conf.monitor.ssh.ssh_enable_pre_commands)
            SSHPreCommands = &conf.monitor.ssh.ssh_pre_commands;
        else
            SSHPreCommands = nullptr;

        return init();
    }

    bool init(string &host_address, int64_t &port_number, string &user_name, string &password, string &ssh_command)
    {
        HostAddress = &host_address;
        PortNumber = &port_number;
        UserName = &user_name;
        Password = &password;
        SSHCommand = &ssh_command;

        if (SSHPreCommands && SSHPreCommands->size()) {
            GL1Y(TAG, "Executing SSH Pre-Commands (Target Setup)");
            for (string &cmd : *SSHPreCommands) {
                GL1M(TAG, "--> ", cmd);
                if (run_ssh_command(cmd))
                    GL1G(TAG, "--> Success");
                else
                    GL1R(TAG, "--> Error");
            }
        }

        string sshpass_args = gen_ssh_command_string(*SSHCommand);

        GL1Y(TAG, "Connection string: sshpass ", sshpass_args);

        SSHProcess.setup("sshpass", "-p " + *Password + " " + sshpass_args,
                         [&](const char *bytes, size_t n) {
                             SSHCallback(bytes, n);
                         });

        return SSHProcess.init();
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
