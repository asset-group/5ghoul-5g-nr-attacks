#include "mpsse_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpsse.h>
#include <unistd.h>

int main(void)
{
	struct mpsse_context *ds1305 = NULL;
	int sec = 0, min = 0, retval = EXIT_FAILURE;
	char *control = NULL, *seconds = NULL, *minutes = NULL;
	
	if((ds1305 = MPSSE(SPI3, ONE_HUNDRED_KHZ, MSB)) != NULL && ds1305->open)
	{
		SetCSIdle(ds1305, 0);

		printf("%s initialized at %dHz (SPI mode 3)\n", GetDescription(ds1305), GetClock(ds1305));
		
		Start(ds1305);
		Write(ds1305, "\x0F", 1);
		control = Read(ds1305, 1);
		Stop(ds1305);
		
		control[0] &= ~0x80;
		
		Start(ds1305);
		Write(ds1305, "\x8F", 1);
		Write(ds1305, control, 1);
		Stop(ds1305);

		free(control);

		while(1)
		{
			sleep(1);

			Start(ds1305);
			Write(ds1305, "\x00", 1);
			seconds = Read(ds1305, 1);
			Stop(ds1305);

			sec = (((seconds[0] >> 4) * 10) + (seconds[0] & 0x0F));

			Start(ds1305);
			Write(ds1305, "\x01", 1);
			minutes = Read(ds1305, 1);
			Stop(ds1305);

			min = (((minutes[0] >> 4) * 10) + (minutes[0] & 0x0F));

			printf("%.2d:%.2d\n", min, sec);

			free(minutes);
			free(seconds);
		}	
	}
	else
	{
		printf("Failed to initialize MPSSE: %s\n", ErrorString(ds1305));
	}

	Close(ds1305);

	return retval;
}
