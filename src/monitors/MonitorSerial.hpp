#pragma once

#ifndef __MONITORSERIAL__
#define __MONITORSERIAL__

#include <functional>
#include <pthread.h>
#include <serial/serial.h>
#include <src/Machine.hpp>
#include <thread>
// #include <pybind11/embed.h>
// #include <pybind11/stl.h>
// #include <src/monitors/MonitorCommon.hpp>

using namespace std;
using namespace serial;

class MonitorSerial {
private:
    const char *TAG = "[Monitor] ";
    string *PortName = nullptr;
    int64_t *BaudRate = nullptr;
    bool *enable = nullptr;

    bool running = false;

    Serial *uart = nullptr;

    vector<string> *MagicWords;

    thread *monitor_serial_thread = nullptr;
    function<void(string)> UserCallback = nullptr;
    function<void(bool)> UserCrashCallback = nullptr;

    bool isOpen;

public:
    string uart_name;
    int uart_baud;

    void printStatus()
    {
        if (this->isOpen)
            GL1G(TAG, "Port ", uart_name, "@", uart_baud, " Opened");
        else if (this->enable && *this->enable)
            GL1R(TAG, "ERROR: Could not open ", uart_name, "@", uart_baud);
        else
            GL1Y(TAG, "Disabled");
    }

    bool IsOpen()
    {
        return this->isOpen;
    }

    ~MonitorSerial()
    {
        if (this->uart) {
            this->uart->close();
            delete this->uart;
        }
        running = false;
    }

    void stop()
    {
        if (this->uart) {
            this->uart->close();
            delete this->uart;
        }
    }

    bool init()
    {
        if (enable && *enable && PortName != nullptr && BaudRate != nullptr)
            return init(*PortName, *BaudRate);

        return false;
    }

    bool init(Config &conf)
    {
        if (conf.monitor.serial_uart.serial_port_name.capacity() < 16)
            conf.monitor.serial_uart.serial_port_name.reserve(16);
        PortName = &conf.monitor.serial_uart.serial_port_name;
        BaudRate = &conf.monitor.serial_uart.serial_baud_rate;
        enable = &conf.monitor.enable;
        MagicWords = &conf.monitor.serial_uart.serial_magic_words;

        return init();
    }

    bool init(string &portname, int64_t &baudrate)
    {
        isOpen = false;
        uart_name = portname;
        uart_baud = baudrate;

        try {
            if (this->uart) {
                delete this->uart;
                this->uart = nullptr;
            }

            this->uart = new Serial(portname,
                                    baudrate,
                                    Timeout::simpleTimeout(100), // Read timeout
                                    false,                       // Disable pooling
                                    true);                       // Disable low latency
            this->isOpen = uart->isOpen();
            uart->setRTS(true);
            uart->setDTR(true);

            if (this->isOpen) {
                // uart->setRTS(true);
                // uart->setDTR(true);
                // uart->setRTS(false);
                // uart->setDTR(false);
                // // Reset
                // this->uart->setDTR(true);
                // usleep(10000);
                // this->uart->setDTR(false);
                // usleep(10000);
            }
        }
        catch (const std::exception &e) {
            this->isOpen = false;
        }

        if (monitor_serial_thread == nullptr) {
            running = true;
            monitor_serial_thread = new thread([&]() {
                enable_idle_scheduler();

                while (running) {
                    bool uart_error = false;
                    if (uart != nullptr && isOpen) {
                        std::string buffer = "";
                        uart->readline(buffer, 4096, "\n", false);
                        int buff_size = buffer.size();

                        if (buff_size > 0) {
                            if (UserCrashCallback != nullptr && MagicWords != nullptr) {
                                for (string &word : *MagicWords) {
                                    if (string_contains(buffer, word)) {
                                        // Crash found
                                        if (enable != nullptr && *enable)
                                            UserCrashCallback(false);
                                        else if (enable == nullptr)
                                            UserCrashCallback(false);
                                        break;
                                    }
                                }
                            }

                            if (UserCallback != nullptr) {
                                if (enable != nullptr && *enable)
                                    UserCallback(buffer);
                                else if (enable == nullptr)
                                    UserCallback(buffer);
                            }
                        }

                        if (uart != nullptr) {
                            uart_error = !uart->isOpen();
                        }
                    }
                    else {
                        uart_error = true;
                    }

                    if (uart_error) {
                        // LOG1("error");
                        isOpen = false;
                        this_thread::sleep_for(1s);
                        init();
                    }
                }
            });
            pthread_setname_np(monitor_serial_thread->native_handle(), "monitor_serial");
        }

        return this->isOpen;
    }

    void SetMagicWords(vector<string> &magicwords)
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
