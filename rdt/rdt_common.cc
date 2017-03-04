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
            if ((int)(res ^ t) < 0) {
                res = (res << 1) ^ crc32_magic;
            }
            else {
                res = res << 1;
            }
            t <<= 1;
        }
    }
    return res;
}
