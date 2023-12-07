#ifndef __OAICOMMUNICATION__
#define __OAICOMMUNICATION__

#include <mqueue.h>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "Machine.hpp"
#include "MiscUtils.hpp"
#include "shm_interface/wd_shm_nas_utils.h"
#include "shm_interface/wd_shm_nr_utils.h"

typedef struct _driver_event_shm_t {
    uint8_t type; // Event type
    struct flags_t {
        uint8_t intercept_tx : 1;
        uint8_t duplicated : 1;
        uint8_t unused : 6;
    } flags;

    uint8_t *data_buffer; // Packet Buffer
    uint16_t data_size;   // Packet size
    uint64_t data_id;     // Used to save original pkt number
} driver_event_shm_t;

/**
 * @brief SHMDriver class opens a shared memory (SHM) interface with multiple channels.
 * The SHM interface can be initialized in server (WD_SHM_SERVER) or client (WD_SHM_CLIENT) mode.
 * Internally it uses the library from src/drivers/shm_interface/wd_shm.h.
 * this library can be used by another program to communicate via SHM to this class.
 * Each channel of the SHM is only to be used between 2 programs in server/client roles.
 */
class SHMDriver {
private:
    static unordered_map<string, SHMDriver *> shm_instances_map;
    static mutex shm_instances_mutex;
    string _shm_key_path;
    uint8_t _shm_mode;
    string _mq_path;
    uint8_t _mq_created;
    int _fifo_fd;
    bool _resync_req;

public:
    const char *TAG = "[SHMDriver] ";
    uint16_t _shm_channel;

    /**
     * @brief
     *
     * @param shm_key_path name of the shared memory segment. It must be the same between two programs
     * @param shm_channel channel number in the share memory segment
     * @param shm_mode SHM mode, server (WD_SHM_SERVER) or client (WD_SHM_CLIENT)
     * @return true if SHM initialization is successful
     * @return false if SHM initialization is NOT successful
     */
    bool init(string shm_key_path, uint16_t shm_channel, uint8_t shm_mode, bool enable_mqueue = false)
    {
        // Init vars
        _shm_key_path = shm_key_path;
        _shm_channel = shm_channel;
        _shm_mode = shm_mode;
        _mq_created = 0;
        _mq_path.clear();
        _resync_req = false;

        // Initialize SHM segment if not initialized before
        SHMDriver::shm_instances_mutex.lock();
        if (!SHMDriver::shm_instances_map.contains(_shm_key_path)) {
            SHMDriver::shm_instances_map[_shm_key_path] = this;
            g_setenv("WD_SHM", "1", NULL); // Required
            shm_init(_shm_mode, WD_SHM_MAX_BUFFER_SIZE, _shm_key_path.c_str());

            shm_set_max_timeout(-1); // No timeout
        }
        SHMDriver::shm_instances_mutex.unlock();

        // Get the correct path for mqueue (path from last / is used)
        _mq_path = "/" + string_split(_shm_key_path, "/").back();

        if (enable_mqueue)
            GL1Y(TAG, "SHM:", _shm_key_path, ", Channel:", _shm_channel, ", Mode:", (int)_shm_mode, ", MQUEUE:", _mq_path);
        else
            GL1Y(TAG, "SHM:", _shm_key_path, ", Channel:", _shm_channel, ", Mode:", (int)_shm_mode);

        if (!enable_mqueue)
            return true;

        // Initialize message queue (create if it does not exist)
        if (shm_mq_init(shm_channel, WD_SHM_MAX_BUFFER_SIZE, _shm_key_path.c_str(), false))
            return false;

        _mq_created = 1;

        return true;
    }

    inline __attribute__((always_inline)) void intercept_tx(uint8_t *pkt_buf = nullptr, uint16_t pkt_size = 0, uint16_t pkt_offset = 0)
    {
        if (G_LIKELY(pkt_buf != NULL))
            memcpy(local_sync.shared_memory[_shm_channel] + 4, pkt_buf + pkt_offset, pkt_size - pkt_offset);

        shm_notify(_shm_channel);
    }

    inline void send(uint8_t *pkt_buf, uint16_t pkt_size, uint16_t pkt_offset = 0, uint64_t pkt_id = 0, int mqueue_channel = -1)
    {
        if (G_UNLIKELY(!_mq_created))
            return;

        uint16_t c_channel;

        if (mqueue_channel != -1)
            c_channel = mqueue_channel;
        else
            c_channel = _shm_channel;

        if (pkt_size < pkt_offset)
        {
            LOG2R(TAG, format("Error: shm_mq_send(len={} < pkt_offset={})", pkt_size, pkt_offset));
            return;
        }
        // Create new buffer
        pkt_size = pkt_size - pkt_offset; // Subtract offset from total packet length
        uint16_t payload_len = pkt_size + 8;
        uint8_t payload_buf[payload_len];
        // Copy pkt_buffer to new buffer
        memcpy(payload_buf, pkt_buf + pkt_offset, pkt_size);
        // Put pkt_id at the end of buffer
        *((uint64_t *)&payload_buf[pkt_size]) = pkt_id;

        if (shm_mq_send(c_channel, payload_buf, payload_len) != 0)
            LOG2R(TAG, format("Error: shm_mq_send(len={})", pkt_size));
    }

    inline __attribute__((always_inline)) driver_event_shm_t receive()
    {
        uint8_t pkt_type;
        driver_event_shm_t::flags_t *pkt_flags;
        uint16_t pkt_size;
        uint8_t *pkt_buf;
        uint64_t pkt_id;
        // Wait on SHM channel
        if (shm_wait(_shm_channel)) {
            if (G_UNLIKELY(_resync_req)) {
                _resync_req = false;
                local_sync.server_mutex[_shm_channel]->count = 0;
                local_sync.client_mutex[_shm_channel]->count = 0;
                return {NULL};
            }

            pkt_size = *((uint16_t *)local_sync.shared_memory[_shm_channel]) - 4 - 8; // subtract length (2), type (1), flags (1) and data_id (8);

            if (G_UNLIKELY(pkt_size > WD_SHM_MAX_BUFFER_SIZE))
                return {NULL};

            pkt_type = local_sync.shared_memory[_shm_channel][2];                                  // Get event type from second position
            pkt_flags = (driver_event_shm_t::flags_t *)&local_sync.shared_memory[_shm_channel][3]; // Get flags from third position
            pkt_buf = &local_sync.shared_memory[_shm_channel][4];                                  // Pointer to the SHM memory
            pkt_id = *((uint64_t *)&local_sync.shared_memory[_shm_channel][pkt_size + 4]);         // Get packet ID

            return {pkt_type, *pkt_flags, pkt_buf, pkt_size, pkt_id};
        }
        return {NULL};
    }

    void Resync()
    {
        Mutex *shm_mutex_current = (_shm_mode == WD_SHM_SERVER ? local_sync.server_mutex[_shm_channel] : local_sync.client_mutex[_shm_channel]);
        if (!shm_mutex_current)
            return;

        _resync_req = true;
        // Trylock current mutex
        pthread_mutex_trylock(&shm_mutex_current->mutex);
        pthread_mutex_unlock(&shm_mutex_current->mutex);
        // Notify server for spurious recv unblock
        local_sync.server_mutex[_shm_channel]->count = 0;
        local_sync.client_mutex[_shm_channel]->count = 0;
        sync_notify(shm_mutex_current);
        // unlock client mutex if client process quit unexpectedly while holding mutex
        pthread_mutex_t *shm_mutex = &(_shm_mode == WD_SHM_SERVER ? local_sync.client_mutex[_shm_channel]->mutex : local_sync.server_mutex[_shm_channel]->mutex);
        if (!shm_mutex)
            return;

        // Trylock other end mutex
        pthread_mutex_trylock(shm_mutex);
        pthread_mutex_unlock(shm_mutex);
    }
};

unordered_map<string, SHMDriver *> SHMDriver::shm_instances_map;
mutex SHMDriver::shm_instances_mutex;

#endif