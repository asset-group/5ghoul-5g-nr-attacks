#pragma once
#ifndef __PYTHONCORE__
#define __PYTHONCORE__
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <sstream>
#include <string>

using namespace std;

namespace py = pybind11;

class PythonRuntime {
private:
    py::gil_scoped_release *release = nullptr;
    string new_path;
    string python_home;
    string python_path;

public:
    const char *TAG = "[Python] ";
    bool started = false;
    py::module sys;
    string version;
    int version_major;
    int version_minor;
    int version_micro;

    bool init()
    {
        try {
            // Set new environment variables for standalone python
            char cwd[256];
            getcwd(cwd, sizeof(cwd));
            python_home = "PYTHONHOME="s + cwd + "/modules/python/install/";
            python_path = "PYTHONPATH="s + cwd + "/modules/python/install/lib/python3.8/";
            new_path = "PATH="s + cwd + "/modules/python/install/bin/:" + getenv("PATH");
            putenv((char *)python_home.c_str());
            putenv((char *)python_path.c_str());
            putenv((char *)new_path.c_str());

            // Initialize runtime
            py::initialize_interpreter(true);

            // Add library paths
            sys = py::module::import("sys");
            sys.attr("path").cast<py::list>().insert(0, "modules");
            sys.attr("path").cast<py::list>().insert(0, "modules/python/libs");

            auto version_info = sys.attr("version_info");
            version_major = version_info.attr("major").cast<int>();
            version_minor = version_info.attr("minor").cast<int>();
            version_micro = version_info.attr("micro").cast<int>();

            // Unlock GIL since initialize_interpreter locks it
            Unlock();

            stringstream ss;
            ss << version_major << "." << version_minor << "." << version_micro;
            version = ss.str();

            started = true;
        }
        catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            started = false;
        }

        return started;
    }

    // Lock GIL
    void Lock()
    {
        if (release != nullptr) {
            delete release;
            release = nullptr;
        }
    }

    // Unlock GIL
    void Unlock()
    {
        if (release == nullptr)
            release = new py::gil_scoped_release();
    }
};

PythonRuntime PythonCore;

#endif
