/*
 * fifo_live.c: HTTP Live Streaming
 *				queue fifo buffer for stream data
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *			<ju_guofeng@odc.hoperun.com>
 *
 *	2013-01-11:	first version
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "util_live.h"
#include "fifo_live.h"

/* moved to "fifo_live.h" */
#if 0
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
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} PacketQueue;

/* ---------------------------------------------------------- */
#endif

/* defined by user */
#if 0
/* FIXME waiting for a better place to be defined
 * each queue must define a different PacketQueue
 */
static PacketQueue httpq;
#endif

/* ---------------------------------------------------------- */

/* init queue and mutex & cond */
static int packet_queue_init(PacketQueue *q)
{
	hls_dbg("packet_queue_init\n");
	
	memset(q, 0, sizeof(PacketQueue));

	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->cond, NULL);
	
	pthread_mutex_init(&q->speed_mutex, NULL);
	pthread_cond_init(&q->speed_cond, NULL);
}

/* free queue buf and destroy mutex & cond */
static void packet_queue_destroy(PacketQueue *q)
{
	hls_dbg("packet_queue_destroy\n");

	/* free left data buff in FIFO */
	HTTPPacket pkt;
	HTTPPacketList *pkt1;
	int ret = 0;

	pthread_mutex_lock(&q->mutex);
	for (;;) {
		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			pkt = pkt1->pkt;
			free(pkt1);
			free(pkt.data);
			continue;
		} else {
			break;
			q->last_pkt = NULL;
			q->first_pkt = NULL;
			q->nb_packets = 0;
			q->size = 0;
		}
	}
	pthread_mutex_unlock(&q->mutex);
	
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->cond);
	
	pthread_mutex_destroy(&q->speed_mutex);
	pthread_cond_destroy(&q->speed_cond);
}

/*
 * no need memcpy
 */
static int http_link_pack(pkt)
{
	/* do nothing */
}

/*
 * need memcpy
 */
static int http_dup_packet(HTTPPacket *pkt)
{
	u8 *ptr;
	
	/* do not forget buffer free by the caller */
	ptr = malloc(pkt->size);
	memcpy(ptr, pkt->data, pkt->size);
	pkt->data = ptr;

	return 0;
}

static int packet_queue_put(PacketQueue *q, HTTPPacket *pkt, int flag)
{
	//hls_dbg("get int packet_queue_put\n");
	
	HTTPPacketList *pkt1;
	
	/* flag == 1, need memcpy */
	if (flag)
		/* memcpy data from m3u8 playlist thread */
		http_dup_packet(pkt);
	else
		/* no need to copy data */
		http_link_pack(pkt);

	pkt1 = malloc(sizeof(HTTPPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	pthread_mutex_lock(&q->mutex);

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;

	pthread_cond_signal(&q->cond);
	//hls_dbg("pthread_cond_signal\n");
	pthread_mutex_unlock(&q->mutex);
	
	//hls_dbg("get out packet_queue_put\n");
	
	return 0;
}

static int packet_queue_get(PacketQueue *q, HTTPPacket *pkt)
{
	//hls_dbg("get int packet_queue_get\n");
	
	HTTPPacketList *pkt1;
	int ret = 0;

	pthread_mutex_lock(&q->mutex);

	for (;;) {
		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			free(pkt1);
			ret = 1;
			break;
		} else {
			//hls_dbg("pthread_cond_wait\n");
			pthread_cond_wait(&q->cond, &q->mutex);
			/* FIXME */
			//ret = 0;
		}
	}

	pthread_mutex_unlock(&q->mutex);

	//hls_dbg("get out packet_queue_get\n");
	
	return ret;
}

/* ---------------------------------------------------------- */

/*
 * called by external, http data input queue
 */
int http_data_put_queue(PacketQueue *q, u8 *data, int size, int flag)
{
	HTTPPacket pkt;

	pkt.data = data;
	pkt.size = size;

	packet_queue_put(q, &pkt, flag);

	return 0;
}

int http_data_get_queue(PacketQueue *q, u8 **data, int *size)
{
	HTTPPacket pkt;
	
	int ret = 0;
	
	ret = packet_queue_get(q, &pkt);
	
	*data = pkt.data;
	*size = pkt.size;
	
	return ret;
}

int http_data_queue_init(PacketQueue *q)
{
	packet_queue_init(q);
	
	return 0;
}

int http_data_queue_destroy(PacketQueue *q)
{
	packet_queue_destroy(q);
	
	return 0;
}

