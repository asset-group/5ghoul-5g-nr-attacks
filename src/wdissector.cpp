// Normal includes
#include <config.h>
#include <errno.h>
#include <functional>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
// Intrinsics include
#ifdef __x86_64__
#include <x86intrin.h>
#elif defined(__ARM_ARCH)
#include <arm_acle.h>
#else
#error "Processor architecture not supported"
#endif

// External libs include
#include <glib.h> // glib 2.0

// Wireshark includes
#include "capture/capture-pcap-util.h"
#include "epan/column-utils.h"
#include "epan/proto.h"
#include "extcap.h"
#include "file.h"
#include "frame_tvbuff.h"
#include "globals.h"
#include "ui/capture_ui_utils.h"
#include "ui/dissect_opts.h"
#include "ui/failure_message.h"
#include "ui/util.h"
#include <cli_main.h>
#include <epan/addr_resolv.h>
#include <epan/column.h>
#include <epan/disabled_protos.h>
#include <epan/dissectors/dissectors.h>
#include <epan/epan.h>
#include <epan/epan_dissect.h>
#include <epan/ftypes/ftypes-int.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/print.h>
#include <epan/proto.h>
#include <epan/stat_tap_ui.h>
#include <epan/tap.h>
#include <epan/timestamp.h>
#include <epan/tvbuff-int.h>
#include <epan/uat-int.h>
#include <epan/uat.h>
#include <setjmp.h>
#include <ui/clopts_common.h>
#include <ui/cmdarg_err.h>
#include <ui/version_info.h>
#include <wiretap/libpcap.h>
#include <wiretap/pcap-encap.h>
#include <wiretap/wtap.h>
#include <wiretap/wtap_opttypes.h>
#include <wsutil/file_util.h>
#include <wsutil/filesystem.h>
#include <wsutil/plugins.h>
#include <wsutil/privileges.h>
#include <wsutil/report_message.h>
#include <wsutil/wslog.h>

// Project includes
#include "libs/jeaiii_to_text.hpp" // Fast int to null terminated string
#include "libs/profiling.h"
#include "libs/whereami.h"
#include "wdissector.h"
#include <config.h>

#include <semaphore.h>
#include <thread>

// WDissector Config

#define DEFAULT_PROFILE_NAME "WDissector"
#define DEFAULT_LINK_TYPE "encap:1"
#define MAX_WD_INSTANCES 16

uint8_t DEMO_PKT_RRC_CONNECTION_SETUP[128] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    0x56, 0xde, 0x35, 0x40, 0x0, 0x40, 0x11, 0x5e,
    0x5f, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    0x1, 0xc9, 0xb6, 0x27, 0xf, 0x0, 0x42, 0xfe,
    0x55, 0x6d, 0x61, 0x63, 0x2d, 0x6c, 0x74, 0x65,
    0x1, 0x1, 0x3, 0x2, 0x28, 0xc3, 0x4, 0xc,
    0x96, 0x1, 0x3c, 0x20, 0x1d, 0x1f, 0x41, 0xa1,
    0xeb, 0x6e, 0x2c, 0x88, 0x68, 0x12, 0x98, 0xf,
    0x1c, 0xce, 0x1, 0x83, 0x80, 0xba, 0x30, 0x79,
    0x43, 0xfb, 0x80, 0x4, 0x23, 0x80, 0x89, 0x1a,
    0x2, 0x44, 0x68, 0xd, 0x90, 0x10, 0x8e, 0xa,
    0x0, 0x0, 0x0};

uint8_t DEMO_PKT_RRC_SETUP_COMPLETE[122] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    0x6c, 0xde, 0x36, 0x40, 0x0, 0x40, 0x11, 0x5e,
    0x48, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    0x1, 0xc9, 0xb6, 0x27, 0xf, 0x0, 0x58, 0xfe,
    0x6b, 0x6d, 0x61, 0x63, 0x2d, 0x6c, 0x74, 0x65,
    0x1, 0x0, 0x3, 0x2, 0x28, 0xc3, 0x4, 0xc,
    0xb0, 0x1, 0x3a, 0x3d, 0x21, 0xe, 0x1f, 0x9,
    0x0, 0xa0, 0x0, 0x0, 0x22, 0x0, 0x9, 0x8e,
    0x10, 0xff, 0xd2, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0};

uint8_t DEMO_PKT_RRC_RECONFIGURATION[114] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    0x64, 0xde, 0x42, 0x40, 0x0, 0x40, 0x11, 0x5e,
    0x44, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    0x1, 0xc9, 0xb6, 0x27, 0xf, 0x0, 0x50, 0xfe,
    0x63, 0x6d, 0x61, 0x63, 0x2d, 0x6c, 0x74, 0x65,
    0x1, 0x1, 0x3, 0x2, 0x28, 0xc3, 0x4, 0xd,
    0x12, 0x1, 0x21, 0x30, 0x1f, 0xa0, 0x3, 0x2,
    0x22, 0x2, 0x35, 0x38, 0x4, 0x67, 0x9c, 0x23,
    0x27, 0x0, 0x3e, 0xa0, 0x5f, 0x1c, 0xe1, 0xd8,
    0x85, 0xba, 0x30, 0x7d, 0xa8, 0x78, 0x0, 0x18,
    0x7, 0xf7, 0x0, 0x8, 0x47, 0x1, 0x12, 0x34,
    0x4, 0x88, 0xd0, 0x1b, 0x20, 0x21, 0x1c, 0x14,
    0x0, 0x92, 0x86, 0x3b, 0xa4, 0x0, 0x0, 0x0,
    0x0};

uint8_t DEMO_PKT_NAS_ATTACH_REQUEST[118] = {
    0x17, 0x37, 0x7b, 0xf3, 0x9f, 0x22, 0x7, 0x41, 0x2,
    0xb, 0xf6, 0x9, 0xf1, 0x7, 0x0, 0x2, 0x1,
    0xe5, 0x0, 0x86, 0xf3, 0x5, 0xf0, 0x70, 0xc0,
    0x40, 0x19, 0x0, 0x2a, 0x2, 0x10, 0xd0, 0x11,
    0xd1, 0x27, 0x23, 0x80, 0x80, 0x21, 0x10, 0x1,
    0x0, 0x0, 0x10, 0x81, 0x6, 0x0, 0x0, 0x0,
    0x0, 0x83, 0x6, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xd, 0x0, 0x0, 0xa, 0x0, 0x0, 0x5, 0x0,
    0x0, 0x10, 0x0, 0x0, 0x11, 0x0, 0x52, 0x9,
    0xf1, 0x7, 0x0, 0x1, 0x5c, 0xa, 0x0, 0x31,
    0x3, 0xe5, 0xe0, 0x3e, 0x90, 0x11, 0x3, 0x57,
    0x58, 0xa6, 0x20, 0xa, 0x60, 0x14, 0x4, 0x22,
    0x91, 0x81, 0xf, 0x1a, 0x1e, 0x50, 0x40, 0x8,
    0x4, 0x2, 0x60, 0x4, 0x0, 0x2, 0x1f, 0x2,
    0x5d, 0x1, 0x2, 0xe0, 0xc1};

// #define NODE_NOT_EXPERT_INFO(node) node->finfo->hfinfo != expert_info;

static void (*wd_log_fcn_g)(const char *msg) = NULL;
static void (*wd_log_fcn_y)(const char *msg) = NULL;
static void (*wd_log_fcn_r)(const char *msg) = NULL;

// Instance Variables
static enum wd_log_level global_log_level;
static uint8_t global_epan_initialized = 0;
static uint8_t global_wd_t_count = 0;
static GMutex wd_t_mutex = {0};      // global wd_t mutex
static GMutex wd_t_epan_mutex = {0}; // global wd_t mutex
static thread_local uint8_t wd_t_count = 0;
wtap_block_t g_wtap_block;
uint32_t *g_wtap_packet_direction;
static thread_local pthread_t current_pthread_instance;
gint (*field_callback)(field_info *fi) = NULL;

// Global Instance Variables
static int encap;
static e_prefs *prefs_p;
capture_file cfile;  // Capture File declaration
static wtap_rec rec; // record
static epan_dissect_t edt;
static int packet_dir = PACK_FLAGS_DIRECTION_UNKNOWN;
uat_t *user_dlt_module_uat = NULL; // DLT var

// Enable or disable full dissection of the tree
static uint8_t full_dissection = FALSE;
static uint8_t fast_full_dissection = FALSE;

// DFilter vars
static dfilter_t *filtercodes[1024]; // Store compiled filters
static uint16_t filtercode_index;
static header_field_info *fields_hfi[2048] = {NULL};

// Frame vars
static guint32 cum_bytes;
static frame_data ref_frame;
static frame_data prev_dis_frame;
static frame_data prev_cap_frame;

// user_dlt struct
typedef struct _user_encap_t {
    guint encap;
    char *payload_proto_name;
    dissector_handle_t payload_proto;
    char *header_proto_name;
    dissector_handle_t header_proto;
    char *trailer_proto_name;
    dissector_handle_t trailer_proto;
    guint header_size;
    guint trailer_size;
} user_encap_t;

typedef struct _wd_t {
    uint8_t protocol_index; // DLT_USER0..15 index
    uint8_t epan_initialized;
    pthread_t pthread_instance;
    uint8_t create_proto_tree;
    wtap_rec rec;
    epan_dissect_t edt;
    uint encap;
    gint (*field_callback)(field_info *fi) = NULL;
    e_prefs *prefs_p;
    capture_file cfile; // Capture File declaration
    uint8_t full_dissection;
    uint8_t fast_full_dissection;
    int packet_dir;
    wtap_block_t g_wtap_block;
    uint32_t *g_wtap_packet_direction;

    guint32 cum_bytes;
    frame_data ref_frame;
    frame_data prev_dis_frame;
    frame_data prev_cap_frame;
    frame_data fdata;

    uat_t *user_dlt_module_uat;
    uint8_t reset_request;

    wmem_scopes_t wmem_scopes; // Hold the address of the mempool for each thread
    pthread_mutex_t mutex_dissection;

} wd_t;

static thread_local wd_t *wd_t_allocated[MAX_WD_INSTANCES]; // Max user DLTs is 16
static thread_local wd_t wd_t_pool[MAX_WD_INSTANCES];       // Max user DLTs is 16
static wd_t **wd_t_global_allocated[MAX_WD_INSTANCES];      // Max user DLTs is 16

// Prototypes
extern "C" struct epan_uat *prefs_get_uat_value(pref_t *pref); // From epan
static char *str_copy_and_advance(const char *src_str, char *dest_str, char append_caracter);

// Functions

void wd_log_g(const char *msg)
{
    if (wd_log_fcn_g)
        wd_log_fcn_g(msg);
    else
        puts(msg);
}

void wd_log_y(const char *msg)
{
    if (wd_log_fcn_y)
        wd_log_fcn_y(msg);
    else
        puts(msg);
}

void wd_log_r(const char *msg)
{
    if (wd_log_fcn_r)
        wd_log_fcn_r(msg);
    else
        puts(msg);
}

void set_wd_log_g(void (*wd_func)(const char *))
{
    wd_log_fcn_g = wd_func;
}
void set_wd_log_y(void (*wd_func)(const char *))
{
    wd_log_fcn_y = wd_func;
}
void set_wd_log_r(void (*wd_func)(const char *))
{
    wd_log_fcn_r = wd_func;
}

const char *packet_read_value_to_string(uint32_t value, const header_field_info *hfinfo)
{
    static WS_THREAD_LOCAL char *str_convert[32];
    const char *s = NULL;

    // Convert special cases
    if (hfinfo->type == FT_BOOLEAN) {
        const true_false_string *tfstring = (const true_false_string *)hfinfo->strings;
        if (!tfstring)
            goto CONV_ERROR;

        return ((value & 0b1) ? tfstring->true_string : tfstring->false_string);
    }

    // Convert value_string for integer fields
    if (hfinfo->display & BASE_RANGE_STRING)
        return try_rval_to_str(value, (const range_string *)hfinfo->strings);

    if (hfinfo->display & BASE_EXT_STRING) {
        if (hfinfo->display & BASE_VAL64_STRING)
            return try_val64_to_str_ext(value, (val64_string_ext *)hfinfo->strings);
        else
            return try_val_to_str_ext(value, (value_string_ext *)hfinfo->strings);
    }

    if (hfinfo->display & BASE_VAL64_STRING)
        return try_val64_to_str(value, (const val64_string *)hfinfo->strings);

    if (hfinfo->display & BASE_UNIT_STRING)
        return unit_name_string_get_value(value, (const struct unit_name_string *)hfinfo->strings);

    s = try_val_to_str(value, (const value_string *)hfinfo->strings);

CONV_ERROR:

    if (!s) {
        if (global_log_level == WD_LOG_LEVEL_DEBUG)
            printf("[WDissector] String dict not found for %s = %d\n", hfinfo->abbrev, value);
        // Convert integer value to string if wireshark string not found
        *jeaiii::to_text_from_integer((char *)str_convert, value) = '\0';
        return (const char *)str_convert;
    }

    return s;
}

const char *
packet_get_value_to_string(guint32 value, const char *field_name)
{
    header_field_info *hfi = packet_get_header_info(field_name);
    if (hfi)
        return packet_read_value_to_string(value, hfi);
    else
        return NULL;
}

// Set protocol fast (do not append "proto:")
gboolean packet_set_protocol_fast(const char *proto_name)
{
    if (user_dlt_module_uat) {
        // Update initialized user dlt table
        dissector_handle_t dhandle = find_dissector(proto_name);
        if (dhandle) {
            encap = WTAP_ENCAP_USER0;
            user_encap_t *rec = (user_encap_t *)UAT_USER_INDEX_PTR(user_dlt_module_uat, 0);
            g_free(rec->payload_proto_name);
            rec->payload_proto_name = g_strdup(proto_name);
            rec->payload_proto = dhandle;
        }
        return TRUE;
    }
    return FALSE;
}

/**
 * Parse a link-type argument of the form "encap:<pcap linktype>" or
 * "proto:<proto name>".  "Pcap linktype" must be a name conforming to
 * pcap_datalink_name_to_val() or an integer; the integer should be
 * a LINKTYPE_ value supported by Wiretap.  "Proto name" must be
 * a protocol name, e.g. "http".
 */
gboolean packet_set_protocol(const char *lt_arg)
{
    // TODO: add new variable to edt->pi (packet_info *) to indicate custom dissection per packet
    const char *spec_ptr = strchr(lt_arg, ':');
    char *p;
    int dlt_val;
    long val;
    dissector_handle_t dhandle;
    GString *pref_str;
    char *errmsg = NULL;

    if (global_epan_initialized) {
        global_epan_initialized = 0;
        epan_dissect_cleanup(&edt);
    }

    if (!spec_ptr)
        return FALSE;

    spec_ptr++;
    if (strncmp(lt_arg, "encap:", strlen("encap:")) == 0) {
        dlt_val = linktype_name_to_val(spec_ptr);
        // printf("spec_ptr:%s\n",spec_ptr);
        if (dlt_val == -1) {
            errno = 0;
            val = strtol(spec_ptr, &p, 10);
            if (p == spec_ptr || *p != '\0' || errno != 0 || val > INT_MAX) {
                return FALSE;
            }
            dlt_val = (int)val;
        }
        /*
         * In those cases where a given link-layer header type
         * has different LINKTYPE_ and DLT_ values, linktype_name_to_val()
         * will return the OS's DLT_ value for that link-layer header
         * type, not its OS-independent LINKTYPE_ value.
         *
         * On a given OS, wtap_pcap_encap_to_wtap_encap() should
         * be able to map either LINKTYPE_ values or DLT_ values
         * for the OS to the appropriate Wiretap encapsulation.
         */
        encap = wtap_pcap_encap_to_wtap_encap(dlt_val);
        if (encap == WTAP_ENCAP_UNKNOWN) {
            return FALSE;
        }
        return TRUE;
    }
    else if (strncmp(lt_arg, "proto:", strlen("proto:")) == 0) {
        dhandle = find_dissector(spec_ptr);
        if (dhandle) {
            encap = WTAP_ENCAP_USER0;

            if (user_dlt_module_uat) {
                // Update initialized user dlt table
                user_encap_t *rec = (user_encap_t *)UAT_USER_INDEX_PTR(user_dlt_module_uat, 0);
                g_free(rec->payload_proto_name);
                rec->payload_proto_name = g_strdup(spec_ptr);
                rec->payload_proto = dhandle;
                return TRUE;
            }

            pref_str = g_string_new("uat:user_dlts:");
            /* This must match the format used in the user_dlts file */
            g_string_append_printf(pref_str,
                                   "\"User 0 (DLT=147)\",\"%s\",\"0\",\"\",\"0\",\"\"",
                                   spec_ptr);

            // prefs_reset(); // Reset preferences here so dissection works (Not needed)
            if (prefs_set_pref(pref_str->str, &errmsg) != PREFS_SET_OK) {
                g_string_free(pref_str, TRUE);
                g_free(errmsg);
                return FALSE;
            }
            g_string_free(pref_str, TRUE);
            // Store user dlt prefs for fast update later
            module_t *user_dlt_module = prefs_find_module("user_dlt");
            pref_t *user_dlt_prefs = prefs_find_preference(user_dlt_module, "encaps_table");
            user_dlt_module_uat = prefs_get_uat_value(user_dlt_prefs);
            return TRUE;
        }
    }
    return FALSE;
}

static const nstime_t *
raw_get_frame_ts(struct packet_provider_data *prov, guint32 frame_num)
{
    if (prov->ref && prov->ref->num == frame_num)
        return &prov->ref->abs_ts;

    if (prov->prev_dis && prov->prev_dis->num == frame_num)
        return &prov->prev_dis->abs_ts;

    if (prov->prev_cap && prov->prev_cap->num == frame_num)
        return &prov->prev_cap->abs_ts;

    return NULL;
}

static epan_t *raw_epan_new(capture_file *cf)
{
    static const struct packet_provider_funcs funcs = {
        raw_get_frame_ts,
        cap_file_provider_get_interface_name,
        cap_file_provider_get_interface_description,
        NULL,
    };

    return epan_new(&cf->provider, &funcs);
}

const char *wdissector_version_info()
{
    return get_appname_and_version();
}

const char *wdissector_profile_info()
{
    return get_profile_name();
}

void wdissector_enable_fast_full_dissection(uint8_t val)
{
    if (global_epan_initialized)
        edt.tree->tree_data->fast_full_dissection = val;
    else
        fast_full_dissection = val;
}

void wdissector_set_field_callback(gint (*fcn_callback)(field_info *fi))
{
    if (global_epan_initialized)
        edt.tree->tree_data->field_callback = fcn_callback;
    else
        field_callback = fcn_callback;
}

void wdissector_enable_full_dissection(uint8_t val)
{
    full_dissection = val;
}

epan_dissect_t *wdissector_get_edt()
{
    return &edt;
}

void wdissector_set_log_level(enum wd_log_level level)
{
    ws_log_set_level((enum ws_log_level)level);
    global_log_level = level;
}

uint8_t wdissector_init(const char *protocol_name)
{
    uint8_t return_status = 1;
    setlocale(LC_ALL, "");

    // Initialize log API with LOG_LEVEL_NONE
    ws_log_init("wdissector", vcmdarg_err);
    ws_log_set_level(LOG_LEVEL_NONE);

    /* Initialize the version information. */
    ws_init_version_info("WDissector Lib (Wireshark)",
                         epan_gather_compile_info,
                         NULL);

    // printf("%s\n", get_appname_and_version());

    /*
     * Get credential information for later use.
     */
    init_process_policies();

    /*
     * Attempt to get the pathname of the directory containing the
     * executable file.
     */
    char current_path[256];
    wai_getExecutablePath(current_path, sizeof(current_path), NULL);

    char *init_progfile_dir_error = configuration_init(current_path, NULL);
    if (init_progfile_dir_error != NULL) {
        printf("Can't get current pathname: %s\n", current_path);
        return_status = 0;
    }

    if (profile_exists(DEFAULT_PROFILE_NAME, TRUE)) {
        set_profile_name(DEFAULT_PROFILE_NAME);
    }
    else {
        printf("Profile \"%s\" not found!\n", DEFAULT_PROFILE_NAME);
        return_status = 0;
    }

    timestamp_set_type(TS_RELATIVE);
    timestamp_set_precision(TS_PREC_AUTO);
    timestamp_set_seconds_type(TS_SECONDS_DEFAULT);

    wtap_init(TRUE);

    /* Register all dissectors; we must do this before checking for the
       "-G" flag, as the "-G" flag dumps information registered by the
       dissectors, and we must do it before we read the preferences, in
       case any dissectors register preferences. */
    if (!epan_init(NULL, NULL, TRUE)) {
        printf("epan couldn't initialize\n");
        return_status = 0;
        exit(1);
    }

    /* Load libwireshark settings from the current profile. */
    prefs_p = epan_load_settings();

    cap_file_init(&cfile);

    // Disable DNS name resolution
    disable_name_resolution();

    /* Notify all registered modules that have had any of their preferences
       changed either from one of the preferences file or from the command
       line that their preferences have changed.
       Initialize preferences before display filters, otherwise modules
       like MATE won't work. */
    prefs_apply_all();

    /*
     * Enabled and disabled protocols and heuristic dissectors as per
     * command-line options.
     */
    setup_enabled_and_disabled_protocols();

    /* Build the column format array */
    build_column_format_array(&cfile.cinfo, prefs_p->num_cols, TRUE);

    /*
     * Immediately relinquish any special privileges we have; we must not
     * be allowed to read any capture files the user running Rawshark
     * can't open.
     */
    relinquish_special_privs_perm();
    if (protocol_name == NULL || !packet_set_protocol(protocol_name)) {
        packet_set_protocol(DEFAULT_LINK_TYPE);
    }

    /* Make sure we got a dissector handle for our payload. */
    if (encap == WTAP_ENCAP_UNKNOWN) {
        printf("No valid payload dissector specified\n");
        return_status = 0;
        exit(2);
    }

    /* Create new epan session for dissection. */
    epan_free(cfile.epan);
    cfile.epan = raw_epan_new(&cfile);
    cfile.provider.wth = NULL;
    cfile.f_datalen = 0; /* not used, but set it anyway */

    /* Set the file name because we need it to set the follow stream filter.
       XXX - is that still true?  We need it for other reasons, though,
       in any case. */
    cfile.filename = g_strdup("dissection");

    /* Indicate whether it's a permanent or temporary file. */
    cfile.is_tempfile = FALSE;

    /* No user changes yet. */
    cfile.unsaved_changes = FALSE;
    cfile.cd_t = WTAP_FILE_TYPE_SUBTYPE_UNKNOWN;
    cfile.open_type = WTAP_TYPE_AUTO;
    cfile.count = 0;
    cfile.drops_known = FALSE;
    cfile.drops = 0;
    cfile.snap = 0;
    nstime_set_zero(&cfile.elapsed_time);
    cfile.provider.ref = NULL;
    cfile.provider.prev_dis = NULL;
    cfile.provider.prev_cap = NULL;

    wtap_rec_init(&rec);
    rec.rec_type = REC_TYPE_PACKET;
    rec.presence_flags = WTAP_HAS_TS | WTAP_HAS_CAP_LEN;
    rec.ts.secs = 0;
    rec.ts.nsecs = 0;

    // Initialize wtap block for link layer flags (Direction, etc)
    g_wtap_block = wtap_block_create(WTAP_BLOCK_PACKET);
    wtap_block_add_uint32_option(g_wtap_block, OPT_PKT_FLAGS, packet_dir);
    wtap_optval_t *g_wtap_packet_flag = wtap_block_get_option(g_wtap_block, OPT_PKT_FLAGS);
    // Get wtap block pointer of packet flags for faster access
    g_wtap_packet_direction = &g_wtap_packet_flag->uint32val;

    // Initialize filtercodes array with NULL
    memset(filtercodes, 0, sizeof(filtercodes));
    return return_status;
}

wd_t *wd_init(const char *protocol_name)
{
    static WS_THREAD_LOCAL uint8_t wd_thread_already_initialized_once = 0;

    g_mutex_lock(&wd_t_epan_mutex);

    if (!wd_thread_already_initialized_once)
        current_pthread_instance = pthread_self(); // Track current thread

    // Only initialize epan 1 time
    if (!global_epan_initialized) {
        setlocale(LC_ALL, "");

        // Initialize log API with LOG_LEVEL_NONE
        ws_log_init("wdissector", vcmdarg_err);
        ws_log_set_level(LOG_LEVEL_NONE);

        /* Initialize the version information. */
        ws_init_version_info("WDissector Lib (Wireshark)",
                             epan_gather_compile_info,
                             NULL);

        /*
         * Get credential information for later use.
         */
        init_process_policies();

        /*
         * Attempt to get the pathname of the directory containing the
         * executable file.
         */
        char current_path[256];
        wai_getExecutablePath(current_path, sizeof(current_path), NULL);

        char *init_progfile_dir_error = configuration_init(current_path, NULL);
        if (init_progfile_dir_error != NULL) {
            printf("Can't get current pathname: %s\n", current_path);
            return NULL;
        }

        if (profile_exists(DEFAULT_PROFILE_NAME, TRUE)) {
            set_profile_name(DEFAULT_PROFILE_NAME);
        }
        else {
            printf("Profile \"%s\" not found!\n", DEFAULT_PROFILE_NAME);
            return NULL;
        }

        timestamp_set_type(TS_RELATIVE);
        timestamp_set_precision(TS_PREC_AUTO);
        timestamp_set_seconds_type(TS_SECONDS_DEFAULT);

        wtap_init(TRUE);

        /* Register all dissectors; we must do this before checking for the
        "-G" flag, as the "-G" flag dumps information registered by the
        dissectors, and we must do it before we read the preferences, in
        case any dissectors register preferences. */
        if (!epan_init(NULL, NULL, TRUE)) {
            printf("epan couldn't initialize\n");
            exit(1);
        }
    }

    // Init wmem_scopes for this thread if not existent
    wmem_scopes_t wmem_scopes = wmem_get_all_scopes();
    if (wmem_scopes.epan_scope == NULL &&
        wmem_scopes.file_scope == NULL &&
        wmem_scopes.packet_scope == NULL) {
        wmem_init_scopes();
    }

    // Initialize global wd_t pool
    if (!global_epan_initialized) {
        for (size_t i = 0; i < MAX_WD_INSTANCES; i++) {
            wd_t_global_allocated[i] = NULL;
        }
    }

    // Alocate new wd_t from static pool of wd_t for each thread
    if (!wd_thread_already_initialized_once) {
        // Initialize wd instances pool
        wd_t_count = 0;
        for (size_t i = 0; i < MAX_WD_INSTANCES; i++) {
            wd_t_allocated[i] = NULL;
            wd_t_pool[i] = {0};
        }
    }

    if (global_wd_t_count >= MAX_WD_INSTANCES) {
        printf("error: maximum number of wdissector instances reached! (16)\n");
        g_mutex_unlock(&wd_t_epan_mutex);
        return NULL;
    }

    // Initialize new wd_t from pool
    wd_t *new_wd_t = &wd_t_pool[wd_t_count];
    wd_t_count += 1;

    // Register new wd_t in global structures
    for (size_t i = 0; i < 16; i++) {
        if (wd_t_allocated[i] == NULL) {
            wd_t_allocated[i] = new_wd_t;
            wd_t_global_allocated[global_wd_t_count] = &wd_t_allocated[i];
            break;
        }
    }

    new_wd_t->protocol_index = global_wd_t_count;
    global_wd_t_count += 1;

    // Configure new wd_t values
    wd_set_dissection_mode(new_wd_t, WD_MODE_NORMAL); // Enable fast full dissection by default
    new_wd_t->create_proto_tree = 1;                  // Always create proto tree by default
    new_wd_t->pthread_instance = pthread_self();
    new_wd_t->wmem_scopes = wmem_get_all_scopes();
    new_wd_t->mutex_dissection = PTHREAD_MUTEX_INITIALIZER;
    new_wd_t->user_dlt_module_uat = NULL;

    /* Load libwireshark settings from the current profile. */
    new_wd_t->prefs_p = epan_load_settings();
    cap_file_init(&new_wd_t->cfile);

    if (!global_epan_initialized) {
        // Disable DNS name resolution
        disable_name_resolution();

        /* Notify all registered modules that have had any of their preferences
             changed either from one of the preferences file or from the command
             line that their preferences have changed.
             Initialize preferences before display filters, otherwise modules
             like MATE won't work. */
        prefs_apply_all();

        /*
         * Enabled and disabled protocols and heuristic dissectors as per
         * command-line options.
         */
        setup_enabled_and_disabled_protocols();
    }

    /* Build the column format array */
    build_column_format_array(&new_wd_t->cfile.cinfo, new_wd_t->prefs_p->num_cols, TRUE);

    /*
     * Immediately relinquish any special privileges we have; we must not
     * be allowed to read any capture files the user running Rawshark
     * can't open.
     */
    if (!global_epan_initialized)
        relinquish_special_privs_perm();

    if (protocol_name == NULL || (!wd_set_protocol(new_wd_t, protocol_name))) {
        if (!wd_set_protocol(new_wd_t, DEFAULT_LINK_TYPE)) {
            printf("Could not set default protocol: %s\n", DEFAULT_LINK_TYPE);
            g_mutex_unlock(&wd_t_epan_mutex);
            return NULL;
        }
    }

    /* Make sure we got a dissector handle for our payload. */
    if (new_wd_t->encap == WTAP_ENCAP_UNKNOWN) {
        printf("error: no valid payload dissector specified\n");
        g_mutex_unlock(&wd_t_epan_mutex);
        return NULL;
    }

    new_wd_t->cfile.f_datalen = 0; /* not used, but set it anyway */

    /* Set the file name because we need it to set the follow stream filter.
       XXX - is that still true?  We need it for other reasons, though,
       in any case. */
    new_wd_t->cfile.filename = g_strdup("dissection");

    /* Indicate whether it's a permanent or temporary file. */
    new_wd_t->cfile.is_tempfile = FALSE;

    /* No user changes yet. */
    new_wd_t->cfile.unsaved_changes = FALSE;
    new_wd_t->cfile.cd_t = WTAP_FILE_TYPE_SUBTYPE_UNKNOWN;
    new_wd_t->cfile.open_type = WTAP_TYPE_AUTO;
    new_wd_t->cfile.count = 0;
    new_wd_t->cfile.drops_known = FALSE;
    new_wd_t->cfile.drops = 0;
    new_wd_t->cfile.snap = 0;
    nstime_set_zero(&new_wd_t->cfile.elapsed_time);
    new_wd_t->cfile.provider.ref = NULL;
    new_wd_t->cfile.provider.prev_dis = NULL;
    new_wd_t->cfile.provider.prev_cap = NULL;

    /* Create single epan session for all dissection instances. */
    if (!wd_thread_already_initialized_once) {
        // Initialization per thread
        epan_free(new_wd_t->cfile.epan);
        new_wd_t->cfile.epan = raw_epan_new(&new_wd_t->cfile);
    }
    else if (!global_epan_initialized) {
        epan_free(wd_t_pool[0].cfile.epan);
        wd_t_pool[0].cfile.epan = raw_epan_new(&wd_t_pool[0].cfile);
    }
    else {
        // Later instances per thread reuse session from first instance
        new_wd_t->cfile.epan = wd_t_pool[0].cfile.epan;
    }
    new_wd_t->cfile.provider.wth = NULL;

    wtap_rec_init(&new_wd_t->rec);
    new_wd_t->rec.rec_type = REC_TYPE_PACKET;
    new_wd_t->rec.presence_flags = WTAP_HAS_TS | WTAP_HAS_CAP_LEN;
    new_wd_t->rec.ts.secs = 0;
    new_wd_t->rec.ts.nsecs = 0;

    // Initialize wtap block for link layer flags (Direction, etc)
    new_wd_t->g_wtap_block = wtap_block_create(WTAP_BLOCK_PACKET);
    wtap_block_add_uint32_option(new_wd_t->g_wtap_block, OPT_PKT_FLAGS, new_wd_t->packet_dir);
    wtap_optval_t *g_wtap_packet_flag = wtap_block_get_option(new_wd_t->g_wtap_block, OPT_PKT_FLAGS);
    // Get wtap block pointer of packet flags for faster access
    new_wd_t->g_wtap_packet_direction = &g_wtap_packet_flag->uint32val;

    // Initializes epan with new default dissection parameters (full_dissection=0, fast_full_dissection=1)
    epan_dissect_init(&new_wd_t->edt, new_wd_t->cfile.epan, TRUE, new_wd_t->full_dissection);
    new_wd_t->edt.tree->tree_data->fast_full_dissection = new_wd_t->fast_full_dissection;

    // Mark wd_t epan as initialized
    new_wd_t->epan_initialized = 1;
    global_epan_initialized = 1;
    wd_thread_already_initialized_once = 1;

    g_mutex_unlock(&wd_t_epan_mutex);
    return new_wd_t;
}

wd_t *wd_get(uint32_t instance_number)
{
    if (G_UNLIKELY(instance_number >= MAX_WD_INSTANCES))
        return NULL;

    return wd_t_allocated[instance_number];
}

void wd_set_log_level(enum wd_log_level level)
{
    ws_log_set_level((ws_log_level)level);
}

void wd_free(wd_t **wd)
{
    // TODO: api not well tested, may be buggy
    if (G_UNLIKELY(((!wd) && (!(*wd)) && (!(*wd)->epan_initialized))))
        return;

    g_mutex_lock(&wd_t_epan_mutex);

    pthread_mutex_lock(&(*wd)->mutex_dissection);

    // Backup scope
    wmem_scopes_t orig_scope = wmem_get_all_scopes();
    wmem_set_all_scopes((*wd)->wmem_scopes);

    cleanup_dissection();
    init_dissection();

    wmem_set_all_scopes(orig_scope);

    *(wd_t_global_allocated[(*wd)->protocol_index]) = NULL;
    wd_t_count -= 1;
    wd_t_global_allocated[(*wd)->protocol_index] = NULL;
    global_wd_t_count -= 1;

    pthread_mutex_unlock(&(*wd)->mutex_dissection);
    *wd = NULL;
    g_mutex_unlock(&wd_t_epan_mutex);
}

void wd_reset(wd_t *wd)
{
    if (G_UNLIKELY((!wd && wd->epan_initialized)))
        return;

    g_mutex_lock(&wd_t_epan_mutex);
    pthread_mutex_lock(&wd->mutex_dissection);

    // save current thread scope
    wmem_scopes_t orig_scope = wmem_get_all_scopes();
    // switch to wd scope
    wmem_set_all_scopes(wd->wmem_scopes);

    cleanup_dissection();
    init_dissection();

    // restore original thread scope
    wmem_set_all_scopes(orig_scope);

    pthread_mutex_unlock(&wd->mutex_dissection);
    g_mutex_unlock(&wd_t_epan_mutex);
}

void wd_reset_all()
{
    wd_t *g_wd = *wd_t_global_allocated[0];
    if (G_UNLIKELY(!g_wd && g_wd->epan_initialized))
        return;

    g_mutex_lock(&wd_t_epan_mutex);

    // save current thread scope
    wmem_scopes_t orig_scope = wmem_get_all_scopes();

    for (size_t i = 0; i < MAX_WD_INSTANCES; i++) {
        wd_t **wd = wd_t_global_allocated[i];
        if ((wd) && *wd) {
            pthread_mutex_lock(&(*wd)->mutex_dissection);
        }
    }

    for (size_t i = 0; i < MAX_WD_INSTANCES; i++) {
        wd_t **wd = wd_t_global_allocated[i];
        if ((wd) && *wd) {
            // switch to wd scope
            wmem_set_all_scopes((*wd)->wmem_scopes);

            cleanup_dissection();
            init_dissection();
        }
    }

    for (size_t i = 0; i < MAX_WD_INSTANCES; i++) {
        wd_t **wd = wd_t_global_allocated[i];
        if ((wd) && *wd) {
            pthread_mutex_unlock(&(*wd)->mutex_dissection);
        }
    }
    // restore original thread scope
    wmem_set_all_scopes(orig_scope);

    g_mutex_unlock(&wd_t_epan_mutex);
}

gboolean wd_set_protocol(wd_t *wd, const char *lt_arg)
{
    const char *spec_ptr = strchr(lt_arg, ':');

    if (!spec_ptr)
        return FALSE;

    spec_ptr++; // Skip ':'

    if (strncmp(lt_arg, "proto:", strlen("proto:")) == 0) {
        dissector_handle_t dhandle = find_dissector(spec_ptr);
        if (dhandle) {
            wd->encap = WTAP_ENCAP_USER0 + wd->protocol_index;

            if (wd->user_dlt_module_uat) {
                // Fast update of initialized user dlt table
                // No need for mutex here since each wd_t will access different indexes
                user_encap_t *u_encap = (user_encap_t *)UAT_USER_INDEX_PTR(wd->user_dlt_module_uat, wd->protocol_index);
                g_free(u_encap->payload_proto_name);
                u_encap->payload_proto_name = g_strdup(spec_ptr);
                u_encap->payload_proto = dhandle;
                return TRUE;
            }

            g_mutex_lock(&wd_t_mutex);
            // Store user dlt prefs pointer for fast update later
            module_t *user_dlt_module = prefs_find_module("user_dlt");
            pref_t *user_dlt_prefs = prefs_find_preference(user_dlt_module, "encaps_table");
            wd->user_dlt_module_uat = prefs_get_uat_value(user_dlt_prefs);

            uint8_t create_proto_prefs = 0;
            if (wd->user_dlt_module_uat->user_data->len == 17) {
                for (size_t i = 0; i < wd->user_dlt_module_uat->user_data->len; i++) {
                    user_encap_t *u_encap = (user_encap_t *)UAT_USER_INDEX_PTR(wd->user_dlt_module_uat, i);
                    // Check if all user encaps are in the correct order, if not, mark to create them
                    if (u_encap->encap != (WTAP_ENCAP_USER0 + i)) {
                        create_proto_prefs = 1;
                        break;
                    }
                }
            }
            else
                create_proto_prefs = 1;

            // Fill User DLT table with 16 protocols
            if (create_proto_prefs) {
                GString *pref_str = g_string_new("uat:user_dlts:");
                /* This must match the format used in the user_dlts file */

                for (uint i = 0; i < 16; i++) {
                    g_string_append_printf(pref_str,
                                           "\"User %d (DLT=%d)\",\"%s\",\"0\",\"\",\"0\",\"\"\n",
                                           i, DLT_USER0 + i, spec_ptr);
                }

                char *errmsg;
                if (prefs_set_pref(pref_str->str, &errmsg) != PREFS_SET_OK) {
                    printf("prefs_set_pref error: %s\n", errmsg);
                    g_string_free(pref_str, TRUE);
                    g_free(errmsg);
                    g_mutex_unlock(&wd_t_mutex);
                    return FALSE;
                }
                g_string_free(pref_str, TRUE);

                for (uint i = 0; i < 16; i++) {
                    user_encap_t *u_encap = (user_encap_t *)UAT_USER_INDEX_PTR(wd->user_dlt_module_uat, i);
                    u_encap->encap = WTAP_ENCAP_USER0 + i;
                }
            }

            // User DLT table already created, just update protocol entry
            g_mutex_unlock(&wd_t_mutex);
            user_encap_t *u_encap = (user_encap_t *)UAT_USER_INDEX_PTR(wd->user_dlt_module_uat, wd->protocol_index);
            g_free(u_encap->payload_proto_name);
            u_encap->payload_proto_name = g_strdup(spec_ptr);
            u_encap->payload_proto = dhandle;

            return TRUE;
        }
    }
    else if (strncmp(lt_arg, "encap:", strlen("encap:")) == 0) {

        if (wd->epan_initialized) {
            wd->epan_initialized = 0;
            epan_dissect_cleanup(&wd->edt);
        }

        int dlt_val = linktype_name_to_val(spec_ptr);
        if (dlt_val == -1) {
            errno = 0;
            char *p;
            long val = strtol(spec_ptr, &p, 10);
            if (p == spec_ptr || *p != '\0' || errno != 0 || val > INT_MAX) {
                return FALSE;
            }
            dlt_val = (int)val;
        }

        /*
         * In those cases where a given link-layer header type
         * has different LINKTYPE_ and DLT_ values, linktype_name_to_val()
         * will return the OS's DLT_ value for that link-layer header
         * type, not its OS-independent LINKTYPE_ value.
         *
         * On a given OS, wtap_pcap_encap_to_wtap_encap() should
         * be able to map either LINKTYPE_ values or DLT_ values
         * for the OS to the appropriate Wiretap encapsulation.
         */
        wd->encap = wtap_pcap_encap_to_wtap_encap(dlt_val);
        if (wd->encap == WTAP_ENCAP_UNKNOWN) {
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}

void wd_packet_dissect(wd_t *wd, unsigned char *raw_packet, uint32_t packet_length)
{
    uint8_t switch_scope = 0;
    wmem_scopes_t saved_scope;

    if (G_UNLIKELY(!wd))
        return;

    pthread_mutex_lock(&wd->mutex_dissection);

    switch_scope = (current_pthread_instance != wd->pthread_instance);
    if (switch_scope) {
        saved_scope = wmem_get_all_scopes();
        wmem_set_all_scopes(wd->wmem_scopes);
    }

    if (!wd->epan_initialized) {
        // Initializes epan with new full_dissection parameter
        epan_dissect_init(&wd->edt, wd->cfile.epan, wd->create_proto_tree, wd->full_dissection);
        wd->edt.tree->tree_data->fast_full_dissection = wd->fast_full_dissection;
        wd->edt.tree->tree_data->field_callback = wd->field_callback;
        wd->epan_initialized = TRUE;
    }
    else if (wd->edt.pi.data_src) {
        // Reset edt before dissection (saves time)
        epan_dissect_reset(&wd->edt);
    }

    // Configure record header
    wd->rec.rec_header.packet_header.caplen = packet_length;
    wd->rec.rec_header.packet_header.len = packet_length;
    wd->rec.rec_header.packet_header.pkt_encap = wd->encap;

    wd->cfile.count++;
    frame_data_init(&wd->fdata, wd->cfile.count, &wd->rec, 0, wd->cum_bytes);
    frame_data_set_before_dissect(&wd->fdata, &wd->cfile.elapsed_time,
                                  &wd->cfile.provider.ref, wd->cfile.provider.prev_dis);

    if (wd->cfile.provider.ref == &wd->fdata) {
        wd->ref_frame = wd->fdata;
        wd->cfile.provider.ref = &wd->ref_frame;
    }

    // Set direction using wtap block
    wd->rec.block = wd->g_wtap_block;              // Recover global rec wtap block
    wtap_block_ref(wd->rec.block);                 // Increment rec wtap block ref
    *wd->g_wtap_packet_direction = wd->packet_dir; // Write packet direction to block packet flags pointer;

    // Run actual packet dissection here
    epan_dissect_run(&wd->edt, wd->cfile.cd_t, &wd->rec,
                     frame_tvbuff_new(&wd->cfile.provider, &wd->fdata, raw_packet),
                     &wd->fdata, &wd->cfile.cinfo);

    frame_data_set_after_dissect(&wd->fdata, &wd->cum_bytes);

    wd->prev_dis_frame = wd->fdata;
    wd->cfile.provider.prev_dis = &wd->prev_dis_frame;

    wd->prev_cap_frame = wd->fdata;
    wd->cfile.provider.prev_cap = &wd->prev_cap_frame;

    frame_data_destroy(&wd->fdata);
    wtap_rec_cleanup(&wd->rec);

    if (switch_scope)
        wmem_set_all_scopes(saved_scope);

    pthread_mutex_unlock(&wd->mutex_dissection);
}

void wd_set_dissection_mode(wd_t *wd, enum wd_dissection_mode wd_mode)
{
    switch (wd_mode) {
    case WD_MODE_NORMAL:
        wd->fast_full_dissection = 1;
        wd->full_dissection = 0;
        break;

    case WD_MODE_FAST:
        wd->fast_full_dissection = 0;
        wd->full_dissection = 0;
        break;

    case WD_MODE_FULL:
        wd->fast_full_dissection = 0;
        wd->full_dissection = 1;
        break;

    default:
        break;
    }

    wd->epan_initialized = FALSE; // Force decoding reinit
}

void wd_set_packet_direction(wd_t *wd, uint32_t packet_dir)
{
    switch (packet_dir) {
    case P2P_DIR_RECV:
        wd->packet_dir = PACK_FLAGS_DIRECTION_INBOUND;
        break;
    case P2P_DIR_SENT:
        wd->packet_dir = PACK_FLAGS_DIRECTION_OUTBOUND;
        break;

    default:
        wd->packet_dir = PACK_FLAGS_DIRECTION_UNKNOWN;
        break;
    }
}

void wd_set_field_callback(wd_t *wd, gint (*fcn_callback)(field_info *fi))
{
    if (wd->epan_initialized)
        wd->edt.tree->tree_data->field_callback = fcn_callback;
    else
        wd->field_callback = fcn_callback;
}

uint8_t wd_packet_direction(wd_t *wd)
{
    switch (wd->packet_dir) {
    case PACK_FLAGS_DIRECTION_INBOUND:
        return P2P_DIR_RECV;
        break;

    case PACK_FLAGS_DIRECTION_OUTBOUND:
        return P2P_DIR_SENT;
        break;

    default:
        return P2P_DIR_UNKNOWN;
        break;
    }
}

epan_dissect_t *wd_edt(wd_t *wd)
{
    return &wd->edt;
}

const char *wd_info_version()
{
    return get_appname_and_version();
}

const char *wd_info_profile()
{
    return get_profile_name();
}

const char *wd_packet_protocol(wd_t *wd)
{
    return col_get_text(wd->edt.pi.cinfo, COL_PROTOCOL);
}

const char *wd_packet_summary(wd_t *wd)
{
    const char *summary = col_get_text(wd->edt.pi.cinfo, COL_INFO);

    if (G_UNLIKELY(!summary))
        return "";

    return summary;
}

const char *wd_packet_show(wd_t *wd)
{
    static thread_local char buffer[65535];

    FILE *stream = fmemopen(buffer, sizeof(buffer), "w");
    print_stream_t *print_text = print_stream_text_stdio_new(stream);

    if (!wd->full_dissection) {
        wd->epan_initialized = FALSE;
        wd->full_dissection = TRUE;
        wd_packet_dissect(wd, (uint8_t *)wd->edt.tvb->real_data, wd->edt.tvb->length);
        wd->full_dissection = FALSE;
    }

    proto_tree_print(print_dissections_expanded, FALSE, &wd->edt, NULL, print_text);

    fclose(stream);

    return buffer;
}

const char *wd_packet_show_pdml(wd_t *wd)
{
    static thread_local char buffer[65535];
    static thread_local output_fields_t *output_fields = NULL;

    if (output_fields == NULL)
        output_fields = output_fields_new();

    FILE *stream = fmemopen(buffer, sizeof(buffer), "w");

    if (!wd->full_dissection) {
        wd->epan_initialized = FALSE;
        wd->full_dissection = TRUE;
        wd_packet_dissect(wd, (uint8_t *)wd->edt.tvb->real_data, wd->edt.tvb->length);
        wd->full_dissection = FALSE;
    }

    write_pdml_preamble(stream, wd->cfile.filename);
    write_pdml_proto_tree(output_fields, NULL, PF_NONE, &wd->edt, &wd->cfile.cinfo, stream, FALSE);
    write_pdml_finale(stream);

    fclose(stream);

    return buffer;
}

uint32_t wd_packet_layers_count(wd_t *wd)
{
    proto_tree *node = wd->edt.tree;
    proto_tree *subnode = NULL;
    uint32_t layers_count = 0;

    // 1. Root Tree Children (Layers) (Loop)
    node = node->first_child;
    while (node != NULL) {
        layers_count++;

        // 2. Node Children(Fields or other layers)(Loop)
        // printf("==== Layer: %s, Type=%s, Size=%d\n", node->finfo->hfinfo->name, node->finfo->value.ftype->name, node->finfo->length);

        if ((subnode = node->first_child)) {

            while (subnode != NULL) {
                // 3.1 Field Group (Intermediary Node)
                if (subnode->first_child != NULL) {
                    // Only process groups not skipped
                    if (subnode->finfo->length) // Only process groups with fields not skipped
                    {
                        if (subnode->finfo->value.ftype->ftype == FT_PROTOCOL || subnode->parent->parent == NULL) {
                            // printf("==== Layer: %s, Type=%s, Size=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->value.ftype->name, subnode->finfo->length);
                            layers_count++;
                        }
                    }
                    // subnode_parent = subnode;
                    subnode = subnode->first_child;
                }
                // 3.2 Leaf (Child Node)
                else if (subnode->next != NULL) {
                    subnode = subnode->next;
                }
                // 3.3 Final Leaf (Last Child Node)
                else {
                    // Return to previous parent recursivelly
                    subnode = subnode->parent;
                    while (subnode->next == NULL) {
                        if (!subnode->parent) {
                            // detect final subnode
                            subnode = NULL;
                            node = NULL;
                            break;
                        }

                        // puts("<--- Return");
                        subnode = subnode->parent;
                    }
                    if (subnode)
                        subnode = subnode->next;
                }
            }
        }

        if (node)
            node = node->next;
    }

    return layers_count - 1;
}

uint32_t wd_packet_dissectors_count(wd_t *wd)
{
    guint count = wmem_list_count(wd->edt.pi.layers);
    // Subtract the frame and user DLT dissectors from the count
    return (count >= 2 ? count - 2 : 0);
}

// Return the name of the dissector given its index
const char *wd_packet_dissector(wd_t *wd, uint8_t layer_index)
{
    if (layer_index >= wmem_list_count(wd->edt.pi.layers) - 2)
        return NULL;

    wmem_list_frame_t *protocol_layer = wmem_list_head(wd->edt.pi.layers);
    protocol_layer = wmem_list_frame_next(protocol_layer);

    // Iterate trough protocol layers
    for (uint8_t i = 0; i <= layer_index; i++) {
        protocol_layer = wmem_list_frame_next(protocol_layer);
    }
    // Get identification of protocol layer
    int protocol_id = GPOINTER_TO_INT(wmem_list_frame_data(protocol_layer));
    // Get name of layer given its identification number
    return proto_get_protocol_filter_name(protocol_id);
}

const char *wd_packet_dissectors(wd_t *wd)
{
    static thread_local gchar local_text[256]; // fixed buffer to print layers
    char *idx = local_text;

    wmem_list_frame_t *protocol_layer = wmem_list_head(wd->edt.pi.layers);
    int layers_count = wd_packet_dissectors_count(wd);
    protocol_layer = wmem_list_frame_next(protocol_layer);

    while (layers_count--) {
        protocol_layer = wmem_list_frame_next(protocol_layer);
        int protocol_id = GPOINTER_TO_INT(wmem_list_frame_data(protocol_layer));
        const char *layer_name = proto_get_protocol_filter_name(protocol_id);

        idx = str_copy_and_advance(layer_name, idx, '/');
    }

    *(idx - 3) = 0;
    return local_text;
}

header_field_info *wd_field(const char *field_name)
{
    return proto_registrar_get_byname(field_name);
}

wd_filter_t wd_filter(const char *filter_string)
{
    wd_filter_t compiled_filter = NULL;
    if (dfilter_compile(filter_string, (dfilter_t **)&compiled_filter, NULL))
        return compiled_filter;
    else
        return NULL;
}

// Register (prune) all fields with the same name
uint8_t wd_register_field(wd_t *wd, wd_field_t hfi)
{
    uint8_t same_name_fields_found = 0;

    if (wd->edt.pi.data_src) {
        epan_dissect_reset(&wd->edt); // reset here, so packet_dissect ignore it later
    }

    while (1) {
        // below is equivalent to epan_dissect_prime_with_hfid(&wd->edt, hfi->id);
        header_field_info *hfinfo = proto_registrar_get_nth(hfi->id);

        /* this field is referenced by a filter so increase the refcount.
           also increase the refcount for the parent, i.e the protocol.
        */
        hfinfo->ref_type = HF_REF_TYPE_DIRECT;
        /* only increase the refcount if there is a parent.
           if this is a protocol and not a field then parent will be -1
           and there is no parent to add any refcounting for.
        */
        if (hfinfo->parent != -1) {
            header_field_info *parent_hfinfo = proto_registrar_get_nth(hfinfo->parent);

            /* Mark parent as indirectly referenced unless it is already directly
             * referenced, i.e. the user has specified the parent in a filter.
             */
            if (parent_hfinfo->ref_type != HF_REF_TYPE_DIRECT)
                parent_hfinfo->ref_type = HF_REF_TYPE_INDIRECT;
        }
        // -----------------------------

        if (hfi->same_name_prev_id != -1)
            hfi = proto_registrar_get_nth(hfi->same_name_prev_id);
        else
            return same_name_fields_found;

        same_name_fields_found++;
    }
}

void wd_register_filter(wd_t *wd, wd_filter_t compiled_filter)
{
    if (wd->edt.pi.data_src) {
        epan_dissect_reset(&wd->edt); // reset here, so packet_dissect ignore it later
    }
    epan_dissect_prime_with_dfilter(&wd->edt, (const struct epan_dfilter *)compiled_filter);
}

field_info *wd_read_field(wd_t *wd, wd_field_t hfi)
{
    while (true) {
        GPtrArray *pointers = (GPtrArray *)g_hash_table_lookup(PTREE_DATA(wd->edt.tree)->interesting_hfids,
                                                               GINT_TO_POINTER(hfi->id));

        if (g_ptr_array_len(pointers) > 0) {
            // return first match of field info
            return (field_info *)pointers->pdata[0];
        }
        // Check id of next field with same name
        if (hfi->same_name_prev_id != -1)
            hfi = proto_registrar_get_nth(hfi->same_name_prev_id);
        else
            return NULL;
    }

    return NULL;
}

GPtrArray *wd_read_all_fields(wd_t *wd, wd_field_t hfi)
{
    while (true) {
        GPtrArray *pointers = (GPtrArray *)g_hash_table_lookup(PTREE_DATA(wd->edt.tree)->interesting_hfids,
                                                               GINT_TO_POINTER(hfi->id));
        if (pointers)
            return pointers;

        // Check id of next field with same name
        if (hfi->same_name_prev_id != -1)
            hfi = proto_registrar_get_nth(hfi->same_name_prev_id);
        else
            return NULL;
    }
}

uint8_t wd_read_filter(wd_t *wd, wd_filter_t compiled_filter)
{
    if (G_UNLIKELY(!compiled_filter))
        return FALSE;

    static GMutex mutex;
    g_mutex_lock(&mutex);
    gboolean ret = dfilter_apply_edt((dfilter_t *)compiled_filter, &wd->edt);
    g_mutex_unlock(&mutex);

    return ret;
}

void packet_set_direction(int dir)
{
    switch (dir) {
    case P2P_DIR_RECV:
        packet_dir = PACK_FLAGS_DIRECTION_INBOUND;
        break;
    case P2P_DIR_SENT:
        packet_dir = PACK_FLAGS_DIRECTION_OUTBOUND;
        break;

    default:
        packet_dir = PACK_FLAGS_DIRECTION_UNKNOWN;
        break;
    }
}

void packet_dissect(unsigned char *raw_packet, uint32_t packet_length)
{
    frame_data fdata;

    if (!global_epan_initialized) {
        // Initializes epan with new full_dissection parameter
        global_epan_initialized = 1;
        epan_dissect_init(&edt, cfile.epan, TRUE, full_dissection);
        edt.tree->tree_data->fast_full_dissection = fast_full_dissection;
        edt.tree->tree_data->field_callback = field_callback;
    }
    else if (edt.pi.data_src) {
        // Reset edt before dissection (saves time)
        // wmem_free_all(wmem_file_scope());
        epan_dissect_reset(&edt);
    }

    rec.rec_header.packet_header.caplen = packet_length;
    rec.rec_header.packet_header.len = packet_length;
    rec.rec_header.packet_header.pkt_encap = encap;
    // --

    cfile.count++;
    frame_data_init(&fdata, cfile.count, &rec, 0, cum_bytes);

    frame_data_set_before_dissect(&fdata, &cfile.elapsed_time,
                                  &cfile.provider.ref, cfile.provider.prev_dis);
    if (cfile.provider.ref == &fdata) {
        ref_frame = fdata;
        cfile.provider.ref = &ref_frame;
    }

    // Set direction using wtap block
    rec.block = g_wtap_block;              // Recover global rec wtap block
    wtap_block_ref(rec.block);             // Increment rec wtap block ref
    *g_wtap_packet_direction = packet_dir; // Write packet direction to block packet flags pointer;

    // Run actual packet dissection here
    epan_dissect_run(&edt, cfile.cd_t, &rec,
                     frame_tvbuff_new(&cfile.provider, &fdata, raw_packet),
                     &fdata, &cfile.cinfo);

    frame_data_set_after_dissect(&fdata, &cum_bytes);

    prev_dis_frame = fdata;
    cfile.provider.prev_dis = &prev_dis_frame;

    prev_cap_frame = fdata;
    cfile.provider.prev_cap = &prev_cap_frame;

    frame_data_destroy(&fdata);

    // epan_dissect_cleanup(&edt); // Cleanup (not necessary)
    wtap_rec_cleanup(&rec);
    // wtap_rec_reset(&rec);
}

const char *packet_show()
{
    static char buffer[16384]; // Yes, asn1 generated structure is over the top

    FILE *stream = fmemopen(buffer, sizeof(buffer), "w");
    print_stream_t *print_text = print_stream_text_stdio_new(stream);

    if (!full_dissection) {
        global_epan_initialized = FALSE;
        full_dissection = TRUE;
        packet_dissect((uint8_t *)edt.tvb->real_data, edt.tvb->length);
        full_dissection = FALSE;
    }

    proto_tree_print(print_dissections_expanded, FALSE, &edt, NULL, print_text);

    fclose(stream);

    full_dissection = FALSE;

    return buffer;
}

const char *packet_show_pdml()
{
    static char buffer[64384]; // Yes, asn1 generated structure is way over the top
    static output_fields_t *output_fields = NULL;

    if (output_fields == NULL)
        output_fields = output_fields_new();

    FILE *stream = fmemopen(buffer, sizeof(buffer), "w");

    if (!full_dissection) {
        global_epan_initialized = FALSE;
        full_dissection = TRUE;
        packet_dissect((uint8_t *)edt.tvb->real_data, edt.tvb->length);
        full_dissection = FALSE;
    }

    write_pdml_preamble(stream, cfile.filename);
    // write_psml_columns(&edt, stream, FALSE);
    write_pdml_proto_tree(output_fields, NULL, PF_NONE, &edt, &cfile.cinfo, stream, FALSE);
    write_pdml_finale(stream);

    full_dissection = FALSE;

    fclose(stream);

    return buffer;
}

char *packet_description()
{
    static char buffer[8192];

    FILE *stream = fmemopen(buffer, sizeof(buffer), "w");
    print_stream_t *print_text = print_stream_text_stdio_new(stream);

    if (!full_dissection) {
        global_epan_initialized = FALSE;
        full_dissection = TRUE;
        packet_dissect((uint8_t *)edt.tvb->real_data, edt.tvb->length);
        full_dissection = FALSE;
    }

    proto_tree_print(print_dissections_collapsed, FALSE, &edt, NULL, print_text);
    fclose(stream);

    full_dissection = FALSE;

    return buffer;
}

const char *packet_summary()
{
    return col_get_text(edt.pi.cinfo, COL_INFO);
}

uint8_t packet_direction()
{
    switch (packet_dir) {
    case PACK_FLAGS_DIRECTION_INBOUND:
        return P2P_DIR_RECV;
        break;
    case PACK_FLAGS_DIRECTION_OUTBOUND:
        return P2P_DIR_SENT;
        break;

    default:
        return P2P_DIR_UNKNOWN;
        break;
    }
}

uint32_t packet_layers_count()
{
    epan_dissect_t *edt = wdissector_get_edt();
    proto_tree *node = edt->tree;
    proto_tree *subnode = NULL, *subnode_parent;
    uint32_t layers_count = 0;

    // 1. Root Tree Children (Layers) (Loop)
    node = node->first_child;
    while (node != NULL) {
        layers_count++;

        // 2. Node Children(Fields or other layers)(Loop)
        // printf("==== Layer: %s, Type=%s, Size=%d\n", node->finfo->hfinfo->name, node->finfo->value.ftype->name, node->finfo->length);

        if ((subnode = node->first_child)) {

            while (subnode != NULL) {
                // 3.1 Field Group (Intermediary Node)
                if (subnode->first_child != NULL) {
                    // Only process groups not skipped
                    if ((subnode->finfo->length || subnode->first_child->finfo->length)) {

                        if (subnode->finfo->length) // Only process groups with fields not skipped
                        {
                            if (subnode->finfo->value.ftype->ftype == FT_PROTOCOL || subnode->parent->parent == NULL) {
                                // printf("==== Layer: %s, Type=%s, Size=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->value.ftype->name, subnode->finfo->length);
                                layers_count++;
                            }
                        }
                        // subnode_parent = subnode;
                        subnode = subnode->first_child;
                    }
                    else {
                        // Skip current group
                        if (subnode->next)
                            subnode = subnode->next;
                        else {
                            // Return to previous parent recursivelly
                            // puts("<--- Return");
                            subnode = subnode->parent;
                            while (subnode->next == NULL) {
                                if (!subnode->parent) {
                                    // detect final subnode
                                    subnode = NULL;
                                    node = NULL;
                                    break;
                                }

                                // puts("<--- Return");
                                subnode = subnode->parent;
                            }
                            if (subnode)
                                subnode = subnode->next;
                        }
                    }
                }
                // 3.2 Leaf (Child Node)
                else if (subnode->next != NULL) {
                    subnode = subnode->next;
                }
                // 3.3 Final Leaf (Last Child Node)
                else {
                    // Return to previous parent recursivelly
                    subnode = subnode->parent;
                    while (subnode->next == NULL) {
                        if (!subnode->parent) {
                            // detect final subnode
                            subnode = NULL;
                            node = NULL;
                            break;
                        }

                        // puts("<--- Return");
                        subnode = subnode->parent;
                    }
                    if (subnode)
                        subnode = subnode->next;
                }
            }
        }

        if (node)
            node = node->next;
    }

    return layers_count - 1;
}

uint32_t packet_dissectors_count()
{ // discount the frame and user dlt from the count
    guint count = wmem_list_count(edt.pi.layers);
    return (count >= 2 ? count - 2 : 0);
}

// return the name of the dissector given its index, starting after the first layer (frame)
const char *packet_dissector(uint8_t layer_index)
{

    if (layer_index >= wmem_list_count(edt.pi.layers) - 2)
        return NULL;

    wmem_list_frame_t *protocol_layer = wmem_list_head(edt.pi.layers);
    protocol_layer = wmem_list_frame_next(protocol_layer);

    // Iterate trough protocol layers
    for (uint8_t i = 0; i <= layer_index; i++) {
        protocol_layer = wmem_list_frame_next(protocol_layer);
    }
    // Get identification of protocol layer
    int protocol_id = GPOINTER_TO_INT(wmem_list_frame_data(protocol_layer));
    // Get name of layer given its identification number
    return proto_get_protocol_filter_name(protocol_id);
}

static char *str_copy_and_advance(const char *src_str, char *dest_str, char append_caracter)
{
    dest_str = stpcpy(dest_str, src_str);
    if (append_caracter) {
        *(dest_str++) = ' ';
        *(dest_str++) = '/';
        *(dest_str++) = ' ';
    }

    return dest_str;
}

const char *packet_dissectors()
{
    static gchar local_text[256]; // fixed buffer to print layers
    char *idx = local_text;

    wmem_list_frame_t *protocol_layer = wmem_list_head(edt.pi.layers);
    int layers_count = packet_dissectors_count();
    protocol_layer = wmem_list_frame_next(protocol_layer);

    while (layers_count--) {
        protocol_layer = wmem_list_frame_next(protocol_layer);
        int protocol_id = GPOINTER_TO_INT(wmem_list_frame_data(protocol_layer));
        const char *layer_name = proto_get_protocol_filter_name(protocol_id);

        idx = str_copy_and_advance(layer_name, idx, '/');
    }

    *(idx - 3) = 0;
    return local_text;
}

const char *packet_relevant_fields()
{
    static gchar local_text[256]; // fixed buffer to print layers
    char *text = local_text;

    if (!full_dissection) {
        global_epan_initialized = FALSE;
        full_dissection = TRUE;
        packet_dissect((uint8_t *)edt.tvb->real_data, edt.tvb->length);
        full_dissection = FALSE;
    }

    proto_node *p_tree = edt.tree->first_child->next;
    proto_node *p_sub_node;

    while (p_tree) {

        text = str_copy_and_advance(p_tree->finfo->hfinfo->abbrev, text, '/');

        if (p_tree->first_child) {
            p_sub_node = p_tree->first_child;
            while (p_sub_node) {
                text = str_copy_and_advance(p_sub_node->finfo->hfinfo->abbrev, text, '/');
                p_sub_node = p_sub_node->next;
            }
        }

        p_tree = p_tree->next;
    }
    *(text - 3) = 0; // truncate the additional slash after last field/layer name
    return local_text;
}

const char *packet_protocol()
{
    return col_get_text(edt.pi.cinfo, COL_PROTOCOL);
}

void packet_cleanup()
{
    if (!global_epan_initialized)
        return;

    // wmem_free_all(wmem_file_scope());
    // wtap_rec_cleanup(&rec);
    epan_dissect_cleanup(&edt);
    epan_free(cfile.epan);

    cfile.epan = raw_epan_new(&cfile);
    cfile.provider.wth = NULL;
    cfile.f_datalen = 0; /* not used, but set it anyway */
    global_epan_initialized = 0;
    wtap_rec_init(&rec);
}

void packet_navigate(uint32_t skip_layers, uint32_t skip_groups, uint8_t (*callback)(proto_tree *, uint8_t, uint8_t *))
{
    proto_tree *node = edt.tree;
    proto_tree *subnode = NULL, *subnode_parent;
    uint32_t layers_count = 0;
    uint32_t groups_count = 0;
    // uint32_t level = 0;

    if (!node || !callback)
        return;

    // 1. Root Tree Children (Layers) (Loop)
    node = node->first_child;
    while (node != NULL) {
        layers_count++;

        if (layers_count > skip_layers) {
            // 2. Node Children(Fields or other layers)(Loop)
            callback(node, WD_TYPE_LAYER, (uint8_t *)edt.tvb->real_data);
            // printf("==== Layer: %s, Type=%s, Size=%d\n", node->finfo->hfinfo->name, node->finfo->value.ftype->name, node->finfo->length);

            if ((subnode = node->first_child)) {
                // level++;

                while (subnode != NULL) {
                    // 3.1 Field Group (Intermediary Node)
                    if (subnode->first_child != NULL) {
                        groups_count++;
                        // Only process groups not skipped
                        if ((groups_count > skip_groups) && (1 || subnode->finfo->length || subnode->first_child->finfo->length)) {

                            if (subnode->finfo->length) // Only process groups with fields not skipped
                            {
                                if (subnode->finfo->value.ftype->ftype == FT_PROTOCOL || subnode->parent->parent == NULL)
                                    callback(subnode, WD_TYPE_LAYER, (uint8_t *)edt.tvb->real_data);
                                // printf("==== Layer: %s, Type=%s, Size=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->value.ftype->name, subnode->finfo->length);
                                else if (subnode->first_child->finfo->length)
                                    callback(subnode, WD_TYPE_GROUP, (uint8_t *)edt.tvb->real_data);
                                // printf("---> Field Group: %s, Type=%s, Size=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->value.ftype->name, subnode->finfo->length);
                                else
                                // Group with a children without size is actually a field
                                {
                                    callback(subnode, WD_TYPE_FIELD, (uint8_t *)edt.tvb->real_data);
                                }
                                // printf("Field: %s, Size=%d, Type=%s, Offset=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->length, subnode->finfo->value.ftype->name, packet_read_field_offset(subnode->finfo));
                            }
                            // subnode_parent = subnode;
                            // level++;
                            subnode = subnode->first_child;
                        }
                        else {
                            // Skip current group
                            if (subnode->next)
                                subnode = subnode->next;
                            else {
                                // Return to previous parent recursivelly
                                // puts("<--- Return");
                                // level--;
                                subnode = subnode->parent;
                                while (subnode->next == NULL) {
                                    if (!subnode->parent) {
                                        // detect final subnode
                                        subnode = NULL;
                                        node = NULL;
                                        break;
                                    }

                                    // puts("<--- Return");
                                    // level--;
                                    subnode = subnode->parent;
                                }
                                if (subnode)
                                    subnode = subnode->next;
                            }
                        }
                    }
                    // 3.2 Leaf (Child Node)
                    else if (subnode->next != NULL) {
                        if (subnode->finfo->length && subnode->finfo->value.ftype->ftype != FT_PROTOCOL && subnode->finfo->value.ftype->ftype != FT_NONE) // Only process fields not skipped
                            callback(subnode, WD_TYPE_FIELD, (uint8_t *)edt.tvb->real_data);
                        // printf("Field: %s, Size=%d, Type=%s, Offset=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->length, subnode->finfo->value.ftype->name, packet_read_field_offset(subnode->finfo));

                        subnode = subnode->next;
                    }
                    // 3.3 Final Leaf (Last Child Node)
                    else {
                        if (subnode->finfo->length && subnode->finfo->value.ftype->ftype != FT_NONE) // Only process fields not skipped
                            callback(subnode, WD_TYPE_FIELD, (uint8_t *)edt.tvb->real_data);
                        // printf("Field: %s, Size=%d, Type=%s, Offset=%d\n", subnode->finfo->hfinfo->name, subnode->finfo->length, subnode->finfo->value.ftype->name, packet_read_field_offset(subnode->finfo));

                        // Return to previous parent recursivelly
                        // puts("<--- Return");
                        // level--;
                        subnode = subnode->parent;
                        while (subnode->next == NULL) {
                            if (!subnode->parent) {
                                // detect final subnode
                                subnode = NULL;
                                node = NULL;
                                break;
                            }

                            // puts("<--- Return");
                            // level--;
                            subnode = subnode->parent;
                        }
                        if (subnode)
                            subnode = subnode->next;
                    }
                }
            }
        }

        if (node)
            node = node->next;
    }
}

uint32_t count_field_errors = 0;
uint32_t count_unchecked_fields = 0;

static uint8_t test_navigate_callback(proto_tree *subnode, uint8_t field_type, uint8_t *pkt_buf)
{
    const char *value_check;
    int64_t raw_val;
    int64_t parsed_val = 0;
    uint8_t check_val = 0;
    const char *parsed_val_str;
    uint64_t mask;
    uint8_t align_to_octet = 0;
    uint32_t field_length = subnode->finfo->length;

    switch (field_type) {
    case WD_TYPE_FIELD:
        // --------------- Raw value ---------------
        // Assume field is 8 bytes (ignore read-only overflow for performance)
        raw_val = *(int64_t *)&pkt_buf[packet_read_field_offset(subnode->finfo)];

        // print raw_val in hexadecimal

        if (subnode->finfo->ds_tvb->bitshift_from_octet != 0) {
            align_to_octet = subnode->finfo->ds_tvb->bitshift_from_octet;
            puts("bitshift_from_octet");
            if (align_to_octet > 0) {
                raw_val >>= align_to_octet;
            }
            else if (align_to_octet < 0) {
                raw_val <<= -align_to_octet;
            }

            ++field_length;
        }

        // Check if format is BIG_ENDIAN and swap bytes for raw value
        if ((field_length > 1) &&
            ((packet_read_field_encoding(subnode->finfo) == FI_BIG_ENDIAN) || (align_to_octet > 0))) {
            switch (field_length) {
            case 2:
                puts("__bswap_16");
                raw_val = __bswap_16(raw_val);
                break;
            case 4:
                puts("__bswap_32");
                raw_val = __bswap_32(raw_val);
                break;
            case 8:
                puts("__bswap_64");
                raw_val = __bswap_64(raw_val);
                break;
            default: {
                if (field_length > 8)
                    break;
                puts("arbitrary length");
                // Swap bytes for raw value to big endian
                uint64_t tmp;
                uint8_t *p = (uint8_t *)&tmp;
                uint8_t *q = (uint8_t *)&raw_val;
                int i;
                for (i = 0; i < field_length; i++)
                    p[i] = q[field_length - i - 1];
                raw_val = tmp;
            } break;
            }
        }

        if (align_to_octet)
            --field_length;

        mask = packet_read_field_bitmask(subnode->finfo);

        if (mask) {
            raw_val = (raw_val & mask) >> packet_read_field_bitmask_offset(mask); // For fields that have mask
        }
        else {
            raw_val &= (UINT64_MAX) >> (64 - (field_length << 3)); // For fields without mask (val << 3 means val * 8)
        }

        // --------------- Get Parsed value from Wireshark ---------------
        parsed_val = packet_read_field_int64(subnode->finfo);
        parsed_val_str = packet_read_field_string(subnode->finfo);

        // Check BASE_OUI
        if (subnode->finfo->hfinfo->display == BASE_OUI) {
            uint8_t p_oui[3];
            p_oui[0] = raw_val >> 16 & 0xFF;
            p_oui[1] = raw_val >> 8 & 0xFF;
            p_oui[2] = raw_val & 0xFF;
            // check oui by converting converting p_oui to uint32_t (1 byte read overflow ignored)
            check_val = (parsed_val == (*((uint32_t *)p_oui) & 0xFFFFFF));
        }
        // Check FT_BYTES up to 8 bytes (64 bits)
        else if (subnode->finfo->value.ftype->ftype == FT_BYTES) {
            parsed_val = 0;
            memcpy(&parsed_val, subnode->finfo->value.value.bytes->data, (field_length > 8 ? 8 : field_length)); // Copy
            check_val = !memcmp(&parsed_val, &raw_val, (field_length > 8 ? 8 : field_length));                   // Truncate comparison to 8 bytes max
        }
        // Check normal integers
        else {
            check_val = (parsed_val == (raw_val + subnode->finfo->value_min));
        }

        // Ignore certain special fields
        if (!check_val &&
            (subnode->finfo->hfinfo->display == BASE_CUSTOM ||
             subnode->finfo->value.ftype->ftype == FT_ETHER ||
             subnode->finfo->value.ftype->ftype == FT_IPv4 ||
             subnode->finfo->value.ftype->ftype == FT_IPv6 ||
             subnode->finfo->value.ftype->ftype == FT_BYTES ||
             subnode->finfo->value.ftype->ftype == FT_STRING ||
             (subnode->finfo->flags & FI_GENERATED))) {
            value_check = "\033[1;33m!\033[00m";
            count_unchecked_fields += 1;
        }
        // Check OK
        else if (check_val) {
            value_check = "\033[36m✓\033[00m";
        }
        // Check failed
        else {
            value_check = "\033[33mX\033[00m";
            count_field_errors += 1;
        }

        printf("     %s Field [%03u:%01d-%01d] \"%s\",Size=%u,Type=%s,%s,Mask=%lX,Value=%ld,Min:%ld,Max=%ld,Raw Value:%ld (0x%lX), String:%s\n",
               value_check,
               packet_read_field_offset(subnode->finfo),
               packet_read_field_bitmask_offset(mask),
               packet_read_field_bitmask_offset(mask) + packet_read_field_size_bits(mask) - 1,
               subnode->finfo->hfinfo->abbrev,
               field_length,
               (packet_read_field_encoding(subnode->finfo) == FI_BIG_ENDIAN) ? "B" : "L",
               subnode->finfo->value.ftype->name,
               mask,
               parsed_val, // TODO: FT_BYTES will print garbage
               subnode->finfo->value_min,
               subnode->finfo->value_max,
               raw_val,
               raw_val,
               parsed_val_str);
        break;

    case WD_TYPE_GROUP:
        parsed_val = packet_read_field_int64(subnode->finfo);
        mask = packet_read_field_bitmask(subnode->finfo);

        printf("\033[36m"
               "---> Group: %s, Size=%u, Type=%s, Encoding=%d, Offset=%u, Mask=%lX, Bit=%d\n"
               "\033[00m",
               subnode->finfo->hfinfo->abbrev,
               field_length,
               subnode->finfo->value.ftype->name,
               packet_read_field_encoding(subnode->finfo),
               packet_read_field_offset(subnode->finfo),
               mask,
               packet_read_field_bitmask_offset(mask));
        break;
    case WD_TYPE_LAYER:
        parsed_val = packet_read_field_int64(subnode->finfo);
        mask = packet_read_field_bitmask(subnode->finfo);
        printf("\033[33m"
               "==== Layer: %s, Size=%u, Type=%s, Offset=%u, Mask=%lX, Bit=%d\n"
               "\033[00m",
               subnode->finfo->hfinfo->abbrev,
               field_length,
               subnode->finfo->value.ftype->name,
               packet_read_field_offset(subnode->finfo),
               mask,
               packet_read_field_bitmask_offset(mask));
        break;

    default:
        break;
    }

    return 0;
}

void packet_benchmark(gboolean detailed)
{
    // for (uint32_t i = 0; i < 1000; i++) {
    //     // packet_dissect(DEMO_PKT_RRC_CONNECTION_SETUP+34, 66); // udp
    //     packet_dissect(DEMO_PKT_RRC_RECONFIGURATION + 49, sizeof(DEMO_PKT_RRC_RECONFIGURATION) - 49);
    // }

    // PROFILING_LOOP_START;
    // packet_get_field("mac-lte.length");
    // PROFILING_LOOP_STOP("packet_get_field");

    // PROFILING_LOOP_START;
    // packet_has_condition("lte-rrc.rrc_TransactionIdentifier==1 && lte-rrc.sr_ConfigIndex==5");
    // PROFILING_LOOP_STOP("packet_has_condition");

    // packet_register_condition("lte-rrc.rrc_TransactionIdentifier==1 && lte-rrc.sr_ConfigIndex==5", 0);
    // PROFILING_LOOP_START;
    // packet_set_condition(0);
    // packet_dissect(DEMO_PKT_RRC_RECONFIGURATION + 49, sizeof(DEMO_PKT_RRC_RECONFIGURATION) - 49);
    // packet_read_condition(0);
    // PROFILING_LOOP_STOP("packet_has_condition (cached)");

    if (detailed) {
        wd_t *wd = wd_init("proto:mac-lte-framed");
        wd_set_dissection_mode(wd, WD_MODE_NORMAL);
        for (uint32_t i = 0; i < 100; i++) {
            PROFILING_LOOP_START;
            wd_packet_dissect(wd, DEMO_PKT_RRC_CONNECTION_SETUP + 49, sizeof(DEMO_PKT_RRC_CONNECTION_SETUP) - 49); // mac-lte-framed
            // packet_dissect(DEMO_PKT_RRC_RECONFIGURATION + 49, sizeof(DEMO_PKT_RRC_RECONFIGURATION) - 49); // mac-lte-framed
            PROFILING_LOOP_STOP("packet_dissect");
        }

        // BLE ATT Packet
        uint8_t packet[] = {0x4b, 0x1e, 0x0, 0x2, 0x0, 0x0, 0x0, 0xa, 0x3,
                            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                            0x70, 0x83, 0x32, 0x9a, 0x2, 0xb, 0x7, 0x0,
                            0x4, 0x0, 0x10, 0x24, 0x0, 0xff, 0xff, 0x0,
                            0x28, 0x14, 0x32, 0xf9};

        wd_set_protocol(wd, "proto:nordic_ble");
        wd_filter_t filter1 = wd_filter("btatt.opcode in {0x04, 0x02, 0x0A, 0x06, 0x08, 0x0E, 0x10, 0x0C, 0x20, 0x18, 0x16, 0x12} || btle.control_opcode in {0x03, 0x05, 0x08, 0x0A, 0x0E, 0x0F, 0x12, 0x14, 0x16, 0x1A, 0x1D, 0x1F, 0x23} || btle.link_layer_data.channel_map > 00:00:00:00:00 and btle.link_layer_data.window_offset >= 0 and btle.link_layer_data.window_offset <= btle.link_layer_data.interval and btle.link_layer_data.interval >= 6 and btle.link_layer_data.interval <= 3200 and btle.link_layer_data.latency >= 0 and btle.link_layer_data.latency <= {{uint(frame[45:2])*2}-1} and btle.link_layer_data.latency < 500 and btle.link_layer_data.timeout > {{1+uint(frame[47:2])}*uint(frame[45:2])*2} and btle.link_layer_data.timeout >= 10 and btle.link_layer_data.timeout <= 3200 and btle.link_layer_data.hop >= 5 and btle.link_layer_data.hop <= 16 and btle.link_layer_data.sleep_clock_accuracy >= 0 and btle.link_layer_data.sleep_clock_accuracy <= 7 and len(btle.crc)*8 >= 20 and len(btle.crc)*8 <= 24");
        wd_field_t field1 = wd_field("btle.advertising_header.pdu_type");

        for (uint32_t i = 0; i < 100; i++) {
            printf("%04d ", i);

            PROFILING_LOOP_START;
            wd_register_filter(wd, filter1);
            wd_register_field(wd, field1);
            wd_packet_dissect(wd, packet, sizeof(packet));
            uint8_t ret = wd_read_filter(wd, filter1);
            wd_field_info_t fi = wd_read_field(wd, field1);
            // puts(wd_packet_summary(wd));
            PROFILING_LOOP_STOP("packet_has_filter");

            // Do some memory cleanup
            wd_reset(wd);
        }
        exit(0);
    }
}

const char *packet_register_filter(const char *filter)
{
    const char *compiled_filter;
    if (dfilter_compile(filter, (dfilter_t **)&compiled_filter, NULL))
        return compiled_filter;
    else
        return NULL;
}

void packet_set_filter(const char *filter)
{
    if (edt.pi.data_src) {
        epan_dissect_reset(&edt); // reset here, so packet_dissect ignore it later
    }
    epan_dissect_prime_with_dfilter(&edt, (const struct epan_dfilter *)filter);
}

gboolean packet_read_filter(const char *filter)
{
    if (filter != NULL) {
        // If filter is enabled and rfcode is registered
        return dfilter_apply_edt((dfilter_t *)filter, &edt);
    }
    return FALSE;
}

void packet_clean_filter(const char **compiled_filter)
{
    if (compiled_filter && *compiled_filter) {
        dfilter_free(*((dfilter_t **)compiled_filter));
        *compiled_filter = NULL;
    }
}

gboolean packet_register_condition(const char *filter, uint16_t condition_index)
{
    if (filtercodes[condition_index] != NULL) {
        dfilter_free(filtercodes[condition_index]);
    }
    return dfilter_compile(filter, &filtercodes[condition_index], NULL);
}

void packet_set_condition(uint16_t condition_index)
{
    if (filtercodes[filtercode_index] != NULL) {
        filtercode_index = condition_index;

        if (edt.pi.data_src) {
            epan_dissect_reset(&edt); // reset here, so packet_dissect ignore it later
        }
        epan_dissect_prime_with_dfilter(&edt, filtercodes[filtercode_index]);
    }
}

gboolean packet_read_condition(uint16_t condition_index)
{

    if (filtercodes[condition_index] != NULL) {
        // If filter is enabled and rfcode is registered
        return dfilter_apply_edt(filtercodes[condition_index], &edt);
    }

    return FALSE;
}

gboolean packet_has_condition(const char *filter)
{
    if (!edt.tvb)
        return FALSE;
    // [Method 2] Compile and use a filter
    dfilter_t *rfcode;
    gboolean result;

    if (filter != NULL) {
        if (dfilter_compile(filter, &rfcode, NULL)) {

            uint8_t *pkt_buf = (uint8_t *)edt.tvb->real_data;
            uint32_t pkt_len = edt.tvb->length;

            if (edt.pi.data_src) {
                epan_dissect_reset(&edt); // reset here, so packet_dissect ignore it later
            }
            epan_dissect_prime_with_dfilter(&edt, rfcode);
            packet_dissect(pkt_buf, pkt_len);
            result = dfilter_apply_edt(rfcode, &edt); // return filter value (true or false)
            dfilter_free(rfcode);
            return result;
        }
    }
    else if (filtercodes[filtercode_index] != NULL) {
        // If filter is enabled and rfcode is registered
        result = dfilter_apply_edt(filtercodes[filtercode_index], &edt);

        return result;
    }

    return FALSE;
}

gboolean packet_register_field(const char *field_name, uint16_t field_hfi_index)
{
    if (field_name != NULL) {
        header_field_info *hfi = proto_registrar_get_byname(field_name);

        if (!hfi)
            return FALSE; // Field was not found

        fields_hfi[field_hfi_index] = hfi;
        return TRUE;
    }

    return FALSE;
}

gboolean packet_set_field(uint16_t hfi_index)
{
    header_field_info *hfi = fields_hfi[hfi_index];

    if (!hfi)
        return FALSE; // Field was not registered

    return (packet_set_field_hfinfo_all(hfi) > 0);
}

gboolean packet_register_set_field(const char *field_name, uint16_t field_hfi_index)
{
    gboolean ret = packet_register_field(field_name, field_hfi_index);

    if (!ret)
        return FALSE;

    return packet_set_field(field_hfi_index);
}

// Return first match of the field
field_info *packet_read_field(uint16_t hfi_index)
{

    GPtrArray *pointers = proto_get_finfo_ptr_array(edt.tree, fields_hfi[hfi_index]->id);

    if (g_ptr_array_len(pointers) > 0) {
        // return first match of field info
        return (field_info *)pointers->pdata[0];
    }

    return NULL;
}

// Return all matches of the field
GPtrArray *packet_read_fields(uint16_t hfi_index)
{
    header_field_info *hfi = fields_hfi[hfi_index];

    if (G_UNLIKELY(!hfi))
        return NULL;

    return packet_read_fields_hfinfo(hfi);
}

field_info *packet_read_field_at(GPtrArray *fields, uint16_t idx)
{
    return (field_info *)fields->pdata[idx];
}

// -------------
header_field_info *packet_register_set_field_hfinfo(const char *field_name)
{
    header_field_info *hfi = packet_get_header_info(field_name);

    if (G_UNLIKELY(!hfi))
        return NULL;

    packet_set_field_hfinfo_all(hfi);

    return hfi;
}

void packet_set_field_hfinfo(header_field_info *hfi)
{
    if (edt.pi.data_src) {
        epan_dissect_reset(&edt); // reset here, so packet_dissect ignore it later
    }

    epan_dissect_prime_with_hfid(&edt, hfi->id);
}

uint8_t packet_set_field_hfinfo_all(header_field_info *hfi)
{
    uint8_t same_name_fields_found = 0;

    if (edt.pi.data_src) {
        epan_dissect_reset(&edt); // reset here, so packet_dissect ignore it later
    }

    while (1) {
        // TODO: measure impact of packet_set_field_hfinfo_all and add into packet_set_field_hfinfo
        // Possibly by exposing proto_tree_prime_with_hfid here
        epan_dissect_prime_with_hfid(&edt, hfi->id);
        if (hfi->same_name_prev_id != -1)
            hfi = proto_registrar_get_nth(hfi->same_name_prev_id);
        else
            return same_name_fields_found;

        same_name_fields_found++;
    }
}

int packet_read_field_exists_hfinfo(header_field_info *hfi)
{
    return ((int *)packet_read_field_hfinfo(hfi) > (int *)0 ? 1 : 0);
}

// Return first match of the field. Such match iterates over all fields with the same name
// Important: All Fields with same name must be pruned
field_info *packet_read_field_hfinfo(header_field_info *hfi)
{
    while (true) {
        GPtrArray *pointers = (GPtrArray *)g_hash_table_lookup(PTREE_DATA(edt.tree)->interesting_hfids,
                                                               GINT_TO_POINTER(hfi->id));

        if (g_ptr_array_len(pointers) > 0) {
            // return first match of field info
            return (field_info *)pointers->pdata[0];
        }
        // Check id of next field with same name
        if (hfi->same_name_prev_id != -1)
            hfi = proto_registrar_get_nth(hfi->same_name_prev_id);
        else
            return NULL;
    }

    return NULL;
}

GPtrArray *packet_read_fields_hfinfo(header_field_info *hfi)
{
    if (!hfi)
        return NULL;

    while (true) {
        GPtrArray *pointers = (GPtrArray *)g_hash_table_lookup(PTREE_DATA(edt.tree)->interesting_hfids,
                                                               GINT_TO_POINTER(hfi->id));
        if (pointers)
            return pointers;

        // Check id of next field with same name
        if (hfi->same_name_prev_id != -1)
            hfi = proto_registrar_get_nth(hfi->same_name_prev_id);
        else
            return NULL;
    }
}

// -------------

const char *packet_read_field_name(field_info *field_match)
{
    if (field_match)
        return field_match->hfinfo->name;
    else
        return NULL;
}

const char *packet_read_field_abbrev(field_info *field_match)
{
    if (field_match)
        return field_match->hfinfo->abbrev;
    else
        return NULL;
}

uint16_t packet_read_field_offset(field_info *field_match)
{

    uint32_t offset = field_match->start + field_match->ds_tvb->offset_from_parent;
    tvbuff_t *tvbuff_parent = field_match->ds_tvb->parent;

    // printf("1) field_match->ds_tvb->offset_from_parent=%d\n", field_match->ds_tvb->offset_from_parent);

    while ((tvbuff_parent != NULL) && (tvbuff_parent->raw_offset == 0)) {
        offset += tvbuff_parent->offset_from_parent;
        tvbuff_parent = tvbuff_parent->parent;
    }

    // puts("---------------------");
    // printf("1) field_match->start=%d\n", field_match->start);
    // printf("1) field_match->ds_tvb->offset_from_parent=%d\n", field_match->ds_tvb->offset_from_parent);

    // // if (field_match->ds_tvb->parent) {
    // //     printf("--------Parent--------\n");
    // //     tvbuff_parent = field_match->ds_tvb->parent;
    // //     printf("2) tvbuff_parent->raw_offset=%d\n", tvbuff_parent->raw_offset);
    // //     printf("2) tvbuff_parent->offset_from_parent=%d\n", tvbuff_parent->offset_from_parent);
    // //     tvbuff_parent = tvbuff_parent->parent;
    // //     if (tvbuff_parent) {
    // //         printf("3) tvbuff_parent->raw_offset=%d\n", tvbuff_parent->raw_offset);
    // //         printf("3) tvbuff_parent->offset_from_parent=%d\n", tvbuff_parent->offset_from_parent);

    // //         tvbuff_parent = tvbuff_parent->parent;
    // //         if (tvbuff_parent) {
    // //             printf("4) tvbuff_parent->raw_offset=%d\n", tvbuff_parent->raw_offset);
    // //             printf("4) tvbuff_parent->offset_from_parent=%d\n", tvbuff_parent->offset_from_parent);
    // //         }
    // //     }
    // // }

    return offset;
}

uint32_t packet_read_field_size(field_info *field_match)
{
    return field_match->length;
}

uint8_t packet_read_field_size_bits(uint64_t bitmask)
{
    if (G_UNLIKELY(!bitmask))
        return 0;

#ifdef __x86_64__
    return abs((int64_t)__bsfq(bitmask) - (int64_t)__bsrq(bitmask)) + 1;
#elif defined(__ARM_ARCH)
    return ::abs((int64_t)__clzll(bitmask) - (int64_t)__clzll(__rbit(bitmask))) + 1;
#else
#error "Processor architecture not supported"
#endif
}

uint64_t packet_read_field_bitmask(field_info *field_match)
{
    int bit_size = FI_GET_BITS_SIZE(field_match);

    // if (field_match->hfinfo->id == wd_field("gsm_a.classmark3.gsm_850_assoc_radio_cap")->id) {
    //     printf("FI_GET_BITS_OFFSET=%d\n", FI_GET_BITS_OFFSET(field_match));
    // }
    // if (field_match->hfinfo->id == wd_field("e212.tai.mcc")->id) {
    //     printf("FI_GET_BITS_OFFSET=%d\n", FI_GET_BITS_OFFSET(field_match));
    // }
    // if (field_match->hfinfo->id == wd_field("gsm_a.len")->id) {
    //     printf("FI_GET_BITS_OFFSET=%d\n", FI_GET_BITS_OFFSET(field_match));
    // }
    // uint8_t flag = 0;
    // if (field_match->hfinfo->id == wd_field("gsm_a.classmark3.gsm_850_assoc_radio_cap")->id) {
    //     printf("FI_GET_BITS_OFFSET=%d\n", FI_GET_BITS_OFFSET(field_match));
    //     flag = 1;
    // }

    // if (field_match->hfinfo->id == wd_field("lte-rrc.nomPDSCH_RS_EPRE_Offset")->id) {
    //     printf("FI_GET_BITS_OFFSET=%d\n", FI_GET_BITS_OFFSET(field_match));
    //     printf("min=%d\n", field_match->value_min);
    //     // flag = 1;
    // }

    // if (bit_size && packet_read_field_encoding(field_match) == FI_LITTLE_ENDIAN)
    //     return field_match->hfinfo->bitmask;

    // if (bit_size) // unaligned bit fields (not per)
    // {

    // Handle big endian case
    //    if (bit_size){
    // Fix 2
    if (bit_size && !field_match->hfinfo->bitmask) {
        uint64_t new_mask = 0xffffffffffffffffUL >> bit_size;
        new_mask = ~new_mask; // Create mask at bit offset 0 msb

        uint64_t a_p = field_match->length << 3;
        uint64_t b_p = FI_GET_BITS_OFFSET(field_match);
        uint64_t c_p = 64 - a_p + b_p;
        uint64_t sc_part = new_mask >> c_p;

        // if (flag) sc_part = 1;

        uint64_t n_mask = sc_part;

        // n_mask = (new_mask >> (64 - (field_match->length << 3) + FI_GET_BITS_OFFSET(field_match)));

        // printf("a_p=%d, b_p=%d, c_p=%d, sc_part=%d\n", a_p, b_p, c_p,sc_part);

        // printf("2. length=%d, bit_size=%d, start=%d, mask=0x%" PRIxFAST64
        //         ",o_mask=%" PRIxFAST64 ",bit_offset=%d,"
        //         "a_p=%llu,b_p=%llu,c_p=%llu,sc_part=%lu\n",
        //        field_match->length, bit_size,
        //        packet_read_field_bitmask_offset(n_mask),
        //        n_mask,
        //        field_match->hfinfo->bitmask,
        //        FI_GET_BITS_OFFSET(field_match),
        //        a_p,b_p,c_p,sc_part);

        return n_mask;
    }
    // Fix 2
    // }

    if (field_match->hfinfo->bitmask && field_match->source_tree && (field_match->source_tree->finfo->flags & FI_LITTLE_ENDIAN))
        return field_match->hfinfo->bitmask;

    // printf("1. length=%d, bit_size=%d, start=%d, mask=0x%" PRIxFAST64 " \n",
    //        field_match->length, bit_size,
    //        packet_read_field_bitmask_offset(field_match->hfinfo->bitmask),
    //        field_match->hfinfo->bitmask);

    // TODO: handle > 64 bits unaligned FT_BYTES
    return field_match->hfinfo->bitmask;
}

uint8_t packet_read_field_bitmask_offset(uint64_t bitmask)
{
    if (G_UNLIKELY(!bitmask))
        return 0;

// Find the first set bit starting from the lsb.
#ifdef __x86_64__
    return __bsfq(bitmask);
#elif defined(__ARM_ARCH)
    return __clzll(__rbitll(bitmask));
#else
#error "Processor architecture not supported"
#endif
}

uint8_t packet_read_field_bitmask_offset_msb(uint64_t bitmask)
{
    if (G_UNLIKELY(!bitmask))
        return 0;

        // Find the first set bit starting from the lsb.
#ifdef __x86_64__
    return __bsrq(bitmask);
#elif defined(__ARM_ARCH)
    return __clzll(bitmask);
#else
#error "Processor architecture not supported"
#endif
}

unsigned char *packet_read_field_ustring(field_info *field_match)
{
    if (field_match->value.value.strbuf)
        return (unsigned char *)field_match->value.value.strbuf->str;
    else
        return NULL;
}

GByteArray *packet_read_field_bytes(field_info *field_match)
{
    return field_match->value.value.bytes;
}

int packet_read_field_type(field_info *field_match)
{
    return (int)field_match->value.ftype->ftype;
}

const char *packet_read_field_type_name(field_info *field_match)
{
    return field_match->value.ftype->name;
}

uint32_t packet_read_field_encoding(field_info *field_match)
{
    if (G_UNLIKELY(!field_match->source_tree))
        return 0;

    return field_match->source_tree->finfo->flags & (FI_LITTLE_ENDIAN | FI_BIG_ENDIAN);
}

const char *packet_read_field_display_name(field_info *field_match)
{
    return proto_field_display_to_string(field_match->hfinfo->display);
}

const char *packet_read_field_encoding_name(field_info *field_match)
{
    uint32_t flags = packet_read_field_encoding(field_match);

    if (G_UNLIKELY(!flags))
        return "unknown";

    return (flags & FI_BIG_ENDIAN ? "FI_BIG_ENDIAN" : "FI_LITTLE_ENDIAN");
}

const char *packet_read_field_string(field_info *field_match)
{
    // Buffer to store last field_string field statically
    static thread_local char stored_f_val_str[256];
    gchar *val_str = fvalue_to_string_repr(NULL, &field_match->value, FTREPR_DISPLAY, field_match->hfinfo->display);
    if (G_UNLIKELY(!val_str))
        return NULL;

    // Safely copy str to dst (truncate)
    g_strlcpy(stored_f_val_str, val_str, sizeof(stored_f_val_str));
    wmem_free(NULL, val_str);
    return stored_f_val_str;
}

uint32_t packet_read_field_uint32(field_info *field_match)
{
    return field_match->value.value.uinteger;
}

int32_t packet_read_field_int32(field_info *field_match)
{
    return field_match->value.value.sinteger;
}

uint64_t packet_read_field_uint64(field_info *field_match)
{
    return field_match->value.value.uinteger64;
}

int64_t packet_read_field_int64(field_info *field_match)
{
    return field_match->value.value.sinteger64;
}

header_field_info *packet_get_header_info(const char *field_name)
{
    header_field_info *hfi = proto_registrar_get_byname(field_name);

    if (G_UNLIKELY(!hfi))
        return NULL;

    return hfi;
}

int packet_get_field_exists(const char *field_name)
{
    return ((int *)packet_get_field(field_name) > (int *)0 ? 1 : 0);
}

field_info *packet_get_field(const char *field_name)
{
    if (G_UNLIKELY(!edt.tvb))
        return NULL;

    header_field_info *hfi = proto_registrar_get_byname(field_name);

    if (G_UNLIKELY(!hfi))
        return NULL;

    uint8_t *pkt_buf = (uint8_t *)edt.tvb->real_data;
    uint32_t pkt_len = edt.tvb->length;

    // [Method 1] Prime tree if the header information exists
    packet_set_field_hfinfo_all(hfi);

    // Dissect again (we have to dissect again if field was not primed before)
    packet_dissect(pkt_buf, pkt_len);

    return packet_read_field_hfinfo(hfi);
}

GPtrArray *packet_get_fields(const char *field_name)
{
    if (G_UNLIKELY(!edt.tvb))
        return NULL;

    header_field_info *hfi = proto_registrar_get_byname(field_name);

    if (G_UNLIKELY(!hfi))
        return NULL;

    // [Method 1] Prime tree if the header information exists
    packet_set_field_hfinfo_all(hfi);

    uint8_t *pkt_buf = (uint8_t *)edt.tvb->real_data;
    uint32_t pkt_len = edt.tvb->length;
    // Dissect again (we have to dissect again if field was not primed before)
    packet_dissect(pkt_buf, pkt_len);

    return packet_read_fields_hfinfo(hfi);
}

// Return field name
const char *packet_get_field_name(const char *field_name)
{
    header_field_info *header_match = packet_get_header_info(field_name);

    if (header_match)
        return header_match->name;
    else
        return NULL;
}

uint32_t packet_get_field_uint32(const char *field_name)
{
    field_info *field_match = packet_get_field(field_name);

    if (field_match)
        return packet_read_field_uint32(field_match);
    else
        return 0;
}

const char *packet_get_field_string(const char *field_name)
{
    field_info *field_match = packet_get_field(field_name);

    if (field_match)
        return packet_read_field_string(field_match);
    else
        return NULL;
}

uint32_t packet_get_field_size(const char *field_name)
{
    field_info *field_match = packet_get_field(field_name);

    if (field_match)
        return packet_read_field_size(field_match);
    else
        return 0;
}

int packet_get_field_type(const char *field_name)
{

    field_info *field_match = packet_get_field(field_name);

    if (field_match)
        return (int)field_match->value.ftype->ftype;
    else
        return 0;
}

const char *packet_get_field_type_name(const char *field_name)
{

    field_info *field_match = packet_get_field(field_name);

    if (field_match)
        return field_match->value.ftype->name;
    else
        return NULL;
}

const char *packet_get_field_encoding_name(const char *field_name)
{

    field_info *field_match = packet_get_field(field_name);

    if (field_match)
        return (field_match->flags & ENC_LITTLE_ENDIAN ? "ENC_LITTLE_ENDIAN" : "ENC_BIG_ENDIAN");
    else
        return NULL;
}

uint32_t packet_get_field_encoding(const char *field_name)
{

    field_info *field_match = packet_get_field(field_name);

    if (field_match)
        return packet_read_field_encoding(field_match);
    else
        return 0;
}

unsigned long packet_get_field_bitmask(const char *field_name)
{
    field_info *field_match = packet_get_field(field_name);

    if (field_match != NULL) {
        int bit_size = FI_GET_BITS_SIZE(field_match);
        if (bit_size) // unaligned bit fields (not per)
        {
            uint64_t new_mask = 0xffffffffffffffffUL >> bit_size;
            new_mask = ~new_mask; // Create mask at bit offset 0 msb

            return new_mask >> (64 - (field_match->length << 3) +
                                FI_GET_BITS_OFFSET(field_match));
        }
        // TODO: handle > 64 bits unaligned FT_BYTES
        return field_match->hfinfo->bitmask;
    }
    else {
        return 0;
    }
}

uint32_t packet_get_field_offset(const char *field_name)
{
    field_info *field_match = packet_get_field(field_name);

    if (field_match != NULL) {
        uint32_t offset = field_match->start;
        proto_tree *parent = field_match->source_tree->parent;

        if (parent && parent->finfo && (offset < parent->finfo->start)) {
            return offset + parent->finfo->start;
        }
        else {
            return offset;
        }
    }
    else {
        return 0;
    }
}

// Print field summary (length, offset, encoding, bitmask, etc)
uint8_t packet_field_summary(uint8_t *raw_packet, uint32_t packet_length, const char *field_name)
{
    // Register field by its string name at cached index 0
    if (!packet_register_set_field(field_name, 0)) { // Register and set target field to index 0
        printf("Failed to register field \"%s\". Check if field syntax is correct!\n", field_name);
        return FALSE;
    }
    // Dissect packet
    packet_dissect(raw_packet, packet_length);
    // Print Summary (Info column on Wireshark)
    printf("Summary: %s\n", packet_summary());
    // Search and get fields info array by cached index 0
    GPtrArray *fields = packet_read_fields(0);
    // Printf info on all fields with the same name within the packet
    if (fields) {
        printf("[ Field: %s ]\n", field_name);
        for (int i = 0; i < fields->len; i++) {
            uint64_t mask, offset, length;
            field_info *field = packet_read_field_at(fields, i);
            printf("----> Match %d\n", i);
            printf("Value: %s\n", packet_read_field_string(field));
            printf("Value Display: %s\n", packet_read_field_display_name(field));
            printf("Byte Encoding: %s\n", packet_read_field_encoding_name(field));
            printf("Offset: %ld\n", offset = packet_read_field_offset(field));
            printf("Byte Length: %ld\n", length = packet_read_field_size(field));
            printf("Type: %s (%d)\n", packet_read_field_type_name(field), packet_read_field_type(field));
            printf("Bitmask: 0x%04lx\n", mask = packet_read_field_bitmask(field));
            printf("Bitmask Offset: %d\n", packet_read_field_bitmask_offset(mask));
            printf("Bitmask Length: %d\n", packet_read_field_size_bits(mask));
            if (length) {
                printf("Raw bytes: ");
                for (size_t i = offset; i < offset + length; i++) {
                    printf("%02X", raw_packet[i]);
                }
                printf("\n");
            }

            printf("Raw bits offset: %d\n", FI_GET_BITS_OFFSET(field));
            printf("Raw bits size: %d\n", FI_GET_BITS_SIZE(field));
        }
        return TRUE;
    }
    return FALSE;
}

// main is not declared when building this code as static library
#ifndef WDISSECTOR_STATIC_LIB
int main(int argc, char **argv)
{
    packet_benchmark(argc > 1 && !strcmp(argv[1], "-b"));

    wdissector_init("proto:radiotap"); // Initialize library

    // wdissector_enable_fast_full_dissection(1); // Enable dissection of all fields without string conversion
    wdissector_enable_full_dissection(1);

    // wdissector_init(NULL); // initialize with default wireshark dissector encap:1 (ethernet)
    // packet_set_protocol("proto:udp"); // use offset 34 for udp (heuristics - slower)

    packet_set_protocol("proto:mac-lte-framed"); // use offset 49 for mac-lte-framed (no udp)

    // exit(0);

    // printf("------- Packet Details -----------------\n");
    // uint8_t packet_mac_rrc_nas[] = {0x0, 0x0, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
    //                                 0xb0, 0x0, 0x3a, 0x1, 0x0, 0xc0, 0xca, 0xac,
    //                                 0xf1, 0x9b, 0xa4, 0x50, 0x46, 0x59, 0xc, 0x91,
    //                                 0x0, 0xc0, 0xca, 0xac, 0xf1, 0x9b, 0xc0, 0xd2,
    //                                 0x0, 0x0, 0x1, 0x0, 0x0, 0x0};

    // packet_set_direction(P2P_DIR_RECV);
    // packet_dissect(packet_mac_rrc_nas, sizeof(packet_mac_rrc_nas));
    // printf("Summary: %s\n", packet_summary());
    // printf("Layers count: %d\n", packet_dissectors_count());
    // printf("Layers: [%s]\n", packet_dissectors());
    // printf("Layer 1 name: %s\n", packet_dissector(1));
    // // puts(packet_show());
    // packet_navigate(1, 0, test_navigate_callback);

    // printf("\n------- Packet Filtering ---------------\n");
    // // Filtering by string (Slow)
    // printf("Packet has field \"lte-rrc.rrc_TransactionIdentifier\": %s\n", (packet_get_field_exists("lte-rrc.rrc_TransactionIdentifier") > 0 ? "yes" : "no"));
    // printf("Field value: %s\n", packet_get_field_string("lte-rrc.rrc_TransactionIdentifier"));
    // printf("Filter \"lte-rrc.rrc_TransactionIdentifier==1 && lte-rrc.sr_ConfigIndex==5\": %d\n", packet_has_condition("rlc-lte.am.fixed.sn"));
    // // Filtering by registering the filter first (Faster)
    // packet_register_condition("lte-rrc.rrc_TransactionIdentifier==1 && lte-rrc.sr_ConfigIndex==5", 0);
    // packet_set_condition(0);

    // VALIDATE: RRC 1
    packet_set_protocol("proto:mac-lte-framed"); // use offset 49 for mac-lte-framed (no udp)
    packet_dissect(DEMO_PKT_RRC_CONNECTION_SETUP + 49, sizeof(DEMO_PKT_RRC_CONNECTION_SETUP) - 49);
    packet_navigate(1, 0, test_navigate_callback);
    puts("----------------------------------------\n\n\n\n\n\n\n\n\n");

    // int filter_passes = packet_read_condition(0);
    // printf("Filter (cached) valid: %d\n", filter_passes);

    // printf("\n------- API DEMO (By field name - slower) --------\n");
    // const char *test_field = "lte-rrc.logicalChannelGroup";
    // printf("Offset of %s: %d\n", test_field, packet_get_field_offset(test_field));
    // printf("Value of %s: %s\n", test_field, packet_get_field_string(test_field));
    // printf("Length of %s: %d\n", test_field, packet_get_field_size(test_field));
    // printf("Type of %s: %d (%s)\n", test_field, packet_get_field_type(test_field), packet_get_field_type_name(test_field));
    // printf("Bitmask of %s: 0x%04lx\n", test_field, packet_get_field_bitmask(test_field));
    // printf("Encoding of %s: %s\n", test_field, packet_get_field_encoding_name(test_field));
    // printf("Direction: %d\n", packet_direction());

    // printf("\n------- API DEMO (By field index - faster) --------\n"); // Faster as redissection is not needed
    // packet_field_summary(DEMO_PKT_RRC_CONNECTION_SETUP + 49, sizeof(DEMO_PKT_RRC_CONNECTION_SETUP) - 49, test_field);

    // // See packet_field_summary to handle multiple "equal" fields within a packet
    // printf("\n------- API DEMO (NAS-EPS-FRAMED) --------\n");

    // VALIDATE: NAS 1
    // packet_set_protocol("proto:nas-eps"); // Change protocol to NAS EPS (framed does not need udp)
    // packet_dissect(DEMO_PKT_NAS_ATTACH_REQUEST, sizeof(DEMO_PKT_NAS_ATTACH_REQUEST));
    // packet_navigate(1, 0, test_navigate_callback);
    // puts("----------------------------------------\n\n\n\n\n\n\n\n\n");

    // const char *test_nas_field1 = "nas_eps.security_header_type";
    // printf("1) Dissecting Field: %s\n", test_nas_field1);

    // packet_field_summary(DEMO_PKT_NAS_ATTACH_REQUEST, sizeof(DEMO_PKT_NAS_ATTACH_REQUEST), test_nas_field1);
    // printf("\n");
    // const char *test_nas_field2 = "nas_eps.emm.nas_key_set_id";
    // printf("2) Dissecting Field: %s\n", test_nas_field2);
    // packet_field_summary(DEMO_PKT_NAS_ATTACH_REQUEST, sizeof(DEMO_PKT_NAS_ATTACH_REQUEST), test_nas_field2);

    // printf("\n------- API DEMO (BTACL) --------\n");
    packet_set_protocol("encap:BLUETOOTH_HCI_H4"); // Bluetooth ACL
    // const char *test_bt_field = "btbbd.type";
    // printf("1) Dissecting Field: %s\n", test_bt_field);

    uint8_t packet_lmp[] = {0x9, 0x76, 0x28, 0x0, 0x0, 0x2b, 0x0, 0x99, 0x1,
                            0x4f, 0x0, 0x50, 0xff, 0xff, 0x8f, 0xfe, 0xdb,
                            0xff, 0x5b, 0x87, 0x49};

    // VALIDATE: Bluetooth 1
    // packet_dissect(packet_lmp, sizeof(packet_lmp));
    // packet_set_direction(1); // RX Packet
    // // // packet_field_summary(packet, sizeof(packet), test_bt_field);
    // // puts("\n\n\n\n\n\n\n\n\n----------------------------------------");
    // packet_navigate(2, 1, test_navigate_callback);

    // {
    //     uint8_t packet[] = {0x9, 0x7d, 0x52, 0x0, 0x0, 0x2d, 0xe, 0xa6, 0x2,
    //                         0x66, 0x0, 0x8, 0x0, 0x1, 0x0, 0x2, 0x4,
    //                         0x4, 0x0, 0x1, 0x0, 0x4f, 0x0, 0x7b};

    //     packet_set_direction(1);
    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());
    // }

    // {
    //     uint8_t packet[] = {0x9, 0xa9, 0x52, 0x0, 0x0, 0x0, 0x12, 0x1e, 0x0,
    //                         0x86, 0x0, 0xc, 0x0, 0x1, 0x0, 0x3, 0x4,
    //                         0x8, 0x0, 0x41, 0x0, 0x4f, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x5d};

    //     packet_set_direction(0);
    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());
    // }

    // {
    //     uint8_t packet[] = {0x9, 0xaa, 0x52, 0x0, 0x0, 0x0, 0x12, 0x1e, 0x0,
    //                         0x86, 0x0, 0xc, 0x0, 0x1, 0x0, 0x4, 0x2,
    //                         0x8, 0x0, 0x4f, 0x0, 0x0, 0x0, 0x1, 0x2,
    //                         0x9b, 0x6, 0xc0};

    //     packet_set_direction(0);
    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());
    // }

    // {
    //     uint8_t packet[] = {0x9, 0xaf, 0x52, 0x0, 0x0, 0x34, 0x2, 0xa6, 0x3,
    //                         0x86, 0x0, 0xc, 0x0, 0x1, 0x0, 0x4, 0x5,
    //                         0x8, 0x0, 0x41, 0x0, 0x0, 0x0, 0x1, 0x2,
    //                         0xa0, 0x2, 0x6a};
    //     packet_set_direction(1);
    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());
    // }

    // {
    //     uint8_t packet[] = {0x9, 0xb1, 0x52, 0x0, 0x0, 0x31, 0x2, 0xa6, 0x1,
    //                         0x76, 0x0, 0xa, 0x0, 0x1, 0x0, 0x5, 0x2,
    //                         0x6, 0x0, 0x41, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0xac};

    //     packet_set_direction(1);
    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());
    // }

    // {
    //     uint8_t packet[] = {0x9, 0xb1, 0x52, 0x0, 0x0, 0x0, 0x12, 0x26, 0x0,
    //                         0x96, 0x0, 0xe, 0x0, 0x1, 0x0, 0x5, 0x5,
    //                         0xa, 0x0, 0x4f, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x1, 0x2, 0xa0, 0x2, 0xe8};

    //     packet_set_direction(0);
    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());
    // }

    // {
    //     uint8_t packet[] = {0x9, 0xb9, 0x52, 0x0, 0x0, 0x41, 0x2, 0xc6, 0x3,
    //                         0xc6, 0x0, 0x14, 0x0, 0x41, 0x0, 0x6, 0x0,
    //                         0x0, 0x0, 0xf, 0x35, 0x3, 0x19, 0x12, 0x0,
    //                         0x2, 0x90, 0x35, 0x5, 0xa, 0x0, 0x0, 0xff,
    //                         0xff, 0x0, 0x7e};

    //     packet_set_direction(1);

    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());
    // }

    // packet_navigate(2, 1, test_navigate_callback);

    // {
    //     uint8_t packet[] = {0x04, 0x2f, 0xff, 0x1, 0x97, 0x0, 0x9e, 0x74, 0x77, 0xfc,
    //                         0x1, 0x0, 0x4, 0x1, 0x2a, 0x23, 0x30, 0xc7,
    //                         0x10, 0x9, 0x44, 0x45, 0x53, 0x4b, 0x54, 0x4f,
    //                         0x50, 0x2d, 0x51, 0x33, 0x50, 0x32, 0x31, 0x4d,
    //                         0x53, 0x2, 0xa, 0xc, 0xd, 0x3, 0xc, 0x11,
    //                         0xa, 0x11, 0x1f, 0x11, 0xe, 0x11, 0xb, 0x11,
    //                         0x1e, 0x11, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                         0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    //     // Change protocol to FULL hci
    //     packet_set_protocol("encap:BLUETOOTH_HCI_H4");
    //     packet_dissect(packet, sizeof(packet));
    //     puts(packet_summary());

    //     printf("----- GET Addresse here -----\n");
    //     printf("BDAddress: %s\n", packet_get_field_string("bthci_evt.bd_addr"));
    //     printf("Name: %s\n", packet_get_field_string("btcommon.eir_ad.entry.device_name"));
    //     packet_set_protocol("proto:hci_h4"); // Change back to normal hci_h4
    // }

    // printf("Layers count: %d\n", packet_dissectors_count());
    // printf("Layers: [%s]\n", packet_dissectors());

    // uint8_t gnb_rar_packet[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                             0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    //                             0x39, 0x2d, 0x5f, 0x40, 0x0, 0x40, 0x11, 0xf,
    //                             0x53, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    //                             0x1, 0xe7, 0xa6, 0x27, 0xf, 0x0, 0x25, 0xfe,
    //                             0x38, 0x6d, 0x61, 0x63, 0x2d, 0x6e, 0x72, 0x2,
    //                             0x1, 0x2, 0x2, 0x0, 0x7f, 0x3, 0x0, 0x0,
    //                             0x7, 0x0, 0xc0, 0x0, 0x7, 0x1, 0x7f, 0x0,
    //                             0x18, 0x10, 0x82, 0x6, 0x2e, 0xfb};

    uint8_t gnb_rrc_setup_request[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
                                       0x3d, 0x2f, 0xc4, 0x40, 0x0, 0x40, 0x11, 0xc,
                                       0xea, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
                                       0x1, 0xca, 0x94, 0x27, 0xf, 0x0, 0x29, 0xfe,
                                       0x3c, 0x6d, 0x61, 0x63, 0x2d, 0x6e, 0x72, 0x2,
                                       0x0, 0x3, 0x2, 0xb1, 0x3b, 0x3, 0x0, 0x1,
                                       0x7, 0x2, 0xdc, 0x0, 0x11, 0x1, 0x34, 0x14,
                                       0x53, 0xd0, 0x64, 0x93, 0xa6, 0x3f, 0x21, 0x21,
                                       0x21, 0x21};

    // uint8_t gnb_uplink_packet[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                                0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    //                                0x3d, 0x2d, 0x62, 0x40, 0x0, 0x40, 0x11, 0xf,
    //                                0x4c, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    //                                0x1, 0xe7, 0xa6, 0x27, 0xf, 0x0, 0x29, 0xfe,
    //                                0x3c, 0x6d, 0x61, 0x63, 0x2d, 0x6e, 0x72, 0x2,
    //                                0x0, 0x3, 0x2, 0x2e, 0xfb, 0x3, 0x0, 0x1,
    //                                0x7, 0x0, 0xc0, 0x0, 0x11, 0x1, 0x3e, 0x1,
    //                                0x0, 0x39, 0x34, 0x32, 0x3f, 0x21, 0x21, 0x21,
    //                                0x21, 0x21};

    // uint8_t gnb_downlink_packet[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                                  0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    //                                  0x47, 0x2d, 0x9a, 0x40, 0x0, 0x40, 0x11, 0xf,
    //                                  0xa, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    //                                  0x1, 0xe7, 0xa6, 0x27, 0xf, 0x0, 0x33, 0xfe,
    //                                  0x46, 0x6d, 0x61, 0x63, 0x2d, 0x6e, 0x72, 0x2,
    //                                  0x1, 0x3, 0x2, 0x2e, 0xfb, 0x3, 0x0, 0x1,
    //                                  0x7, 0x0, 0xca, 0x0, 0x1, 0x1, 0x3d, 0x1c,
    //                                  0x3f, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                                  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                                  0x0, 0x0, 0x0, 0x0};

    // uint8_t gnb_uplink_rlc_packet[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                                    0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    //                                    0x81, 0x2f, 0xc7, 0x40, 0x0, 0x40, 0x11, 0xc,
    //                                    0xa3, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    //                                    0x1, 0xca, 0x94, 0x27, 0xf, 0x0, 0x6d, 0xfe,
    //                                    0x80, 0x6d, 0x61, 0x63, 0x2d, 0x6e, 0x72, 0x2,
    //                                    0x0, 0x3, 0x2, 0xb1, 0x3b, 0x3, 0x0, 0x1,
    //                                    0x7, 0x2, 0xf7, 0x0, 0x9, 0x1, 0x1, 0x23,
    //                                    0xc0, 0x0, 0x0, 0x0, 0x12, 0x0, 0x5, 0xdf,
    //                                    0x80, 0x10, 0x5e, 0x40, 0x3, 0x40, 0x40, 0xbe,
    //                                    0x16, 0x7c, 0x3f, 0xc0, 0x0, 0x0, 0x0, 0x0,
    //                                    0x0, 0x4, 0xcb, 0x80, 0xbc, 0x1c, 0x0, 0x0,
    //                                    0x0, 0x0, 0x0, 0x3d, 0x0, 0x39, 0x3f, 0x35,
    //                                    0x3f, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
    //                                    0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
    //                                    0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
    //                                    0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
    //                                    0x21, 0x21, 0x21, 0x21, 0x21, 0x21};

    // uint8_t gnb_downlink_rlc_packet[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //                                      0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    //                                      0x38, 0x2f, 0xc8, 0x40, 0x0, 0x40, 0x11, 0xc,
    //                                      0xeb, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    //                                      0x1, 0xca, 0x94, 0x27, 0xf, 0x0, 0x24, 0xfe,
    //                                      0x37, 0x6d, 0x61, 0x63, 0x2d, 0x6e, 0x72, 0x2,
    //                                      0x1, 0x3, 0x2, 0xb1, 0x3b, 0x3, 0x0, 0x1,
    //                                      0x7, 0x2, 0xf7, 0x0, 0x11, 0x1, 0x41, 0x0,
    //                                      0x3, 0x0, 0x1, 0x0, 0x3f};

    // printf("\n------- API DEMO (5G MAC-NR) --------\n");
    // packet_set_protocol("proto:mac-lte-framed");

    // packet_dissect(gnb_rar_packet + 48, sizeof(gnb_rar_packet) - 48);
    // puts(packet_summary());
    // puts("-------------------------------------");

    // packet_dissect(gnb_rrc_setup_request + 48, sizeof(gnb_rrc_setup_request) - 48);
    // puts(packet_summary());
    // // packet_navigate(1, 0, test_navigate_callback);
    // puts("-------------------------------------");

    // packet_dissect(gnb_uplink_packet + 48, sizeof(gnb_uplink_packet) - 48);
    // puts(packet_summary());
    // puts("-------------------------------------");

    // uint8_t packet_rrc_test[128] = {
    //     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //     0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    //     0x56, 0xde, 0x35, 0x40, 0x0, 0x40, 0x11, 0x5e,
    //     0x5f, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    //     0x1, 0xc9, 0xb6, 0x27, 0xf, 0x0, 0x42, 0xfe,
    //     0x55, 0x6d, 0x61, 0x63, 0x2d, 0x6c, 0x74, 0x65,
    //     0x1, 0x1, 0x3, 0x2, 0x28, 0xc3, 0x4, 0xc,
    //     0x96, 0x1, 0x3c, 0x20, 0x1d, 0x1f, 0x41, 0xa1,
    //     0xeb, 0x6e, 0x2c, 0x88, 0x68, 0x12, 0x98, 0xf,
    //     0x1c, 0xce, 0x1, 0x83, 0x80, 0xba, 0x30, 0x79,
    //     0x43, 0xfb, 0x80, 0x4, 0x23, 0x80, 0x89, 0x1a,
    //     0x2, 0x44, 0x68, 0xd, 0x90, 0x10, 0x8e, 0xa,
    //     0x0, 0x0, 0x0};
    // uint8_t packet_rrc_test[] = {
    //     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    //     0x0, 0x0, 0x0, 0x8, 0x0, 0x45, 0x0, 0x0,
    //     0x56, 0xde, 0x35, 0x40, 0x0, 0x40, 0x11, 0x5e,
    //     0x5f, 0x7f, 0x0, 0x0, 0x1, 0x7f, 0x0, 0x0,
    //     0x1, 0xc9, 0xb6, 0x27, 0xf, 0x0, 0x42, 0xfe,
    //     0x55, 0x6d, 0x61, 0x63, 0x2d, 0x6c, 0x74, 0x65,
    //     0x1, 0x1, 0x3, 0x2, 0x28, 0xc3, 0x4, 0xc,
    //     0x96, 0x1, 0x3c, 0x20, 0x1d, 0x1f, 0x41, 0xa1,
    //     0xeb, 0x6e, 0x2c, 0x88,
    //     0x60, 0x12, 0x9b, 0x2e, 0x66, 0x1e, 0x82, 0xf2, 0xe0, 0xcc, 0xc8, 0x60, 0xd3,
    //     0x00, 0x00, 0x99, 0x02, 0x00, 0x03, 0xe0};

    // uint8_t packet_rrc_test2[] = {
    //     0x60, 0x12, 0x9b, 0x2e, 0x66, 0x1e, 0x82, 0xf2, 0xe0, 0xcc, 0xc8, 0x60, 0xd3,
    //     0x00, 0x00, 0x99, 0x02, 0x00, 0x03, 0xe0};

    // uint8_t packet_rrc_test3[] = {
    //     0x60, 0x12, 0x9b, 0x2e, 0x66, 0x1e, 0x82, 0xf2, 0xe0,
    //     0xcc, 0xc8, 0x60, 0xd3, 0x0, 0x0, 0x99, 0x2,
    //     0x0, 0x3, 0xe0};

    // uint8_t packet_rrc_test3[] = {
    //     0x8, 0x0, 0x69, 0xbb, 0xda, 0x15, 0x57, 0xa0, 0x0,
    //     0x3a, 0xe8, 0x8, 0x0, 0x17, 0x83, 0x80};

    // uint8_t packet_rrc_test3[] = {
    //     0x20, 0x16, 0x10, 0x80, 0x0, 0x0, 0x6, 0x8b, 0x2,
    //     0x80, 0x12, 0x89, 0xee, 0xb4, 0x4d, 0x57, 0x0,
    //     0x41, 0xd0, 0x80, 0x4f, 0x81, 0x80, 0x0, 0x3c,
    //     0x44, 0x0, 0x1, 0xc0, 0x7, 0x54, 0x80, 0x70,
    //     0x40, 0x41, 0xc1, 0xc1, 0x9c, 0xdc, 0x9c, 0xd8,
    //     0x5c, 0x1b, 0x81, 0x40, 0x6b, 0x4, 0x0, 0x0,
    //     0x89, 0xc2, 0x20, 0x0, 0x3, 0x41, 0x2, 0x2,
    //     0x2, 0x2, 0x14, 0x2, 0xfd, 0x80, 0x3c, 0x44,
    //     0x0, 0x0, 0x46, 0xae, 0x58, 0xf1, 0xa5, 0x84,
    //     0xc0, 0x3c, 0x44, 0x0, 0x0, 0x48, 0xc1, 0x7d,
    //     0x2e, 0x58, 0xf1, 0xa5, 0x98, 0x9f, 0x7, 0xd4,
    //     0xb, 0xe3, 0xa4, 0x3c, 0x73, 0x3c, 0xb8, 0x33,
    //     0x32, 0x18, 0x34, 0xc0, 0x0, 0x26, 0x40, 0x80,
    //     0x0, 0xf8};

    uint8_t packet_rrc_test3[] = {
        0x22, 0x2, 0x1, 0x79, 0x70, 0x66, 0x64, 0x30, 0x69,
        0x80, 0x0, 0x50, 0x81, 0x0, 0x2, 0x30};

    uint8_t packet[] = {0x9, 0x7d, 0x52, 0x0, 0x0, 0x2d, 0xe, 0xa6, 0x2,
                        0x66, 0x0, 0x8, 0x0, 0x1, 0x0, 0x2, 0x4,
                        0x4, 0x0, 0x1, 0x0, 0x4f, 0x0, 0x7b};

    uint8_t packet_lte_mac[] = {
        0x01, 0x01, 0x03, 0x02, 0x00, 0x47, 0x04, 0x05, 0x37, 0x01, 0x30, 0x20, 0x14, 0x1f, 0x41,
        0xa0, 0x30, 0xf8, 0xf6, 0x28, 0x60, 0x12, 0x9b, 0x2e, 0x66, 0x1e, 0x82, 0xf2, 0xe0, 0xcc,
        0xC8, 0x60, 0xd3, 0x00, 0x00, 0xa1, 0x02, 0x00, 0x04, 0x60, 0x00, 0x00};

    // Initialize wdissector instances. Check for NULL to handle errors
    // Can initialize a maximum of 16 (TODO: add support for more encapsulations)

    // puts(wd_version_info());
    // printf("Wireshark Profile: %s\n", wd_profile_info());

    // create semaphore
    // sem_t sem;
    // sem_init(&sem, 0, 0);

    // wd_t *wd3;

    // // Dissect first protocol (LTE RRC)
    // std::thread t1([&]() {
    //     puts("t1");
    //     wd_t *wd1 = wd_init("proto:lte_rrc.dl_dcch");
    //     // puts("t2");
    //     // wd_t *wd3 = wd_init("encap:BLUETOOTH_HCI_H4");
    //     sem_wait(&sem);
    //     for (size_t i = 0; i < 1 * 1e6; i++) {
    //         wd_packet_dissect(wd1, packet_rrc_test3, sizeof(packet_rrc_test3));
    //         printf("Summary: %s\n", wd_packet_summary(wd1));
    //         // usleep(1000);
    //     }
    //     sem_post(&sem);
    // });

    // // Dissect first protocol (LTE RRC)
    // std::thread t2([&]() {
    //     puts("t2");
    //     wd3 = wd_init("encap:BLUETOOTH_HCI_H4");
    //     sem_wait(&sem);
    //     for (size_t i = 0; i < 1 * 1e6; i++) {
    //         wd_set_packet_direction(wd3, P2P_DIR_RECV);
    //         wd_packet_dissect(wd3, packet, sizeof(packet));
    //         printf("Summary: %s\n", wd_packet_summary(wd3));
    //         // wd_reset(wd3);
    //         // wd_reset_all();
    //         // usleep(1000);
    //     }
    //     sem_post(&sem);
    // });

    // // Dissect second protocol (Bluetooth)
    // std::thread t3([&]() {
    //     puts("t3");
    //     wd_t *wd2 = wd_init("proto:lte_rrc.dl_dcch");

    //     sem_wait(&sem);
    //     for (size_t i = 0; i < 1 * 1e6; i++) {
    //         wd_packet_dissect(wd2, packet_rrc_test3, sizeof(packet_rrc_test3));
    //         printf("Summary: %s\n", wd_packet_summary(wd2));
    //         // usleep(1000);
    //     }
    //     sem_post(&sem);
    // });

    // sleep(1);
    // sem_post(&sem);
    // sem_post(&sem);
    // sem_post(&sem);
    // sleep(5);

    // sem_wait(&sem);
    // sem_wait(&sem);
    // sem_wait(&sem);

    // wd_packet_dissect(wd3, packet_lte_mac, sizeof(packet_lte_mac));
    // printf("Summary: %s\n", wd_packet_summary(wd3));

    // packet_set_protocol("proto:lte_rrc.dl_dcch");

    //// packet_dissect(packet_rrc_test + 49, sizeof(packet_rrc_test) - 49);
    // packet_dissect(packet_rrc_test3, sizeof(packet_rrc_test3));
    // printf("Summary: %s\n", packet_summary());
    // printf("Layers count: %d\n", packet_dissectors_count());
    // printf("Layers: [%s]\n", packet_dissectors());
    // printf("Layer 1 name: %s\n", packet_dissector(0));

    // puts(packet_show());
    // packet_navigate(1, 0, test_navigate_callback);
    // packet_field_summary(packet_rrc_test + 49, sizeof(packet_rrc_test) - 49, "lte-rrc.widebandCQI_element");
    // packet_field_summary(packet_rrc_test2, sizeof(packet_rrc_test2), "lte-rrc.widebandCQI_element");
    // packet_field_summary(packet_rrc_test3, sizeof(packet_rrc_test3), "lte-rrc.ue_TransmitAntennaSelection");

    // puts(packet_show_pdml());

    // uint8_t packet_mac_test[] = {
    //     0x01, 0x01, 0x03, 0x02, 0x00, 0x47, 0x04, 0x05, 0x37, 0x01, 0x30, 0x20, 0x14, 0x1f, 0x41,
    //     0xa0, 0x30, 0xf8, 0xf6, 0x28, 0x60, 0x12, 0x9b, 0x2e, 0x66, 0x1e, 0x82, 0xf2, 0xe0, 0xcc,
    //     0xC8, 0x60, 0xd3, 0x00, 0x00, 0xa1, 0x02, 0x00, 0x04, 0x60, 0x00, 0x00};

    // packet_set_protocol("proto:mac-lte-framed");
    // packet_dissect(packet_mac_test, sizeof(packet_mac_test));
    // printf("Summary: %s\n", packet_summary());
    // printf("Layers count: %d\n", packet_dissectors_count());
    // printf("Layers: [%s]\n", packet_dissectors());
    // printf("Layer 1 name: %s\n", packet_dissector(1));

    // packet_navigate(1, 0, test_navigate_callback);

    // printf("\n\n");
    // packet_field_summary(packet_mac_test, sizeof(packet_mac_test), "mac-lte.dlsch.header");
    // packet_field_summary(packet_mac_test, sizeof(packet_mac_test), "mac-lte.sch.format2558");
    // packet_field_summary(packet_mac_test, sizeof(packet_mac_test), "mac-lte.sch.format2558");
    // packet_field_summary(packet_mac_test, sizeof(packet_mac_test), "mac-lte.sch.format2");

    // puts("-------------------------------------");

    // packet_dissect(gnb_uplink_rlc_packet + 48, sizeof(gnb_uplink_rlc_packet) - 48);
    // puts(packet_summary());
    // puts("-------------------------------------");

    // packet_dissect(gnb_downlink_rlc_packet + 48, sizeof(gnb_downlink_rlc_packet) - 48);
    // puts(packet_summary());
    // puts("-------------------------------------");

    // printf("\nLayers count: %d\n", packet_dissectors_count());
    // printf("Layers: [%s]\n", packet_dissectors());

    // VALIDATE: Wi-Fi 802.11
    packet_set_protocol("proto:radiotap");
    uint8_t PKT_WIFI_BEACON_FRAME[] = {0x0, 0x0, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x80, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff,
                                       0xff, 0xff, 0x0, 0xc0, 0xca, 0xac, 0xf1, 0x9b,
                                       0x0, 0xc0, 0xca, 0xac, 0xf1, 0x9b, 0x0, 0x0,
                                       0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x64, 0x0, 0x11, 0x4, 0x0, 0x8, 0x54, 0x45,
                                       0x53, 0x54, 0x5f, 0x4b, 0x52, 0x41, 0x1, 0x8,
                                       0x82, 0x84, 0x8b, 0x96, 0xc, 0x12, 0x18, 0x24,
                                       0x3, 0x1, 0x9, 0x5, 0x4, 0x0, 0x1, 0x0,
                                       0x0, 0x7, 0xa, 0x55, 0x53, 0x20, 0x1, 0xb,
                                       0x1e, 0xc, 0x3, 0x0, 0x0, 0x2a, 0x1, 0x4,
                                       0x32, 0x4, 0x30, 0x48, 0x60, 0x6c, 0x30, 0x14,
                                       0x1, 0x0, 0x0, 0xf, 0xac, 0x4, 0x1, 0x0,
                                       0x0, 0xf, 0xac, 0x4, 0x1, 0x0, 0x0, 0xf,
                                       0xac, 0x8, 0xc, 0x0, 0x2d, 0x1a, 0x2c, 0x0,
                                       0x1f, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x0, 0x0, 0x0, 0x2c, 0x1, 0x1, 0x0, 0x0,
                                       0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x3d, 0x16, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0xc3, 0x2, 0x0, 0x36, 0xdd, 0x18, 0x0, 0x50,
                                       0xf2, 0x2, 0x1, 0x1, 0x81, 0x0, 0x3, 0xa4,
                                       0x0, 0x0, 0x27, 0xa4, 0x0, 0x0, 0x42, 0x43,
                                       0x5e, 0x0, 0x62, 0x32, 0x2f, 0x0, 0xdd, 0x7,
                                       0x0, 0xe0, 0x4c, 0x0, 0x2, 0x0, 0x4};
    packet_dissect(PKT_WIFI_BEACON_FRAME, sizeof(PKT_WIFI_BEACON_FRAME));
    packet_navigate(2, 0, test_navigate_callback);
    puts("----------------------------------------\n\n\n\n\n\n\n\n\n");

    // VALIDATE: BLE Link Layer
    packet_set_protocol("proto:nordic_ble");
    uint8_t PKT_BLE_CONN_REQ[] = {0x3, 0x35, 0x0, 0x2, 0xa0, 0x29, 0x6, 0xa, 0x1,
                                  0x27, 0x39, 0x0, 0x0, 0x96, 0x0, 0x0, 0x0,
                                  0xd6, 0xbe, 0x89, 0x8e, 0x45, 0x22, 0x55, 0x92,
                                  0x87, 0xb4, 0x4b, 0x42, 0x6e, 0x33, 0x4c, 0x5,
                                  0x61, 0x3c, 0x93, 0xaa, 0x9a, 0xaf, 0xf2, 0x42,
                                  0xe6, 0x3, 0x9, 0x0, 0x27, 0x0, 0x0, 0x0,
                                  0xf4, 0x1, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x29,
                                  0x5a, 0xa2, 0x2d};

    packet_dissect(PKT_BLE_CONN_REQ, sizeof(PKT_BLE_CONN_REQ));
    packet_navigate(3, 0, test_navigate_callback);

    puts("----------------------------------------");
    printf("Field Errors=%d\n", count_field_errors);
    printf("Unchecked Fields=%d\n", count_field_errors);
    puts("----------------------------------------");

    // wd_field_t fd1 = wd_field("rlc-nr.am.dc");
    // const char *s = packet_read_value_to_string(0, fd1);
    // if (s)
    //     puts(s);

    return 0;
}
#endif
