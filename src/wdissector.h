/*
** WDissector Project, 2020-2022
*/

// TODO: Remove all internal index functions, only allow string (slow) or manual (fast) field operations

#pragma once
#ifndef __WDISSECTOR
#define __WDISSECTOR


// Public Defines
#define WD_TYPE_FIELD 0
#define WD_TYPE_GROUP 1
#define WD_TYPE_LAYER 2

#define WD_DIR_TX 0
#define WD_DIR_RX 1
#define WD_DIR_DL 0
#define WD_DIR_UL 1
#define WD_DIR_SENT 0
#define WD_DIR_RECV 1
#define WD_DIR_ANY 3 // Special case

#define WD_0 0
#define WD_1 1
#define WD_2 2
#define WD_3 3
#define WD_4 4
#define WD_5 5
#define WD_6 6
#define WD_7 7
#define WD_8 8
#define WD_9 9
#define WD_10 10
#define WD_11 11
#define WD_12 12
#define WD_13 13
#define WD_14 14
#define WD_15 15

#ifndef GEN_PY_MODULE
// Common includes
#include <inttypes.h>
#ifdef __cplusplus
#include <functional>
#endif
// External libs include
#include <glib.h> // glib 2.0
// Wireshark include
#include <config.h>
#include <epan/epan_dissect.h>
#include <epan/ftypes/ftypes-int.h>
#include <epan/proto.h>
#include <epan/tvbuff-int.h>

#else
// Add appropriate types for python module generation
#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long
#define int64_t signed long
#define int32_t signed int
#define gboolean unsigned char
#define gchar char
#define gint int
#define guint8 unsigned char
#define guint32 unsigned int
#define guchar unsigned char
#define guint unsigned int
#define guint64 unsigned long
#define field_info void
#define header_field_info void
#define gpointer void
#define epan_dissect_t void

/* field types */
enum ftenum {
    FT_NONE, /* used for text labels with no value */
    FT_PROTOCOL,
    FT_BOOLEAN, /* TRUE and FALSE come from <glib.h> */
    FT_CHAR,    /* 1-octet character as 0-255 */
    FT_UINT8,
    FT_UINT16,
    FT_UINT24, /* really a UINT32, but displayed as 6 hex-digits if FD_HEX*/
    FT_UINT32,
    FT_UINT40, /* really a UINT64, but displayed as 10 hex-digits if FD_HEX*/
    FT_UINT48, /* really a UINT64, but displayed as 12 hex-digits if FD_HEX*/
    FT_UINT56, /* really a UINT64, but displayed as 14 hex-digits if FD_HEX*/
    FT_UINT64,
    FT_INT8,
    FT_INT16,
    FT_INT24, /* same as for UINT24 */
    FT_INT32,
    FT_INT40, /* same as for UINT40 */
    FT_INT48, /* same as for UINT48 */
    FT_INT56, /* same as for UINT56 */
    FT_INT64,
    FT_IEEE_11073_SFLOAT,
    FT_IEEE_11073_FLOAT,
    FT_FLOAT,
    FT_DOUBLE,
    FT_ABSOLUTE_TIME,
    FT_RELATIVE_TIME,
    FT_STRING,
    FT_STRINGZ,     /* for use with proto_tree_add_item() */
    FT_UINT_STRING, /* for use with proto_tree_add_item() */
    FT_ETHER,
    FT_BYTES,
    FT_UINT_BYTES,
    FT_IPv4,
    FT_IPv6,
    FT_IPXNET,
    FT_FRAMENUM, /* a UINT32, but if selected lets you go to frame with that number */
    FT_PCRE,     /* a compiled Perl-Compatible Regular Expression object */
    FT_GUID,     /* GUID, UUID */
    FT_OID,      /* OBJECT IDENTIFIER */
    FT_EUI64,
    FT_AX25,
    FT_VINES,
    FT_REL_OID, /* RELATIVE-OID */
    FT_SYSTEM_ID,
    FT_STRINGZPAD, /* for use with proto_tree_add_item() */
    FT_FCWWN,
    FT_NUM_TYPES /* last item number plus one */
};

typedef struct _GPtrArray {
    gpointer *pdata;
    guint len;
} GPtrArray;

typedef struct _GByteArray {
    guint8 *data;
    guint len;
} GByteArray;
typedef struct _proto_node {
    struct _proto_node *first_child;
    struct _proto_node *last_child;
    struct _proto_node *next;
    struct _proto_node *parent;
    void *finfo;
    void *tree_data;
} proto_node;

/** A protocol tree element. */
typedef proto_node proto_tree;

#endif

#ifdef __cplusplus
// C++ macro include
#define WD_PUBLIC extern "C"
#else
#ifndef GEN_PY_MODULE
// C macro include
#define WD_PUBLIC extern
#else
// Python C macro include
#define WD_PUBLIC
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// --- Public Structures ---

enum wd_log_level {
    WD_LOG_LEVEL_NONE,     /* not user facing */
    WD_LOG_LEVEL_NOISY,    /* extra verbose debugging */
    WD_LOG_LEVEL_DEBUG,    /* normal debugging level */
    WD_LOG_LEVEL_INFO,     /* chatty status but not debug */
    WD_LOG_LEVEL_MESSAGE,  /* default level, doesn't show file/function name */
    WD_LOG_LEVEL_WARNING,  /* can be set to fatal */
    WD_LOG_LEVEL_CRITICAL, /* always enabled, can be set to fatal */
    WD_LOG_LEVEL_ERROR,    /* "error" is always fatal (aborts) */
    WD_LOG_LEVEL_ECHO,      /* Always print message, never fatal */
    WD_LOG_LEVEL_LAST
};

enum wd_dissection_mode {
    WD_MODE_NORMAL, // Tree is preserved and all fields are not converted to string
    WD_MODE_FAST,   // Tree is simplified and all fields are not converted to string
    WD_MODE_FULL,   // All dissected value are converted to string, slow but useful for debugging
};

// Test packets (use offset 49 to dissect correctly with mac-lte-framed dissector)
WD_PUBLIC uint8_t DEMO_PKT_RRC_CONNECTION_SETUP[128];
WD_PUBLIC uint8_t DEMO_PKT_RRC_SETUP_COMPLETE[122];
WD_PUBLIC uint8_t DEMO_PKT_RRC_RECONFIGURATION[114];
WD_PUBLIC uint8_t DEMO_PKT_NAS_ATTACH_REQUEST[118];

typedef struct _wd_t wd_t; // Hidden wdissector structure
typedef header_field_info *wd_field_t;
typedef field_info *wd_field_info_t;
typedef struct epan_dfilter *wd_filter_t;

// Public API

// --- Initialization ---
uint8_t wdissector_init(const char *protocol_name);       // Initialize library
void wdissector_set_log_level(enum wd_log_level level);   // Set log level
void wdissector_enable_fast_full_dissection(uint8_t val); // Enable dissection of all fields without string conversion
void wdissector_enable_full_dissection(uint8_t val);

// --- Common dissection functions ---
gboolean packet_set_protocol(const char *lt_arg);
gboolean packet_set_protocol_fast(const char *proto_name);
void packet_dissect(unsigned char *raw_packet, uint32_t packet_length);
void packet_set_direction(int dir);
void packet_cleanup();
void packet_navigate(uint32_t skip_layers, uint32_t skip_groups, uint8_t (*callback)(proto_tree *, uint8_t, uint8_t *)); // Iterates every field of the packet
epan_dissect_t *wdissector_get_edt();

// --- Wireshark version info ---
const char *wdissector_version_info();
const char *wdissector_profile_info();

// --- Filtering related functions ---
// Condition (internal index)
gboolean packet_register_condition(const char *filter, uint16_t condition_index); // Register rule
void packet_set_condition(uint16_t condition_index);                              // Set rule
gboolean packet_read_condition(uint16_t condition_index);                         // Read rule (true or false)
// Raw Filter array
const char *packet_register_filter(const char *filter);
void packet_set_filter(const char *filter);
gboolean packet_read_filter(const char *filter);

// --- Fields related functions (string - slow) ---
header_field_info *packet_register_set_field_hfinfo(const char *field_name);
int packet_get_field_exists(const char *field_name);
gboolean packet_has_condition(const char *filter);
field_info *packet_get_field(const char *field_name);
GPtrArray *packet_get_fields(const char *field_name);
const char *packet_get_field_name(const char *field_name);   // Return field name
const char *packet_get_field_string(const char *field_name); // Return fiels value string
uint32_t packet_get_field_offset(const char *field_name);    // Read field offset
uint32_t packet_get_field_size(const char *field_name);      // Read field size
uint64_t packet_get_field_bitmask(const char *field_name);   // Read field mask (for bitfields)
uint32_t packet_get_field_encoding(const char *field_name);  // Read field encoding (little or big endian)
int packet_get_field_type(const char *field_name);           // Read field type (wireshark equivalent type)
const char *packet_get_field_type_name(const char *field_name);
const char *packet_get_field_encoding_name(const char *field_name);
uint32_t packet_get_field_uint32(const char *field_name);

// --- Fields related functions (field_info - faster) ---
// GET field by header info (header_field_info)
void packet_set_field_hfinfo(header_field_info *hfi);         // Set field (must be called before dissection)
uint8_t packet_set_field_hfinfo_all(header_field_info *hfi);  // Set all fields with same name field (must be called before dissection) and returns number of found fields
int packet_read_field_exists_hfinfo(header_field_info *hfi);  // Check if field with the same hfi exists in packet
field_info *packet_read_field_hfinfo(header_field_info *hfi); // Read field information structure
GPtrArray *packet_read_fields_hfinfo(header_field_info *hfi); // Read multiple fields information structure
// GET field by internal index (hfi_index)
gboolean packet_register_field(const char *field_name, uint16_t field_hfi_index);     // Register target field
gboolean packet_register_set_field(const char *field_name, uint16_t field_hfi_index); // Register and set field (simplify if caching is not needed)
gboolean packet_set_field(uint16_t hfi_index);                                        // Set field (must be called before dissection)
field_info *packet_read_field(uint16_t hfi_index);                                    // Read field information structure
GPtrArray *packet_read_fields(uint16_t hfi_index);                                    // Read multiple fields information structure

// --- Common Fields functions ---
header_field_info *packet_get_header_info(const char *field_name); // Get header info (name, abbreviation, ...)
field_info *packet_read_field_at(GPtrArray *fields, uint16_t idx); // Read field from field information array
const char *packet_read_field_name(field_info *field_match);       // Return field name
const char *packet_read_field_abbrev(field_info *field_match);     // Return field abbreviation
uint16_t packet_read_field_offset(field_info *field_match);        // Read field offset
uint32_t packet_read_field_size(field_info *field_match);          // Read field size in aligned bytes
uint8_t packet_read_field_size_bits(uint64_t bitmask);             // Read field size in bits
unsigned long packet_read_field_bitmask(field_info *field_match);  // Read field mask (for bitfields)
uint8_t packet_read_field_bitmask_offset(uint64_t bitmask);        // Read field mask bit offset
uint8_t packet_read_field_bitmask_offset_msb(uint64_t bitmask);    // Read field mask from LSB
uint32_t packet_read_field_encoding(field_info *field_match);      // Read field encoding (little or big endian)
int packet_read_field_type(field_info *field_match);               // Read field type (wireshark equivalent type)
const char *packet_read_field_type_name(field_info *field_match);
const char *packet_read_field_encoding_name(field_info *field_match);
const char *packet_read_field_display_name(field_info *field_match);
const char *packet_read_field_string(field_info *field_match);
unsigned char *packet_read_field_ustring(field_info *field_match);
GByteArray *packet_read_field_bytes(field_info *field_match);
uint32_t packet_read_field_uint32(field_info *field_match);
int32_t packet_read_field_int32(field_info *field_match);
uint64_t packet_read_field_uint64(field_info *field_match);
int64_t packet_read_field_int64(field_info *field_match);
const char *packet_read_value_to_string(uint32_t value, const header_field_info *hfi); // Get packet field string value from header info

// --- Summary related functions ---
const char *packet_show();
const char *packet_summary();
const char *packet_dissectors();
const char *packet_dissector(uint8_t dissector_index);
uint32_t packet_dissectors_count();
uint32_t packet_layers_count();
const char *packet_relevant_fields();
char *packet_description();
uint8_t packet_direction();
const char *packet_protocol();
const char *packet_show_pdml();
uint8_t packet_field_summary(uint8_t *raw_packet, uint32_t packet_length, const char *field_name);

// --- Misc ---
void wd_log_g(const char *msg);
void wd_log_y(const char *msg);
void wd_log_r(const char *msg);
void set_wd_log_g(void (*wd_func)(const char *));
void set_wd_log_y(void (*wd_func)(const char *));
void set_wd_log_r(void (*wd_func)(const char *));

// ----- Multi-threaded API -----
wd_t *wd_init(const char *protocol_name);
wd_t *wd_get(uint32_t instance_number);
void wd_free(wd_t **wd);
void wd_reset(wd_t *wd);
void wd_reset_all();
void wd_set_log_level(enum wd_log_level level);
void wd_packet_dissect(wd_t *wd, unsigned char *raw_packet, uint32_t packet_length);
gboolean wd_set_protocol(wd_t *wd, const char *lt_arg);
void wd_set_dissection_mode(wd_t *wd, enum wd_dissection_mode wd_mode);
void wd_set_packet_direction(wd_t *wd, uint32_t packet_dir);
void wd_set_field_callback(wd_t *wd, gint (*fcn_callback)(wd_field_info_t fi));
uint8_t wd_packet_direction(wd_t *wd);
epan_dissect_t *wd_edt(wd_t *wd);
const char *wd_info_version();
const char *wd_info_profile();
const char *wd_packet_protocol(wd_t *wd);
const char *wd_packet_summary(wd_t *wd);
const char *wd_packet_show(wd_t *wd);
const char *wd_packet_show_pdml(wd_t *wd);
uint32_t wd_packet_layers_count(wd_t *wd);
uint32_t wd_packet_dissectors_count(wd_t *wd);
const char *wd_packet_dissector(wd_t *wd, uint8_t layer_index);
const char *wd_packet_dissectors(wd_t *wd);

// Field related functions
wd_field_t wd_field(const char *field_name);
uint8_t wd_register_field(wd_t *wd, wd_field_t hfi);
wd_field_info_t wd_read_field(wd_t *wd, wd_field_t hfi);
GPtrArray *wd_read_all_fields(wd_t *wd, wd_field_t hfi);

// Filter related functions
wd_filter_t wd_filter(const char *filter_string);
void wd_register_filter(wd_t *wd, wd_filter_t compiled_filter);
uint8_t wd_read_filter(wd_t *wd, wd_filter_t compiled_filter);

#ifdef __cplusplus
}
#endif

#endif