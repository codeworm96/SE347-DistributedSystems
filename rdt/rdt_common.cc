/*
 * FILE: rdt_common.cc
 * DESCRIPTION: Common functions for sender & receiver
 */

#include "rdt_struct.h"
#include "rdt_common.h"

const unsigned int crc32_magic = 0x04C11DB7;

unsigned int crc32(const packet & pack)
{
    unsigned int res = *(unsigned int *)(pack.data + (RDT_PKTSIZE - 4));
    for (int i = RDT_PKTSIZE - 5; i >= 0; --i) {
        unsigned char t = pack.data[i];
        for (int j = 0; j < 8; ++j) {
            res = (res << 1) + ((t & 0x80) >> 7);
            t <<= 1;
            if (res >= crc32_magic) {
                res ^= crc32_magic;
            }
        }
    }
    return res;
}
