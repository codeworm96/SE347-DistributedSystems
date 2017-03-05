/*
 * FILE: rdt_common.cc
 * DESCRIPTION: Common functions for sender & receiver
 */

#include "rdt_struct.h"
#include "rdt_common.h"

const unsigned int crc32_magic = 0x04C11DB7;

/* calculates crc32 with some math magic */
unsigned int crc32(char *data, unsigned int len)
{
    unsigned char byte;
    unsigned int res, mask;
    for (unsigned int i = 0; i < len; ++i) {
        byte = data[i];
        res = res ^ byte;
        for (int j = 0; j < 8; ++j) {
            mask = -(res & 1);
            res = (res >> 1) ^ (crc32_magic & mask);
        }
    }
    return ~res;
}
