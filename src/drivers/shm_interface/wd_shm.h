#ifndef __WDSHMIFACE__
#define __WDSHMIFACE__

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define WD_SHM_DEFAULT_PATH "/tmp/wshm"
#define WD_SHM_CLIENT 0
#define WD_SHM_SERVER 1
#define WD_SHM_MAX_MUTEXES 16
#define WD_SHM_MAX_BUFFER_SIZE 16384
#define WD_MQUEUE_MAX_MESSAGES 32
#define WD_SHM_DIR_DOWNLINK 0
#define WD_SHM_DIR_UPLINK 1
#define WD_SHM_DIR_TX 0
#define WD_SHM_DIR_RX 1

#define WD_SHM_MUTEX_0 0
#define WD_SHM_MUTEX_1 1
#define WD_SHM_MUTEX_2 2
#define WD_SHM_MUTEX_3 3
#define WD_SHM_MUTEX_4 4
#define WD_SHM_MUTEX_5 5
#define WD_SHM_MUTEX_6 6
#define WD_SHM_MUTEX_7 7
#define WD_SHM_MUTEX_8 8
#define WD_SHM_MUTEX_9 9
#define WD_SHM_MUTEX_10 10
#define WD_SHM_MUTEX_11 11
#define WD_SHM_MUTEX_12 12
#define WD_SHM_MUTEX_13 13
#define WD_SHM_MUTEX_14 14
#define WD_SHM_MUTEX_15 15

#define WD_SHM_MQ_0 0
#define WD_SHM_MQ_1 1
#define WD_SHM_MQ_2 2
#define WD_SHM_MQ_3 3
#define WD_SHM_MQ_4 4
#define WD_SHM_MQ_5 5
#define WD_SHM_MQ_6 6
#define WD_SHM_MQ_7 7
#define WD_SHM_MQ_8 8
#define WD_SHM_MQ_9 9
#define WD_SHM_MQ_10 10
#define WD_SHM_MQ_11 11
#define WD_SHM_MQ_12 12
#define WD_SHM_MQ_13 13
#define WD_SHM_MQ_14 14
#define WD_SHM_MQ_15 15

#ifdef __cplusplus
extern "C" {
#endif

struct Sync {
    int segment_id;
    uint8_t *raw_shared_memory;
    uint8_t *shared_memory[WD_SHM_MAX_MUTEXES];
    struct Mutex *client_mutex[WD_SHM_MAX_MUTEXES];
    struct Mutex *server_mutex[WD_SHM_MAX_MUTEXES];
    int mutex_created;
};

struct Mutex {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int count;
};

typedef struct _mq_msg_t {
    uint8_t* msg_buf;
    uint16_t msg_size;
} mq_msg_t;

extern struct Sync local_sync;
extern uint64_t shm_is_enabled;

/**
 * @brief Initializes SHM interface if environment variable
 * WD_SHM is set to "1" (export WD_SHM=1)
 *
 * @param _is_server
 * @param shm_size
 * @param segment_key_path
 */
int shm_init(uint8_t _is_server, uint32_t shm_size, const char *segment_key_path);
void shm_notify(uint16_t mutex_num);
void sync_notify(struct Mutex *sync);
uint8_t shm_wait(uint16_t mutex_num);
void shm_set_max_timeout(uint16_t timeout);
void shm_cleanup(void);

int shm_mq_init(uint8_t channel, uint32_t shm_size, const char *segment_key_path, uint8_t use_thread);
mq_msg_t *shm_mq_recv(uint8_t channel);
size_t shm_mq_available(uint8_t channel);
int shm_mq_send(uint8_t channel, uint8_t *msg_buf, uint16_t msg_len);

#ifdef __cplusplus
}
#endif

#endif /* SHM_SYNC_COMMON_H */
