#include "mpsse_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpsse.h>

int main(void)
{
	struct mpsse_context *io = NULL;
	int i = 0, retval = EXIT_FAILURE;

	io = MPSSE(GPIO, 0, 0);
	
	if(io && io->open)
	{
		for(i=0; i<10; i++)
		{
			PinHigh(io, GPIOL0);
			printf("GPIOL0 State: %d\n", PinState(io, GPIOL0, -1));
			sleep(1);
			
			PinLow(io, GPIOL0);
			printf("GPIOL0 State: %d\n", PinState(io, GPIOL0, -1));
			sleep(1);
		}
	
		retval = EXIT_SUCCESS;
	}
	else
	{
		printf("Failed to open MPSSE: %s\n", ErrorString(io));
	}
		
	Close(io);

	return retval;
}
