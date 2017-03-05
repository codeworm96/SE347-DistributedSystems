/*
 * FILE: rdt_common.h
 * DESCRIPTION: The header file for common functions of sender
 *     and receiver.
 */


#ifndef _RDT_COMMON_H_
#define _RDT_COMMON_H_

const unsigned int header_size = 9;
const unsigned int window_size = 20;
const double timeout = 0.3;
unsigned int crc32(char *data, unsigned int len);

#endif /* _RDT_COMMON_H_ */
