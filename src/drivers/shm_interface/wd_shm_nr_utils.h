#pragma once
#ifndef SHM_NR_UTILS_
#define SHM_NR_UTILS_

#include "wd_shm.h"

// Custom Events
#define W_INTERCEPT (0x0100)
#define W_INJECTED  (0x0200)
#define W_GNB_PHY_MIB (200)
#define W_GNB_MAC_UE_DL_SIB (201)
#define W_GNB_PHY_INITIATE_RA_PROCEDURE (202)
#define W_GNB_MAC_UE_DL_RAR_PDU_WITH_DATA (203)
#define W_GNB_MAC_UE_DL_PDU_WITH_DATA (204)
#define W_GNB_MAC_UE_UL_PDU_WITH_DATA (205)
#define W_UE_UL_PDU_WITH_DATA (210)
#define W_GNB_PDCP_PLAIN_DL (206)
#define W_GNB_PDCP_ENC_DL (207)
#define W_GNB_PDCP_PLAIN_UL (208)
#define W_GNB_PDCP_ENC_UL (209) // Not used

// MQUEUE Channels
#define W_MQ_MAC_DL WD_SHM_MQ_0
#define W_MQ_MAC_UL WD_SHM_MQ_1
#define W_MQ_PDCP_DL WD_SHM_MQ_2
#define W_MQ_PDCP_UL WD_SHM_MQ_3
#define W_MQ_NAS_DL WD_SHM_MQ_4
#define W_MQ_NAS_UL WD_SHM_MQ_5


/* radioType */
#define NR_FDD_RADIO 1
#define NR_TDD_RADIO 2

/* Direction */
#define NR_DIRECTION_UPLINK 0
#define NR_DIRECTION_DOWNLINK 1

/* rntiType */
#define NR_NO_RNTI 0
#define NR_RA_RNTI 2
#define NR_C_RNTI 3
#define NR_SI_RNTI 4

/* Wireshark tags */

// --------------------------------- MAC NR ------------------------------------
#define MAC_NR_PAYLOAD_TAG 0x01
#define MAC_NR_RNTI_TAG 0x02
#define MAC_NR_UEID_TAG 0x03
#define MAC_NR_FRAME_SLOT_TAG 0x07

// --------------------------------- PDCP --------------------------------------
/* Conditional field. This field is mandatory in case of User Plane PDCP PDU.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag). The allowed values are defined above. */
#define PDCP_NR_SEQNUM_LENGTH_TAG 0x02
/* 1 byte */
/* Optional fields. Attaching this info should be added if available.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag) */
#define PDCP_NR_DIRECTION_TAG 0x03
/* 1 byte */
#define PDCP_NR_BEARER_TYPE_TAG 0x04
/* 1 byte */
#define PDCP_NR_BEARER_ID_TAG 0x05
/* 1 byte */
#define PDCP_NR_UEID_TAG 0x06
/* 2 bytes, network order */
#define PDCP_NR_ROHC_COMPRESSION_TAG 0x07
/* 0 byte */
/* N.B. The following ROHC values only have significance if rohc_compression
   is in use for the current channel */
#define PDCP_NR_ROHC_IP_VERSION_TAG 0x08
/* 1 byte */
#define PDCP_NR_ROHC_CID_INC_INFO_TAG 0x09
/* 0 byte */
#define PDCP_NR_ROHC_LARGE_CID_PRES_TAG 0x0A
/* 0 byte */
#define PDCP_NR_ROHC_MODE_TAG 0x0B
/* 1 byte */
#define PDCP_NR_ROHC_RND_TAG 0x0C
/* 0 byte */
#define PDCP_NR_ROHC_UDP_CHECKSUM_PRES_TAG 0x0D
/* 0 byte */
#define PDCP_NR_ROHC_PROFILE_TAG 0x0E
/* 2 bytes, network order */
#define PDCP_NR_MACI_PRES_TAG 0x0F
/* 0 byte */
#define PDCP_NR_SDAP_HEADER_TAG 0x10
/* 1 byte, bitmask with PDCP_NR_UL_SDAP_HEADER_PRESENT and/or PDCP_NR_DL_SDAP_HEADER_PRESENT */
#define PDCP_NR_CIPHER_DISABLED_TAG 0x11
/* 0 byte */
/* PDCP PDU. Following this tag comes the actual PDCP PDU (there is no length, the PDU
   continues until the end of the frame) */
#define PDCP_NR_PAYLOAD_TAG 0x01

// --------------------------------- Structures ------------------------------------

enum pdcp_plane_nr {
    SIGNALING_PLANE_NR = 1,
    USER_PLANE_NR = 2
};

typedef enum nr_bearer_type {
    BEARER_DCCH = 1,
    BEARER_BCCH_BCH = 2,
    BEARER_BCCH_DL_SCH = 3,
    BEARER_CCCH = 4,
    BEARER_PCCH = 5,
} nr_bearer;

typedef struct _shm_nr_data_t {
    uint64_t rx_ul_count;
} shm_nr_data_t;

typedef struct _nr_mac_injection_t {
    bool    flag_mac;
    int     rlc_len;
    int     mac_len;
    uint8_t *mac_buf;
} nr_mac_injection_t;

// --------------------------------- Functions ------------------------------------

static inline uint16_t send_pdu_data_nr(uint16_t evt,
                                        uint8_t direction,
                                        uint8_t rnti_type,
                                        uint8_t rnti_value,
                                        uint16_t frame,
                                        uint16_t slot,
                                        uint64_t pkt_id,
                                        uint8_t *payload, uint16_t payload_size)
{
    if (!payload_size || !shm_is_enabled)
        return 0;

    uint8_t shm_channel;
    uint16_t payload_offset;
    uint8_t event = evt & 0xFF;
    uint16_t s_idx = 2;

    if (direction == NR_DIRECTION_DOWNLINK) {
        // Broadcast DL events
        if ((event == W_GNB_MAC_UE_DL_SIB) ||
            (event == W_GNB_PHY_MIB))
            shm_channel = WD_SHM_MUTEX_2;
        else
            // All other DL events
            shm_channel = WD_SHM_MUTEX_0;
    }
    else
        // All UL events
        shm_channel = WD_SHM_MUTEX_1;

    uint8_t *shm_buffer_ptr = local_sync.shared_memory[shm_channel];

    // Event and flags
    *((uint16_t *)&shm_buffer_ptr[s_idx]) = evt;
    s_idx += 2;
    // Wireshark payload starts here
    shm_buffer_ptr[s_idx++] = NR_TDD_RADIO;
    shm_buffer_ptr[s_idx++] = direction;
    shm_buffer_ptr[s_idx++] = rnti_type;

    if (rnti_type == NR_C_RNTI || rnti_type == NR_RA_RNTI) {
        shm_buffer_ptr[s_idx++] = MAC_NR_RNTI_TAG;
        shm_buffer_ptr[s_idx++] = (rnti_value >> 8) & 0xFF;
        shm_buffer_ptr[s_idx++] = rnti_value & 0xFF;
    }

    shm_buffer_ptr[s_idx++] = MAC_NR_FRAME_SLOT_TAG;
    shm_buffer_ptr[s_idx++] = (frame >> 8) & 0xFF;
    shm_buffer_ptr[s_idx++] = frame & 0xFF;
    shm_buffer_ptr[s_idx++] = (slot >> 8) & 0xFF;
    shm_buffer_ptr[s_idx++] = slot & 0xFF;

    if (payload_size) {
        shm_buffer_ptr[s_idx++] = MAC_NR_PAYLOAD_TAG;
        payload_offset = s_idx;
        memcpy(shm_buffer_ptr + s_idx,
               payload,
               payload_size);
        s_idx += payload_size;
    }

    // Put pkt_id at the end of payload buffer
    *((uint64_t *)&shm_buffer_ptr[s_idx]) = pkt_id;
    s_idx += 8;

    // Place length on first index
    *((uint16_t *)shm_buffer_ptr) = s_idx;
  
    

    // Notify new payload
    shm_notify(shm_channel);
    // Wait response
    if ((direction == NR_DIRECTION_UPLINK) && (event == W_UE_UL_PDU_WITH_DATA)) {
        shm_wait(shm_channel);
        memcpy(payload, local_sync.shared_memory[shm_channel] + payload_offset, payload_size);
    }
    else if ((direction == NR_DIRECTION_DOWNLINK)) {
        shm_wait(shm_channel);
        memcpy(payload, local_sync.shared_memory[shm_channel] + payload_offset, payload_size);
    }

    return payload_offset; // Return offset where payload was inserted in shared memory
}

static inline uint16_t send_pdu_data_pdcp_nr(uint8_t direction,
                                             uint16_t evt,
                                             uint8_t radio_bearer_type,
                                             uint8_t radio_bearer_id,
                                             uint8_t seqnum_length,
                                             uint8_t has_integrity_or_ciphering, uint64_t pkt_id,
                                             uint8_t *payload, uint16_t payload_size)
{
    if (!payload_size || !shm_is_enabled)
        return 0;

    uint16_t payload_offset;
    uint8_t shm_channel;
    uint16_t s_idx = 2;
    uint8_t plane_type = evt & 0xFF;

    if (direction == NR_DIRECTION_DOWNLINK)
        shm_channel = WD_SHM_MUTEX_3;
    else
        shm_channel = WD_SHM_MUTEX_4;

    uint8_t *shm_buffer_ptr = local_sync.shared_memory[shm_channel];

    // Event
    if (direction == NR_DIRECTION_UPLINK)
        // gNB Uplink is always plain for now
        shm_buffer_ptr[s_idx++] = W_GNB_PDCP_PLAIN_UL;
    else
        shm_buffer_ptr[s_idx++] = (has_integrity_or_ciphering ? W_GNB_PDCP_ENC_DL : W_GNB_PDCP_PLAIN_DL); 

    // Flags
    shm_buffer_ptr[s_idx++] = evt >> 8;

    // Wireshark payload starts here
    shm_buffer_ptr[s_idx++] = plane_type; // Plane type (User plane or Control plane)
    // Dynamic fields
    shm_buffer_ptr[s_idx++] = PDCP_NR_DIRECTION_TAG;
    shm_buffer_ptr[s_idx++] = direction;

    shm_buffer_ptr[s_idx++] = PDCP_NR_BEARER_TYPE_TAG;
    shm_buffer_ptr[s_idx++] = radio_bearer_type; // Usually DCCH

    shm_buffer_ptr[s_idx++] = PDCP_NR_BEARER_ID_TAG;
    shm_buffer_ptr[s_idx++] = radio_bearer_id;

    shm_buffer_ptr[s_idx++] = PDCP_NR_SEQNUM_LENGTH_TAG;
    shm_buffer_ptr[s_idx++] = seqnum_length;

    shm_buffer_ptr[s_idx++] = PDCP_NR_PAYLOAD_TAG;
    payload_offset = s_idx;
    memcpy(shm_buffer_ptr + s_idx,
           payload,
           payload_size);
    s_idx += payload_size;

    // Put pkt_id at the end of payload buffer
    *((uint64_t *)&shm_buffer_ptr[s_idx]) = pkt_id;
    s_idx += 8;

    // Place length on first index
    *((uint16_t *)shm_buffer_ptr) = s_idx;

    // Notify new payload
    shm_notify(shm_channel);
    // Wait response
    if (direction == NR_DIRECTION_DOWNLINK) {
        shm_wait(shm_channel);
        memcpy(payload, local_sync.shared_memory[shm_channel] + payload_offset, payload_size);
    }

    return payload_offset; // Return offset where payload was inserted in shared memory
}

#endif
