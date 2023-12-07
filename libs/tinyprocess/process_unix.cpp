#include "libs/strtk.hpp"
#include "process.hpp"
#include <cstdlib>
#include <iostream>
#include <pty.h>
#include <signal.h>
#include <stdexcept>
#include <sys/prctl.h>
#include <unistd.h>
namespace TinyProcessLib {

    static void enable_idle_scheduler()
    {
        // Set schedule priority
        struct sched_param sp;

        sp.sched_priority = sched_get_priority_min(SCHED_IDLE);
        sched_setscheduler(0, SCHED_IDLE, &sp);
    }

    Process::Data::Data() : id(-1) {}

    Process::Process(std::function<void()> function,
                     std::function<void(const char *, size_t)> read_stdout,
                     std::function<void(const char *, size_t)> read_stderr,
                     bool detached,
                     bool open_stdin, size_t buffer_size) : closed(true), read_stdout(std::move(read_stdout)), read_stderr(std::move(read_stderr)),
                                                            _detached(detached), open_stdin(open_stdin), buffer_size(buffer_size)
    {
        open(function);
        async_read();
    }

    Process::id_type Process::open(std::function<void()> function)
    {

        if (open_stdin)
            stdin_fd = std::unique_ptr<fd_type>(new fd_type);
        if (read_stdout)
            stdout_fd = std::unique_ptr<fd_type>(new fd_type);
        if (read_stderr)
            stderr_fd = std::unique_ptr<fd_type>(new fd_type);

        int master_stdout, slave_stdout;
        int master_stderr, slave_stderr;
        if (stdout_fd)
            openpty(&master_stdout, &slave_stdout, NULL, NULL, NULL);
        if (stderr_fd)
            openpty(&master_stderr, &slave_stderr, NULL, NULL, NULL);

        // signal(SIGCHLD, SIG_IGN);

        pid_t ppid_before_fork = getpid();
        id_type pid = fork();
        if (pid == 0) {
            enable_idle_scheduler();

            // Force close child if parent dies
            if (!_detached)
                prctl(PR_SET_PDEATHSIG, SIGHUP);
            // Avoid race condition if parent dies just here
            if (getppid() != ppid_before_fork)
                exit(1);

            // Child
            if (function) {
                if (process_cwd.size() > 0)
                    chdir(process_cwd.c_str());

                if (stdout_fd)
                    dup2(slave_stdout, STDOUT_FILENO);

                if (stderr_fd)
                    dup2(slave_stderr, STDERR_FILENO);

                if (stdout_fd) {
                    close(slave_stdout);
                    close(master_stdout);
                }
                if (stderr_fd) {
                    close(slave_stderr);
                    close(master_stderr);
                }

                // Close all previous opened file descriptors (cleanup from parent)
                int fd_max = std::min(8192, static_cast<int>(sysconf(_SC_OPEN_MAX))); // Truncation is safe
                if (fd_max < 0)
                    fd_max = 8192;
                for (int fd = 3; fd < fd_max; fd++)
                    close(fd);

                if (_detached)
                    setsid();

                if (function)
                    function();

                _exit(EXIT_FAILURE);
            }
        }
        data.id = pid;
        if (stdout_fd)
            close(slave_stdout);
        if (stderr_fd)
            close(slave_stderr);

        if (stdout_fd)
            *stdout_fd = master_stdout;
        if (stderr_fd)
            *stderr_fd = master_stderr;

        closed = false;

        return pid;
    }

    Process::id_type Process::open(const std::string &command, const std::string &path)
    {
        return open([this, &command, &path] {
            // If path is defined, change working directory to path
            if (!path.empty()) {
                auto path_escaped = path;
                size_t pos = 0;
                // Based on https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxsxyb7
                while ((pos = path_escaped.find('\'', pos)) != std::string::npos) {
                    path_escaped.replace(pos, 1, "'\\''");
                    pos += 4;
                }
                execl("/bin/sh", "sh", "-c", ("cd '" + path_escaped + "' && " + command).c_str(), NULL);
            }
            else {
                // Else create array of arguments separeted by space
                int res = 0;
                auto space_idx = command.find(" ");
                std::string proc_name;
                std::string args_str;
                std::string args_vec[16]; // Max 16 arguments
                printf("%s\n", command.c_str());
                if (space_idx != command.npos && command.size() >= 2 &&
                    command.find(">") == command.npos && // Not using bash
                    command.find("<") == command.npos &&
                    command.find("\"") == command.npos &&
                    command.find("'") == command.npos &&
                    command.find("|") == command.npos &&
                    command.find("&") == command.npos) {
                    proc_name = command.substr(0, space_idx);
                    if (space_idx != command.size() - 1) {
                        size_t vec_idx_start = 0;
                        size_t vec_idx_end = 0;
                        args_str = command.substr(space_idx + 1, command.size() - 1);
                        printf("%s\n", args_str.c_str());
                        for (size_t i = 0; i < sizeof(args_vec) / sizeof(std::string); i++) {
                            vec_idx_end = args_str.find(" ", vec_idx_start);

                            args_vec[i] = args_str.substr(vec_idx_start, vec_idx_end - vec_idx_start);
                            vec_idx_start = vec_idx_end + 1;
                            if (vec_idx_end >= args_str.npos)
                                break;
                        }
                    }

                    if (proc_name[0] != '/') {
                        std::vector<std::string> p_paths;
                        strtk::parse(proc_name, "/", p_paths);
                        std::string p_name_only = p_paths.back();
                        // Convert relative path to absolute
                        char c_path[128];
                        if (getcwd(c_path, sizeof(c_path)) != NULL) {
                            if (process_cwd.size() > 0)
                                // absolute path with workdir change
                                proc_name = std::string(c_path) + "/" + p_name_only;
                            else
                                // absolute path without workdir change
                                proc_name = std::string(c_path) + "/" + proc_name;
                        }
                    }
                }

                printf("%s\n", proc_name.c_str());

                if (proc_name.size() > 0) {
                    int i = 0;
                    res = execl(proc_name.c_str(), proc_name.c_str(),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0), (args_vec[i][0] != 0 ? args_vec[i++].c_str() : 0),
                                NULL);
                }
                else
                    res = execl("/bin/sh", "bash", "-c", command.c_str(), NULL);

                printf("Exit Value = %d\n", res);
            }
        });
    }

    void Process::async_read()
    {
        if (data.id <= 0)
            return;

        if (stdout_fd) {
            stdout_thread = std::thread([this]() {
                enable_idle_scheduler();
                sigset_t set;
                sigemptyset(&set);
                sigaddset(&set, SIGPIPE);
                pthread_sigmask(SIG_BLOCK, &set, NULL);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
                char buffer[buffer_size];
                ssize_t n;
                std::lock_guard<std::mutex> lock(mutex_stdout);

                while (!close_fds_req && stdout_fd && (n = read(*stdout_fd, buffer, buffer_size)) > 0) {
                    if (read_stdout)
                        read_stdout(buffer, static_cast<size_t>(n));
                }
            });
            pthread_setname_np(stdout_thread.native_handle(), "stdout_thread");
        }

        if (stderr_fd) {
            stderr_thread = std::thread([this]() {
                enable_idle_scheduler();
                sigset_t set;
                sigemptyset(&set);
                sigaddset(&set, SIGPIPE);
                pthread_sigmask(SIG_BLOCK, &set, NULL);
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
                char buffer[buffer_size];
                ssize_t n;

                std::lock_guard<std::mutex> lock(mutex_stderr);
                while (!close_fds_req && stderr_fd && (n = read(*stderr_fd, buffer, buffer_size)) > 0) {
                    if (read_stderr)
                        read_stderr(buffer, static_cast<size_t>(n));
                }
            });
            pthread_setname_np(stderr_thread.native_handle(), "stderr_thread");
        }
    }

    int Process::get_exit_status()
    {
        if (data.id <= 0)
            return -1;

        int exit_status;
        waitpid(data.id, &exit_status, 0);
        {
            // std::lock_guard<std::mutex> lock(mutex_close);
            closed = true;
        }

        if (exit_status >= 256)
            exit_status = exit_status >> 8;
        return exit_status;
    }

    bool Process::try_get_exit_status(int &exit_status)
    {
        if (data.id <= 0)
            return false;

        id_type p = waitpid(data.id, &exit_status, WNOHANG);
        if (p <= 0)
            return false;

        {
            // std::lock_guard<std::mutex> lock(mutex_close);
            closed = true;
        }

        if (exit_status >= 256)
            exit_status = exit_status >> 8;

        return exit_status;
    }

    void Process::close_fds()
    {
        close_fds_req = true;

        // Close stdout thread
        read_stdout = nullptr;
        pthread_cancel(stdout_thread.native_handle());
        if (stdout_thread.joinable()) {
            stdout_thread.join();
        }

        // Close stderr thread
        read_stderr = nullptr;
        pthread_cancel(stderr_thread.native_handle());
        if (stderr_thread.joinable()) {
            stderr_thread.join();
        }

        if (stdin_fd)
            close_stdin();

        if (stdout_fd) {
            {
                std::lock_guard<std::mutex> lock(mutex_stdout);
                if (data.id > 0)
                    close(*stdout_fd);
                stdout_fd.reset();
            }
        }
        if (stderr_fd) {
            {
                std::lock_guard<std::mutex> lock(mutex_stderr);
                if (data.id > 0)
                    close(*stderr_fd);
                stderr_fd.reset();
            }
        }
        close_fds_req = false;
    }

    bool Process::write(const char *bytes, size_t n)
    {
        if (!open_stdin)
            throw std::invalid_argument("Can't write to an unopened stdin pipe."
                                        "Please set open_stdin=true when constructing "
                                        "the process.");

        std::lock_guard<std::mutex> lock(mutex_stdin);
        if (stdin_fd) {
            if (::write(*stdin_fd, bytes, n) >= 0) {
                return true;
            }
            else {
                return false;
            }
        }
        return false;
    }

    void Process::close_stdin()
    {
        std::lock_guard<std::mutex> lock(mutex_stdin);

        if (stdin_fd) {
            if (data.id > 0)
                close(*stdin_fd);
            stdin_fd.reset();
        }
    }

    void Process::kill(bool force)
    {
        std::lock_guard<std::mutex> lock(mutex_close);

        close_fds();
        if (data.id > 0 && !closed) {
            if (force) {
                ::kill(-data.id, SIGKILL);
                ::kill(data.id, SIGKILL);
            }
            else {
                ::kill(-data.id, SIGQUIT);
                ::kill(data.id, SIGQUIT);
            }
        }
    }

    void Process::kill(id_type id, bool force)
    {
        std::lock_guard<std::mutex> lock(mutex_close);

        if (id <= 0)
            return;

        if (force) {
            ::kill(-data.id, SIGKILL);
            ::kill(data.id, SIGKILL);
        }
        else {
            ::kill(-data.id, SIGQUIT);
            ::kill(data.id, SIGQUIT);
        }
    }

} // namespace TinyProcessLib
