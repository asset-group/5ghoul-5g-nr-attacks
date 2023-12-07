#pragma once

#ifndef __USBHUBCONTROL__
#define __USBHUBCONTROL__

#include <unordered_map>
#include <utility>

#include "Framework.hpp"
#include "Machine.hpp"
#include "MiscUtils.hpp"

#define UHUBCTLS "3rd-party/uhubctl/uhubctls"

class SvcUSBHubControl {
private:
    uint64_t _ext_timeouts_count = 0;

    unordered_map<string, vector<pair<string, string>>> MappedLocations;
    vector<function<void(void)>> callbacks_power_restart;
    function<void(bool)> callback_power_change = nullptr;

    vector<pair<string, string>> vidpid_to_hub_location_port(string vidpid)
    {

        vector<pair<string, string>> output;
        vector<string> bus_location = GetVIDPIDBusLocation(vidpid);

        if (!bus_location.size()) {
            LOG3R(TAG, "Error: Failure getting bus location for ", vidpid);
            return output;
        }

        if (MappedLocations.find(vidpid) != MappedLocations.end())
            MappedLocations[vidpid].clear();
        else
            MappedLocations[vidpid] = vector<pair<string, string>>();

        for (string &l : bus_location) {
            vector<string> location_parsed;
            strtk::parse(l, ".", location_parsed);

            if (!location_parsed.size())
                continue;

            string hub_location = "";
            if (location_parsed.size() >= 2) {
                for (int i = 0; i < location_parsed.size() - 1; i++) {
                    hub_location += location_parsed[i];
                    if (i < location_parsed.size() - 2)
                        hub_location += ".";
                }
            }
            else {
                LOG3R(TAG, "Error: Failure parsing bus location for ", l);
                return output;
            }

            string hub_port = location_parsed.back();

            MappedLocations[vidpid].push_back(make_pair(hub_location, hub_port));

            output.push_back(make_pair(hub_location, hub_port));
        }

        return output;
    }

public:
    const char *TAG = "[USBHubControl] ";

    bool init()
    {
        UsbHubControl &hub = StateMachine.config.services.usb_hub_control;
        
        if (!hub.enable)
        {
            GL1Y(TAG, "Disabled in config. file");
            return false;
        }

        bool ret = true;
        for (auto &dev : hub.usb_devices) {

            if (!dev.enabled)
                continue;

            auto busid = vidpid_to_hub_location_port(dev.vidpid);
            if (busid.size()) {
                GL1G(TAG, format("Found Device: {}, Busid: {}", dev.name, dev.vidpid));
            }
            else {
                GL1R(TAG, format("Error: Device: {}, Busid: Not Found", dev.name, dev.vidpid));
                ret = false;
            }

            if (dev.reset_on_program_startup && *dev.reset_on_program_startup)
                RestartHubPort(dev.vidpid);
        }

        return ret;
    }

    bool SetPowerStateON(string vidpid)
    {
        return SetPowerState(vidpid, true);
    }

    bool SetPowerStateOFF(string vidpid)
    {
        return SetPowerState(vidpid, false);
    }

    bool SetPowerState(string vidpid, bool power_level)
    {
        vector<pair<string, string>> hublocations;
        bool ret = true;

        // Power ON->OFF
        if (!power_level)
            hublocations = vidpid_to_hub_location_port(vidpid);
        // Power OFF->ON
        else if (MappedLocations.find(vidpid) != MappedLocations.end())
            hublocations = MappedLocations[vidpid];

        if (!hublocations.size()) {
            GL1R("Error: Bus location for ", vidpid, " not found. Check if device is plugged");
            return false;
        }

        if (callback_power_change)
            callback_power_change(power_level);

        for (auto &hubloc : hublocations)
            ret &= SetPowerState(hubloc.first, hubloc.second, power_level);

        return ret;
    }

    bool SetPowerState(string hub_location, string hub_port, bool power_level)
    {
        string power_level_str = (power_level == true ? "on" : "off");

        GL1Y(TAG, "Powering ", strtk::as_uppercase(power_level_str), " hub:", hub_location, ", Port:", hub_port);

        ProcessExec(format(UHUBCTLS " -l {} -p {} -a {}", hub_location, hub_port, power_level_str));

        if (!power_level) {
            // Ensure USB device is unbided in Linux < 6.0
            ProcessExec(format("echo {}.{} > /sys/bus/usb/drivers/usb/unbind", hub_location, hub_port));
        }

        return true;
    }

    bool RestartHubPort(string vidpid)
    {
        bool ret = true;
        UsbHubControl &hub = StateMachine.config.services.usb_hub_control;
        GL1M(TAG, "Restarting USB Device: ", vidpid);
        ret &= SetPowerStateOFF(vidpid);
        this_thread::sleep_for(chrono::milliseconds(hub.toggle_power_delay_ms));
        ret &= SetPowerStateON(vidpid);
        return ret;
    }

    bool RestartHubPort()
    {
        bool ret = true;
        UsbHubControl &hub = StateMachine.config.services.usb_hub_control;

        if (G_UNLIKELY(!hub.enable))
            return false;

        if (callbacks_power_restart.size())
            for (auto &fcn : callbacks_power_restart)
                fcn();

        for (auto &dev : hub.usb_devices) {
            if (!dev.enabled)
                continue;
            ret &= RestartHubPort(dev.vidpid);
        }

        return ret;
    }

    bool IndicateTimeout(WDGlobalTimeout &global_timeout, int ext_timeouts_count)
    {
        UsbHubControl &hub = StateMachine.config.services.usb_hub_control;

        if (G_UNLIKELY(!hub.enable))
            return false;

        if (ext_timeouts_count >= hub.global_timeouts_count) {
            RestartHubPort();
            // Indicate that this is a final timeout action and timeout counter must be reset
            return true;
        }

        return false;
    }

    void SetPowerStateCallback(function<void(bool)> fcn)
    {
        callback_power_change = [&](bool power_state) {
            fcn(power_state);
        };
    }

    template <class T, typename... U>
    void CallIndicateTargetReset(T &Instance, U&&... args)
    {
        callbacks_power_restart.push_back([&]() {
            Instance.IndicateTargetReset(args...);
        });
    }
};

#endif