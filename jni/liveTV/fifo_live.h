/*
 * fifo_live.h: HTTP Live Streaming
 *				queue fifo buffer for stream data
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *			<ju_guofeng@odc.hoperun.com>
 *
 *	2013-01-11:	first version
 *
 */
#ifndef __FIFO_LIVE_H
#define __FIFO_LIVE_H

#include "util_live.h"

/* ---------------------------------------------------------- */

typedef struct HTTPPacket {
	unsigned char *data;
	unsigned int size;
} HTTPPacket;

typedef struct HTTPPacketList {
	HTTPPacket pkt;
	struct HTTPPacketList *next;
} HTTPPacketList;

typedef struct PacketQueue {
	HTTPPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	pthread_mutex_t mutex, speed_mutex;
	pthread_cond_t cond, speed_cond;
} PacketQueue;

/* ---------------------------------------------------------- */

int http_data_put_queue(PacketQueue *q, u8 *data, int size, int flag);
int http_data_get_queue(PacketQueue *q, u8 **data, int *size);
int http_data_queue_init(PacketQueue *q);
int http_data_queue_destroy(PacketQueue *q);

#endif
