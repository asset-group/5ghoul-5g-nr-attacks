#include "includes.h"

#if (!defined(BYTE_ORDER) || !defined(BIG_ENDIAN) || !defined(LITTLE_ENDIAN))
#error "Unknown platform endianness!"
#endif

/**
 *  Switch from host byte order to network byte order
 */
uint128_t htonl128(uint128_t ui128)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    return swap_endianness(ui128);
#else
    return ui128;
#endif
}

/**
 *  Switch from network byte order to host byte order
 */
uint128_t ntohl128(uint128_t ui128)
{
    return htonl128(ui128);
}

/**
 *  Specialized method to swap the endianness on this platform
 */
uint128_t swap_endianness(uint128_t i128)
{
    union
    {
        __uint128_t ui128;
        uint32_t ui32[4];
    } _128_as_32, _32_as_128;

    _128_as_32.ui128 = i128;
    _32_as_128.ui32[3] = ntohl(_128_as_32.ui32[0]);
    _32_as_128.ui32[2] = ntohl(_128_as_32.ui32[1]);
    _32_as_128.ui32[1] = ntohl(_128_as_32.ui32[2]);
    _32_as_128.ui32[0] = ntohl(_128_as_32.ui32[3]);
    return _32_as_128.ui128;
}