
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wd_shm.h"

// local_sync is a global structure which contains the shared memory and a mutex
// local_sync.shared_memory is your shared memory buffer to work with
// ---- Client code ------
int main()
{
	// Initialize shared memory
	// sleep(2);
	shm_init(WD_SHM_CLIENT, 8192, "/tmp/wshm_test"); // 8192 is the size of the shared memory
	// Receive packet from server and modify it
	while (1)
	{
		shm_wait(0); // Wait for server to send something

		int pkt_size = local_sync.shared_memory[0][0]; // Supose this indicates a packet size of 200 bytes
		if (pkt_size > 0)
		{
			memset(local_sync.shared_memory[0] + 1, '0', pkt_size); // write 200 bytes of character '1'
			printf("Client received and modified %d bytes\n", pkt_size);
		}
		shm_notify(0); // Notify the server of the packet to transmit
	}
}
