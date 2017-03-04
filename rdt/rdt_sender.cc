/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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

#include <deque>
#include <utility>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_common.h"

static std::deque<message *> pending_msg;
static std::deque<std::pair<packet *, double> > window;
static unsigned int seq;
static double last_update;

/* create a packet from pending messages
 * returns NULL if not possible
 */
packet* create_packet()
{
    if (pending_msg.empty()) {
        return NULL;
    }
    else {
        fprintf(stdout, "create packet\n");
        packet *res = (packet *)malloc(sizeof(packet));
        memset(res, 0, sizeof(packet));
        unsigned int off = header_size;
        while (!pending_msg.empty()) {
            message *msg = pending_msg.front();
            pending_msg.pop_front();

            int size = RDT_PKTSIZE - off;
            if (msg->size < size) {
                size = msg->size;
            }
            memcpy(res->data + off, msg->data, size);
            off += size;
            if (size == msg->size) {
                free(msg->data);
                free(msg);
            }
            else {
                msg->size = msg->size - size;
                char *d = (char *)malloc(sizeof(char) * msg->size);
                memcpy(d, msg->data + size, msg->size);
                free(msg->data);
                msg->data = d;
                pending_msg.push_front(msg);
                break;
            }
        }
        res->data[8] = off - header_size;
        *(unsigned int *)(res->data + 4) = seq;
        ++seq;
        unsigned int crc = crc32(*res);
        *(unsigned int *)res->data = crc;
        return res;
    }
}

/*
 * send some packet if possible
 */
void send_packet()
{
    while (window.size() < window_size) {
        packet *pkt = create_packet();
        if (pkt == NULL) {
            break;
        }
        else {
            Sender_ToLowerLayer(pkt);
            window.push_back(std::make_pair(pkt, timeout));
        }
    }
}

/*
 * update packet state
 */
void update_state()
{
    double last = last_update;
    last_update = GetSimulationTime();
    fprintf(stdout, "time: %lf, %d, %d\n", last_update, pending_msg.size(), window.size());
    double passed = last_update - last;
    double min = 10;
    for (unsigned int i = 0; i < window.size(); ++i) {
        if (window[i].second <= passed) {
            Sender_ToLowerLayer(window[i].first); // resend timeout packages
            window[i].second = timeout;
        }
        if (window[i].second < min) {
            min = window[i].second;
        }
    }
    if (min < 5) {
        Sender_StartTimer(min);
    }
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    pending_msg.clear();
    window.clear();
    seq = 1;
    last_update = 0.0;
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    message *m = (message *)malloc(sizeof(message));
    m->size = msg->size;
    m->data = (char *)malloc(sizeof(char) * m->size);
    memcpy(m->data, msg->data, m->size);

    pending_msg.push_back(m);

    update_state();
    send_packet();
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    //if (crc32(*pkt) == 0) { // ignore corrupted packets
        unsigned int ack = *(unsigned int *)(pkt->data + 4);
        while (!window.empty() && *(unsigned int *)(window.front().first->data + 4) <= ack) {
            free(window.front().first);
            window.pop_front();
        }
        update_state();
        send_packet();
        //}
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    update_state();
}
