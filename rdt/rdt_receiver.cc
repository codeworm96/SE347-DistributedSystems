/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  4 byte  ->|<- 4 byte ->|<- 1 byte ->|<-             the rest            ->|
 *       |<-  CRC32   ->|<-   seq  ->|<-  size  ->|<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"
#include "rdt_common.h"

static unsigned int expect_seq;

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    expect_seq = 1;
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
    //   if (crc32(*pkt) == 0) { // ignore corrupted packets
        unsigned int seq = *(unsigned int *)(pkt->data + 4);
        fprintf(stdout, "received %d, expect %d\n", seq, expect_seq);
        if (seq == expect_seq) { // Go Back To N
            ++expect_seq;
            /* construct a message and deliver to the upper layer */
            message *msg = (message *)malloc(sizeof(message));
            ASSERT(msg!=NULL);
            msg->size = pkt->data[8];
            msg->data = (char *)malloc(sizeof(char) * msg->size);
            ASSERT(msg->data!=NULL);
            memcpy(msg->data, pkt->data+header_size, msg->size);
            Receiver_ToUpperLayer(msg);
            /* don't forget to free the space */
            if (msg->data!=NULL) free(msg->data);
            if (msg!=NULL) free(msg);
            /* ack */
            packet *ack = (packet *)malloc(sizeof(packet));
            memset(ack, 0, sizeof(packet));
            *(unsigned int *)(ack->data + 4) = seq;
            unsigned int crc = crc32(*ack);
            *(unsigned int *)ack->data = crc;
            Receiver_ToLowerLayer(ack);
            free(ack);
        }
        else if (seq < expect_seq) { // ack is missing
            packet *ack = (packet *)malloc(sizeof(packet));
            memset(ack, 0, sizeof(packet));
            *(unsigned int *)(ack->data + 4) = expect_seq - 1;
            unsigned int crc = crc32(*ack);
            *(unsigned int *)ack->data = crc;
            Receiver_ToLowerLayer(ack);
            free(ack);
        }
        /*
    }
    else {
    fprintf(stdout, "Corrupted packet\n");
    }
        */
}
