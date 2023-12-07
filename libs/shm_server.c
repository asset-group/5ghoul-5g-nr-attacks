
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wd_shm.h"

// local_sync is a global structure which contains the shared memory and a mutex
// local_sync.shared_memory is your shared memory buffer to work with
// ---- Server code ------
int main()
{
	// Initialize shared memory
	// sleep(2);
	shm_init(WD_SHM_CLIENT, 8192, "/tmp/wshm_test"); // 8192 is the size of the shared memory
	// Send packet
	int pkt_size = 200; // Supose this indicates a packet size of 200 bytes

	while (1)
	{
		local_sync.shared_memory[0][0] = pkt_size;				// first byte of shm represents packet size
		memset(local_sync.shared_memory[0] + 1, '1', pkt_size); // write 200 bytes of character '1'
		shm_notify(0);											// Notify the client of the packet to transmit
		shm_wait(0);											// Wait client to process it
		printf("Server done\n");
		sleep(1);
	}
}
