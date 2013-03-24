/*
 * fifo_live.h: HTTP Live Streaming
 *				queue fifo buffer for stream data
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *			<ju_guofeng@odc.hoperun.com>
 *
 *	2013-01-14:	first version
 *
 */
#ifndef __M3U8_LIVE_H
#define __m3u8_LIVE_H

#include <stdio.h>
#include <stdlib.h>

#include "util_live.h"
#include "curl_live.h"
#include "util_live.h"
#include "fifo_live.h"

/* -------------------------------------------------- */
/* movied from m3u8_live.c */
struct variant_info {
	char bandwidth[20];
};

struct playlist {
	mtime_t last;			/* playlist last loaded */
	mtime_t wakeup;			/* next reload time */
	int tries;				/* times it was not changed */
};

struct segment {
	int duration;
	char url[HLS_MAX_URL_SIZE];
	int id;
};

struct variant {
	int bandwidth;
	char url[HLS_MAX_URL_SIZE];
	int redection;					/* url redection flag */
	
	int index;
	int finished;
	int target_duration;
	int start_seq_no;			/* FIXME must be reset by segment[0].id (eg. sohu TV) */
	int cur_seq_no;

	int n_segments;
	struct segment **segments;

	struct http_payload segment_payload;
	struct http_payload seg_url_detec;

	struct playlist playlist;
	
	volatile int exit_flag;

	PacketQueue m3u8_playlist_q;	/* for m3u8 playlist parsing */

	void *priv_data;				/* here use to save (HSLContext *) */
};

#define LETV_SRC "http://live.gslb.letv.com"

enum livetv_src_e {
	ERR = 0,
	CNTV,
	PPTV,
	QQTV,
	LETV,
	SOHU,
	CUTV,
	MIX,
};

typedef struct HLSContext {
	/* FIXME save request url, for LETV relative url cast */
	char *org_url;
	int src_flag;
	
	char *live_url;
	int redection;					/* url redection flag */
	
	int n_variants;
	struct variant **variants;

	pthread_mutex_t *hls_curl_lock;
	sem_t *proxy_start_sem;

	struct http_payload variant_payload;
	struct http_payload var_url_detec;

	/* thread for downloading ts data from playlist queue FIFO */
	pthread_t th_hls_download;

	volatile int exit_flag;
	sem_t exit_sem;
	
	PacketQueue m3u8_download_q;
	void *priv_data;				/* here use to save (hls_sys_t *) */
} HLSContext;
/* -------------------------------------------------- */

typedef void (*hls_callback_t)(struct HLSContext *hls_ctx);

/* hls m3u8 playlist parsing */
int hls_live_play(hls_callback_t hls_ctx_callback, void *user_data);


#endif
