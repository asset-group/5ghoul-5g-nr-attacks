#ifndef _T_defs_H_
#define _T_defs_H_

/* comment (and recompile everything) to not send time in events */
#define T_SEND_TIME

/* maximum number of arguments for the T macro */
#define T_MAX_ARGS 16

/* maximum size of a message - increase if needed */
#if BASIC_SIMULATOR
   /* let's have 100 RBs functional for the basic simulator */
#  define T_BUFFER_MAX (1024*64*2)
#else
#  define T_BUFFER_MAX (1024*64)
#endif

/* size of the local cache for messages (must be pow(2,something)) */
#if BASIC_SIMULATOR
   /* we don't need much space for the basic simulator */
#  define T_CACHE_SIZE 1024
#else
#  define T_CACHE_SIZE (8192 * 2)
#endif

/* maximum number of bytes a message can contain */
#ifdef T_SEND_TIME
#  define T_PAYLOAD_MAXSIZE (T_BUFFER_MAX-sizeof(int)-sizeof(struct timespec))
#else
#  define T_PAYLOAD_MAXSIZE (T_BUFFER_MAX-sizeof(int))
#endif

typedef struct {
  /* 'busy' is a bit field
   * bit 0: 1 means that slot is acquired by writer
   * bit 1: 1 means that slot is ready for consumption
   */
  volatile int busy;
  char buffer[T_BUFFER_MAX];
  int length;
} T_cache_t;

/* number of VCD functions (to be kept up to date! see in T_messages.txt) */
#define VCD_NUM_FUNCTIONS (228)

/* number of VCD variables (to be kept up to date! see in T_messages.txt) */
#define VCD_NUM_VARIABLES (177) 

/* first VCD function (to be kept up to date! see in T_messages.txt) */
#define VCD_FIRST_FUNCTION    ((uintptr_t)T_VCD_FUNCTION_RT_SLEEP)

/* first VCD variable (to be kept up to date! see in T_messages.txt) */
#define VCD_FIRST_VARIABLE    ((uintptr_t)T_VCD_VARIABLE_FRAME_NUMBER_TX0_ENB)

#endif /* _T_defs_H_ */
