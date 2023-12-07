#pragma once
#ifndef __PYTHONHOSTAPDCONFIG__
#define __PYTHONHOSTAPDCONFIG__

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <pthread.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include <Machine.hpp>
#include <python_modules/PythonRuntime.hpp>

using namespace std;

class PythonHostAPDConfig {

private:
    bool module_loaded = false;
    py::object HostapdConf;
    py::object HostapdFile;
    py::function ConfWriter;

public:
    const char *TAG = "[PyHostAPDConfig] ";
    string conf_file_path;

    bool init(string conf_file_path)
    {
        if (!PythonCore.started)
            return false;

        this->conf_file_path = conf_file_path;

        try {

            {
                py::gil_scoped_acquire acquire;
                // Add server folder to PATH
                if (!module_loaded) {
                    HostapdConf = py::module::import("pyhostapdconf.parser").attr("HostapdConf");
                }
            }

            GL1G(TAG, "pyhostapdconf Module imported");
            module_loaded = true;

            // Load and update hostapd conf settings to conf_file_path
            if (LoadConfFile()) {
                return UpdateConfFile();
            }
        }
        catch (const std::exception &e) {
            LOGR(e.what());
        }
        return false;
    }

    bool LoadConfFile(string new_conf_path)
    {
        conf_file_path = new_conf_path;
        return LoadConfFile();
    }

    bool LoadConfFile()
    {
        if (!module_loaded)
            return false;

        try {

            {
                py::gil_scoped_acquire acquire;

                HostapdFile = HostapdConf(conf_file_path.c_str());
                ConfWriter = HostapdFile.attr("write"); // Register
            }

            return true;
        }
        catch (const std::exception &e) {
            LOGR(e.what());
        }

        return false;
    }

    bool UpdateConfFile()
    {
        if (!module_loaded)
            return false;

        Wifi &wconf = StateMachine.config.wifi;
        GL1Y(TAG, "Wi-Fi Configuration:");
        GL1C(TAG, "Interface: ", wconf.wifi_interface);
        GL1C(TAG, "Channel: ", wconf.wifi_channel);
        GL1C(TAG, "SSID: ", wconf.wifi_ssid);
        GL1C(TAG, "EAP Username: ", wconf.wifi_username);
        GL1C(TAG, "Password: ", wconf.wifi_password);
        GL1C(TAG, "Key Auth Type: ", wconf.wifi_key_auth_list[wconf.wifi_key_auth]);
        GL1C(TAG, "RSN Crypto Type: ", wconf.wifi_rsn_crypto_list[wconf.wifi_rsn_crypto]);
        GL1C(TAG, "802.11w (Protected Management Frames): ", wconf.wifi802_11_w);

        try {
            {
                py::gil_scoped_acquire acquire;
                GL1C(TAG, "ctrl_interface: ", HostapdFile["ctrl_interface"].cast<string>());
                HostapdFile["interface"] = wconf.wifi_interface;
                HostapdFile["channel"] = wconf.wifi_channel;
                HostapdFile["ssid"] = wconf.wifi_ssid;
                HostapdFile["country_code"] = wconf.wifi_country_code;
                HostapdFile["wpa_passphrase"] = wconf.wifi_password;
                HostapdFile["wpa_key_mgmt"] = wconf.wifi_key_auth_list[wconf.wifi_key_auth];
                HostapdFile["rsn_pairwise"] = wconf.wifi_rsn_crypto_list[wconf.wifi_rsn_crypto];
                HostapdFile["ieee80211w"] = (wconf.wifi802_11_w ? 2 : 0);

                if (wconf.wifi_key_auth == 0) {
                    // If WPA-EAP is selected
                    HostapdFile["wpa"] = 2;
                    HostapdFile["auth_algs"] = 3;
                    HostapdFile["ieee8021x"] = 1;
                    HostapdFile["eap_server"] = 1;
                    // Update EAP username configuration file
                    std::ofstream ofs("configs/wifi_ap/hostapd.eap_user", std::ofstream::out);
                    // EAP Stage 1
                    ofs << "\"" + wconf.wifi_username + "\"\t" +
                               wconf.wifi_eap_method_list[wconf.wifi_eap_method] + "\t\"" +
                               wconf.wifi_password + "\"\n";
                    // EAP Stage 2
                    ofs << "\"" + wconf.wifi_username + "\"\t" +
                               "MSCHAPV2,TTLS-MSCHAPV2\t\"" +
                               wconf.wifi_password + "\" [2]\n";
                    ofs.close();
                }
                else if (wconf.wifi_key_auth == 1) {
                    // If WPA-PSK is selected
                    HostapdFile["wpa"] = 2;
                    HostapdFile["auth_algs"] = 3;
                    HostapdFile["ieee8021x"] = 0;
                    HostapdFile["eap_server"] = 0;
                }
                else if (wconf.wifi_key_auth == 2) {
                    // If SAE (WPA3) is selected
                    HostapdFile["wpa"] = 2;
                    HostapdFile["auth_algs"] = 3;
                    HostapdFile["ieee8021x"] = 0;
                    HostapdFile["eap_server"] = 0;
                }

                ConfWriter();
            }
            GL1M("Updated ", conf_file_path, " from fuzzer config file");
            return true;
        }
        catch (const std::exception &e) {
            LOGR(e.what());
        }
        return false;
    }

    void Close()
    {
        {
            // Destroy python members
            if (module_loaded) {
                py::gil_scoped_acquire acquire;
                HostapdFile.~object();
                ConfWriter.~function();
                HostapdConf.~object();
            }
        }
    }

    ~PythonHostAPDConfig()
    {
        Close();
    }
};

#endif