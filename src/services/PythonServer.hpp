#pragma once
#ifndef __SOCKETIOSERVER__
#define __SOCKETIOSERVER__

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

class SvcPythonServer {
private:
    py::object server;
    py::function start;
    py::function disconnect_all;
    py::function stop;
    py::function register_callback;
    py::function send_event;
    thread *server_thread;
    bool module_loaded = false;

    // map<string, function<void()>> user_fcn_callbacks;
    map<string, py::cpp_function> user_fcn_callbacks;

    Config *_conf = nullptr;

    function<void(bool)> on_connection_change = nullptr;

public:
    const char *TAG = "[PythonAPIServer] ";
    bool started = false;
    string listen_address;
    int port;
    bool enable_events = true;
    string s_type;
    string s_namespace;
    bool logging = false;
    int exclude_core = -1;
    int clients;

    bool init(Config &conf, int exclude_core_ = -1)
    {
        _conf = &conf;
        PythonApiServer &server = conf.services.python_api_server;
        enable_events = server.enable_events;

        if (server.server_module >= server.server_modules_list.size()) {
            LOG2R(TAG, "Module in Modules List out of range!");
            return false;
        }

        return init(server.listen_address, server.port,
                    server.logging,
                    server.server_modules_list[server.server_module],
                    server.api_namespace, exclude_core_);
    }

    bool init(string &address_, int port_, bool logging_ = false,
              string s_type_ = "SocketIOServer", string s_namespace_ = "/",
              int exclude_core_ = -1)
    {
        if (!PythonCore.started || started)
            return false;

        listen_address = address_;
        port = port_;
        exclude_core = exclude_core_;
        s_type = s_type_;
        logging = logging_;
        s_namespace = s_namespace_;

        try {

            clients = 0;
            {
                py::gil_scoped_acquire acquire;
                // Add server folder to PATH
                if (!module_loaded) {
                    py::module sys = py::module::import("sys");
                    sys.attr("path").cast<py::list>().insert(0, "modules/server");
                }
                // Import server module according to server type name. Passes namespace to constructor
                server = py::module::import(s_type.c_str()).attr(s_type.c_str())(s_namespace);
                start = server.attr("start");
                stop = server.attr("stop");
                disconnect_all = server.attr("disconnect_all");
                register_callback = server.attr("register_callback");
                send_event = server.attr("send_event");
            }

            GL1G(TAG, "Server Module \"", s_type, ".py\" imported");
            module_loaded = true;

            server_thread = new thread([&]() {
                enable_idle_scheduler();

                // Prevent thread from running on excluded core
                if (exclude_core != -1) {
                    cpu_set_t cpuset;
                    int res = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
                    if (res == 0) {
                        CPU_CLR(exclude_core, &cpuset);
                        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
                    }
                }
                // Wait 200ms
                this_thread::sleep_for(200ms);
                {
                    py::gil_scoped_acquire acquire;
                    GL1Y(TAG, "Server Started");
                    start(port, listen_address, logging);
                    started = false;
                    GL1Y(TAG, "Server Closed");
                }
                pthread_exit(NULL);
            });
            pthread_setname_np(server_thread->native_handle(), "server_thread");

            py::gil_scoped_acquire acquire;
            // Register default callbacks
            RegisterEventCallback("connect", [&](py::list args) -> void {
                this->clients += 1;
                if (on_connection_change != nullptr)
                    on_connection_change(true);
            });
            

            RegisterEventCallback("disconnect", [&](py::list args) -> void {
                if (clients > 0)
                    clients -= 1;
                if (on_connection_change != nullptr)
                    on_connection_change(false);
            });

            RegisterEventCallback(
                "GetModelConfig", [&](py::list args) -> string {
                    return StateMachine.GetConfig();
                },
                "Returns Fuzzer Configuration File in JSON");

            RegisterEventCallback(
                "SetModelConfig", [&](py::list args) -> string {
                    if (args.size() > 0) {
                        auto config_string = args[0].cast<string>();
                        GL1Y(TAG, config_string);
                        return (StateMachine.SetConfig(config_string) ? "OK" : "ERROR");
                    }
                    else {
                        return "ERROR";
                    }
                },
                "Sets Fuzzer Configuration from JSON input");

            RegisterEventCallback(
                "GetDefaultConfig", [&](py::list args) -> string {
                    return StateMachine.GetDefaultConfig();
                },
                "Returns default Fuzzer Configuration file");

            RegisterEventCallback(
                "ResetConfig", [&](py::list args) -> string {
                    return (StateMachine.ResetConfig() ? "OK" : "ERROR");
                },
                "Restore Fuzzer Configuration to default. Requires fuzzer restart");

            RegisterEventCallback(
                "Shutdown", [&](py::list args) -> string {
                    LOGY("Shutdown Requested");
                    // Notify main thread of closed window
                    loop.onTimeoutMS(100, []() {
                        kill(getpid(), SIGUSR1);
                    });
                    return "OK";
                },
                "Shutdown Fuzzer Process");

            RegisterEventCallback(
                "GraphDot", [&](py::list args) -> string {
                    return StateMachine.get_graph();
                },
                "Returns GraphDot String of Current State Machine View");

            // Re-register user functions
            for (auto &u_fcn : user_fcn_callbacks) {
                GL1Y(TAG, "--> Re-registering ", u_fcn.first);
                register_callback(u_fcn.first, u_fcn.second);
            }

            started = true;
            return true;
        }
        catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
        }
        return false;
    }

    void Restart()
    {
        new thread([&]() {
            if (started) {
                Stop();
            }
            this_thread::sleep_for(400ms);

            if (_conf != nullptr)
                init(*_conf);
        });
    }

    void Stop()
    {
        if (started) {
            py::gil_scoped_acquire acquire;
            stop();
            started = false;
        }
    }

    void DisconnectAll()
    {
        if (started) {
            py::gil_scoped_acquire acquire;
            disconnect_all();
        }
    }

    bool HasConnection()
    {
        return (clients > 0 ? true : false);
    }

    void RegisterEventCallback(string event_name, py::cpp_function function_callback,
                               string description = "")
    {
        if (started || !module_loaded) {
            // Assume this function was called outside class
            // Save this function for later re-registration
            user_fcn_callbacks[event_name] = function_callback;
        }

        py::gil_scoped_acquire acquire;
        register_callback(event_name, function_callback,
                          (description.size() ? description : "None"));
    }

    void SendEvent(const string &event, const string &msg)
    {

        if (started && clients > 0 && enable_events) {
            py::gil_scoped_acquire acquire;
            send_event(event, msg);
        }
    }

    void SendGraph(const string state_name, const string graph_string)
    {
        if (started && clients > 0 && enable_events) {
            // LOG1("Sending Event");
            json j;
            j["graph"] = graph_string;
            j["stateName"] = state_name;
            SendEvent("GraphUpdate", j.dump());
        }
    }

    void EnableEvents(bool value)
    {
        enable_events = value;
    }

    void SendAnomaly(const string code, const string msg, bool error = false)
    {
        if (started && clients > 0 && enable_events) {
            json j;
            j["code"] = code;
            j["msg"] = msg;
            j["error"] = error;
            SendEvent("Anomaly", j.dump());
        }
    }

    // TODO: Send fitness by casting struct to json
    void SendFitness()
    {
    }

    void OnConnectionChange(function<void(bool)> callback)
    {
        if (callback != nullptr)
            on_connection_change = callback;
    }
};

#endif