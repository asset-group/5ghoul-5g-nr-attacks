#pragma once
#ifndef __PROCESS__
#define __PROCESS__

#include "libs/log_misc_utils.hpp"
#include "libs/strtk.hpp"
#include "libs/termcolor.hpp"
#include "libs/tinyprocess/process.hpp"
#include <functional>
#include <iostream>
#include <mutex>
#include <sched.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string>

using namespace std;

/**
 * @brief Class that starts or stops a process.
 * This class also allows the output of the process to be redirected
 * to a user provided callback for logging purposes
 *
 */
class ProcessRunner {
private:
    const char *TAG = "[Process] ";
    shared_ptr<TinyProcessLib::Process> process;
    thread *thread_restart_process = nullptr;
    function<void(const char *bytes, size_t n)> process_callback = nullptr;
    function<void(const char *bytes, size_t n)> error_callback = nullptr;
    function<void()> start_callback = nullptr;
    function<void()> stop_callback = nullptr;
    bool ignore_init_callback = false;
    bool detached = false;
    bool restart_on_exit = false;
    int restart_delay_ms = 0;
    bool req_kill_forcebly = false;
    bool req_kill = false;
    thread *thread_p_killer = nullptr;
    mutex mutex_process;
    sem_t sem_on_kill;

    bool start_process()
    {
        std::lock_guard<std::mutex> lock(mutex_process);
        bool ret = false;

        if (process_started) {
            cerr << "Process already started, stopping now..." << endl;
            stop(true, true);
        }

        if (!process_name.length())
        {
            LOG2R(TAG, "Error: Empty process_name");
            return false;
        }
        string process_cmd = process_name;
        vector<string> p_name;

        strtk::parse(process_name, "/", p_name);
        this->process_name_only = p_name.back();
        if (this->change_working_dir) {
            if (p_name.size() > 1) {
                this->process_exec_dir = "";
                for (size_t i = 0; i < p_name.size() - 1; i++) {
                    this->process_exec_dir += p_name[i] + "/";
                }
            }
            else {
                this->process_exec_dir = "./";
            }
        }

        function<void(const char *bytes, size_t n)> stdout_callback = process_callback;
        function<void(const char *bytes, size_t n)> stderr_callback = error_callback;

        if (process_args.size())
            process_cmd += " " + process_args;

        if (stderr == nullptr) {
            stdout_callback = process_callback;
            stderr_callback = process_callback;
        }
        else {
            stdout_callback = process_callback;
            stderr_callback = error_callback;
        }

        if (!this->ignore_init_callback && start_callback)
            start_callback();

        // Start process now
        this->process = make_shared<TinyProcessLib::Process>(process_cmd, "", stdout_callback, stderr_callback, process_exec_dir, this->detached);
        process_started = (process != nullptr);

        // Start process restarter thread
        if (this->restart_on_exit && thread_restart_process == nullptr) {
            thread_restart_process = new thread([&]() {
                enable_idle_scheduler();

                while (this->restart_on_exit) {
                    WaitProcess();
                    process_started = false;
                    if (this->restart_on_exit) {
                        restart(true, true, false);
                    }
                }
            });
            thread_restart_process->detach();
            pthread_setname_np(thread_restart_process->native_handle(), "thread_restart_process");
        }

        // Start process killer thread
        if (thread_p_killer == nullptr) {
            sem_init(&sem_on_kill, 0, 0);
            thread_p_killer = new thread([&]() {
                enable_idle_scheduler();

                while (true) {
                    sem_wait(&sem_on_kill);
                    if (req_kill) {
                        req_kill = false;
                        this->process->kill(req_kill_forcebly);
                        process_started = false;
                        this->process.reset();
                        this->process = nullptr;
                        stopping = false;
                        stopped = true;
                    }
                }
            });
            thread_p_killer->detach();
            pthread_setname_np(thread_p_killer->native_handle(), "thread_process_killer");
        }

        if (process_started) {
            this->process_id = this->process->get_id();
            this->stopped = false;
        }

        return process_started;
    }

public:
    string process_name;
    string process_args;
    string process_name_only;
    pid_t process_id;
    bool process_started;
    bool stopping = false;
    bool stopped = false;
    bool change_working_dir = false;
    string process_exec_dir = "";

    void SetStartCallback(function<void()> fcn)
    {
        start_callback = fcn;
    }

    void SetStopCallback(function<void()> fcn)
    {
        stop_callback = fcn;
    }

    void SetRestartDelayMS(int delay_ms)
    {
        restart_delay_ms = delay_ms;
    }

    /**
     * @brief Wait process to exit and return its exit code
     *
     * @return true
     * @return false
     */
    bool WaitProcess()
    {
        if (!process_started || !this->process)
            return false;

        return this->process->get_exit_status();
    }

    int GetPID()
    {
        if (!process_started || !this->process)
            return -1;

        return this->process->get_id();
    }

    /**
     * @brief Stop the running process
     *
     * @param wait Block this function until the process stops
     * @param force Forces the process to stop by sending a SIGKILL signal (9)
     */
    void stop(bool force = true, bool wait = false, bool autorestart = false)
    {
        if (stopping)
            return;

        if (!autorestart)
            this->restart_on_exit = false;

        stopping = true;
        if (stop_callback && (!stopped))
            stop_callback();

        if (this->detached) {
            stopping = false;
            stopped = true;
            return;
        }

        if (!wait) {
            if (this->process && process_started) {
                req_kill_forcebly = force;
                req_kill = true;
                sem_post(&sem_on_kill);
            }
        }
        else {
            if (this->process) {
                this->process->kill(force);
                this->process.reset();
                this->process = nullptr;
            }
            process_started = false;
            stopping = false;
            stopped = true;
        }
    }

    /**
     * @brief Restart the running process
     *
     * @param force
     * @param ignore_init_callback
     */
    void restart(bool force = false, bool wait = false, bool ignore_init_callback = false)
    {
        if (!restart_on_exit || wait) {
            this->ignore_init_callback = ignore_init_callback;
            if (!process_started)
                stop(force, true, restart_on_exit);
            this_thread::sleep_for(chrono::milliseconds(restart_delay_ms));
            if (!init())
                LOG3R(TAG, "Error: Failed to restart process ", this->process_name_only);
            this->ignore_init_callback = false;
        }
        else {
            stop(force, wait, true);
        }
    }

    void setup(string process_name, string args, function<void(const char *bytes, size_t n)> process_callback,
               bool change_working_dir = false,
               bool restart_on_exit = false,
               int restart_delay_ms = 0)
    {
        this->change_working_dir = change_working_dir;
        this->process_args = args;
        this->process_name = process_name;
        this->process_callback = process_callback;
        this->error_callback = nullptr;
        this->restart_on_exit = restart_on_exit;
        this->restart_delay_ms = restart_delay_ms;
    }

    /**
     * @brief Start a new process with previous saved parameters
     *
     * @return true
     * @return false
     */
    bool init()
    {
        return init(process_name, process_args, process_callback, error_callback, change_working_dir);
    }

    bool init(string process_name, function<void(const char *bytes, size_t n)> process_callback, bool change_working_dir = false)
    {
        this->change_working_dir = change_working_dir;
        this->process_args = "";
        this->process_name = process_name;
        this->process_callback = process_callback;
        this->error_callback = nullptr;

        return start_process();
    }

    bool init(string process_name, function<void(const char *bytes, size_t n)> process_callback,
              function<void(const char *bytes, size_t n)> error_callback,
              bool change_working_dir = false)
    {
        this->change_working_dir = change_working_dir;
        this->process_args = "";
        this->process_name = process_name;
        this->process_callback = process_callback;
        this->error_callback = error_callback;

        return start_process();
    }

    bool init(string process_name, string args, function<void(const char *bytes, size_t n)> process_callback,
              bool change_working_dir = false,
              bool restart_on_exit = false,
              int restart_delay_ms = 0)
    {
        this->change_working_dir = change_working_dir;
        this->process_args = args;
        this->process_name = process_name;
        this->process_callback = process_callback;
        this->error_callback = process_callback;
        this->restart_on_exit = restart_on_exit;
        this->restart_delay_ms = restart_delay_ms;

        return start_process();
    }

    bool init(string process_name, string args, function<void(const char *bytes, size_t n)> process_callback,
              function<void(const char *bytes, size_t n)> error_callback,
              bool change_working_dir = false)
    {
        this->process_args = args;
        this->process_name = process_name;
        this->process_callback = process_callback;
        this->error_callback = error_callback;
        this->change_working_dir = change_working_dir;

        return start_process();
    }

    /**
     * @brief Set the arguments of an already configured process
     * through the function setup
     *
     * @param args
     */
    void set_args(string args)
    {
        this->process_args = args;
    }

    /**
     * @brief Allows the running process to continue running even if
     * the main program closes
     *
     * @param en
     */
    void setDetached(bool en)
    {
        this->detached = en;
    }

    /**
     * @brief Returns whether the current process is running
     *
     * @return true
     * @return false
     */
    bool isRunning()
    {
        if (this->process) {
            int ec;
            bool r = this->process->try_get_exit_status(ec);
            this->process_started = (r == 0);
            return this->process_started;
        }
        else {
            return false;
        }
    }
};

#endif