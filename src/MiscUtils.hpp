#pragma once

#ifndef __MISCUTILS__
#define __MISCUTILS__

#include <fcntl.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <fstream>
#include <thread>
#include <vector>

#include "libs/fast_vector.hpp"
#include "libs/log_misc_utils.hpp"
#include "libs/strtk.hpp"
#include <boost/type_index.hpp>
#include <fmt/chrono.h>
#include <fmt/format.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define IOPRIO_CLASS_SHIFT 13
#define IOPRIO_PRIO_VALUE(class, data) (((class) << IOPRIO_CLASS_SHIFT) | data)

#define CLASS_NAME(x) boost::typeindex::type_id_with_cvr<x>().pretty_name()
#define CLASS_NAME_INST(x) boost::typeindex::type_id_with_cvr<decltype(x)>().pretty_name()

#define millis() (duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

enum {
    IOPRIO_CLASS_NONE,
    IOPRIO_CLASS_RT,
    IOPRIO_CLASS_BE,
    IOPRIO_CLASS_IDLE,
};

enum {
    IOPRIO_WHO_PROCESS = 1,
    IOPRIO_WHO_PGRP,
    IOPRIO_WHO_USER,
};

extern "C" const char *__progname;

static inline int ioprio_set(int which, int who, int ioprio)
{
    return syscall(SYS_ioprio_set, which, who, ioprio);
}

namespace misc_utils {
    using namespace std;

    template <class... T>
    void lgs(T &&...args)
    {
        std::ostringstream string_converter;
        (string_converter << ... << args); // Convert n arguments to string
        std::cout << string_converter.str() << std::endl;
    }

    template <typename T>
    struct list_d : lni::fast_vector<T> {
        lni::fast_vector<int> slices;

        list_d() = default;

        list_d(lni::fast_vector<T> &new_data)
            : lni::fast_vector<T>(new_data)
        {
        }

        list_d(uint8_t *buf, size_t buf_size)
            : lni::fast_vector<T>(buf, buf + buf_size)
        {
        }

        int slice_last_offset() const
        {
            if (slices.size())
                return slices.back();
            else
                return 0;
        }

        int slice_last_size() const
        {
            if (slices.size())
                return this->size() - slices.back();
            else
                return 0;
        }

        int slice_offset(int pos) const
        {
            if (slices.size())
                return slices[pos];
            else
                return 0;
        }

        int slice_size(int pos) const
        {
            if (slices.size() > pos + 1)
                return slices[pos + 1] - slices[pos];
            else if (slices.size() == pos + 1)
                return this->size() - slices.back();
            else
                return 0;
        }

        T index(int pos) const
        {
            return this->at(pos);
        }

        const char *c_str() const
        {
            return (const char *)&this->at(0);
        }

        string str() const
        {
            return string((const char *)&this->at(0), this->size());
        }

        string hex()
        {
            return strtk::convert_bin_to_hex(string((char *)&this->at(0), this->size()));
        }

        const char *slice_c_str(int slice_pos)
        {
            return &this->at(slices[slice_pos]);
        }

        inline void slice()
        {
            slices.push_back(this->size());
        }

        inline string get_slice(int idx)
        {
            int offset = slice_offset(idx);
            return string((char *)&this->at(offset), slice_size(idx));
        }

        inline lni::fast_vector<uint8_t> get_slice_buf(int idx)
        {
            int offset = slice_offset(idx);
            return lni::fast_vector<uint8_t>((uint8_t *)&this->at(offset), (uint8_t *)&this->at(offset) + slice_size(idx));
        }

        void append(lni::fast_vector<T> &src_buf)
        {
            this->insert(this->end(), src_buf.begin(), src_buf.end());
        }

        void append_slice(lni::fast_vector<T> &src_buf)
        {
            slice();
            this->insert(this->end(), src_buf.begin(), src_buf.end());
        }

        void append(T val)
        {
            this->push_back(val);
        }

        void append_slice(T val)
        {
            slice();
            this->push_back(val);
        }

        void append(uint8_t *buf, size_t buf_size)
        {
            this->insert(this->end(), buf, buf + buf_size);
        }

        void append_slice(uint8_t *buf, size_t buf_size)
        {
            slice();
            this->insert(this->end(), buf, buf + buf_size);
        }

        list_d<T> &operator+(T val)
        {
            this->push_back(val);
            return *this;
        }

        list_d<T> &operator+(lni::fast_vector<T> &src_buf)
        {
            this->insert(this->end(), src_buf.begin(), src_buf.end());
            return *this;
        }

        void pop()
        {
            this->pop_back();
        }
    };

} // namespace misc_utils

inline void string_trim_trailing(std::string &str, const std::string &string_to_trim)
{
    strtk::parse(str, "", strtk::trim_trailing(string_to_trim, str).ref());
}

inline std::vector<std::string> string_split(std::string &str, const std::string &delim)
{
    std::vector<std::string> str_list;
    strtk::parse(str, delim, str_list);
    return str_list;
}

inline std::vector<std::string> string_split(const char *str, const std::string &delim)
{
    std::vector<std::string> str_list;
    strtk::parse(str, delim, str_list);
    return str_list;
}

inline bool string_contains(const std::string &str, const std::string &substr)
{
    return (str.find(substr) != std::string::npos);
}

inline bool string_begins(const std::string &str, const std::string &substr)
{
    return (str.rfind(substr, 0) == 0);
}

inline std::string string_file_extension(std::string &str)
{
    std::string file_name = string_split(str, "/").back();
    auto str_ext = string_split(file_name, ".");

    if (str_ext.size() > 1)
        return str_ext.back();
    else
        return "";
}

inline void string_to_lowercase(std::string &str)
{
    strtk::convert_to_lowercase(str);
}

static void set_affinity_core(int core_num)
{
    if (core_num == -1)
        return;

    cpu_set_t cpuset;

    int ncores = std::thread::hardware_concurrency();
    if (core_num < ncores) {
        CPU_ZERO(&cpuset);
        CPU_SET(core_num, &cpuset);
        int res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (res != 0) {
            LOGR("set_affinity_core: failure");
            return;
        }

        LOG3Y("set_affinity_core: ", core_num, " OK");
    }
    else {
        LOGR("set_affinity_core: invalid core number");
    }
}

static void set_affinity_no_hyperthreading(bool print_allowed_cores = false)
{
    cpu_set_t cpuset;
    pthread_t thread = pthread_self();
    int ncores = std::thread::hardware_concurrency();
    std::vector<std::string> siblings_list;
    std::vector<int> main_core_list;
    bool nosmt = false;
    int res;

    CPU_ZERO(&cpuset);

    if (print_allowed_cores)
        LOG2("Logical Cores: ", ncores);

    for (size_t i = 0; i < ncores; i++) {
        // Get sibling list of each assumed "logical" core
        std::string cpu_n_str = std::to_string(i);
        std::ifstream s_path("/sys/devices/system/cpu/cpu" + cpu_n_str + "/topology/thread_siblings_list");
        // Check if path exists
        if (s_path.good()) {
            std::string s_cpus;
            std::string cpus_delimiter;
            std::getline(s_path, s_cpus);

            if (string_contains(s_cpus, "-"))
                cpus_delimiter = "-";
            else if (string_contains(s_cpus, ","))
                cpus_delimiter = ",";
            else
                cpus_delimiter = "";
            // If no delimiter is found, assume the core is not hyperthreaded
            if (cpus_delimiter.empty()) {
                LOG1("No SMT support, running on all cores");
                nosmt = true;
                break;
            }

            std::vector<std::string> cpu_pair = string_split(s_cpus, cpus_delimiter);

            if (std::find(siblings_list.begin(), siblings_list.end(), cpu_pair[0]) == siblings_list.end() &&
                std::find(siblings_list.begin(), siblings_list.end(), cpu_pair[1]) == siblings_list.end()) {
                // If no common siblings present in siblings list, add first sibling core to thread affinity
                CPU_SET(std::stoi(cpu_pair[0]), &cpuset);
                siblings_list.push_back(cpu_pair[0]);
                siblings_list.push_back(cpu_pair[1]);
                // break;
            }
        }
    }

    if (!nosmt) {
        res = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
        if (res != 0) {
            LOGR("CPU Affinity write failure");
            return;
        }
    }

    /* Check the actual affinity mask assigned to the thread */
    res = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (res != 0) {
        LOGR("CPU Affinity read failure");
        return;
    }

    LOGY("Assigned CPUSET:");
    std::cout << termcolor::green;
    for (int j = 0; j < ncores; j++) {
        if (CPU_ISSET(j, &cpuset)) {
            fmt::print("{}", j);
            if (j < ncores - 2)
                fmt::print(", ");
        }
    }
    fmt::print("\n");
    std::cout << termcolor::reset;
}

static bool set_affinity(uint8_t core_number)
{
    int res;
    int ncores = std::thread::hardware_concurrency();
    pthread_t thread = pthread_self();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    CPU_SET(core_number, &cpuset);

    res = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    if (res != 0) {
        LOGR("CPU Affinity write failure");
        return false;
    }

    res = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (res != 0) {
        LOGR("CPU Affinity read failure");
        return false;
    }

    LOGY("Assigned CPUSET:");
    for (int j = 0; j < ncores; j++) {
        if (CPU_ISSET(j, &cpuset)) {
            LOG3G("CPU ", j, " Allowed");
        }
    }

    return true;
}

static void enable_coredump(bool val)
{
    static bool already_logged = false;

    if (val) {
        if (!already_logged)
            LOGM("Enabling Core dump for this process: ulimit -c unlimited");
        system(("prlimit --core=unlimited:unlimited --pid " + std::to_string(getpid())).c_str());
        system("sysctl -w kernel.core_pattern='core' > /dev/null");
    }
    else {
        if (!already_logged)
            LOGM("Disabling Core dump for this process: ulimit -c 0");
        system(("prlimit --core=0:0 --pid " + std::to_string(getpid())).c_str());
    }
    already_logged = true;
}

static bool enable_rt_scheduler(uint8_t use_full_time = 0)
{
    // Configure hard limits
    system(("prlimit --rtprio=unlimited:unlimited --pid " + std::to_string(getpid())).c_str());
    system(("prlimit --nice=unlimited:unlimited --pid " + std::to_string(getpid())).c_str());

    // Set schedule priority
    struct sched_param sp;
    int policy = 0;

    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_t this_thread = pthread_self();

    int ret = sched_setscheduler(0, SCHED_FIFO, &sp);
    if (ret) {
        LOGR("Error: sched_setscheduler: Failed to change scheduler to RR");
        return false;
    }

    ret = pthread_getschedparam(this_thread, &policy, &sp);
    if (ret) {
        LOGR("Error: Couldn't retrieve real-time scheduling parameters");
        return false;
    }

    // LOG2G("Thread priority is ", sp.sched_priority);

    // Allow thread to be cancelable
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    // Set IO prioriy
    ioprio_set(IOPRIO_WHO_PROCESS, 0, IOPRIO_PRIO_VALUE(IOPRIO_CLASS_RT, 0));

    if (use_full_time) {
        int fd = ::open("/proc/sys/kernel/sched_rt_runtime_us", O_RDWR);
        if (fd) {
            if (::write(fd, "-1", 2) > 0)
                LOGG("/proc/sys/kernel/sched_rt_runtime_us = -1");
        }
    }

    return true;
}

static void enable_idle_scheduler()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGABRT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    // Set schedule priority to IDLE (lowest)
    struct sched_param sp;
    sp.sched_priority = sched_get_priority_min(SCHED_IDLE);
    pthread_setschedparam(pthread_self(), SCHED_IDLE, &sp);
    // Allow thread to be cancelable
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

long gettid()
{
    return syscall(SYS_gettid);
}

// Remove ANSI unicode(remove colors)
static inline const char *remove_colors(const char *buf)
{
    static std::string filter;
    uint8_t idx = 0;

    filter = std::string(buf);

    filter.erase(remove_if(filter.begin(), filter.end(),
                           [&idx](char c) {
                               if (c == 27)
                                   idx = 7; // Start scape
                               else if ((idx && c == 'm') || c == '\n')
                                   idx = '\x01'; // End unicode
                               return (idx > 0 ? idx-- : '\0');
                           }),
                 filter.end());
    return filter.c_str();
}

// Remove ANSI unicode(remove colors)
static inline const char *remove_colors_only(const char *buf)
{
    static std::string filter;
    uint8_t idx = 0;

    filter = std::string(buf);

    filter.erase(remove_if(filter.begin(), filter.end(),
                           [&idx](char c) {
                               if (c == 27)
                                   idx = 7; // Start scape
                               else if ((idx && c == 'm'))
                                   idx = '\x01'; // End unicode
                               return (idx > 0 ? idx-- : '\0');
                           }),
                 filter.end());
    return filter.c_str();
}

static void print_buffer(uint8_t *buf, uint16_t size)
{
    for (size_t i = 0; i < size; i++) {
        printf("%02X", buf[i]);
    }
    printf("\n");
}

bool SaveAllLogs(std::string folder_name)
{
    if (!folder_name.size())
        return false;
    std::string save_cmd = "";
    save_cmd += folder_name + "/session.* ";
    save_cmd += folder_name + "/events.* ";
    save_cmd += folder_name + "/monitor.* ";
    int res = system(("cat " + save_cmd + " | sort -n > " + folder_name + "/logs_merged.txt").c_str());
    if (!res)
        LOGY("Merged logs saved to " + folder_name + "/logs_merged.txt");
    return !res;
}

static sigjmp_buf badreadjmpbuf;

int TestPtrAccess(void *ptr, int length)
{
    struct sigaction sa, osa;
    int ret = 0;

    /*init new handler struct*/
    sa.sa_handler = [](int signo) {
        siglongjmp(badreadjmpbuf, 1);
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    /*retrieve old and set new handlers*/
    if (sigaction(SIGSEGV, &sa, &osa) < 0)
        return (-1);

    if (sigsetjmp(badreadjmpbuf, 1) == 0) {
        int i, hi = length / sizeof(int), remain = length % sizeof(int);
        int *pi = (int *)ptr;
        char *pc = (char *)ptr + hi;
        for (i = 0; i < hi; i++) {
            int tmp = *(pi + i);
        }
        for (i = 0; i < remain; i++) {
            char tmp = *(pc + i);
        }
    }
    else {
        ret = 1;
    }

    /*restore prevouis signal actions*/
    if (sigaction(SIGSEGV, &osa, NULL) < 0)
        return (-1);

    return ret;
}

static inline const std::string &p2p_dir_to_string(int dir)
{
    static const std::string p2p_array[3] = {"TX", "RX", "U"};

    if (likely(dir < 2))
        return p2p_array[dir];
    else
        return p2p_array[2];
}

std::string GetPathDirName(const std::string &str)
{
    size_t found;
    found = str.find_last_of("/");
    return str.substr(0, found);
}

std::string ProcessExecGetResult(std::string cmd, bool verbose = false, double timeout_seconds = 5.0)
{
    std::array<char, 1024> buffer;
    std::string result;

    cmd = fmt::format("timeout --preserve-status -k {} -s QUIT {} {}", timeout_seconds + 2.0, timeout_seconds, cmd);

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

    if (verbose && !pipe) {
        LOGR("Error: Command \"" + cmd + "\" failed");
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
        if (verbose)
            LOG1(result);
    }

    return result;
}

int ProcessExec(std::string cmd, bool verbose = false, double timeout_seconds = 5.0)
{
    std::array<char, 1024> buffer;
    if (verbose)
        LOG1(cmd);

    cmd = fmt::format("timeout --preserve-status -k {} -s QUIT {} {}", timeout_seconds + 2.0, timeout_seconds, cmd);
    FILE *proc = popen(cmd.c_str(), "r");
    int ret = pclose(proc);
    if (verbose && ret)
        LOG2R("Error: Command \"" + cmd + "\" failed: ", strerror(errno));
    return ret;
}

std::string ProcessName()
{
    std::string sp;
    std::ifstream("/proc/self/comm") >> sp;
    return sp;
}

std::vector<std::string> GetVIDPIDBusLocation(std::string vid, std::string pid)
{
    std::vector<std::string> vid_bus_location_parsed;
    std::vector<std::string> pid_bus_location_parsed;
    std::vector<std::string> output;

    std::string vid_bus_location = ProcessExecGetResult(fmt::format("grep {} /sys/bus/usb/devices/*/idVendor | cut -d/ -f6", vid));
    std::string pid_bus_location = ProcessExecGetResult(fmt::format("grep {} /sys/bus/usb/devices/*/idProduct | cut -d/ -f6", pid));

    strtk::parse(vid_bus_location, "\n", vid_bus_location_parsed);
    strtk::parse(pid_bus_location, "\n", pid_bus_location_parsed);

    if (!vid_bus_location_parsed.size() || !pid_bus_location_parsed.size())
        // Return empty string vector
        return output;

    // Add all bus locations for same vid pid
    for (std::string &vid_bus : vid_bus_location_parsed) {
        for (std::string &pid_bus : pid_bus_location_parsed) {
            if (vid_bus == pid_bus)
                output.push_back(vid_bus);
        }
    }

    return output;
}

std::vector<std::string> GetVIDPIDBusLocation(std::string vidpid)
{
    std::vector<std::string> vidpid_parsed;
    strtk::parse(vidpid, ":", vidpid_parsed);

    if (vidpid_parsed.size() < 2) {
        LOG3R("Could not parse vidpid string \"", vidpid, "\"");
        return std::vector<std::string>();
    }

    return GetVIDPIDBusLocation(vidpid_parsed[0], vidpid_parsed[1]);
}

void SetFolderPermission(std::string perm_str, std::string folder_path, int user = 1000, int group = 1000, bool recursive = true)
{
    if (recursive)
        ProcessExec(fmt::format("chmod -R {} {}", perm_str, folder_path));
    else
        ProcessExec(fmt::format("chmod {} {}", perm_str, folder_path));

    if (recursive)
        ProcessExec(fmt::format("chown -R {}:{} {}", user, group, folder_path));
    else
        ProcessExec(fmt::format("chown {}:{} {}", user, group, folder_path));
}

// Check if a given folder exists and create one if not
void EnsureFolder(std::string folder_path, int user = 1000, int group = 1000, std::string perm_str = "0755")
{
    std::ifstream f(folder_path);

    if (f.good())
        return;

    ProcessExec(fmt::format("mkdir -p {}", folder_path));
    ProcessExec(fmt::format("chown {}:{} {}", user, group, folder_path));
    ProcessExec(fmt::format("chmod {} {}", perm_str, folder_path));
}

std::vector<int> &GetKernelVersion()
{
    static std::vector<int> current_version = {0, 0, 0};

    if (current_version.size() == 3)
        return current_version;

    struct utsname unamedata;
    uname(&unamedata);

    std::vector<std::string> fullversion_str = string_split(unamedata.release, ".");

    if (fullversion_str.size() >= 3) {
        std::vector<std::string> patch_version = string_split(fullversion_str[2], "-");
        fullversion_str[2] = patch_version[0];

        for (auto &ver : fullversion_str)
            current_version.emplace_back(atoi(ver.c_str()));
    }

    return current_version;
}

void PrintKernelVersion()
{
    std::vector<int> &ver = GetKernelVersion();
    LOGC(fmt::format("Kernel Version: {}.{}.{}", ver[0], ver[1], ver[2]));
}

template <typename T>
// Add useful namespaces
using list_data = struct misc_utils::list_d<T>;
using namespace lni;

#endif