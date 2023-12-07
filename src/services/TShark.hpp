#ifndef __SVCTSHARK__
#define __SVCTSHARK__

#include "libs/folly/folly/concurrency/UnboundedQueue.h"

#include "Machine.hpp"
#include "MiscUtils.hpp"

class SvcTShark {
private:
    vector<thread> CaptureThreadList;
    folly::UMPSCQueue<uint8_t, true> wait_queue;

public:
    const char *TAG = "[TShark] ";

    bool init()
    {
        auto &TSharkCfg = StateMachine.config.services.t_shark;
        if (!TSharkCfg.enable)
            return false;

        for (auto &iface_name : TSharkCfg.interfaces_name) {
            if (string_contains(iface_name, "usbmon"))
                ProcessExec("modprobe usbmon");

            CaptureThreadList.emplace_back(thread([&]() {
                int file_count = 1;
                while (true) {
                    GL1Y(TAG, "Starting capturing on \"", iface_name, "\"...");
                    string out_log_folder = "./logs/" + StateMachine.config.name;
                    EnsureFolder(out_log_folder);
                    string out_log_path = out_log_folder + "/" + iface_name + "." + to_string(file_count) + ".pcapng";

                    string display_filter;
                    if (TSharkCfg.enable_display_filter)
                        display_filter = "-Y \'" + TSharkCfg.display_filter + "\'";

                    string capture_cmd;

                    if (TSharkCfg.use_tcp_dump_for_capture)
                        capture_cmd = format("tcpdump -i {} -w -", iface_name);
                    else
                        capture_cmd = format("bin/tshark -i {} -q -M 100000 -w -", iface_name);

                    FILE *proc = popen(format("bash -c \"{} | bin/tshark -r - -q -M 100000 {} -w {}\"",
                                              capture_cmd,
                                              display_filter,
                                              out_log_path)
                                           .c_str(),
                                       "r");

                    if (file_count == 1)
                        wait_queue.enqueue(1);

                    pclose(proc);

                    ++file_count;
                    GL1R(TAG, "Error: capturing on \"", iface_name, "\" stopped, restarting...");
                    this_thread::sleep_for(1s);
                }
            }));

            uint8_t res;
            wait_queue.dequeue(res);
        }

        return true;
    }
};

#endif