#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include "mpsse.h"

#ifdef __cplusplus
extern "C"
{
#endif
    int raw_write(struct mpsse_context *mpsse, unsigned char *buf, int size);
    int raw_read(struct mpsse_context *mpsse, unsigned char *buf, int size);
    void set_timeouts(struct mpsse_context *mpsse, int timeout);
    uint16_t freq2div(uint32_t system_clock, uint32_t freq);
    uint32_t div2freq(uint32_t system_clock, uint16_t div);
    unsigned char *build_block_buffer(struct mpsse_context *mpsse, uint8_t cmd, const unsigned char *data, size_t size, int *buf_size);
    int set_bits_high(struct mpsse_context *mpsse, int port);
    int set_bits_low(struct mpsse_context *mpsse, int port);
    int gpio_write(struct mpsse_context *mpsse, int pin, int direction);
    int is_valid_context(struct mpsse_context *mpsse);
#ifdef __cplusplus
}
#endif
#endif
