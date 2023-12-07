#pragma once
#ifndef SHM_LTE_UTILS_
#define SHM_LTE_UTILS_

#include "common/utils/T/shared_memory.h"

#define SHM_MUTEX_0 0
#define SHM_MUTEX_1 1

// Custom
#define T_ENB_MAC_UE_DL_SIB (254)
#define T_ENB_PDCP_PLAIN (253)
#define T_ENB_PDCP_ENC (252)

/* radioType */
#define FDD_RADIO 1
#define TDD_RADIO 2

/* Direction */
#define DIRECTION_UPLINK 0
#define DIRECTION_DOWNLINK 1

/* rntiType */
#define WS_NO_RNTI 0
#define WS_P_RNTI 1
#define WS_RA_RNTI 2
#define WS_C_RNTI 3
#define WS_SI_RNTI 4
#define WS_SPS_RNTI 5
#define WS_M_RNTI 6
#define WS_SL_BCH_RNTI 7
#define WS_SL_RNTI 8
#define WS_SC_RNTI 9
#define WS_G_RNTI 10

#define MAC_LTE_RNTI_TAG 0x02
/* 2 bytes, network order */

#define MAC_LTE_UEID_TAG 0x03
/* 2 bytes, network order */

#define MAC_LTE_FRAME_SUBFRAME_TAG 0x04
/* 2 bytes, network order, SFN is stored in 12 MSB and SF in 4 LSB */

#define MAC_LTE_PREDEFINED_DATA_TAG 0x05
/* 1 byte */

#define MAC_LTE_RETX_TAG 0x06
/* 1 byte */

#define MAC_LTE_CRC_STATUS_TAG 0x07
/* 1 byte */

#define MAC_LTE_EXT_BSR_SIZES_TAG 0x08
/* 0 byte */

#define MAC_LTE_SEND_PREAMBLE_TAG 0x09
/* 2 bytes, RAPID value (1 byte) followed by RACH attempt number (1 byte) */

#define MAC_LTE_CARRIER_ID_TAG 0x0A
/* 1 byte */

#define MAC_LTE_PHY_TAG 0x0B
/* variable length, length (1 byte) then depending on direction
   in UL: modulation type (1 byte), TBS index (1 byte), RB length (1 byte),
          RB start (1 byte), HARQ id (1 byte), NDI (1 byte)
   in DL: DCI format (1 byte), resource allocation type (1 byte), aggregation level (1 byte),
          MCS index (1 byte), redundancy version (1 byte), resource block length (1 byte),
          HARQ id (1 byte), NDI (1 byte), TB (1 byte), DL reTx (1 byte) */

#define MAC_LTE_SIMULT_PUCCH_PUSCH_PCELL_TAG 0x0C
/* 0 byte */

#define MAC_LTE_SIMULT_PUCCH_PUSCH_PSCELL_TAG 0x0D
/* 0 byte */

#define MAC_LTE_CE_MODE_TAG 0x0E
/* 1 byte containing mac_lte_ce_mode enum value */

#define MAC_LTE_NB_MODE_TAG 0x0F
/* 1 byte containing mac_lte_nb_mode enum value */

#define MAC_LTE_N_UL_RB_TAG 0x10
/* 1 byte containing the number of UL resource blocks: 6, 15, 25, 50, 75 or 100 */

#define MAC_LTE_SR_TAG 0x11
/* 2 bytes for the number of items, followed by that number of ueid, rnti (2 bytes each) */

/* MAC PDU. Following this tag comes the actual MAC PDU (there is no length, the PDU
   continues until the end of the frame) */
#define MAC_LTE_PAYLOAD_TAG 0x01

/* Fixed fields.  This is followed by the following 3 mandatory fields:
   - no_header_pdu (1 byte)
   - plane (1 byte)
   - rohc_compression ( byte)
   (where the allowed values are defined above) */

/* Conditional field. This field is mandatory in case of User Plane PDCP PDU.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag). The allowed values are defined above. */

#define PDCP_LTE_SEQNUM_LENGTH_TAG 0x02
/* 1 byte */

// --------------------------------- PDCP --------------------------------------

enum pdcp_plane
{
   SIGNALING_PLANE = 1,
   USER_PLANE = 2
};

/* Optional fields. Attaching this info to frames will allow you
   to show you display/filter/plot/add-custom-columns on these fields, so should
   be added if available.
   The format is to have the tag, followed by the value (there is no length field,
   it's implicit from the tag) */

#define PDCP_LTE_DIRECTION_TAG 0x03
/* 1 byte */

#define PDCP_LTE_LOG_CHAN_TYPE_TAG 0x04
/* 1 byte */

#define PDCP_LTE_BCCH_TRANSPORT_TYPE_TAG 0x05
/* 1 byte */

#define PDCP_LTE_ROHC_IP_VERSION_TAG 0x06
/* 2 bytes, network order */

#define PDCP_LTE_ROHC_CID_INC_INFO_TAG 0x07
/* 1 byte */

#define PDCP_LTE_ROHC_LARGE_CID_PRES_TAG 0x08
/* 1 byte */

#define PDCP_LTE_ROHC_MODE_TAG 0x09
/* 1 byte */

#define PDCP_LTE_ROHC_RND_TAG 0x0A
/* 1 byte */

#define PDCP_LTE_ROHC_UDP_CHECKSUM_PRES_TAG 0x0B
/* 1 byte */

#define PDCP_LTE_ROHC_PROFILE_TAG 0x0C
/* 2 bytes, network order */

#define PDCP_LTE_CHANNEL_ID_TAG 0x0D
/* 2 bytes, network order */

#define PDCP_LTE_UEID_TAG 0x0E
/* 2 bytes, network order */

/* PDCP PDU. Following this tag comes the actual PDCP PDU (there is no length, the PDU
   continues until the end of the frame) */
#define PDCP_LTE_PAYLOAD_TAG 0x01

static inline void util_print_buffer(uint8_t *buf, uint16_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        printf("%02X", buf[i]);
    }
    printf('\n');
}

static inline uint16_t send_pdu_data(int event,
                                     int direction,
                                     int rnti_type,
                                     int rnti_number,
                                     int frame_number,
                                     int subframe_number,
                                     uint8_t *payload, int payload_size,
                                     int8_t preamble)
{
   uint16_t payload_offset;
   uint16_t s_idx = 2;
   uint8_t *shm_buffer_ptr = local_sync.shared_memory[SHM_MUTEX_0];

   shm_buffer_ptr[s_idx++] = event;
   // Wireshark payload starts here
   shm_buffer_ptr[s_idx++] = FDD_RADIO;
   shm_buffer_ptr[s_idx++] = direction;
   shm_buffer_ptr[s_idx++] = rnti_type;
   if (rnti_type == WS_C_RNTI || rnti_type == WS_RA_RNTI)
   {
      shm_buffer_ptr[s_idx++] = MAC_LTE_RNTI_TAG;
      shm_buffer_ptr[s_idx++] = (rnti_number >> 8) & 0xFF;
      shm_buffer_ptr[s_idx++] = rnti_number & 0xFF;
   }
   int fsf = (frame_number << 4) + subframe_number;

   shm_buffer_ptr[s_idx++] = MAC_LTE_FRAME_SUBFRAME_TAG;
   shm_buffer_ptr[s_idx++] = (fsf >> 8) & 0xFF;
   shm_buffer_ptr[s_idx++] = fsf & 0xFF;

   if (preamble != -1)
   {
      shm_buffer_ptr[s_idx++] = MAC_LTE_SEND_PREAMBLE_TAG;
      shm_buffer_ptr[s_idx++] = preamble;
      shm_buffer_ptr[s_idx++] = 0; /* rach attempt - always 0 for us (not sure of this) */
   }

   shm_buffer_ptr[s_idx++] = MAC_LTE_PAYLOAD_TAG;
   payload_offset = s_idx;
   memcpy(shm_buffer_ptr + s_idx,
          payload,
          payload_size);
   s_idx += payload_size;
   // Place length on first index
   shm_buffer_ptr[0] = s_idx & 0xFF;
   shm_buffer_ptr[1] = (s_idx >> 8) & 0xFF;
   // Notify new payload
   shm_notify(SHM_MUTEX_0);
   // Wait response
   if(direction == DIRECTION_DOWNLINK)
   {
      shm_wait(SHM_MUTEX_0);
      memcpy(payload, local_sync.shared_memory[SHM_MUTEX_0] + payload_offset, payload_size);
   }
   return payload_offset; // Return offset where payload was inserted in shared memory
}

static inline uint16_t send_pdu_data_pdcp(int direction,
                                          int plane_type,
                                          int channel_id,
                                          int has_integrity,
                                          int rohc_profile,
                                          int seqnum_length,
                                          int ueid,
                                          uint8_t *payload, int payload_size)
{
   uint16_t payload_offset;
   uint16_t s_idx = 2;
   uint8_t *shm_buffer_ptr = local_sync.shared_memory[SHM_MUTEX_1];

   shm_buffer_ptr[s_idx++] = (has_integrity > 0 ? T_ENB_PDCP_ENC : T_ENB_PDCP_PLAIN); // Event
   // Wireshark payload starts here
   shm_buffer_ptr[s_idx++] = FALSE;                      // No header PDU (True only for NB-IoT)
   shm_buffer_ptr[s_idx++] = plane_type;                 // Plane type (User plane or Control plane)
   shm_buffer_ptr[s_idx++] = (rohc_profile > 0 ? 1 : 0); // Has rohc compression
   // Dynamic fields
   shm_buffer_ptr[s_idx++] = PDCP_LTE_DIRECTION_TAG;
   shm_buffer_ptr[s_idx++] = direction;
   shm_buffer_ptr[s_idx++] = PDCP_LTE_LOG_CHAN_TYPE_TAG;
   shm_buffer_ptr[s_idx++] = 1; //DCCH
   shm_buffer_ptr[s_idx++] = PDCP_LTE_SEQNUM_LENGTH_TAG;
   shm_buffer_ptr[s_idx++] = seqnum_length;

   shm_buffer_ptr[s_idx++] = PDCP_LTE_CHANNEL_ID_TAG;
   shm_buffer_ptr[s_idx++] = (channel_id >> 8) & 0xFF;
   shm_buffer_ptr[s_idx++] = channel_id & 0xFF;

   shm_buffer_ptr[s_idx++] = PDCP_LTE_UEID_TAG;
   shm_buffer_ptr[s_idx++] = (ueid >> 8) & 0xFF;
   shm_buffer_ptr[s_idx++] = ueid & 0xFF;

   shm_buffer_ptr[s_idx++] = PDCP_LTE_ROHC_PROFILE_TAG;
   shm_buffer_ptr[s_idx++] = (rohc_profile >> 8) & 0xFF;
   shm_buffer_ptr[s_idx++] = rohc_profile & 0xFF;

   shm_buffer_ptr[s_idx++] = PDCP_LTE_PAYLOAD_TAG;
   payload_offset = s_idx;
   memcpy(shm_buffer_ptr + s_idx,
          payload,
          payload_size);
   s_idx += payload_size;
   // Place length on first index
   shm_buffer_ptr[0] = s_idx & 0xFF;
   shm_buffer_ptr[1] = (s_idx >> 8) & 0xFF;
   // Notify new payload
   shm_notify(SHM_MUTEX_1);
   // Wait response
   if(direction == DIRECTION_DOWNLINK)
      shm_wait(SHM_MUTEX_1);
   // Copy modified payload
   // printf("buf addr:%08X\n", (uint64_t)local_sync.shared_memory[SHM_MUTEX_1]);
   // uint8_t *bb = shm_buffer_ptr;
   // printf( "%02x %02x %02x %02x %02x %02x \n", bb[3], bb[4], bb[5], bb[6], bb[7], bb[8]);
   memcpy(payload, local_sync.shared_memory[SHM_MUTEX_1] + payload_offset, payload_size);

   return payload_offset; // Return offset where payload was inserted in shared memory
}

#endif