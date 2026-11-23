#pragma once

#ifndef __MODULES_WD__
#define __MODULES_WD__

#include <algorithm>
#include <inttypes.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unordered_map>
#include <vector>

#include "libs/dynalo/include/dynalo/dynalo.hpp"
#include "libs/tinydir.h"

#include "Framework.hpp"
#include "Machine.hpp"
#include "ModulesInclude.hpp"
#include "ParseArgs.hpp"

#include "wdissector.h"

using namespace std;
using namespace misc_utils;

#define WD_MODULES_MAX_GROUPS 256

#define timespeccmp(a, b, CMP)            \
    (((a)->tv_sec == (b)->tv_sec)         \
         ? ((a)->tv_nsec CMP(b)->tv_nsec) \
         : ((a)->tv_sec CMP(b)->tv_sec))

typedef struct
{
    string name;
    bool enable = false;
    bool in_group = false;
    vector<int> groups;
    bool require_setup = false;
    dynalo::library *lib;
    wd_modules_ctx_t ctx; // unique ctx per module (if in_group = false)

    // Module functions
    const char *(*module_name)() = NULL;
    int (*setup)(wd_modules_ctx_t *ctx) = NULL;
    int (*tx_pre_dissection)(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx) = NULL;
    int (*tx_post_dissection)(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx) = NULL;
    int (*rx_pre_dissection)(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx) = NULL;
    int (*rx_post_dissection)(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx) = NULL;

    bool configure(bool enable, wd_modules_ctx_t *ctx = nullptr)
    {
        bool ret = false;

        if (enable && this->require_setup) {
            this->require_setup = false;

            if (!this->setup(ctx)) {
                GL1G("[Modules] ", name, " configured and ready!");
                this->enable = true;
                ret = true;
            }
            else {
                GL1R("[Modules] ", name, ".so error (setup returns != 0)");
            }
        }
        else {
            this->require_setup = true;
            this->enable = false;
            ret = true;
        }

        return ret;
    }

} wd_module_func_t;

class WDModules {
private:
    bool any_enabled;

    std::map<int, string> GroupsPrefixMap;
    std::vector<string> PrefixList;
    // TODO: move wd_module_func_t and wd_modules_ctx_t to another structure (wd_module_t)
    std::map<int, vector<pair<wd_module_func_t *, wd_modules_ctx_t>>> ModulesGroupMap;
    std::map<int, vector<pair<wd_module_func_t *, wd_modules_ctx_t *>>> EnabledModulesGroupMap;

    void (*requests_handler_fcn)(wd_modules_ctx_t *ctx, wd_module_request_t *req) = NULL;
    void (*requests_handler_group_fcn)(wd_modules_ctx_t *ctx, wd_module_request_t *req, int group) = NULL;

    string gen_module_glue(const string source_path)
    {
        stringstream glue;
        string module_name = string_split(source_path.c_str(), "/").back();
        ifstream ifs(source_path + ".cpp");
        if (ifs.good()) {
            glue << ifs.rdbuf();
            // Add glue code
            glue << "\nextern \"C\" const char * " + module_name + "_module_name(){return module_name();}";
            glue << "\nextern \"C\" int " + module_name + "_setup(wd_modules_ctx_t *ctx){return setup(ctx);}";
            glue << "\nextern \"C\" int " + module_name + "_tx_pre_dissection(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx){return tx_pre_dissection(pkt_buf,pkt_length,ctx);}";
            glue << "\nextern \"C\" int " + module_name + "_tx_post_dissection(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx){return tx_post_dissection(pkt_buf,pkt_length,ctx);}";
            glue << "\nextern \"C\" int " + module_name + "_rx_pre_dissection(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx){return rx_pre_dissection(pkt_buf,pkt_length,ctx);}";
            glue << "\nextern \"C\" int " + module_name + "_rx_post_dissection(uint8_t *pkt_buf, int pkt_length, wd_modules_ctx_t *ctx){return rx_post_dissection(pkt_buf,pkt_length,ctx);}";
            ifs.close();
        }
        else
            return "";
        return glue.str();
    }

    bool was_modified(const string source_path)
    {
        struct stat attrib1, attrib2;

        if (stat((source_path + ".cpp").c_str(), &attrib1))
            return false;

        if (stat((source_path + ".so").c_str(), &attrib2))
            return true;

        return timespeccmp(&attrib1.st_mtim, &attrib2.st_mtim, >);
    }

    // A build step is judged by the artifact it has to produce, not by the status
    // system()/pclose() returns: under load the shell can fail to start (or be
    // killed) and then there is a non-zero status with no compiler output at all.
    // Treating that as a compile failure hides the real cause and is wrong when
    // the artifact was in fact produced.
    string build_log_path() const
    {
        return "/tmp/5g_fuzzer_build_" + to_string(getpid()) + ".log";
    }

    static bool artifact_ready(const string &path)
    {
        struct stat st;
        return (stat(path.c_str(), &st) == 0 && st.st_size > 0);
    }

    void report_build_failure(int rc, const string &log_path, const string &what, const string &artifact)
    {
        GL1R(TAG, "Failed to compile ", what, " ", artifact);
        if (WIFSIGNALED(rc))
            GL1R(TAG, "  compiler killed by signal ", (int)WTERMSIG(rc));
        else if (WIFEXITED(rc))
            GL1R(TAG, "  compiler exited with code ", (int)WEXITSTATUS(rc));
        else
            GL1R(TAG, "  compiler could not be started (system() returned ", rc, ")");
        FILE *lf = fopen(log_path.c_str(), "r");
        if (lf) {
            char lline[512];
            int shown = 0;
            while (fgets(lline, sizeof lline, lf) && shown++ < 40)
                GL1R(TAG, "  ", lline);
            fclose(lf);
        }
        ::unlink(log_path.c_str());
    }

public:
    const char *TAG = "[Modules] ";

    int modules_count;
    int modules_compiled;
    int modules_loaded;
    bool has_gcc;

    unordered_map<string, wd_module_func_t> modules_map;
    vector<wd_module_func_t *> modules_ptr;
    vector<wd_module_func_t *> enabled_modules_ptr;

    bool init()
    {
        Config &conf = StateMachine.config;
        return init((WD_MODULES_PATH + conf.name).c_str());
    }

    bool init(const char *modules_path)
    {
        any_enabled = false;
        modules_count = 0;
        modules_compiled = 0;
        modules_loaded = 0;
        has_gcc = false;
        bool no_err = true;

        GL1Y(TAG, "Loading C++ Modules at \"", modules_path, "\"");

        if (modules_path == NULL)
            return false;

        EnsureFolder(modules_path);

        // Check g++ 2 times in case of random failures
        int has_gpp = system("which g++ > /dev/null 2>&1");
        if (has_gpp) {
            usleep(10000); // Wait 10 ms at least
            has_gpp &= system("which g++ > /dev/null 2>&1");
        }

        if (!has_gpp)
            has_gcc = true;
        else
            LOGY("GCC/G++ not found, modules won't be compiled from source.");

        tinydir_dir dir;

        if (tinydir_open(&dir, modules_path) == -1)
            return false;

        tinydir_file file;

        // First pass check cpps and compile modules (.so)
        while (dir.has_next) {
            if (!tinydir_readfile(&dir, &file)) {
                if (file.is_dir) {
                    tinydir_next(&dir);
                    continue;
                }

                if (!strcmp(file.extension, "cpp")) {
                    string module_name = string_split(file.name, ".")[0];
                    string source_path = modules_path + "/"s + module_name;
                    // Compile module
                    if (has_gcc && was_modified(source_path)) {
                        string stub_cpp = "src/ModulesStub.cpp";
                        string stub_path = string(modules_path) + "/ModulesStub.o";
                        struct stat stub_src_st, stub_obj_st;
                        if (stat(stub_cpp.c_str(), &stub_src_st) || stat(stub_path.c_str(), &stub_obj_st) ||
                            timespeccmp(&stub_src_st.st_mtim, &stub_obj_st.st_mtim, >)) {
                            string stub_log = build_log_path();
                            if (system(("g++ -std=c++17 -fPIC -w -O3 " + stub_cpp + " -c -o " + stub_path +
                                        " 2> " + stub_log).c_str()) && !artifact_ready(stub_path))
                                report_build_failure(0, stub_log, "module stub", stub_path);
                        }
                        GL1G("Compiling ", module_name, ".cpp");
                        // Compile obj
                        string g_cmd_obj = "g++ -x c++ -std=c++17 -fPIC -w -O3 -c "s +
                                           "-o " + source_path +
                                           ".o "
                                           "-I src/ "
                                           "-I libs/ "
                                           "-I libs/wireshark "
                                           "-I libs/wireshark/include "
                                           "-I/usr/include/glib-2.0 "
                                           "-I/usr/lib/x86_64-linux-gnu/glib-2.0/include -";

                        LOG1(g_cmd_obj);

                        string obj_path = source_path + ".o";
                        string obj_log = build_log_path();
                        ::unlink(obj_path.c_str());
                        string gen_code = gen_module_glue(source_path);
                        int rc_obj = 0;
                        for (int attempt = 1; attempt <= 2; attempt++) {
                            FILE *p_fd = popen((g_cmd_obj + " 2> " + obj_log).c_str(), "w");
                            if (!p_fd)
                                break;
                            // Add api glue to code (optional)
                            fwrite(&gen_code[0], 1, gen_code.size(), p_fd);
                            rc_obj = pclose(p_fd);
                            if (artifact_ready(obj_path))
                                break;
                            usleep(20000); // transient fork/exec failure under load
                        }
                        if (artifact_ready(obj_path))
                            modules_compiled++;
                        else {
                            report_build_failure(rc_obj, obj_log, "object", obj_path);
                            no_err = false;
                        }

                        // Compile shared library
                        string so_path = source_path + ".so";
                        string so_tmp = so_path + ".tmp";
                        string link_log = build_log_path();
                        string g_cmd_shared = "g++ -std=c++17 -fPIC -w -O3 -shared -o "s + so_tmp + " " +
                                              source_path + ".o " + modules_path + "/ModulesStub.o " +
                                              "-Lbin/ -Wl,-rpath,'$ORIGIN/bin/' -Wl,-rpath,'$ORIGIN/../../../bin/' "
                                              "-lwdissector -lwireshark";

                        LOG1(g_cmd_shared);

                        bool linked = false;
                        int rc_link = 0;
                        if (artifact_ready(source_path + ".o")) {
                            for (int attempt = 1; attempt <= 2; attempt++) {
                                ::unlink(so_tmp.c_str());
                                rc_link = system((g_cmd_shared + " 2> " + link_log).c_str());
                                if ((linked = artifact_ready(so_tmp)))
                                    break;
                                usleep(20000); // transient fork/exec failure under load
                            }
                        }
                        if (linked) {
                            if (::rename(so_tmp.c_str(), so_path.c_str())) {
                                GL1R(TAG, "Failed to install shared library ", so_path);
                                no_err = false;
                            }
                        } else if (artifact_ready(source_path + ".o")) {
                            report_build_failure(rc_link, link_log, "shared library", so_path);
                            no_err = false;
                        }
                    }
                }
            }

            tinydir_next(&dir);
        }
        tinydir_close(&dir);

        if (tinydir_open(&dir, modules_path) == -1)
            return false;

        // Second pass, load modules
        while (dir.has_next) {
            if (!tinydir_readfile(&dir, &file)) {
                if (file.is_dir) {
                    tinydir_next(&dir);
                    continue;
                }

                if (!strcmp(file.extension, "so")) {
                    string module_name = string_split(file.name, ".")[0];
                    string source_path = modules_path + "/"s + module_name;

                    try {

                        // Create main module structures
                        if (!modules_map.contains(module_name)) {
                            wd_module_func_t m;
                            m.lib = new dynalo::library(source_path + ".so");
                            m.enable = false;
                            m.module_name = m.lib->get_function<const char *()>(module_name + "_module_name");
                            m.setup = m.lib->get_function<int(wd_modules_ctx_t *)>(module_name + "_setup");
                            m.tx_pre_dissection = m.lib->get_function<int(uint8_t *, int, wd_modules_ctx_t *)>(module_name + "_tx_pre_dissection");
                            m.tx_post_dissection = m.lib->get_function<int(uint8_t *, int, wd_modules_ctx_t *)>(module_name + "_tx_post_dissection");
                            m.rx_pre_dissection = m.lib->get_function<int(uint8_t *, int, wd_modules_ctx_t *)>(module_name + "_rx_pre_dissection");
                            m.rx_post_dissection = m.lib->get_function<int(uint8_t *, int, wd_modules_ctx_t *)>(module_name + "_rx_post_dissection");
                            m.name = module_name;
                            m.in_group = false;
                            m.require_setup = true;

                            // Initialize default ctx config (non-group)
                            m.ctx = {0};
                            m.ctx.wd = StateMachine.wd;
                            m.ctx.config = &StateMachine.config;
                            m.ctx.state = &StateMachine.DissectedStateName;

                            // Save module structure to modules map
                            modules_map[module_name] = move(m);

                            GL1Y("[Modules] --> " + module_name + ".so loaded");
                            modules_loaded++;
                        }

                        // Update module structure
                        wd_module_func_t &m = modules_map[module_name];

                        // Register module to grouped modules
                        for (auto &group_prefix : GroupsPrefixMap) {
                            if (string_begins(module_name, group_prefix.second)) {
                                if (!ModulesGroupMap.contains(group_prefix.first))
                                    ModulesGroupMap[group_prefix.first] = {};

                                m.in_group = true;
                                m.groups.push_back(group_prefix.first);
                                // Create group context with default params
                                wd_modules_ctx_t ctx = {0};
                                ctx.wd = StateMachine.wd;
                                ctx.config = &StateMachine.config;
                                ctx.state = &StateMachine.DissectedStateName;
                                // Register grouped module
                                ModulesGroupMap[group_prefix.first].push_back({&m, ctx});
                            }
                        }

                        // Register non-grouped module
                        if (!m.in_group)
                            modules_ptr.push_back(&m);
                    }
                    catch (const std::exception &e) {
                        no_err = false;
                        std::cerr << e.what() << '\n';
                    }

                    modules_count++;
                }
            }

            tinydir_next(&dir);
        }

        if (no_err)
            GL1G(TAG, modules_loaded, "/", modules_count, " Modules Compiled / Loaded");
        else
            LOG5R(TAG, modules_loaded, "/", modules_count, " Error when loading or compiling C Modules");

        return no_err;
    }

    void SetRequestsHandler(void (*fcn)(wd_modules_ctx_t *ctx, wd_module_request_t *req))
    {
        requests_handler_fcn = fcn;
    }

    void SetRequestsHandler(void (*fcn)(wd_modules_ctx_t *ctx, wd_module_request_t *req, int group_number))
    {
        requests_handler_group_fcn = fcn;
    }

    // Run requests handler for non-grouped modules
    void RunRequestsHandler()
    {
        if (G_LIKELY(!any_enabled))
            return;

        int ret = 0;
        for (auto &m : enabled_modules_ptr) {
            wd_module_request_t *req = &m->ctx.request;
            requests_handler_fcn(&m->ctx, &m->ctx.request);
            m->ctx.request = {0}; // Reset request
        }
    }

    // Run requests handler for grouped modules
    void RunRequestsHandler(int group_number)
    {
        if (G_LIKELY(!any_enabled))
            return;

        if (!EnabledModulesGroupMap.contains(group_number))
            return;

        int ret = 0;
        for (auto &[m, ctx] : EnabledModulesGroupMap[group_number]) {
            requests_handler_group_fcn(ctx, &ctx->request, group_number);
            ctx->request = {0}; // reset request
        }
    }

    bool enable_module(const string module_name, bool enable)
    {
        auto module_iter = modules_map.find(module_name);

        if (module_iter != modules_map.end()) {
            wd_module_func_t &m = module_iter->second;

            // Use ungrouped context for module setup
            m.configure(enable, &m.ctx);

            if (enable) {
                GL1Y(TAG, "Enabled module \"", module_name, "\"");
                any_enabled = true;
                // Register module to enabled module map or vector
                if (m.in_group)
                    for (auto &group : m.groups) {
                        for (auto &M : ModulesGroupMap[group]) {
                            if (&m == M.first)
                                EnabledModulesGroupMap[group].push_back({M.first, &M.second});
                        }
                    }
                else
                    enabled_modules_ptr.emplace_back(&m);
            }
            else {
                if (m.in_group) {
                    // remove all references of this module from EnabledModulesGroupMap
                    for (auto &group : m.groups) {
                        auto &en_modules = EnabledModulesGroupMap[group];
                        auto it = std::find_if(en_modules.begin(), en_modules.end(), [&](auto &val) {
                            if (val.first == &m)
                                return true;
                            else
                                return false;
                        });
                        if (it != en_modules.end())
                            en_modules.erase(it);

                        if (!en_modules.size())
                            EnabledModulesGroupMap.erase(group);
                    }
                }
                else {
                    // remove reference of this module from enabled_modules_ptr
                    auto it = std::find(enabled_modules_ptr.begin(), enabled_modules_ptr.end(), &m);
                    if (it != enabled_modules_ptr.end())
                        enabled_modules_ptr.erase(it);
                }
            }
            return true;
        }

        return false;
    }

    void printAvailableModules()
    {
        if (!modules_ptr.size() && !ModulesGroupMap.size()) {
            LOG2Y(TAG, " Available Exploits: None");
            return;
        }

        LOG2Y(TAG, " Available Exploits:");
        for (auto &m : modules_ptr) {
            LOG4Y(TAG, "--> '", m->name, "'");
        }

        unordered_map<string, string> module_groups_str;
        for (auto &M : GroupsPrefixMap) {
            for (auto &[m, ctx] : ModulesGroupMap[M.first]) {
                if (!module_groups_str.contains(m->name))
                    module_groups_str[m->name] = "";

                module_groups_str[m->name] += format("[{}:{}] ", M.second, M.first);
            }
        }

        for (auto &module_name : module_groups_str) {
            LOG2Y(TAG, format("--> {} Groups: {}", module_name.first, module_name.second));
        }
    }

    void printAvailableGroups()
    {
        LOG2Y(TAG, "Available Groups:");

        if (!GroupsPrefixMap.size()) {
            LOG2Y(TAG, "Groups: No groups");
            return;
        }

        for (auto &M : GroupsPrefixMap) {
            LOG2Y(TAG, format("Group: ID:{}, Prefix:\"{}\"", M.first, M.second));
        }
    }

    void printAvailablePrefixes()
    {
        LOG2Y(TAG, "Available Prefixes:");

        if (!GroupsPrefixMap.size()) {
            LOG2Y(TAG, "Prefixes: No prefixes");
            return;
        }

        for (auto &prefix : PrefixList) {
            LOG2Y(TAG, format("Prefix:\"{}\"", prefix));
        }
    }

    void DisableAllModules()
    {
        for (auto &m : modules_ptr) {
            m->enable = false;
        }

        enabled_modules_ptr.clear();

        for (auto &M : ModulesGroupMap) {
            for (auto &[m, ctx] : M.second) {
                m->enable = false;
            }
        }

        EnabledModulesGroupMap.clear();

        any_enabled = false;
    }

    void SetStatePtr(string *state_ptr, int group_number)
    {
        for (auto &[m, ctx] : ModulesGroupMap[group_number]) {
            ctx.state = state_ptr;
        }
    }

    void SetStatePtr(string *state_ptr)
    {
        for (auto &m : modules_ptr) {
            m->ctx.state = state_ptr;
        }
    }

    bool RegisterGroupPrefix(int group_number, string group_prefix)
    {
        GroupsPrefixMap[group_number] = group_prefix;
        ModulesGroupMap[group_number] = {};

        if (std::find(PrefixList.begin(), PrefixList.end(), group_prefix) == PrefixList.end())
            PrefixList.emplace_back(group_prefix);

        return true;
    }

    inline int run_tx_pre_dissection(uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &m : enabled_modules_ptr) {
            if (G_UNLIKELY(wd != NULL))
                m->ctx.wd = wd;
            ret |= m->tx_pre_dissection(pkt_buf, pkt_length, &m->ctx);
        }

        return ret;
    }

    inline int run_tx_pre_dissection(int group_number, uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &[m, ctx] : EnabledModulesGroupMap[group_number]) {
            if (G_LIKELY(wd != NULL))
                ctx->wd = wd;
            ret |= m->tx_pre_dissection(pkt_buf, pkt_length, ctx);
        }

        return ret;
    }

    inline int run_tx_post_dissection(uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &m : enabled_modules_ptr) {
            if (G_UNLIKELY(wd != NULL))
                m->ctx.wd = wd;
            ret |= m->tx_post_dissection(pkt_buf, pkt_length, &m->ctx);
        }

        return ret;
    }

    inline int run_tx_post_dissection(int group_number, uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &[m, ctx] : EnabledModulesGroupMap[group_number]) {
            if (G_LIKELY(wd != NULL))
                ctx->wd = wd;
            ret |= m->tx_post_dissection(pkt_buf, pkt_length, ctx);
        }

        return ret;
    }

    inline int run_rx_pre_dissection(uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &m : enabled_modules_ptr) {
            if (G_UNLIKELY(wd != NULL))
                m->ctx.wd = wd;
            ret |= m->rx_pre_dissection(pkt_buf, pkt_length, &m->ctx);
        }

        return ret;
    }

    inline int run_rx_pre_dissection(int group_number, uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &[m, ctx] : EnabledModulesGroupMap[group_number]) {
            if (G_LIKELY(wd != NULL))
                ctx->wd = wd;
            ret |= m->rx_pre_dissection(pkt_buf, pkt_length, ctx);
        }

        return ret;
    }

    inline int run_rx_post_dissection(uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &m : enabled_modules_ptr) {
            if (G_UNLIKELY(wd != NULL))
                m->ctx.wd = wd;
            ret |= m->rx_post_dissection(pkt_buf, pkt_length, &m->ctx);
        }

        return ret;
    }

    inline int run_rx_post_dissection(int group_number, uint8_t *pkt_buf, int pkt_length, wd_t *wd = NULL)
    {
        if (G_LIKELY(!any_enabled))
            return 0;

        int ret = 0;
        for (auto &[m, ctx] : EnabledModulesGroupMap[group_number]) {
            if (G_LIKELY(wd != NULL))
                ctx->wd = wd;
            ret |= m->rx_post_dissection(pkt_buf, pkt_length, ctx);
        }

        return ret;
    }

    void AddArgs(WDParseArgs &ParseArgs)
    {
        ParseArgs.AddArgCallback("Modules/exploit", "Launch Exploit", json::value_t::string, [&](string arg_name, json &val) {
            string module_name = val.get<string>();
            if (!modules_map.contains(module_name)) {
                LOG3R("No exploit \"", module_name, "\" found. Select only the available exploits below:");
                printAvailableModules();
                exit(0);
            }
            enable_module(module_name, true);
        });

        ParseArgs.AddArgCallback("Modules/list-exploits", "List Available Exploits", json::value_t::boolean, [&](string arg_name, json &val) {
            printAvailableModules();
            exit(0);
        });

        ParseArgs.AddArgCallback("Modules/list-exploits-groups", "List Exploit Groups (Debug)", json::value_t::boolean, [&](string arg_name, json &val) {
            printAvailableGroups();
            exit(0);
        });

        ParseArgs.AddArgCallback("Modules/list-exploits-prefixes", "List Exploit Prefixes (Debug)", json::value_t::boolean, [&](string arg_name, json &val) {
            printAvailablePrefixes();
            exit(0);
        });
    }

    vector<string> GetModulesList()
    {
        vector<string> ModulesList;

        for (auto &m : modules_map) {
            ModulesList.emplace_back(m.first);
        }

        return ModulesList;
    }

    vector<string> GetModulesListNotInGroup()
    {
        vector<string> ModulesList;

        for (auto &m : modules_map) {
            if (!m.second.in_group)
                ModulesList.emplace_back(m.first);
        }

        return ModulesList;
    }

    vector<string> GetModulesListInGroup()
    {
        vector<string> ModulesList;

        for (auto &m : modules_map) {
            if (m.second.in_group)
                ModulesList.emplace_back(m.first);
        }

        return ModulesList;
    }

    vector<string> &GetPrefixList()
    {
        return PrefixList;
    }

    bool CheckAllModulesPrefix()
    {
        auto ModulesNotInGroup = GetModulesListNotInGroup();

        if (ModulesNotInGroup.size()) {
            GL1R(TAG, "Error: Modules without prefix:");
            for (auto &m : ModulesNotInGroup)
                GL1R("---> ", m);

            GL1R(TAG, "Please name modules with the following prefixes:");
            for (auto &prefix : PrefixList)
                GL1R("    \"", prefix, "\"");

            return false;
        }

        GL1G(TAG, "All modules using prefix");
        return true;
    }
};

#endif