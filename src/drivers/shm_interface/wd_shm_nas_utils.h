#pragma once
#ifndef SHM_NAS_UTILS_
#define SHM_NAS_UTILS_

#include "wd_shm.h"

#define WD_SHM_EVT_NAS_5GS_DL_PLAIN 0x01
#define WD_SHM_EVT_NAS_5GS_DL_ENC 0x02
#define WD_SHM_EVT_NAS_5GS_UL_PLAIN 0x03
#define WD_SHM_EVT_NAS_5GS_UL_ENC 0x04

static inline uint16_t wd_shm_send_pdu_nas(uint16_t evt, uint64_t pkt_id, uint8_t *payload, uint16_t payload_size)
{
  if (!payload_size || !shm_is_enabled)
    return 0;

  uint8_t shm_channel;
  uint16_t s_idx = 2;
  uint16_t payload_offset;
  uint8_t direction_dl = 0;
  uint8_t event = evt & 0xFF;

  if ((event == WD_SHM_EVT_NAS_5GS_DL_PLAIN) || (event == WD_SHM_EVT_NAS_5GS_DL_ENC))
    direction_dl = 1;

  if (direction_dl)
    shm_channel = WD_SHM_MUTEX_5;
  else
    shm_channel = WD_SHM_MUTEX_6;

  uint8_t *shm_buffer_ptr = local_sync.shared_memory[shm_channel];

  *((uint16_t *)&shm_buffer_ptr[s_idx]) = evt;
  s_idx += 2;

  memcpy(shm_buffer_ptr + s_idx, payload, payload_size);
  payload_offset = s_idx;
  s_idx += payload_size;

  // Put pkt_id at the end of payload buffer
  *((uint64_t *)&shm_buffer_ptr[s_idx]) = pkt_id;
  s_idx += 8;

  // Place length on first index
  *((uint16_t *)shm_buffer_ptr) = s_idx;

  // Notify new payload
  shm_notify(shm_channel);

  if (direction_dl) {
    shm_wait(shm_channel);
    memcpy(payload, local_sync.shared_memory[shm_channel] + payload_offset, payload_size);
  }

  return payload_offset;
}

#endif