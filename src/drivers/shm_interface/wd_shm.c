#include <assert.h>
#include <errno.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "lfbb.h"
#include "wd_shm.h"

#define DEFAULT_SEGMENT_KEY_STR "/tmp/wshm"
#define DEFAULT_MSGQ_KEY_STR "/wshm"
#define DEFAULT_MAX_WAIT_TIME 2

typedef struct Arguments {
    int size;
    int count;
    char segment_key_path[240];

} Arguments;

typedef struct _Mq_t {
    mqd_t handler;
    char key[240];
    struct mq_attr attr;
    uint8_t msg_buf[WD_SHM_MAX_BUFFER_SIZE];
    uint16_t msg_size;
    uint8_t channel;
    // Used only if use_thread is set to 1
    uint8_t use_thread;
    pthread_t thread;
    LFBB_Inst_Type msg_queue;
    uint8_t msg_queue_buf[WD_SHM_MAX_BUFFER_SIZE * WD_MQUEUE_MAX_MESSAGES];
    uint8_t msg_queue_acquired;
} Mq_t;

uint32_t max_wait_time = 2;
struct Sync local_sync;
struct Arguments local_args;
uint8_t is_server = 0;
uint64_t shm_is_enabled = 0;
static Mq_t *mqueues[WD_SHM_MAX_MUTEXES] = {0};

// Prototypes
void throw_custom(const char *message);
int generate_key(const char *path);
void init_sync(struct Sync *sync);
void destroy_sync(struct Sync *sync);
void cleanup(struct Sync *sync);
uint8_t sync_wait(struct Mutex *sync);
void sync_notify(struct Mutex *sync);
void create_sync(struct Sync *sync, struct Arguments *args);
void *shm_mq_thread_handler(void *param);

// ---- Functions ----

noreturn void throw_custom(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

int generate_key(const char *path)
{
    // Generate a random key from the given file path
    // (inode etc.) plus the arbitrary character
    return ftok(path, 'X');
}

void init_sync(struct Sync *sync)
{
    for (size_t i = 0; i < WD_SHM_MAX_MUTEXES; i++) {
        // These structures are used to initialize mutexes
        // and condition variables. We will use them to set
        // the PTHREAD_PROCESS_SHARED attribute, which enables
        // more than one process (and any thread in any of those
        // processes) to access the mutex and condition variable.
        pthread_mutexattr_t mutex_attributes;
        pthread_condattr_t condition_attributes;

        // These methods initialize the attribute structures
        // with default values so that we must only change
        // the one we are interested in.
        if (pthread_mutexattr_init(&mutex_attributes) != 0) {
            throw_custom("Error initializing mutex attributes");
        }
        if (pthread_condattr_init(&condition_attributes) != 0) {
            throw_custom("Error initializing condition variable attributes");
        }

        // Here we set the "process-shared"-attribute of the mutex
        // and condition variable to PTHREAD_PROCESS_SHARED. This
        // means multiple processes may access these objects. If
        // we wouldn't do this, the attribute would be PTHREAD_PROCESS
        // _PRIVATE, where only threads created by the process who
        // initialized the objects would be allowed to access them.
        // By passing PTHREAD_PROCESS_SHARED the objects may also live
        // longer than the process creating them.
        // clang-format off
    if (pthread_mutexattr_setpshared(
                &mutex_attributes, PTHREAD_PROCESS_SHARED) != 0) {
        throw_custom("Error setting process-shared attribute for mutex");
    }

    if (pthread_condattr_setpshared(
                &condition_attributes, PTHREAD_PROCESS_SHARED) != 0) {
        throw_custom("Error setting process-shared attribute for condition variable");
    }
        // clang-format on

        // Initialize the mutex and condition variable and pass the attributes
        if (pthread_mutex_init(&sync->server_mutex[i]->mutex, &mutex_attributes) != 0) {
            throw_custom("Error initializing mutex");
        }
        if (pthread_cond_init(&sync->server_mutex[i]->condition, &condition_attributes) != 0) {
            throw_custom("Error initializing condition variable");
        }

        if (pthread_mutex_init(&sync->client_mutex[i]->mutex, &mutex_attributes) != 0) {
            throw_custom("Error initializing mutex");
        }
        if (pthread_cond_init(&sync->client_mutex[i]->condition, &condition_attributes) != 0) {
            throw_custom("Error initializing condition variable");
        }

        // Destroy the attribute objects
        if (pthread_mutexattr_destroy(&mutex_attributes)) {
            throw_custom("Error destroying mutex attributes");
        }
        if (pthread_condattr_destroy(&condition_attributes)) {
            throw_custom("Error destroying condition variable attributes");
        }

        sync->server_mutex[i]->count = 0;
        sync->client_mutex[i]->count = 0;
    }
}

void destroy_sync(struct Sync *sync)
{
    for (size_t i = 0; i < WD_SHM_MAX_MUTEXES; i++) {
        if (pthread_mutex_destroy(&sync->server_mutex[i]->mutex)) {
            throw_custom("Error destroying mutex");
        }
        if (pthread_cond_destroy(&sync->server_mutex[i]->condition)) {
            throw_custom("Error destroying condition variable");
        }
        if (pthread_mutex_destroy(&sync->client_mutex[i]->mutex)) {
            throw_custom("Error destroying mutex");
        }
        if (pthread_cond_destroy(&sync->client_mutex[i]->condition)) {
            throw_custom("Error destroying condition variable");
        }
    }
}

void cleanup(struct Sync *sync)
{
    // Detach the shared memory from this process' address space.
    // If this is the last process using this shared memory, it is removed.
    shmdt((void *)sync->raw_shared_memory);

    /*
        Deallocate manually for security. We pass:
            1. The shared memory ID returned by shmget.
            2. The IPC_RMID flag to schedule removal/deallocation
                 of the shared memory.
            3. NULL to the last struct parameter, as it is not relevant
                 for deletion (it is populated with certain fields for other
                 calls, notably IPC_STAT, where you would pass a struct shmid_ds*).
    */
    shmctl(sync->segment_id, IPC_RMID, NULL);
    if (sync->mutex_created) {
        destroy_sync(sync);
    }
}

uint8_t sync_wait(struct Mutex *sync)
{
    // puts("sync_wait");
    uint8_t status = 1;
    // Lock the mutex
    if (pthread_mutex_lock(&sync->mutex) != 0) {
        puts("sync_wait: Error locking mutex");
        status = 0;
    }

    // Move into waiting for the condition variable to be signalled.
    // For this, it is essential that the mutex first be locked (above)
    // to avoid data races on the condition variable (e.g. the signal
    // being sent before the waiting process has begun). In fact, behaviour
    // is undefined otherwise. Once the mutex has begun waiting, the mutex
    // is unlocked so that other threads may do something and eventually
    // signal the condition variable. At that point, this thread wakes up
    // and *re-acquires* the lock immediately. As such, when this method
    // returns the lock will be owned by the calling thread.
    while (sync->count == 0) {
        struct timespec max_wait;
        clock_gettime(CLOCK_REALTIME, &max_wait);
        max_wait.tv_sec += max_wait_time;
        if (pthread_cond_timedwait(&sync->condition, &sync->mutex, &max_wait) != 0) {
            puts("[SHM] Timeout waiting for sync");
            if (!is_server) {
                // Client tries to reinitialize shm structure upon timeout
                create_sync(&local_sync, &local_args);
            }
            status = 0;
        }
    }
    sync->count = 0;
    if (pthread_mutex_unlock(&sync->mutex) != 0) {
        puts("Error unlocking mutex");
        status = 0;
    }

    return status;
}

void sync_notify(struct Mutex *sync)
{
    struct timespec timeoutTime;
    clock_gettime(CLOCK_REALTIME, &timeoutTime);
    timeoutTime.tv_sec += 1;
    if (pthread_mutex_timedlock(&sync->mutex, &timeoutTime) != 0) {
        // throw_custom("Error locking mutex");
        pthread_mutex_unlock(&sync->mutex);
        puts("sync_notify: Error locking mutex");
        return;
    }

    sync->count = 1;
    if (pthread_cond_signal(&sync->condition) != 0) {
        throw_custom("Error signalling condition variable");
    }

    if (pthread_mutex_unlock(&sync->mutex) != 0) {
        throw_custom("Error unlocking mutex");
    }
}

void create_sync(struct Sync *sync, struct Arguments *args)
{
func_create_sync:
    memset(sync, 0, sizeof(struct Sync));

    // The identifier for the shared memory segment

    // Key for the memory segment
    char sys_touch_cmd[256];
    snprintf(sys_touch_cmd, sizeof(sys_touch_cmd), "touch %s", local_args.segment_key_path);
    system((const char *)sys_touch_cmd);
    key_t segment_key = generate_key(local_args.segment_key_path);

    // The size for the segment
    uint32_t raw_size = (args->size * WD_SHM_MAX_MUTEXES) + (sizeof(struct Mutex) * WD_SHM_MAX_MUTEXES);
    uint32_t page_size = ((raw_size / 4096) + 1) * 4096;

    /*
        The call that actually allocates the shared memory segment.
        Arguments:
            1. The shared memory key. This must be unique across the OS.
            2. The number of bytes to allocate. This will be rounded up to the OS'
                 pages size for alignment purposes.
            3. The creation flags and permission bits, where:
                 - IPC_CREAT means that a new segment is to be created
                 - IPC_EXCL means that the call will fail if
                     the segment-key is already taken (removed)
                 - 0666 means read + write permission for user, group and world.
        When the shared memory key already exists, this call will fail. To see
        which keys are currently in use, and to remove a certain segment, you
        can use the following shell commands:
            - Use `ipcs -m` to show shared memory segments and their IDs
            - Use `ipcrm -m <segment_id>` to remove/deallocate a shared memory segment
    */

    sync->segment_id = shmget(segment_key, page_size, IPC_CREAT | IPC_EXCL | 0666);
    if (errno == EEXIST) {
        // printf("[SHM] SHM exists\n");
        sync->segment_id = shmget(segment_key, page_size, 0666);
        // printf("segment_id=%d\n",sync->segment_id);

        if (sync->segment_id < 0) {
            printf("[SHM] SHM segment size mismatch for ID: %d\n", sync->segment_id);
            sync->segment_id = shmget(segment_key, 0, 0666);
            if (shmctl(sync->segment_id, IPC_RMID, NULL) < 0) {
                fprintf(stderr, "[SHM] Remove error for shmid=%d: %s\n", sync->segment_id, strerror(errno));
                exit(-1);
            }
            else {
                printf("[SHM] Deleted SHM segment ID: %d\n", sync->segment_id);
                usleep(10000UL);
                goto func_create_sync;
            }
        }
    }
    else {
        sync->mutex_created = 1;
    }

    if (sync->segment_id < 0) {
        usleep(1000000UL);
        perror("[SHM] Error allocating segment, retrying...");
        goto func_create_sync;
    }

    /*
Once the shared memory segment has been created, it must be
attached to the address space of each process that wishes to
use it. For this, we pass:
    1. The segment ID returned by shmget
    2. A pointer at which to attach the shared memory segment. This
         address must be page-aligned. If you simply pass NULL, the OS
         will find a suitable region to attach the segment.
    3. Flags, such as:
         - SHM_RND: round the second argument (the address at which to
             attach) down to a multiple of the page size. If you don't
             pass this flag but specify a non-null address as second argument
             you must ensure page-alignment yourself.
         - SHM_RDONLY: attach for reading only (independent of access bits)
shmat will return a pointer to the address space at which it attached the
shared memory. Children processes created with fork() inherit this segment.
*/
    sync->raw_shared_memory = (uint8_t *)shmat(sync->segment_id, NULL, 0);

    if ((intmax_t)sync->raw_shared_memory < 0) {
        throw_custom("Could not attach segment");
    }

    for (size_t i = 0; i < WD_SHM_MAX_MUTEXES; i++) {
        sync->shared_memory[i] = sync->raw_shared_memory + (i * args->size);
        sync->client_mutex[i] = (struct Mutex *)(sync->raw_shared_memory + (args->size * WD_SHM_MAX_MUTEXES)) + (2 * i);
        sync->server_mutex[i] = (struct Mutex *)(sync->raw_shared_memory + (args->size * WD_SHM_MAX_MUTEXES) + sizeof(struct Mutex)) + (2 * i);
    }

    if (is_server) {
        memset(sync->raw_shared_memory, 0, args->size);
        init_sync(sync);
        sync->mutex_created = 1;
    }
}

int shm_init(uint8_t _is_server, uint32_t shm_size, const char *segment_key_path)
{
    char *enable_shm = getenv("WD_SHM");
    if ((enable_shm == NULL) || (strcmp(enable_shm, "1") != 0)) {
        printf("[SHM] Disabled\n");
        return 0;
    }

    shm_is_enabled = 1;
    is_server = _is_server;
    local_args.size = shm_size; // Total size of the shared memory (it's not the same as packet size)
    local_args.segment_key_path[0] = 0;

    if (segment_key_path != NULL) {
        if (memcmp("/", segment_key_path, 1) != 0) {
            // C++ -> local_args.segment_key_path = "/tmp/"
            strcpy(local_args.segment_key_path, "/tmp/");
        }
        // C++ -> local_args.segment_key_path = local_args.segment_key_path + segment_key_path
        strncat(local_args.segment_key_path, segment_key_path, sizeof(local_args.segment_key_path));
        // printf("[SHM] Segment Key Path = %s\n", local_args.segment_key_path);
    }
    else {
        memcpy(local_args.segment_key_path, DEFAULT_SEGMENT_KEY_STR, sizeof(DEFAULT_SEGMENT_KEY_STR));
        // printf("[SHM] Using default Segment Key Path = %s\n", local_args.segment_key_path);
    }

    create_sync(&local_sync, &local_args); // Function comes from shm-_sync-common.c

    return 1;
}

void shm_set_max_timeout(uint16_t timeout)
{
    max_wait_time = timeout;
}

void shm_notify(uint16_t mutex_num)
{
    sync_notify((is_server ? local_sync.client_mutex[mutex_num] : local_sync.server_mutex[mutex_num]));
}

uint8_t shm_wait(uint16_t mutex_num)
{
    return sync_wait((is_server ? local_sync.server_mutex[mutex_num] : local_sync.client_mutex[mutex_num]));
}

void shm_cleanup()
{
    cleanup(&local_sync);
}

void *shm_mq_thread_handler(void *param)
{
    Mq_t *mq = (Mq_t *)param;
    // printf("[SHM] Started thread for channel %d\n", mq->channel);

    while (true) {
        // Function may block
        int res = mq_receive(mq->handler, (char *)mq->msg_buf, mq->attr.mq_msgsize, NULL);
        // If error is returned
        if (res < 0) {
            perror("[SHM] Error: mq_receive(): returned -1");
            usleep(10000000UL); // Wait 10ms
            continue;
        }

        // If nothing is returned
        if (!res)
            continue;

        // printf("[SHM]shm_mq_thread_handler: Received %d bytes\n", res);

        uint8_t *write_location = LFBB_WriteAcquire(&mq->msg_queue, res);
        if (!write_location) {
            printf("[SHM] Error: LFBB_WriteAcquire() returned NULL\n");
            usleep(10000);
            continue;
        }
        memcpy(write_location, mq->msg_buf, res);
        LFBB_WriteRelease(&mq->msg_queue, res);
    }

    return NULL;
}

int shm_mq_init(uint8_t channel, uint32_t shm_size, const char *queue_name, uint8_t use_thread)
{
    char *enable_shm = getenv("WD_SHM");
    if ((enable_shm == NULL) || (strcmp(enable_shm, "1") != 0)) {
        printf("[SHM] Disabled\n");
        return -1;
    }

    Mq_t *mq = (Mq_t *)calloc(1, sizeof(Mq_t));
    if (mq == NULL) {
        printf("[SHM] Mq_t malloc failed\n");
        return -1;
    }

    if (mqueues[channel] != NULL) {
        printf("[SHM] Mq_t channel %d already initialized, freeing it\n", channel);
        free(mqueues[channel]);
    }

    mq->channel = channel;

    mq->attr.mq_msgsize = WD_SHM_MAX_BUFFER_SIZE;
    mq->attr.mq_maxmsg = WD_MQUEUE_MAX_MESSAGES;
    mq->use_thread = use_thread;
    int flags = O_RDWR | O_CREAT;
    if (!use_thread)
        flags |= O_NONBLOCK;

    mq->attr.mq_flags = flags;

    // Ensure we have enough messages queue resources
    struct rlimit r;
    getrlimit(RLIMIT_MSGQUEUE, &r);

    rlim_t mqueue_bytes_min = mq->attr.mq_maxmsg * mq->attr.mq_msgsize;
    if (r.rlim_cur <= mqueue_bytes_min) {
        r.rlim_cur = mqueue_bytes_min;
    }
    if (r.rlim_max <= mqueue_bytes_min) {
        r.rlim_max = mqueue_bytes_min;
    }

    if (setrlimit(RLIMIT_MSGQUEUE, &r)) {
        free(mq);
        printf("[SHM] failed to set RLIMIT: %s", strerror(errno));
        return -1;
    }

    char cmd_buf[128];
    sprintf(cmd_buf, "echo %lu > /proc/sys/fs/mqueue/queues_max", mq->attr.mq_maxmsg);
    system(cmd_buf);
    system("ulimit -q unlimited");

    // Get string from the last '/' path delimiter
    char *n_queue_name = (char *)queue_name;
    char *ptr = strrchr(queue_name, '/');
    if (ptr)
        n_queue_name = ptr;

    snprintf(mq->key, sizeof(mq->key), "%s_%d", (n_queue_name != NULL ? n_queue_name : DEFAULT_MSGQ_KEY_STR), channel);

    mq->handler = mq_open(mq->key, flags, 0666, &mq->attr);
    if (mq->handler == -1) {
        free(mq);
        printf("[SHM] mq_open() failed: %s\n", strerror(errno));
        return -1;
    }

    if (mq->use_thread) {
        LFBB_Init(&mq->msg_queue, mq->msg_queue_buf, sizeof(mq->msg_queue_buf));
        pthread_create(&mq->thread, NULL, shm_mq_thread_handler, (void *)mq);
    }

    // Register the queue to the global queues channel array
    mqueues[channel] = mq;

    return 0;
}

mq_msg_t *shm_mq_recv(uint8_t channel)
{
    static __thread mq_msg_t mq_msg = {0};
    Mq_t *mq = mqueues[channel];

    if (__glibc_unlikely(mq == NULL))
        return NULL;

    // Using threads
    if (mq->use_thread) {
        // Use LFBB queue if use_thread is enabled
        if (mq->msg_queue_acquired) {
            LFBB_ReadRelease(&mq->msg_queue, mq_msg.msg_size);
            mq->msg_queue_acquired = 0;
        }
        size_t data_available;
        uint8_t *read_ptr = LFBB_ReadAcquire(&mq->msg_queue, &data_available);
        if (!read_ptr)
            return NULL;

        mq_msg.msg_buf = read_ptr;
        mq_msg.msg_size = data_available;
        mq->msg_size = data_available;
        mq->msg_queue_acquired = 1;
        // printf("[SHM] shm_mq_recv: Received %d bytes\n", mq_msg.msg_size);
        return &mq_msg;
    }

    // Not using threads
    if (!mq->msg_queue_acquired) {
        int res = mq_receive(mq->handler, (char *)mq->msg_buf, mq->attr.mq_msgsize, NULL);

        if (res <= 0)
            return NULL;

        mq->msg_size = res;
    }

    mq->msg_queue_acquired = 0;
    mq_msg.msg_size = mq->msg_size;
    mq_msg.msg_buf = mq->msg_buf;

    return &mq_msg;
}

size_t shm_mq_available(uint8_t channel)
{
    Mq_t *mq = mqueues[channel];

    if (__glibc_unlikely(mq == NULL))
        return 0;

    // Using threads
    if (mq->use_thread) {
        if (mq->msg_queue_acquired) {
            LFBB_ReadRelease(&mq->msg_queue, mq->msg_size);
            mq->msg_queue_acquired = 0;
        }
        size_t data_available;
        uint8_t *read_ptr = LFBB_ReadAcquire(&mq->msg_queue, &data_available);
        if (!read_ptr)
            return 0;

        // printf("[SHM] shm_mq_available: Received %lu bytes\n", data_available);

        return data_available;
    }

    // Not using threads
    if (!mq->msg_queue_acquired) {
        int res = mq_receive(mq->handler, (char *)mq->msg_buf, mq->attr.mq_msgsize, NULL);

        if (res <= 0)
            return 0;

        mq->msg_size = res;
        mq->msg_queue_acquired = 1;
    }

    return mq->msg_size;
}

int shm_mq_send(uint8_t channel, uint8_t *msg_buf, uint16_t msg_len)
{
    Mq_t *mq = mqueues[channel];

    if (__glibc_unlikely(mq == NULL)) {
        return -1;
    }

    if (msg_len > mq->attr.mq_msgsize) {
        return -1;
    }

    int res = mq_send(mq->handler, (char *)msg_buf, msg_len, 0);

    if (res == -1)
        printf("[SHM] Error: mq_send() failed: %s\n", strerror(errno));

    return res;
}