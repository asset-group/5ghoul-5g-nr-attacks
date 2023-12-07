#pragma once

#ifndef __WDFRAMEWORK__
#define __WDFRAMEWORK__

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <unordered_map>

#include "libs/folly/folly/concurrency/UnboundedQueue.h"

#include "Machine.hpp"
#include "MiscUtils.hpp"
#include "Process.hpp"

// 3rd-party programs for internal use
#define IPTABLES "./3rd-party/hostapd/idemptables"

// Common defines
#define WD_MODULES_PATH "modules/exploits/"
#define WD_PYTHON "modules/python/install/bin/python3"

// Common macros
#define WD_HANDLER_PKT_DRV_TYPE function<void(wd_t *, WDEventQueue<pkt_evt_t> &, T &)>
#define WD_HANDLER_PKT_TYPE function<void(wd_t *, WDEventQueue<pkt_evt_t> &)>

enum wd_hang_detection_t {
    WD_HANG_DETECTION_TX = 0,
    WD_HANG_DETECTION_RX,
    WD_HANG_DETECTION_ANY,
    WD_HANG_DETECTION_CUSTOM
};

enum wd_report_t {
    WD_REPORT_CRASH = 0,
    WD_REPORT_TIMEOUT
};

typedef struct {
    bool label_status;
    string pkt_label;
    string pkt_summary;
    const char *pkt_field_name;
    uint32_t pkt_field_value;
    long label_timing_ns; // Optional
} wd_pkt_label;

typedef struct _pkt_evt_t {
    uint8_t evt;
    uint8_t *pkt_buf;
    uint32_t pkt_len;
    uint8_t pkt_dir;
    char pkt_summary[256];
    uint8_t pkt_save;
    uint8_t pkt_fuzzed;
    uint8_t pkt_duplicated;
    uint64_t pkt_duplicated_id;
    uint64_t time_latency;
    uint32_t o_pkt_len;       // Original Packet Length
    uint8_t o_pkt_buf[16384]; // Original Packet Buffer
    uint8_t _pkt_buf[16384];  // Current Packet Buffer

    /**
     * @brief Saves the input packet with provided length and offset.
     * @param pkt The input packet to be saved.
     * @param length The length of the input packet.
     * @param offset The offset value to be used (default = 0).
     * @details This function saves the input packet with length and offset provided. The input packet is copied to o_pkt_buf.
     * @remarks If pkt_real_len is greater than the size of _pkt_buf, the function will return.
     * @return None.
     */
    inline void save_packet(uint8_t *pkt, uint32_t length, uint16_t offset = 0)
    {
        uint32_t pkt_real_len = length - offset;

        if (G_UNLIKELY(pkt_real_len > sizeof(_pkt_buf)))
            return;

        o_pkt_len = pkt_real_len;
        pkt_len = pkt_real_len;
        pkt_buf = o_pkt_buf;

        memcpy(o_pkt_buf, pkt + offset, pkt_real_len);
    }

    /**
     * @brief Sets the summary string for the packet.
     *
     * @param summary_str A pointer to the summary string to be set.
     *
     * @details This function sets the summary string for the packet with the provided input string.
     *
     * @remarks The maximum length of the summary string is limited by the size of the pkt_summary array.
     *
     * @return Void.
     */
    inline void set_summary(const char *summary_str)
    {
        strncpy(pkt_summary, summary_str, sizeof(pkt_summary));
    }

} pkt_evt_t;

/**
 * @brief WDSignalHandler handles signals and program exit.
 * Only 1 instance of WDSignalHandler per process is allowed.
 * Otherwise, undefined behaviour may happen
 *
 */
class WDSignalHandler {

private:
    static WDSignalHandler *this_cls;
    vector<function<void(void)>> InstancesToHandle;
    unordered_map<int, function<int(int)>> SignalHandlers;
    unordered_map<int, int> SignalEntered;
    function<void(void)> UserExitHandle;
    thread *watchdog_thread;
    React::Loop watchdog_loop;

    /**
     * @brief Restores the default signal handler and raises a signal.
     *
     * @param signal The signal to be raised.
     *
     * @details This function restores the default signal handler of a given signal and raises it.
     * The function first creates a sigaction struct with empty signal mask, SIG_DFL as handler, and 0 flags.
     * Then, the sigaction function is called to set the signal action to the default value using the created struct.
     * Finally, the raise function is called to raise the signal.
     *
     * @remarks This function should only be used if necessary as it might interfere with system processes.
     *
     * @return This function does not return a value.
     */
    static void call_default_sighandler(int signal)
    {
        struct sigaction sa_default;

        sigemptyset(&sa_default.sa_mask);
        sa_default.sa_handler = SIG_DFL;
        sa_default.sa_flags = 0;

        sigaction(signal, &sa_default, NULL);
        raise(signal);
    }

    /**
     * @brief Handles a signal and performs the corresponding action.
     *
     * @details This function is called when a signal is received. Depending on the signal received, the
     * function will perform the corresponding action like initiating the exit procedure, calling signal handlers,
     * setting folder permissions etc.
     *
     * @remarks The function sets a flag to indicate the ongoing exit procedure, and terminates the process if
     * a timeout occurs. It also logs messages to the console and exits the process gracefully if the user
     * specifies a user exit handle. The function also checks for user handlers and instance handlers.
     *
     * @param signal The signal identifier to handle.
     *
     * @return void.
     */
    static void signal_handler(int signal)
    {
        WDSignalHandler *cls = WDSignalHandler::this_cls;

        if (!cls)
            return;

        if (!cls->Exiting &&
            ((signal == SIGINT) ||
             (signal == SIGTERM) ||
             (signal == SIGQUIT) ||
             (signal == SIGUSR1))) {
            // Ensure process is quit after a timeout
            cls->watchdog_loop.onTimeout(2, []() {
                LOGR("Exit Timeout (2s)!!! Forcing program exit with SIGQUIT");
                call_default_sighandler(SIGQUIT);
            });
        }

        // Flag to indicate ongoing exit procedure
        cls->Exiting = true;

        // Write new line to console
        ::putc_unlocked('\n', stdout);

        if (signal == SIGUSR1)
            LOG2M(cls->TAG, "GUI Closed");

        if (cls->SignalEntered.find(signal) != cls->SignalEntered.end()) {
            if (cls->SignalEntered[signal] == 0)
                cls->SignalEntered[signal] = 1;
            else
                return;
        }

        if (cls->SignalHandlers.find(signal) != cls->SignalHandlers.end()) {
            // If user handler returns 0, do not perform exit procedure for this signal
            int ret = cls->SignalHandlers[signal](signal);
            cls->SignalEntered[signal] = 0;
        }

        // Call Instance handlers
        for (auto &inst : cls->InstancesToHandle) {
            inst();
        }

        LOG3M(cls->TAG, "Exiting ", ProcessName());

        if (cls->UserExitHandle) {
            cls->UserExitHandle();
        }

        SetFolderPermission("0775", "logs/" + StateMachine.config.name);

        if ((signal == SIGINT) ||
            (signal == SIGUSR1)) {
            call_default_sighandler(SIGINT);
        }
    }

public:
    const char *TAG = "[SignalHandler] ";

    bool Exiting = false;

    /**
     * @brief Initializes the object and sets up signal handlers and watchdog loop.
     *
     * @details The function initializes the object and sets up the signal handlers for SIGUSR1 and SIGINT. It also creates
     * a new thread for the watchdog loop and sets its name to "watchdog_loop".
     *
     * @remarks This function must be called before using any other function of the object.
     *
     * @return true if successful, false otherwise.
     */
    bool init()
    {
        WDSignalHandler::this_cls = this;
        SignalEntered[SIGUSR1] = 0;
        SignalEntered[SIGINT] = 0;
        std::signal(SIGUSR1, signal_handler);
        std::signal(SIGINT, signal_handler);

        // Start signal watchdog loop
        watchdog_thread = new thread([&]() {
            enable_idle_scheduler();
            watchdog_loop.run();
        });
        pthread_setname_np(watchdog_thread->native_handle(), "watchdog_loop");
        watchdog_thread->detach();

        return true;
    }

    /**
     * @brief Sets the callback function for signal handling.
     * @param sig The signal number to be handled.
     * @param fcn The function to be called upon signal reception.
     * @details This function sets the callback function to be executed when a specific signal is received. This function also initializes SignalEntered[sig] to zero and maps SignalHandlers[sig] to the provided function.
     * @remarks The function signal_handler is called when the signal is received, which in turn calls the callback function set by this function.
     * @return void
     */
    void SetSignalCallback(int sig, function<int(int)> fcn)
    {
        SignalEntered[sig] = 0;
        SignalHandlers[sig] = fcn;
        std::signal(sig, signal_handler);
    }

    /**
     * @brief Sets the callback function for the user-defined exit handling.
     *
     * @details The SetExitCallback function accepts a user-defined function and sets it as the exit callback function
     *          for the program. This function will be invoked when the program is terminated.
     *
     * @param fcn The user-defined function to be set as the exit callback.
     *
     * @remarks The SetExitCallback function can be used to customize the behavior of the program when it is about to
     *          exit. Users can define their own function and set it as the exit callback function to handle any custom
     *          cleanup or resource release tasks.
     *
     * @return None.
     */
    void SetExitCallback(function<void(void)> fcn)
    {
        UserExitHandle = fcn;
    }

    template <class T>
    bool CallStop(T &Instance, bool call_stop_on_exit = true, function<void(T &)> optional_callback = nullptr)
    {
        constexpr bool has_stop = requires(T &t) { t.stop(); };

        if constexpr (!has_stop)
            GL1R(TAG, "Class ", CLASS_NAME(T), " does not have stop() function");

        InstancesToHandle.push_back([this, &Instance, call_stop_on_exit, has_stop, optional_callback]() {
            if (optional_callback)
                optional_callback(Instance);

            if (call_stop_on_exit) {
                // Only call stop() if instance implements it
                if constexpr (std::is_same_v<ProcessRunner, T>) {
                    Instance.stop(true, true);
                }
                if constexpr (has_stop) {
                    Instance.stop();
                }
            }
        });

        return true;
    }
};

WDSignalHandler *WDSignalHandler::this_cls = nullptr;

template <class T>
class WDEventQueue {

private:
    folly::UMPSCQueue<T, true, 10> queue_evt;
    function<void(T &)> evt_callback = nullptr;

public:
    const char *TAG = "[EventQueue] ";

    /**
     * Push an event into the object queue.
     *
     * @tparam T Type of the event object.
     * @param pkt_evt The event object to push.
     * @details This function takes ownership of the event object and enqueues it to the object queue.
     * @remarks In the case that the event object is a packet event, the function will perform a copy of the packet buffer.
     * @return void
     */
    void PushEvent(T &pkt_evt)
    {
        if constexpr (std::is_same_v<pkt_evt_t, T>) {
            if (G_LIKELY((pkt_evt.pkt_buf != pkt_evt.o_pkt_buf) && pkt_evt.pkt_len)) {
                memcpy(pkt_evt._pkt_buf, pkt_evt.pkt_buf, pkt_evt.pkt_len);
            }
        }

        // Take ownership and enqueue object
        queue_evt.enqueue(std::move(pkt_evt));
    }

    /**
     * @brief Set the event handler function.
     *
     * @details This function sets the event handler function for the event queue.
     * The function is called when an event is dequeued from the queue. If the
     * realtime parameter is set to true, the real-time scheduler is enabled.
     * Otherwise, the idle scheduler is enabled.
     *
     * @param fcn The function to be called when an event is dequeued.
     * @param realtime If set to true, enables real-time scheduler. Defaults to false.
     *
     * @remarks This function spawns a thread to manage the event queue. If the
     * task has already been started, it exits early. The thread is detached and
     * runs independently from the thread that called SetEventHandler.
     *
     * @return void
     */
    void SetEventHandler(function<void(T &)> fcn, bool realtime = false)
    {
        static uint8_t task_started = 0;

        evt_callback = fcn;

        if (task_started)
            return;

        thread *t = new thread([this, realtime] {
            if (realtime)
                enable_rt_scheduler();
            else
                enable_idle_scheduler();

            T pkt_evt = {0};

            while (true) {
                queue_evt.dequeue(pkt_evt);
                if (G_UNLIKELY(!evt_callback))
                    continue;

                evt_callback(pkt_evt);
            }
        });
        pthread_setname_np(t->native_handle(), "wd_event_queue");
        t->detach();

        task_started = 1;
    }
};

/**

@class WDPacketHandler
@brief This class provides an interface to handle network packets and manage the threads used for packet handling.
- Uses a WDEventQueue to handle packet events and manage the event handler callback.
    -# The class has two main methods for adding packet handlers: AddPacketHandlerWithDriver and AddPacketHandler.
    -# The first one takes an additional driver parameter and a callback function with a driver parameter, while the second one takes just the callback function.
    -# Both methods take the following parameters: direction, dissection_mode, realtime and proto_name.
- The class uses the wdissector library to initialize and handle packets. It also uses the folly library for thread-safe queues.
- It uses a vector of threads to store the threads that are created for handling packets.
- The direction parameter is used to specify the packet direction (ingress or egress).
- The dissection_mode parameter is used to specify the mode of dissection (normal or verbose).
- The realtime parameter is used to enable real-time scheduling for the thread handling the packets.
- The proto_name parameter is used to specify the protocol name for the wdissector instance.
- The class also provides a SetPacketEventsHandler method to set the event handler callback and a PushPacketEvent method to push packet events to the WDEventQueue.
- The class has a Run method that starts the packet handling threads and a stop method to cancel and join all the threads.
*/
class WDPacketHandler {
private:
    vector<thread> threads_list;
    vector<function<bool()>> LauncherList;
    WDEventQueue<pkt_evt_t> PacketEventQueue;
    vector<wd_t *> WDInstances;
    folly::UMPMCQueue<long, true> startup_queue;
    folly::UMPMCQueue<long, true> ready_queue;

    template <class T>
    void handler_startup_with_driver(T &Driver,
                                     WD_HANDLER_PKT_DRV_TYPE user_callback,
                                     uint8_t direction,
                                     wd_dissection_mode dissection_mode,
                                     bool enable_rt,
                                     string proto_name)
    {
        // Name thread
        string thread_name = format("ph_{}", string_split(proto_name, ":").back());
        if (thread_name.size() >= 15) // Truncate name if needed
            thread_name = string(&thread_name.c_str()[0], &thread_name.c_str()[14]);
        pthread_setname_np(pthread_self(), thread_name.c_str());

        // Init wdissector
        wd_t *wd = wd_init(proto_name.c_str());
        WDInstances.push_back(wd);

        if (!wd) {
            LOG4R(TAG, "wd_init for ", proto_name, " failed");
            startup_queue.enqueue(0);
            return;
        }

        wd_set_packet_direction(wd, direction);
        wd_set_dissection_mode(wd, dissection_mode);

        // Notify correct initialization of wdissector
        startup_queue.enqueue(gettid());

        long run;
        // Wait Run to be called
        ready_queue.dequeue(run);

        if (!run)
            return;

        if (enable_rt)
            enable_rt_scheduler();
        else
            enable_idle_scheduler();

        user_callback(wd, PacketEventQueue, Driver);
    }

    /**
     * @brief Initializes a new thread instance for the specified network protocol and configures it.
     *
     * @param user_callback A pointer to the user's custom callback function to be executed by the thread instance.
     * @param direction The direction of network traffic (inbound or outbound) to be monitored by the thread instance.
     * @param dissection_mode The desired mode for protocol dissection by the thread instance.
     * @param enable_rt A boolean flag indicating whether real-time scheduling should be enabled for the handler thread.
     * @param proto_name The name of the network protocol to be monitored by the thread instance.
     *
     * @details This function initializes a new thread instance using the specified network protocol name and adds it to the global WDInstances vector.
     * The instance configuration is performed by setting the packet direction and dissection mode using wd_set_packet_direction() and wd_set_dissection_mode() functions, respectively.
     * The function also enqueues the current thread ID to the startup queue using startup_queue.enqueue(gettid()) for synchronization purposes.
     * If wd_init() fails, a message is logged using LOG4R() and the function returns immediately.
     * After thread initialization, the function dequeues the thread ID from the startup queue and cancels the thread's cancel state using pthread_setcancelstate().
     * If real-time scheduling (enable_rt) is requested, the enable_rt_scheduler() function is invoked to activate it.
     * Finally, the user-defined callback function (user_callback) is executed with the newly created thread instance as the first argument and the global PacketEventQueue as the second argument.
     *
     * @remarks This function assumes that the PacketEventQueue global variable has already been initialized.
     *
     * @return void
     */
    void handler_startup(WD_HANDLER_PKT_TYPE user_callback,
                         uint8_t direction,
                         wd_dissection_mode dissection_mode,
                         bool enable_rt,
                         string proto_name)
    {
        // Name thread
        string thread_name = format("ph_{}", string_split(proto_name, ":").back());
        if (thread_name.size() >= 15) // Truncate name if needed
            thread_name = string(&thread_name.c_str()[0], &thread_name.c_str()[14]);
        pthread_setname_np(pthread_self(), thread_name.c_str());

        // Init wdissector
        wd_t *wd = wd_init(proto_name.c_str());
        WDInstances.push_back(wd);

        if (!wd) {
            LOG4R(TAG, "wd_init for ", proto_name, " failed");
            startup_queue.enqueue(0);
            return;
        }

        wd_set_packet_direction(wd, direction);
        wd_set_dissection_mode(wd, dissection_mode);

        // Notify correct initialization of wdissector
        startup_queue.enqueue(gettid());

        long run;
        // Wait Run to be called
        startup_queue.dequeue(run);

        if (!run)
            return;

        // Allow thread to be cancelable
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        if (enable_rt)
            enable_rt_scheduler();

        user_callback(wd, PacketEventQueue);
    }

public:
    const char *TAG = "[PacketHandler] ";

    template <class T>
    bool AddPacketHandlerWithDriver(T &DriverPtr,
                                    WD_HANDLER_PKT_DRV_TYPE fcn,
                                    uint8_t direction = WD_DIR_ANY,
                                    wd_dissection_mode dissection_mode = WD_MODE_NORMAL,
                                    bool realtime = true,
                                    const char *proto_name = NULL,
                                    int module_group = 0)
    {
        string proto_name_str;

        if (proto_name == NULL)
            proto_name_str = StateMachine.config.options.default_protocol_name;
        else
            proto_name_str = proto_name;

        LauncherList.push_back([this, proto_name_str, &DriverPtr, fcn, direction, dissection_mode, realtime]() -> bool {
            threads_list.push_back(thread(&WDPacketHandler::handler_startup_with_driver<T>, this, ref(DriverPtr), fcn, direction, dissection_mode, realtime, proto_name_str));
            long init_val;
            // Wait thread to correctly initialize wdissector instance
            startup_queue.dequeue(init_val);
            GL1Y(TAG, "Added \"", proto_name_str, "\", Dir:", (int)direction, ", Realtime:", realtime, ", TID:", init_val);
            return init_val;
        });

        return true;
    }

    /**
     * @brief Add a packet handler to the list of handlers to be launched with Run()
     *
     * @param fcn WD_HANDLER_PKT_TYPE representing the function pointer to the packet handler
     * @param direction uint8_t representing the packet direction to be handled
     * @param dissection_mode wd_dissection_mode representing the dissection mode for the handler
     * @param realtime bool representing whether the handler should run in real-time or not
     * @param proto_name const char* representing the protocol name for the handler. If NULL, default protocol name is used
     *
     * @details This function adds a packet handler to the list of handlers to be launched at application startup.
     * The application will launch a thread for each handler added, passing the provided parameters.
     *
     * @remarks Packet handlers added using this function will be launched in a separate thread on application startup.
     * Each thread will be named "handler_protocol_name_str", where protocol_name_str is either the provided protocol name or the default protocol name
     *
     * @return bool representing whether the addition of the packet handler was successful or not
     */
    bool AddPacketHandler(WD_HANDLER_PKT_TYPE fcn,
                          uint8_t direction = WD_DIR_ANY,
                          wd_dissection_mode dissection_mode = WD_MODE_NORMAL,
                          bool realtime = true,
                          const char *proto_name = NULL)
    {
        string proto_name_str;

        if (proto_name == NULL)
            proto_name_str = StateMachine.config.options.default_protocol_name;
        else
            proto_name_str = proto_name;

        LauncherList.push_back([this, proto_name_str, fcn, direction, dissection_mode, realtime]() -> bool {
            threads_list.push_back(thread(&WDPacketHandler::handler_startup, this, fcn, direction, dissection_mode, realtime, proto_name_str));
            long init_val;
            // Wait thread to correctly initialize wdissector instance
            startup_queue.dequeue(init_val);
            GL1Y(TAG, "Added \"", proto_name_str, "\", Dir:", (int)direction, ", Realtime:", realtime, ", TID:", init_val);
            return init_val;
        });

        return true;
    }

    /**
     * @brief Sets the event handler for packet events in the PacketEventQueue.
     *
     * @param fcn A function with signature (pkt_evt_t&), which will be called when a packet event occurs.
     * @param realtime If true, the event handler will be called in real-time. Default value is false.
     * @details This function sets the event handler for packet events in the PacketEventQueue. The event handler will be called
     * whenever a packet event occurs. The event handler function should have a signature of (pkt_evt_t&). The event handler can
     * be set to be called in real-time or not.
     * @remarks This function returns true always, as there is no error that can prevent the event handler from being set.
     * @return true always.
     */
    bool SetPacketEventsHandler(function<void(pkt_evt_t &)> fcn, bool realtime = false)
    {
        PacketEventQueue.SetEventHandler(fcn, realtime);
        return true;
    }

    /**
     * @brief Pushes a packet event to the queue.
     *
     * @param pkt_evt Packet event to be pushed.
     *
     * @details This function is used to push a packet event to the queue for processing.
     *
     * @return None.
     */
    inline void PushPacketEvent(pkt_evt_t &pkt_evt)
    {
        PacketEventQueue.PushEvent(pkt_evt);
    }

    /**
     * @brief Executes all the functions in the list of LauncherList
     *
     * @details This function runs all the functions in the LauncherList and then enters an infinite while loop that only sleeps for 1000 milliseconds.
     *
     * @remarks This function assumes that LauncherList has already been initialized.
     *
     * @return void
     */
    void Run()
    {
        for (auto &l : LauncherList) {
            l();
        }

        for (thread &t : threads_list) {
            // Notify that user callback can run
            ready_queue.enqueue(1);
        }

        while (true)
            sleep(1000);
    }

    /**
     * @brief Stop function cancels all threads and frees all work distributors
     * @details The function first frees all work distributors by calling the "wd_free" function
     * for each instance of the "WDInstances" vector. Then, it cancels all threads by calling
     * the "pthread_cancel" function on each thread's native handle. Finally, it joins all threads
     * that are still joinable by calling the "join" function.
     *
     * @remarks This function should be called when the program needs to be stopped or when
     * no further work distribution is required. It is important to note that the function directly
     * cancels the threads' native handles and may cause undefined behavior if the threads are not
     * prepared for it.
     *
     * @return void
     */
    void stop()
    {
        // Cancel all threads
        GL1Y(TAG, "Stopping Threads");
        for (auto &wd : WDInstances)
            wd_free(&wd);

        for (thread &t : threads_list) {
            pthread_cancel(t.native_handle());
            if (t.joinable())
                t.join();
        }
    }
};

class WDGlobalTimeout {
private:
    shared_ptr<React::IntervalWatcher> timeout = nullptr;
    MainLoop event_loop;
    shared_ptr<thread> event_thread;
    vector<function<bool(void)>> TimeoutHandlers;
    vector<function<bool(void)>> TimeoutUsrCallbacks;

    bool loop_running = false;
    int _timeout_ms;

    /**
     * @brief Global timeout callback function that executes a set of user-defined and internal timeout callbacks.
     *
     * @details This function is called when a timer expires and executes a set of user-defined timeout callbacks
     *          as well as any internal timeout callbacks. The function also increments the timeout counters
     *          (TimeoutCounter and TotalTimeoutCounter) and sets a flag to indicate that the timeout has already been triggered.
     *          The function returns a boolean value indicating whether to rearm (true) or not (false) the timer.
     *
     * @remarks The function iterates over two sets of callback functions: TimeoutUsrCallbacks and TimeoutHandlers, and
     *          executes each callback function in the set. The function also keeps track of the number of callbacks executed
     *          and whether any of the callbacks returned a non-false value. If a callback function in the TimeoutHandlers set
     *          returns a true value, the function resets the TimeoutCounter and logs a message indicating that the counter has been
     *          reset. The function may also return a true value if there are no user-defined callbacks to execute.
     *
     * @return A boolean value indicating whether to rearm (true) or not (false) the timer.
     */
    bool global_timeout_callback()
    {
        bool ret = false;
        bool has_usr_fcns = true;
        int n_callbacks = 0;

        TimeoutCounter++;
        TotalTimeoutCounter++;

        GL1Y(TAG, "Timeout triggered. TimeoutCounter=", TimeoutCounter, ", TotalTimeoutCounter=", TotalTimeoutCounter);

        has_usr_fcns = (TimeoutUsrCallbacks.size() ? true : false);

        for (auto &h : TimeoutUsrCallbacks) {
            n_callbacks += 1;
            ret |= h();
        }

        for (auto &h : TimeoutHandlers) {
            n_callbacks += 1;
            if (h() == true) {
                TimeoutCounter = 0;
                GL1Y(TAG, "TimeoutCounter reset");
                break;
            }
        }

        TimeoutAlreadyTriggered = true;

        GL1Y(TAG, "Timeout Finished with ", n_callbacks, " callbacks executed");

        // Whether to rearm (true) or not (false) the timer
        if (has_usr_fcns)
            return ret;
        else
            return true;
    }

    /**
     * @brief Starts the internal callback function.
     *
     * The function cancels the timeout and sets it to null, initializes with the timeout and logs the initialization message.
     *
     * @details The function initializes the timeout by calling the `onIntervalMS` function of the event loop.
     * It takes the `_timeout_ms` and a lambda expression which calls the `global_timeout_callback()` function.
     *
     * @remarks The function assumes `timeout` is a global variable of type `std::unique_ptr<TimerHandle>`.
     *
     * @return None.
     */
    void StartInternalCallback()
    {
        if (timeout) {
            timeout->cancel();
            timeout = nullptr;
        }

        GL1Y(TAG, "Initialized with ", _timeout_ms / 1000, " seconds");
        timeout = event_loop.onIntervalMS(_timeout_ms, [&]() -> bool {
            return global_timeout_callback();
        });
    }

public:
    const char *TAG = "[GlobalTimeout] ";
    int TimeoutCounter = 0;
    int TotalTimeoutCounter = 0;
    bool TimeoutAlreadyTriggered = false;

    /**
     * \brief Initializes GlobalTimeout configuration.
     *
     * \details This function initializes GlobalTimeout configuration by setting the global timeout
     *          if enabled in the configuration file.
     *
     * \remarks If the global timeout is not enabled in the configuration file, this function will return.
     *
     * \return No value is returned.
     */
    void init()
    {
        Fuzzing &fuzzing = StateMachine.config.fuzzing;
        if (!fuzzing.global_timeout) {
            GL1R(TAG, "Not enabled in config. file");
            return;
        }

        init(fuzzing.global_timeout_seconds * 1000);
    }

    /**
     * Initializes the watchdog timer with the specified parameters.
     *
     * @param timeout_ms The timeout duration in milliseconds.
     * @param autostart Indicates whether to automatically start the timer or not.
     *        Default value is true.
     * @param fcn A function pointer to a callback that will be executed when the
     *        timeout occurs. Default value is nullptr.
     *
     * @details This function initializes the watchdog timer with the given timeout
     * duration and callback function. If autostart is true (default), then the
     * timer will automatically start running. Otherwise, it will need to be started
     * manually using the StartInternalCallback() function.
     *
     * @remarks The function uses a thread to run the event loop, and assigns a name
     * to the thread. If fcn is not null, the callback function will be added to the
     * internal list of timeout callbacks to be called on timer expiry.
     *
     * @return void
     */
    void init(uint64_t timeout_ms, bool autostart = true, function<bool(WDGlobalTimeout &, void *usr_ptr)> fcn = nullptr)
    {
        if (!loop_running)
            loop_running = true;

        _timeout_ms = timeout_ms;

        // Event Loop
        event_thread = make_shared<thread>([&]() {
            enable_idle_scheduler();
            event_loop.run(true);
        });
        pthread_setname_np(event_thread->native_handle(), "wdtimer_event_loop");
        event_thread->detach();

        if (fcn)
            AddTimeoutCallback(fcn);

        if (autostart) {
            StartInternalCallback();
        }
    }

    template <class T>
    void CallIndicateTimeout(T &Instance, function<void(void)> pre_timeout_callback = nullptr)
    {
        constexpr bool has_indicate_timeout = requires(T &t) { t.IndicateTimeout(*this, TimeoutCounter); };

        if constexpr (!has_indicate_timeout) {
            GL1M(TAG, "Class ", CLASS_NAME(T), " does not have IndicateTimeout(WDGlobalTimeout&, int) function");
            return;
        }

        TimeoutHandlers.push_back([this, &Instance, has_indicate_timeout, pre_timeout_callback]() -> bool {
            // Only call IndicateTimeout() if instance implements it
            GL1(TAG, "Signalling ", CLASS_NAME(T));
            if constexpr (has_indicate_timeout) {
                if (pre_timeout_callback)
                    pre_timeout_callback();
                return Instance.IndicateTimeout(*this, TimeoutCounter);
            }
        });
    }

    template <class T>
    void CallStop(T &Instance, function<void(void)> pre_timeout_callback = nullptr)
    {
        TimeoutHandlers.push_back([this, &Instance, pre_timeout_callback]() -> bool {
            // Only call IndicateTimeout() if instance implements it
            GL1(TAG, "Stopping ", CLASS_NAME(T));
            if (pre_timeout_callback)
                pre_timeout_callback();
            Instance.stop();
            return false;
        });
    }

    template <class T>
    void CallRestart(T &Instance, function<void(void)> pre_timeout_callback = nullptr)
    {
        TimeoutHandlers.push_back([this, &Instance, pre_timeout_callback]() -> bool {
            // Only call IndicateTimeout() if instance implements it
            GL1(TAG, "Restarting ", CLASS_NAME(T));
            if (pre_timeout_callback)
                pre_timeout_callback();
            Instance.restart(true);
            return false;
        });
    }

    /**
     * @brief Adds a timeout callback function to the TimeoutUsrCallbacks list.
     *
     * The callback function will be invoked with the WDGlobalTimeout and data_ptr arguments.
     *
     * @param fcn The callback function to add.
     * @param data_ptr A void pointer used to pass any additional data required by the callback function.
     * @details The callback function will be invoked whenever the global timeout expires.
     * @remarks This function does nothing and returns if the callback function is NULL.
     * @return void
     */
    void AddTimeoutCallback(function<bool(WDGlobalTimeout &, void *)> fcn, void *data_ptr = NULL)
    {
        if (G_UNLIKELY(!fcn))
            return;

        TimeoutUsrCallbacks.push_back([this, fcn, data_ptr]() -> bool {
            return fcn(*this, data_ptr);
        });
    }

    void AddTimeoutCallback(function<bool()> fcn)
    {
        if (G_UNLIKELY(!fcn))
            return;

        TimeoutUsrCallbacks.push_back(fcn);
    }

    void AddTimeoutCallback(function<void()> fcn)
    {
        if (G_UNLIKELY(!fcn))
            return;

        TimeoutUsrCallbacks.push_back([fcn]() -> bool {
            fcn();
            return true;
        });
    }

    /**
     * \brief Sets the timeout for a given number of milliseconds.
     *
     * \param timeout_ms The timeout in milliseconds.
     *
     * \details If the timeout has not previously been set, this function starts
     * an internal callback. The `TimeoutAlreadyTriggered` flag is reset to false.
     *
     * \remarks This function uses `_timeout_ms` and `timeout` variables to set
     * the timeout interval.
     *
     * \return None.
     */
    void SetTimeout(int timeout_ms)
    {
        if (G_UNLIKELY(!timeout))
            StartInternalCallback();

        _timeout_ms = timeout_ms;
        TimeoutAlreadyTriggered = false;
        timeout->set(((double)_timeout_ms) / 1000.0);
    }

    /**
     * @brief Stops the current timeout if it exists.
     *
     * @details Checks if there is a current timeout and if so, cancels it. If there is no current timeout, this function does nothing.
     *
     * @return None.
     */
    inline void StopTimeout()
    {
        if (G_UNLIKELY(!timeout))
            return;

        timeout->cancel();
    }

    /**
     * @brief RestartTimeout restarts timer timeout.
     * @param clear_timeout_counter If true, timeout counter will be set to 0.
     * @details This function restarts the timer for the timeout event. If the timeout
     *          is not set, it will exit gracefully. If clear_timeout_counter is true,
     *          the timeout counter will be reset to 0.
     * @remarks This function is implemented inline for faster execution.
     * @return void
     */
    inline void RestartTimeout(bool clear_timeout_counter = false)
    {
        if (G_UNLIKELY(!timeout))
            return;

        TimeoutAlreadyTriggered = false;
        timeout->set(timeout->current_interval);

        if (clear_timeout_counter)
            TimeoutCounter = 0;
    }

    /**
     * @brief Clears all the callbacks registered for timeouts.
     *
     * @details This function removes all the functions that were registered
     * to be called when any timeout occurs.
     *
     * @remarks Please note that this function only clears the registered
     * callbacks and does not cancel any timeouts that are already set.
     *
     * @return void
     */
    void ClearTimeoutCallbacks()
    {
        TimeoutUsrCallbacks.clear();
    }

    /**
     * @brief Clears all callback functions and their associated timeout handlers.
     *
     * @details This function clears TimeoutCallbacks by calling ClearTimeoutCallbacks and clears TimeoutHandlers
     * by clearing the associated vector. This ensures that all registered callbacks and handler functions are removed
     * and will no longer be executed when the associated timeout elapses.
     *
     * @remarks This function should be use when resetting or shutting down a system that utilizes timeout
     * callbacks to prevent unwanted callback execution.
     *
     * @return void
     */
    void ClearAllCallbacks()
    {
        ClearTimeoutCallbacks();
        TimeoutHandlers.clear();
    }

    /**
     * @brief Alias for StopTimeout
     *
     * @return void.
     */
    void stop()
    {
        StopTimeout();
    }
};

class WDAnomalyReport {
private:
    function<void(WDAnomalyReport &, wd_report_t)> usr_report_callback = nullptr;
    vector<function<void(string, bool)>> LoggerSinks;

public:
    const char *TAG = "[AnomalyReport] ";

    uint32_t counter_crashes = 0;
    uint32_t counter_timeouts = 0;

    /**
     * @brief Add Sink to anomaly report
     * with logger
     *
     * @param logger
     * @param global_timeout
     */
    template <class T>
    bool AddLogSink(T &logger, bool init_instance = false)
    {

        LoggerSinks.push_back([&](string msg, bool error_msg) {
            logger.writeLog(msg, error_msg);
        });

        constexpr bool has_init = requires(T &t) { t.init(); };

        if (init_instance) {
            if constexpr (has_init)
                logger.init();
            else
                GL1R(TAG, "Class ", CLASS_NAME(T), " does not have \"void init()\" function");
        }

        GL1Y(TAG, "Added Logging Sink: ", CLASS_NAME(T));

        return true;
    }

    void IndicateCrash()
    {
        IndicateCrash(format("[Crash] Crash detected at state \"{}\"", StateMachine.GetCurrentStateName()));
    }

    void IndicateCrash(string msg, bool error_msg = true)
    {
        GL1R(TAG, msg);

        counter_crashes += 1;

        if (usr_report_callback)
            usr_report_callback(*this, WD_REPORT_CRASH);

        if (LoggerSinks.size()) {
            for (auto &_logger : LoggerSinks)
                _logger(msg, error_msg);
        }
        else
            GL1R(TAG, "No logger sink added");
    }

    bool IndicateTimeout(WDGlobalTimeout &global_timeout, int ext_timeouts_count)
    {
        IndicateTimeout("[Timeout] Target is not responding",
                        !global_timeout.TimeoutAlreadyTriggered);
        return false;
    }

    void IndicateTimeout(string msg, bool error_msg = true)
    {
        GL1R(TAG, msg);

        counter_timeouts += 1;

        if (usr_report_callback)
            usr_report_callback(*this, WD_REPORT_TIMEOUT);

        if (LoggerSinks.size()) {
            for (auto &_logger : LoggerSinks)
                _logger(msg, error_msg);
        }
        else
            GL1R(TAG, "No logger sink added");
    }

    void IndicateTargetReset(string msg, bool error_msg = true)
    {
        IndicateCrash(msg, error_msg);
    }

    void IndicateTargetReset()
    {
        IndicateCrash("[Reset] Target Power Reset");
    }

    void SetReportCallback(function<void(WDAnomalyReport &, wd_report_t)> fcn)
    {
        usr_report_callback = fcn;
    }

    void ClearTimeoutsCount()
    {
        counter_timeouts = 0;
    }

    void ClearCrashCount()
    {
        counter_crashes = 0;
    }

    void Clear()
    {
        ClearTimeoutsCount();
        ClearCrashCount();
    }
};

class WDPacketLabelGenerator {
private:
    const char *TAG = "[PktLabelGen] ";
    Machine PacketMapper;
    wd_t *wd;
    uint8_t measure_time = 0;

public:
    bool init(const char *config_file, uint8_t enable_time_measurement = 0)
    {
        // Initialize StateMachine object to compile state mapping filtering rules
        bool ret = PacketMapper.init(config_file);

        if (!ret) {
            GL1R(TAG, "Error: Machine instance not initialized successfully");
            return false;
        }

        // Check if StateMap vector is not empty
        if (!PacketMapper.GetStateMap().size()) {
            GL1R(TAG, "Error: Mapping Rules are empty or not compiled correctly");
            return false;
        }

        // Get wd instance that was initialized by PacketMapper.init
        wd = PacketMapper.wd;
        // Enable or disable processing time measurement
        measure_time = enable_time_measurement;
        // Set dissection mode to fast since we won't read all fields in the packet
        wd_set_dissection_mode(wd, WD_MODE_FAST);
        // Set protocol to property "DefaultProtocolEncapName" from the config_file
        if (wd_set_protocol(wd, PacketMapper.config.options.default_protocol_encap_name.c_str()))
            GL1G(TAG, "Protocol set to ", PacketMapper.config.options.default_protocol_encap_name);
        else {
            GL1R(TAG, "Error setting protocol to ", PacketMapper.config.options.default_protocol_encap_name);
            wd_free(&wd);
            return false;
        }

        return true;
    }

    wd_pkt_label LabelPacket(uint32_t pkt_direction, uint8_t *pkt_buf, uint32_t pkt_length)
    {
        // Declare label structure to be returned to caler
        wd_pkt_label pkt_label;
        struct timespec time_start;

        // Start measure processing time if measure_time is enabled
        if (G_UNLIKELY(measure_time))
            clock_gettime(CLOCK_MONOTONIC, &time_start);
        // Prepare state mapper (must be done before each decoding)
        PacketMapper.PrepareStateMapper(wd);
        // Set direction of packet (WD_DIR_TX or WD_DIR_RX)
        wd_set_packet_direction(wd, pkt_direction);
        // Decode packet buffer
        wd_packet_dissect(wd, pkt_buf, pkt_length);
        // Run State Mapper to generate a label of the packet based on the mapping rules
        if (PacketMapper.RunStateMapper(wd, false, false, false, true)) {
            // Indicate successful labeling
            pkt_label.label_status = true;
            // Copy generated label to pkt_label
            pkt_label.pkt_label = PacketMapper.DissectedStateName;
            // Copy name of matched "StateNameField" to pkt_label
            pkt_label.pkt_field_name = PacketMapper.DissectedStateFieldName;
            // Copy value of matched "StateNameField" to pkt_label
            pkt_label.pkt_field_value = PacketMapper.DissectedStateFieldValue;
            // Copy decoded summary to pkt_label
            pkt_label.pkt_summary = wd_packet_summary(wd);

            if (G_UNLIKELY(measure_time)) {
                // If measure_time is enabled, calculate total processing time to generate packet label
                struct timespec time_end;
                clock_gettime(CLOCK_MONOTONIC, &time_end);
                pkt_label.label_timing_ns = (time_end.tv_sec - time_start.tv_sec) * (long)1e9 + (time_end.tv_nsec - time_start.tv_nsec);
            }
            else
                pkt_label.label_timing_ns = 0;
        }
        else
            pkt_label.label_status = false; // Indicate failure during packet labeling

        // return pkt_label to caler. pkt_label local structure is often moved to the caler. Otherwise a copy is made.
        return pkt_label;
    }
};

#endif