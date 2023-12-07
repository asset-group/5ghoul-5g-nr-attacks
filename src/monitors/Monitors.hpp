#pragma once

#ifndef __MONITORCOMMON__
#define __MONITORCOMMON__

#include <inttypes.h>
#include <serial/serial.h>
#include <src/Machine.hpp>
#include <src/monitors/MonitorADB.hpp>
#include <src/monitors/MonitorMicrophone.hpp>
#include <src/monitors/MonitorSSH.hpp>
#include <src/monitors/MonitorSerial.hpp>

using namespace std;

class Monitors {
private:
    enum MONITORS {
        SERIAL = 0,
        SSH = 1,
        MICROPHONE = 2,
        ADB = 3,
    };

    MonitorSerial *_MonitorSerial;
    MonitorSSH *_MonitorSSH;
    MonitorMicrophone *_MonitorMicrophone;
    MonitorADB *_MonitorADB;

    bool valid_class = false;
    int monitor_type = 0;

public:
    ~Monitors()
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                delete _MonitorSerial;
                break;
            case MONITORS::SSH:
                delete _MonitorSSH;
                break;
            case MONITORS::MICROPHONE:
                delete _MonitorSSH;
                break;
            case MONITORS::ADB:
                delete _MonitorADB;
                break;
            default:
                break;
            }
        }
    }

    int Type()
    {
        return monitor_type;
    }

    void stop()
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                return _MonitorSerial->stop();
                break;
            default:
                break;
            }
        }
    }

    void printStatus()
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                return _MonitorSerial->printStatus();
                break;
            case MONITORS::SSH:
                return _MonitorSSH->printStatus();
                break;
            case MONITORS::MICROPHONE:
                return _MonitorMicrophone->printStatus();
                break;
            case MONITORS::ADB:
                return _MonitorADB->printStatus();
                break;
            default:
                break;
            }
        }
    }

    bool setup(Config &conf)
    {
        if (conf.monitor.active_monitor_types[0] < conf.monitor.monitors_type_list.size()) {
            monitor_type = conf.monitor.active_monitor_types[0];
            switch (monitor_type) {
            case MONITORS::SERIAL:
                _MonitorSerial = new MonitorSerial();
                valid_class = true;
                return true;
                break;

            case MONITORS::SSH:
                _MonitorSSH = new MonitorSSH();
                valid_class = true;
                return true;
                break;
            case MONITORS::MICROPHONE:
                _MonitorMicrophone = new MonitorMicrophone();
                valid_class = true;
                return true;
                break;
            case MONITORS::ADB:
                _MonitorADB = new MonitorADB();
                valid_class = true;
                return true;
                break;
            default:
                return false;
                break;
            }
        }
        return false;
    }

    bool IsOpen()
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                return _MonitorSerial->IsOpen();
                break;
            case MONITORS::SSH:
                return _MonitorSSH->IsOpen();
                break;
            case MONITORS::MICROPHONE:
                return _MonitorMicrophone->IsOpen();
                break;
            case MONITORS::ADB:
                return _MonitorADB->IsOpen();
                break;
            default:
                break;
            }
        }
        return false;
    }

    bool init()
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                return _MonitorSerial->init();
                break;
            case MONITORS::SSH:
                return _MonitorSSH->init();
                break;
            case MONITORS::MICROPHONE:
                return _MonitorMicrophone->init();
                break;
            case MONITORS::ADB:
                return _MonitorADB->init();
                break;
            default:
                break;
            }
        }
        return false;
    }

    bool init(Config &conf)
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                return _MonitorSerial->init(conf);
                break;
            case MONITORS::SSH:
                return _MonitorSSH->init(conf);
                break;
            case MONITORS::MICROPHONE:
                return _MonitorMicrophone->init(conf);
                break;
            case MONITORS::ADB:
                return _MonitorADB->init(conf);
                break;
            default:
                break;
            }
        }
        return false;
    }

    void SetMagicWords(vector<string> &magicwords)
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                _MonitorSerial->SetMagicWords(magicwords);
                break;
            case MONITORS::SSH:
                _MonitorSSH->SetMagicWords(magicwords);
                break;
            case MONITORS::MICROPHONE:
                _MonitorMicrophone->SetMagicWords(magicwords);
                break;
            case MONITORS::ADB:
                _MonitorADB->SetMagicWords(magicwords);
                break;
            default:
                break;
            }
        }
    }

    void SetCallback(function<void(string)> callback)
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                _MonitorSerial->SetCallback(callback);
                break;
            case MONITORS::SSH:
                _MonitorSSH->SetCallback(callback);
                break;
            case MONITORS::MICROPHONE:
                _MonitorMicrophone->SetCallback(callback);
                break;
            case MONITORS::ADB:
                _MonitorADB->SetCallback(callback);
                break;
            default:
                break;
            }
        }
    }

    void SetCrashCallback(function<void(bool)> callback)
    {
        if (valid_class) {
            switch (monitor_type) {
            case MONITORS::SERIAL:
                _MonitorSerial->SetCrashCallback(callback);
                break;
            case MONITORS::SSH:
                _MonitorSSH->SetCrashCallback(callback);
                break;
            case MONITORS::MICROPHONE:
                _MonitorMicrophone->SetCrashCallback(callback);
                break;
            case MONITORS::ADB:
                _MonitorADB->SetCrashCallback(callback);
                break;
            default:
                break;
            }
        }
    }
};

#endif
