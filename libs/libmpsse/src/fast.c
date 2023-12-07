/*
 * Fast function for libmpsse.
 * For use only with SPI modes.
 *
 * Paolo Zambotti
 * 20 March 2013
 */

#include <string.h>
#include "mpsse_config.h"
#include "mpsse.h"
#include "support.h"

unsigned char fast_rw_buf[SPI_RW_SIZE + CMD_SIZE];

/* Builds a block buffer for the Fast* functions. For internal use only. */
static int fast_build_block_buffer(struct mpsse_context *mpsse, uint8_t cmd, unsigned char *data, size_t size, int *buf_size)
{
	int i = 0;
	uint16_t rsize = 0;

	*buf_size = 0;

	/* The reported size of this block is block size - 1 */
	rsize = size - 1;

	/* Copy in the command for this block */
	fast_rw_buf[i++] = cmd;
	fast_rw_buf[i++] = (rsize & 0xFF);
	fast_rw_buf[i++] = ((rsize >> 8) & 0xFF);

	/* On a write, copy the data to transmit after the command */
	if ((cmd == mpsse->tx || cmd == mpsse->txrx) && (i + size) <= sizeof(fast_rw_buf))
	{
		memcpy(fast_rw_buf + i, data, size);

		/* i == offset into buf */
		i += size;
	}

	*buf_size = i;

	return MPSSE_OK;
}

/*
 * Function for performing fast writes in MPSSE.
 *
 * @mpsse - libmpsse context pointer.
 * @data  - The data to write.
 * @size  - The number of bytes to write.
 *
 * Returns MPSSE_OK on success, MPSSE_FAIL on failure.
 */
int FastWrite(struct mpsse_context *mpsse, const char *data, size_t size)
{
	int buf_size = 0, txsize = 0;
	size_t n = 0;

	if (is_valid_context(mpsse))
	{
		if (mpsse->mode)
		{
			while (n < size)
			{
				txsize = size - n;
				if (txsize > mpsse->xsize)
				{
					txsize = mpsse->xsize;
				}

				if (fast_build_block_buffer(mpsse, mpsse->tx, (unsigned char *)(data + n), txsize, &buf_size) == MPSSE_OK)
				{
					if (raw_write(mpsse, fast_rw_buf, buf_size) == MPSSE_OK)
					{
						n += txsize;
					}
					else
					{
						return MPSSE_FAIL;
						break;
					}
				}
				else
				{
					return MPSSE_FAIL;
					break;
				}
			}

			if (n == size)
			{
				return MPSSE_OK;
			}
		}
	}

	return MPSSE_FAIL;
}

/*
 * Function for performing fast reads in MPSSE.
 *
 * @mpsse - libmpsse context pointer.
 * @data  - The destination buffer to read data into.
 * @size  - The number of bytes to read.
 *
 * Returns MPSSE_OK on success, MPSSE_FAIL on failure.
 */
int FastRead(struct mpsse_context *mpsse, char *data, size_t size)
{
	int rxsize = 0, data_size = 0;
	size_t n = 0;

	if (is_valid_context(mpsse))
	{
		if (mpsse->mode)
		{
			while (n < size)
			{
				rxsize = size - n;
				if (rxsize > mpsse->xsize)
				{
					rxsize = mpsse->xsize;
				}

				if (fast_build_block_buffer(mpsse, mpsse->rx, NULL, rxsize, &data_size) == MPSSE_OK)
				{
					if (raw_write(mpsse, fast_rw_buf, data_size) == MPSSE_OK)
					{
						n += raw_read(mpsse, (unsigned char *)(data + n), rxsize);
					}
					else
					{
						return MPSSE_FAIL;
						break;
					}
				}
				else
				{
					return MPSSE_FAIL;
					break;
				}
			}

			if (n == size)
			{
				return MPSSE_OK;
			}
		}
	}

	return MPSSE_FAIL;
}

/*
 * Function to perform fast transfers in MPSSE.
 *
 * @mpsse - libmpsse context pointer.
 * @wdata - The data to write.
 * @rdata - The destination buffer to read data into.
 * @size  - The number of bytes to transfer.
 *
 * Returns MPSSE_OK on success, MPSSE_FAIL on failure.
 */
int FastTransfer(struct mpsse_context *mpsse, const char *wdata, char *rdata, size_t size)
{
	int data_size = 0, rxsize = 0;
	size_t n = 0;

	if (is_valid_context(mpsse))
	{
		if (mpsse->mode >= SPI0 && mpsse->mode <= SPI3)
		{
			while (n < size)
			{
				rxsize = size - n;
				/* When sending and recieving, FTDI chips don't seem to like large data blocks. Limit the size of each block to SPI_TRANSFER_SIZE */
				if (rxsize > SPI_TRANSFER_SIZE)
				{
					rxsize = SPI_TRANSFER_SIZE;
				}

				if (fast_build_block_buffer(mpsse, mpsse->txrx, (unsigned char *)(wdata + n), rxsize, &data_size) == MPSSE_OK)
				{
					if (raw_write(mpsse, fast_rw_buf, data_size) == MPSSE_OK)
					{
						n += raw_read(mpsse, (unsigned char *)(rdata + n), rxsize);
					}
					else
					{
						return MPSSE_FAIL;
						break;
					}
				}
				else
				{
					return MPSSE_FAIL;
					break;
				}
			}

			if (n == size)
			{
				return MPSSE_OK;
			}
		}
	}

	return MPSSE_FAIL;
}
