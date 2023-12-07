/*
 * Example code of using the low-latency FastRead and FastWrite functions (SPI and C only).
 * Contrast to spiflash.c.
 */
#define _GNU_SOURCE
#include "mpsse_config.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <mpsse.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <libusb.h>
#include <sched.h>
#include <semaphore.h>

#define IOPRIO_CLASS_SHIFT 13
#define IOPRIO_PRIO_VALUE(class, data) (((class) << IOPRIO_CLASS_SHIFT) | data)

#define SIZE 4096				// Size of SPI flash device: 1MB
#define RCMD "\x03\x00\x00\x00" // Standard SPI flash read command (0x03) followed by starting address (0x000000)
#define FOUT "flash.bin"		// Output file

#define SAMPLE_TESTS 1000000U

enum
{
	IOPRIO_CLASS_NONE,
	IOPRIO_CLASS_RT,
	IOPRIO_CLASS_BE,
	IOPRIO_CLASS_IDLE,
};

enum
{
	IOPRIO_WHO_PROCESS = 1,
	IOPRIO_WHO_PGRP,
	IOPRIO_WHO_USER,
};

static inline int ioprio_set(int which, int who, int ioprio)
{
	return syscall(SYS_ioprio_set, which, who, ioprio);
}

static void enable_rt_scheduler(uint8_t use_full_time)
{
	// Set schedule priority
	struct sched_param sp;
	int policy = 0;

	sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_t this_thread = pthread_self();

	int ret = sched_setscheduler(0, SCHED_FIFO, &sp);

	ret = pthread_getschedparam(this_thread, &policy, &sp);
	if (ret != 0)
	{

		return;
	}

	// Set IO prioriy
	ioprio_set(IOPRIO_WHO_PROCESS, 0, IOPRIO_PRIO_VALUE(IOPRIO_CLASS_RT, 0));

	if (use_full_time)
	{
		int fd = open("/proc/sys/kernel/sched_rt_runtime_us", O_RDWR);
		if (fd)
		{
			if (write(fd, "-1", 2) > 0)
				puts("/proc/sys/kernel/sched_rt_runtime_us = -1");
		}
	}
}

static void set_affinity_core(int core_num)
{
	if (core_num == -1)
		return;

	cpu_set_t cpuset;

	int ncores = 12;
	if (core_num < ncores)
	{
		CPU_ZERO(&cpuset);
		CPU_SET(core_num, &cpuset);
		int res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
		if (res != 0)
		{
			puts("set_affinity_core: failure");
			return;
		}

		printf("set_affinity_core: %d OK\n", core_num);
	}
	else
	{
		puts("set_affinity_core: invalid core number");
	}
}

sem_t mutex1;
sem_t mutex2;

void task1(void *p)
{
	puts("task1");
	while (1)
	{
		sem_wait(&mutex2);
		usleep(40);
		sem_post(&mutex1);
	}
}

int main(void)
{
	FILE *fp = NULL;
	char data[SIZE] = {0xAB};
	char *data_rx = NULL;
	char tx_data[SIZE] = {0xAB};
	memset(tx_data, 0x00, SIZE);
	int retval = EXIT_FAILURE;
	struct mpsse_context *flash = NULL;

	enable_rt_scheduler(1);
	// set_affinity_core(2);
	sem_init(&mutex1, 0, 0);
	sem_init(&mutex2, 0, 0);

	pthread_t thread_task1;
	pthread_create(&thread_task1, NULL, task1, NULL);

	for (size_t i = 0; i < SIZE; i++)
	{
		tx_data[i] = i;
	}

	if ((flash = MPSSE(SPI0, 7500000, MSB)) != NULL && flash->open)
	{
		printf("%s initialized at %dHz (SPI mode 0)\n", GetDescription(flash), GetClock(flash));

		ftdi_set_latency_timer(&flash->ftdi, 0);

		Start(flash);

		unsigned long time_in_micros1 = 0;
		unsigned long time_in_micros2 = 0;
		struct timeval tv;
		struct timeval tvf;
		uint64_t final = 0;
		uint32_t errors = 0;
		uint8_t expected_sequence = 0;
		struct timespec ns;
		ns.tv_sec = 0;

		tx_data[0] = 0xEE;
		FastTransfer(flash, tx_data, data, 32);
		usleep(100);
		expected_sequence = (uint8_t)data[31] + 1;

		for (uint32_t i = 0; i < SAMPLE_TESTS; i++)
		{
			gettimeofday(&tv, NULL);
			// FastTransfer(flash, tx_data, data, 32);
			// FastWrite(flash, tx_data, 32);
			// FastTransfer(flash, tx_data, data, 32);
			FastRead(flash, data, 32);
			gettimeofday(&tvf, NULL);
			// SetClock(flash, 7500000);

			// if (i % 2)
			// {
			// 	gettimeofday(&tv, NULL);
			// 	FastTransfer(flash, tx_data, data, 32);
			// 	gettimeofday(&tvf, NULL);
			// 	SetClock(flash, 30000000);
			// }
			// else
			// {
			// 	gettimeofday(&tv, NULL);
			// 	FastWrite(flash, tx_data, 32);
			// 	gettimeofday(&tvf, NULL);
			// 	SetClock(flash, 7500000);
			// }

			time_in_micros1 = (1000000 * tv.tv_sec) + tv.tv_usec;
			time_in_micros2 = (1000000 * tvf.tv_sec) + tvf.tv_usec;
			final = (time_in_micros2 - time_in_micros1);
			// printf("%d,%d\n", (uint8_t)data[31], final);
			// printf("%d\n", final);

			if ((uint8_t)data[31] != expected_sequence++)
			{
				puts("sequence error");
				// expected_sequence--;
				// errors++;
			}
			// printf("%s", data);
			sem_post(&mutex2);
			sem_wait(&mutex1);

			// if ((final < 130) && (i % 512))
			// 	usleep(15);
			// usleep(15);
		}

		Stop(flash);
	}
	else
	{
		printf("Failed to initialize MPSSE: %s\n", ErrorString(flash));
	}

	Close(flash);

	return retval;
}
