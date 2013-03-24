/*
 * main_live.c: HTTP Live Streaming
 *				call m3u8 parser and mongoose proxy
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *			<ju_guofeng@odc.hoperun.com>
 *
 *	2013-01-22:	first version
 *
 */
#ifndef __MAIN_LIVE_H
#define __MAIN_LIVE_H

#include "m3u8_live.h"

/* --------------------------------------------------------------------------- */

typedef struct hls_obj {
	char *live_url;
	
	HLSContext *hls_ctx;
	
	pthread_t th_hls_live;
	
	sem_t *proxy_start_sem;
	
	/* if we have two HLSContexts, but now we use only libcurl thread singlly */
	//pthread_mutex_t *hls_curl_lock;
	
	struct hls_obj *pre_hls_obj;		/* save the previous (hls_obj_t *) */
} hls_obj_t;

/* --------------------------------------------------------------------------- */

//int hls_proxy_init(char *hls_url, int url_size);
int hls_proxy_init(void);
int hls_proxy_uninit(void);

int hls_proxy_prepare(char *hls_url, int url_size);

//int hls_event_send(hls_obj_t *hls_obj, int event);
int hls_event_send(void *priv_data, int event);

#endif
