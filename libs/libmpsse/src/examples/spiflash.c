#include "mpsse_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpsse.h>

#define SIZE	0x1000			// Size of SPI flash device: 1MB
#define RCMD	"\x03\x00\x00\x00"	// Standard SPI flash read command (0x03) followed by starting address (0x000000)
#define FOUT	"flash.bin"		// Output file

int main(void)
{
	FILE *fp = NULL;
	char *data = NULL, *data1 = NULL;
	int retval = EXIT_FAILURE;
	struct mpsse_context *flash = NULL;
	
	if((flash = MPSSE(SPI0, ONE_MHZ, MSB)) != NULL && flash->open)
	{
		printf("%s initialized at %dHz (SPI mode 0)\n", GetDescription(flash), GetClock(flash));
		
		Start(flash);
		Write(flash, RCMD, sizeof(RCMD) - 1);
		data = Read(flash, SIZE);
		data1 = Read(flash, SIZE);
		Stop(flash);
		
		if(data)
		{
			fp = fopen(FOUT, "wb");
			if(fp)
			{
				fwrite(data, 1, SIZE, fp);
				fwrite(data1, 1, SIZE, fp);
				fclose(fp);
				
				printf("Dumped %d bytes to %s\n", SIZE, FOUT);
				retval = EXIT_SUCCESS;
			}

			free(data);
		}
	}
	else
	{
		printf("Failed to initialize MPSSE: %s\n", ErrorString(flash));
	}

	Close(flash);

	return retval;
}
