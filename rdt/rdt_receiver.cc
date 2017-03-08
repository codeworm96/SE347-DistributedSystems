/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  4 byte  ->|<- 4 byte ->|<- 1 byte ->|<-             the rest            ->|
 *       |<-  CRC32   ->|<-  seq   ->|<-  size  ->|<-             payload             ->|
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <deque>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_common.h"

static unsigned int expect_seq;
static std::deque<packet *> window;

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    expect_seq = 1;
    window.clear();
    window.resize(window_size);
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    if (crc32(pkt->data + 4, RDT_PKTSIZE - 4) == *(unsigned int *)pkt->data) { // ignore corrupted packets
        unsigned int seq = *(unsigned int *)(pkt->data + 4);
        if (seq >= expect_seq && seq - expect_seq < window_size) { // selective repeat
            if (window[seq - expect_seq] == NULL) {
                packet *p = (packet *)malloc(sizeof(packet));
                *p = *pkt;
                window[seq - expect_seq] = p;
            }
            while (window.front()) {
                ++expect_seq;
                packet *p = window.front();
                window.pop_front();
                window.push_back(NULL);

                /* construct a message and deliver to the upper layer */
                message *msg = (message *)malloc(sizeof(message));
                ASSERT(msg!=NULL);
                msg->size = p->data[8];
                msg->data = (char *)malloc(sizeof(char) * msg->size);
                ASSERT(msg->data!=NULL);
                memcpy(msg->data, p->data+header_size, msg->size);
                Receiver_ToUpperLayer(msg);
                /* don't forget to free the space */
                free(p);
                if (msg->data!=NULL) free(msg->data);
                if (msg!=NULL) free(msg);
            }
            /* ack */
            packet *ack = (packet *)malloc(sizeof(packet));
            memset(ack, 0, sizeof(packet));
            *(unsigned int *)(ack->data + 4) = expect_seq - 1;
            unsigned int crc = crc32(ack->data + 4, RDT_PKTSIZE - 4);
            *(unsigned int *)ack->data = crc;
            Receiver_ToLowerLayer(ack);
            free(ack);
        }
        else if (seq < expect_seq) { // ack is missing
            packet *ack = (packet *)malloc(sizeof(packet));
            memset(ack, 0, sizeof(packet));
            *(unsigned int *)(ack->data + 4) = expect_seq - 1;
            unsigned int crc = crc32(ack->data + 4, RDT_PKTSIZE - 4);
            *(unsigned int *)ack->data = crc;
            Receiver_ToLowerLayer(ack);
            free(ack);
        }
    }
}
