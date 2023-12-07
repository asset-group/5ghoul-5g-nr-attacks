#include <inttypes.h>

struct _wd_modules_ctx_t;

// ---------------- Main API (declarations) ----------------

// Name
const char *__attribute__((weak)) module_name()
{
    return 0;
}

// Setup
int __attribute__((weak)) setup(void *p)
{
    return 0;
}

// TX PRE
int __attribute__((weak)) tx_pre_dissection(uint8_t *pkt_buf, int pkt_length, _wd_modules_ctx_t *ctx)
{
    return 0;
}
// TX POST
int __attribute__((weak)) tx_post_dissection(uint8_t *pkt_buf, int pkt_length, _wd_modules_ctx_t *ctx)
{
    return 0;
}

// RX PRE
int __attribute__((weak)) rx_pre_dissection(uint8_t *pkt_buf, int pkt_length, _wd_modules_ctx_t *ctx)
{
    return 0;
}
// RX POST
int __attribute__((weak)) rx_post_dissection(uint8_t *pkt_buf, int pkt_length, _wd_modules_ctx_t *ctx)
{
    return 0;
}