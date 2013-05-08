/*
 * m3u8_live.c: HTTP Live Streaming
 *				m3u8 playlist parsing
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *			<ju_guofeng@odc.hoperun.com>
 *
 *	2013-01-11:	first version
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "m3u8_live.h"

#include "main_live.h"

/**
 * may need modify
 */
static void free_segment_list(struct variant *var)
{
	int i;
	for (i = 0; i < var->n_segments; i++) {
		if (var->segments[i])
			free(var->segments[i]);
	}
	hls_freep(&var->segments);
	/* reset the count */
	var->n_segments = 0;
}

static void free_variant_list(HLSContext *c)
{
	int i;

	for (i = 0; i < c->n_variants; i++) {
		struct variant *var = c->variants[i];
		free_segment_list(var);

		free(var);
	}
	/* ??? really need? 
	 * yes, malloc(or realloc) in hls_dynarray_add(...)
	 */
	hls_freep(&c->variants);
	c->n_variants = 0;
}

static struct variant *new_variant(HLSContext *c, int bandwidth,
		const char *url, const char *base)
{
	/* ###must use hls_mallocz (malloc and memset)### */
	struct variant *var = hls_mallocz(sizeof(struct variant));
	if (!var)
		return NULL;

	var->bandwidth = bandwidth;
	hls_make_absolute_url(var->url, sizeof(var->url), base, url);
	dynarray_add(&c->variants, &c->n_variants, var);

	return var;
}

static void handle_variant_args(struct variant_info *info, const char *key,
		int key_len, char **dest, int *dest_len)
{
	if (!strncmp(key, "BANDWIDTH=", key_len)) {
		*dest = info->bandwidth;
		*dest_len = sizeof(info->bandwidth);
	}
}

/** 
 * parsing the http url
 */
static int is_hls_http_url(char *s)
{
	if (!s)
		return HLS_FALSE;

	//hls_dbg("%s\n", s);

	if (strlen(s) < 4) {
		//hls_dbg("length < 4\n");
		return HLS_FALSE;
	}	

	if (strncmp(s, "http", strlen("http")) == 0) {
		//hls_dbg("true\n");
		return HLS_TRUE;
	} else {
		//hls_dbg("false\n");
		return HLS_FALSE;
	}
}

/**
 * get m3u8 playlist line text
 */
static int hls_get_line(char *s, int total_size, char *line, int size)
{
	int i = 0;

	char *end = s + total_size - 1;

	//if (!s)
	//	return HLS_ERR;

	//hls_dbg(">>>---------------------------\n");
	//hls_dbg("%s", s);
	//hls_dbg("---------------------------<<<\n");

	if (*s) {
		/* FIXME 
		 * sometimes the playlist end with '\r' && '\n' or '\n' only,
		 * but we must save '\n' only, or the url which send to the curl_http_download(...)
		 * will be parsed to an error url request with the format below (use wireshark tool)
		 * ----------------------------------------------------------------------
		 * Hypertext Transfer Protocol
		 *  GET /live/238baa4ac12f427896cecf12234159ef/1357992785.ts\r
		 * ----------------------------------------------------------------------
		 * but the correct fotmat is
		 * ----------------------------------------------------------------------
		 * Hypertext Transfer Protocol
		 *  GET /live/238baa4ac12f427896cecf12234159ef/1357992785.ts HTTP/1.1\r\n
		 * ----------------------------------------------------------------------
		 * and the error format will couse
		 -------------------------------------------- 
		 <html>
		 <head><title>400 Bad Request</title></head>
		 <body bgcolor="white">
		 <center><h1>400 Bad Request</h1></center>
		 <hr><center>nginx/5.1</center>
		 </body>
		 </html>
		 --------------------------------------------
		 * ###be careful###
		 */
		/* FIX BUG CUTV's HTTP context last line may end with '\0', not '\r' or '\n'*/
		while ((*s != '\n') && (*s != '\r') && (*s != '\0')) {
			line[i++] = *s++;
			if (s > end) {
				//hls_dbg("to the end - 1\n", line);
				line[i++] = '\n';
				line[i] = '\0';
				return total_size;
			}	
		}
		line[i++] = '\n';
		line[i] = '\0';

		/* remove each line's left '\r' and '\n' */
		while ((*s == '\r') || (*s == '\n')) {
			s++;
			i++;
			if (s > end) {
				/* to the end, but no '\r' or '\n' or '\0' found */
				//hls_dbg("to the end - 2\n", line);
				return total_size;
			}
		}

		//hls_dbg("%s", line);

		if (*s == '\0') {
			/* skip the last '\0' */
			//hls_dbg("char 0\n");
			return i;
		} else
			return i - 1;
	} else {
		/* if use the sizeof(), then the last char is the string end char '\0' */
		hls_dbg("hls string end\n");
		/* only return the '\0' size */
		line[0] = '\0';

		return 1;
	}
}

/**
 * put segment's url to the queue FIFO
 */
static int put_segment_to_queue(struct variant *var, int index)
{
	struct segment *seg = var->segments[index];

	if (is_hls_http_url(seg->url) == HLS_TRUE) {
		//hls_dbg("hls queue FIFO\n");
		http_data_put_queue(&var->m3u8_playlist_q, seg->url, sizeof(seg->url), HLS_TRUE);
	}

	/* FIXME where to free the seg->url's memory??? */

	return HLS_SUCCESS;
}

/** 
 * need a mutex lock
 * call libcurl to download http body data
 */
static int hls_open_curl_download(HLSContext *hls_ctx, char *url_live, http_payload_t *http_payload)
{
	/* call libcurl for http downloading */
	curl_http_download(url_live, http_payload);

	return 0;
}

/**
 * probe if we need a rederection
 */
//static int hls_url_detec(HLSContext *hls_ctx, struct http_payload *php, char **live_url)
static int hls_url_detec(HLSContext *hls_ctx, struct http_payload *host_url, struct http_payload *payload, char **live_url)
{
	struct http_payload *url_detec = host_url;
	struct http_payload *http_payload = payload;
	char **new_url = live_url;

	int flag, i = 0;

	char *free_url = NULL;

	/* sometimes(CNTV's source), a url(most : variant's url) may have url rederection more than once
	 * do detec until HTTP 200 OK */
	do {
		/* reset to zero */
		flag = 0;

		/* remember times of url rederection */
		i++;

		/* call libcurl for http downloading */
		//curl_http_header(*new_url, url_detec, &flag);
		curl_http_header(*new_url, url_detec, payload, &flag);

		if (flag == HLS_REDIRECT) {
			hls_dbg("new url >>> %s\n", url_detec->memory);
			/* variant or segment url request? */
			if (hls_ctx->n_variants)
				/* FIXME we only use variants[0] now, need a BandwidthAdaptation() func */
				hls_ctx->variants[0]->redection = HLS_REDIRECT;
			else
				hls_ctx->redection = HLS_REDIRECT;
			/* FIXME may cause memory lost
			 * if this happen, then the memory free job must be down by the new_url(not a good idea),
			 * because the url_detec->memory will be cover by another malloc(1) */
			*new_url = url_detec->memory;
			if (i > 1) {
				if (free_url) {
					free(free_url);
					free_url = NULL;
				}
			}
			/* make a copy of url_detec->memory */
			free_url = url_detec->memory;
		} else if (i > 1) {
			/* if second time, HTTP is 200 OK, not use url_detec->memory, put it back
			 * to the first *new_url, then free it in the parse_m3u8_playlist() bottom */
			free(url_detec->memory);
			url_detec->memory = *new_url;
		}
	} while (flag == HLS_REDIRECT);

	return 0;
}

/**
 * parse hls url from m3u8 playlist and put them to the queue fifo
 */
static int parse_m3u8_playlist(HLSContext *hls_ctx, char *url, struct variant *var, int redection)
{
	char line[1024];
	int total_size = 0;
	int read_size = 0;
	int line_size = 0;
	const char *ptr;
	int duration = 0, bandwidth = 0, is_segment = 0, is_variant = 0, tmp_seq_no = 0;

	char *http_context;
	struct http_payload http_payload;

	//hls_dbg("before parse_m3u8_playlist:\r\n%s\n", url);

	if (!var) {
		http_payload = hls_ctx->variant_payload;
	} else {
		http_payload = var->segment_payload;
	}
	//hls_dbg("open url >>> %s\n", url);
	/* no need download again when redection = HLS_FALSE) */
	if (redection == HLS_FALSE)
		hls_open_curl_download(hls_ctx, url, &http_payload);
	http_context = http_payload.memory;
	total_size = http_payload.size;
	//hls_dbg("hls_open_curl_download success\n");
	//hls_dbg("http_context = %d\n", total_size);

	if (var) {
		free_segment_list(var);
		var->finished = 0;
	}

	/* ... */
	while (total_size) {
		/* get each line text */
		line_size = hls_get_line(http_context + read_size, total_size, line, sizeof(line));
		total_size -= line_size;
		read_size += line_size;

		if (line[0] == '\0')
			break;

		//hls_dbg("%s", line);

		if (!strncmp(line, "#EXTM3U", 7)) {
			continue;
		} else if (hls_strstart(line, "#EXT-X-ALLOW-CACHE:", &ptr)) {
			/* TODO this tag catch up in CNTV's source */
			continue;
		} else if (hls_strstart(line, "#EXT-X-VERSION:", &ptr)) {
			/* get m3u8 version (from CUTV) */
			continue;
		} else if (hls_strstart(line, "#EXT-X-STREAM-INF:", &ptr)) {
			/* get stream info */
			struct variant_info info = {{0}};
			is_variant = 1;
			hls_parse_key_value(ptr, (hls_parse_key_val_cb) handle_variant_args,
					&info);
			bandwidth = atoi(info.bandwidth);
			//hls_dbg("bandwidth %d\n", bandwidth);
		} else if (hls_strstart(line, "#EXT-X-KEY:", &ptr)) {
			/* TODO get AES key*/
		} else if (hls_strstart(line, "#EXT-X-TARGETDURATION:", &ptr)) {
			/* get target duration */
			if (!var) {
				hls_dbg(">>>no variants, but segment is here, so create variant first	-- 1\n");
				var = new_variant(hls_ctx, 0, url, NULL);
				if (!var) {
					hls_err("new_variant failed!\n");
					return HLS_ERR;
				}
			}
			var->target_duration = atoi(ptr);
			//hls_dbg("target_duration %d\n", var->target_duration);
		} else if (hls_strstart(line, "#EXT-X-MEDIA-SEQUENCE:", &ptr)) {
			/* get media sequence */
			if (!var) {
				hls_dbg(">>>no variants, but segment is here, so create variant first	--2\n");
				var = new_variant(hls_ctx, 0, url, NULL);
				if (!var) {
					hls_err("new_variant failed!\n");
					return HLS_ERR;
				}
			}
			tmp_seq_no = 
				var->start_seq_no = atoi(ptr);
			//hls_dbg("target_seq_no %d\n", var->start_seq_no);
		} else if (hls_strstart(line, "#EXT-X-ENDLIST", &ptr)) {
			/* if finished, never happen in liveTV */
			if (var)
				var->finished = 1;
		} else if (hls_strstart(line, "#EXT-X-DISCONTINUITY", &ptr)) {
			/* TODO add for QQTV */
			continue;
		} else if (hls_strstart(line, "#EXTINF:", &ptr)) {
			/* get duration */
			is_segment = 1;		/* It applies only to the media segment that follows it */
			duration = atoi(ptr);
			//hls_dbg("duration %d\n", duration);
		} else if (hls_strstart(line, "#", NULL)) {
			/* do nothing continue */
			hls_dbg("possable a new m3u8 tag! continue\n");
			continue;
		} else if (line[0]) {
			//hls_dbg("is_hls_http_url\n");
			if (is_segment) {
				struct segment *seg;
				if (!var) {
					hls_dbg(">>>no variants, but segment is here, so create variant first	--3\n");
					var = new_variant(hls_ctx, 0, url, NULL);
					if (!var) {
						hls_err("new_variant failed!\n");
						return HLS_ERR;
					}
				}
				seg = malloc(sizeof(struct segment));
				if (!seg) {
					hls_err("malloc seg failed!\n");
					return HLS_ERR;
				}
				seg->duration = duration;
				/* give each segment's url an indivial id */
				seg->id = tmp_seq_no++;
				
				/* FIXME sohu if different from letv */
				if (hls_ctx->src_flag == LETV) {
					hls_make_absolute_url(seg->url, sizeof(seg->url), hls_ctx->org_url, line);
					//hls_dbg("************************\n");
					//hls_dbg("absolute_url ===> %s\n", seg->url);
				} else {
					hls_make_absolute_url(seg->url, sizeof(seg->url), url, line);
				}
				dynarray_add(&var->segments, &var->n_segments, seg);

				//hls_dbg("hls queue %s\n", seg->url);
				//hls_dbg("hls queue %s\n", line);
				is_segment = 0;
			} else if (is_variant) {
				hls_dbg("------------------------------\n");
				/* put variant bandwidth url to hls_ctx */
				if (!new_variant(hls_ctx, bandwidth, line, url)) {
					return HLS_ERR;
				}
				bandwidth = 0;
				is_variant = 0;
			}
		}
	}
	if (var) {
		/* FIXBUG reset the var->start_seq_no (sohu TV) */
		var->start_seq_no = var->segments[0]->id;

		/* may not a good idea
		 * free the seg_url_detec.memory */
		if (var->seg_url_detec.memory) {
			free(var->seg_url_detec.memory);
			var->seg_url_detec.memory = NULL;
		}
	}
fail:
	/* TODO do some close or free */
	if (http_context) {
		free(http_context);
		http_context = NULL;
	}

	return HLS_TRUE;
}

/**
 * download and parsing playlist
 * and compare changes, then put it to FIFO
 */
static int hls_update_playlist(HLSContext *hls_ctx, char *url, struct variant *var, int plls_flag)
{
	int n_changes = 0;
	int i = 0, count = 0;
	int seg_index = 0;
	int change_flag = HLS_NOCHANGE;

	char *new_url = var->url; 

	/* FIXME careful 
	 * CNTV's source will need a rederection between variant and segments,
	 * so we may need a url rederection
	 */
	/* ###!!!### don't modify var->url, only change the url(rederection url)
	 * called by parse_m3u8_playlist() */
	//hls_url_detec(hls_ctx, &var->seg_url_detec, &new_url);
	hls_url_detec(hls_ctx, &var->seg_url_detec, &var->segment_payload, &new_url);

	parse_m3u8_playlist(hls_ctx, new_url, var, plls_flag);

	count = var->n_segments;

	hls_dbg("segments[0].id => %d <-> cur_seq_no => %d <-> n_segments => %d\n", var->segments[0]->id, var->cur_seq_no, count);

	for (i = 0; i < count; i++) {
		if (var->cur_seq_no - var->segments[i]->id > 0)
			continue;
		else {
			change_flag = HLS_CHANGE;
			n_changes = count - i;
			break;
		}
	}

	/* if changed ? */
	if (change_flag == HLS_CHANGE) {
		/* merge the new playlist to the FIFO queue */
		for (i = 0; i < n_changes; i++) {
			seg_index = var->n_segments - n_changes + i;
			put_segment_to_queue(var, seg_index);
			var->cur_seq_no++;
		}
		return HLS_CHANGE;
	} else {
		//hls_dbg("playlist not changed\n");
		return HLS_NOCHANGE;
	}
}

/**
 * parsing if the body is a valid m3u8 playlist
 */
/* static */ int isHTTPLiveStreaming(char *s)
{
	const char *peek = s;

	int size = strlen(s);
	if (size < 7)
		return HLS_FALSE;

	if (memcmp(peek, "#EXTM3U", 7) != 0)
		return HLS_FALSE;

	peek += 7;
	size -= 7;

	/* Parse stream and search for
	 * EXT-X-TARGETDURATION or EXT-X-STREAM-INF tag, see
	 * http://tools.ietf.org/html/draft-pantos-http-live-streaming-04#page-8
	 */
	while (size--) {
		static const char *const ext[] = {
			"TARGETDURATION",
			"MEDIA-SEQUENCE",
			"KEY",
			"ALLOW-CACHE",
			"ENDLIST",
			"STREAM-INF",
			"DISCONTINUITY",
			"VERSION"
		};

		if (*peek++ != '#')
			continue;

		if (size < 6)
			continue;

		if (memcmp(peek, "EXT-X-", 6))
			continue;

		peek += 6;
		size -= 6;

		size_t i = 0;
		for (i = 0; i < HLS_ARRAY_SIZE(ext); i++) {
			int len = strlen(ext[i]);
			if (size < 0 || (size_t)size < len)
				continue;
			if (!memcmp(peek, ext[i], len))
				return HLS_TRUE;
		}
	}

	return HLS_FALSE;
}

/**
 * hls url probe
 * [same function with isHTTPLiveStreaming()]
 */
/* static */ int hls_url_probe(char *s)
{
	//hls_dbg("%s\n", url);

	if (strncmp(s, "#EXTM3U", 7))
		return HLS_FALSE;
	if (strstr(s, "#EXT-X-STREAM-INF:") ||
			strstr(s, "EXT-X-TARGETDURATION:") ||
			strstr(s, "EXT-X-MEDIA-SEQUENCE:"))
		return HLS_TRUE;

	return HLS_FALSE;
}

/**
 * reload the m3u8 playlist
 */
static int hls_reload_playlist(struct variant *var)
{
	/* do not forget we need an time interval to control the reload speed */
	struct variant *v = var;
	HLSContext *c = v->priv_data;
	int ret, i;

	/* from vlc project */
	double wait = 0.5;
	
	/* need a stop flag */
	while (v->exit_flag != HLS_EXIT) {
		mtime_t now = mdate();
		if (now >= v->playlist.wakeup) {
			/* reload the m3u8 playlist */
			if (hls_update_playlist(c, v->url, v, HLS_TRUE) == HLS_NOCHANGE) {
				/* No change in playlist, then backoff */
				v->playlist.tries++;
				if (v->playlist.tries == 1)
					wait = 0.5;
				else if (v->playlist.tries == 2)
					wait = 1;
				else if (v->playlist.tries >= 3) 
					wait = 2;
			} else {
				v->playlist.tries = 0;
				wait = 0.5;
			}

			/* determine next time to update playlist */
			v->playlist.last = now;
			v->playlist.wakeup = now + ((mtime_t)(v->target_duration * wait)
					* (mtime_t)1000000);
		}
		/* waiting for wakeup */
		mwait(v->playlist.wakeup);
	}
	
	hls_info(">>>>>>>>>>>>>>>>>>>[hls_reload_playlist thread] exit...\n");
}

/**
 * for first playlist update, then began reloading
 */
static int hls_first_playlist_queue(struct variant *var)
{
	int i = 0, index = 0;

	int n_put = var->n_segments - (var->cur_seq_no - var->start_seq_no);

	//hls_dbg("------->n_put = %d, cur_seq_no = %d\n", n_put, var->cur_seq_no);

	for (i = 0; i < n_put; i++) {
		index = var->cur_seq_no++ - var->start_seq_no;
		if (index > var->n_segments) {
			hls_dbg("arrary index too large, break\n");
			break;
		}
		put_segment_to_queue(var, index);
	}

	hls_dbg("after first playlist : cur_seq_no = %d\n", var->cur_seq_no);

	return 0;
}

//#define __ENABLE_FILE_SAVE_

/**
 * http live streaming download thread
 */
static void *hls_download(void *arg)
{
	/* FIXME very dangerous call
	 * seperate mode, free source when thread working is over
	 */
	//pthread_detach(pthread_self());
	
	HLSContext *hls_ctx = (HLSContext *)arg;
	/* FIXME we just use first bandwidth variant */
	struct variant *var = hls_ctx->variants[0];
	struct http_payload hls_ts_payload;

	hls_dbg("hls download thread are working now...\n");

#ifdef __ENABLE_FILE_SAVE_
	FILE *pout = NULL;
	pout = fopen("./live.ts", "w");
	if (pout == NULL)
		hls_err("open file failed\n");
#endif

	/** 
	 * get hls url from the queue fifo
	 */
	/* while(flag) {...} */

	u8 *http_url = NULL;
	int size;
	
#ifdef __ANDROID_
	int event_send = HLS_TRUE;
	int ret = 0;
#endif

	while (hls_ctx->exit_flag != HLS_EXIT) {
		/* get playlist from queue FIFO */
		http_data_get_queue(&var->m3u8_playlist_q, &http_url, &size);

		//hls_dbg("download ===> %s\n", http_url);

		/* first -> download ts data */
		hls_open_curl_download(hls_ctx, http_url, &hls_ts_payload);
	
#ifdef __ANDROID_
		/* TODO just for test libhls_event_callback */
		if (event_send) {
			hls_info("hls proxy buffing down, notice MediaPlayer start playing...\n");
			ret = hls_event_send(hls_ctx->priv_data, libhls_MediaPlayerPlaying);
			if (ret == HLS_ERR)
				goto fail;
			event_send = HLS_FALSE;
		}
#endif

		hls_dbg("put ts data to queue...%d\n", hls_ts_payload.size);

#ifdef __ENABLE_FILE_SAVE_
		/** 
		 * just for test
		 * write ts data to local file */
		fwrite(hls_ts_payload.memory, 1, hls_ts_payload.size, pout);

		/* do not forget free the memory */
		if (hls_ts_payload.memory) {
			free(hls_ts_payload.memory);
			hls_ts_payload.memory = NULL;
		}
#else
		/* ###careful###
		 * need speed control
		 * second -> queue ts data to FIFO */
		http_data_put_queue(&hls_ctx->m3u8_download_q, hls_ts_payload.memory, hls_ts_payload.size, HLS_FALSE);
		pthread_mutex_lock(&hls_ctx->m3u8_download_q.speed_mutex);
		if (hls_ctx->m3u8_download_q.nb_packets > 5)
			pthread_cond_wait(&hls_ctx->m3u8_download_q.speed_cond, &hls_ctx->m3u8_download_q.speed_mutex);
		pthread_mutex_unlock(&hls_ctx->m3u8_download_q.speed_mutex);
#endif

		/* free memory malloced by the queue FIFO */
		if (http_url) {
			free(http_url);
			http_url = NULL;
		}
	}
	
	/* notice hls playlist reload thread exit */
	var->exit_flag = HLS_EXIT;

#ifdef __ENABLE_FILE_SAVE_
	/* close the file */
	fclose(pout);
#endif

fail:
	hls_info(">>>>>>>>>>>>>>>>>>>[hls_download thread] exit...\n");

	pthread_exit(NULL);
}

/* -------------------------------------------------- */
/** 
 * hls main thread: get url from user
 * call libcurl doing http download
 * parse m3u8 playlist and get the ts data's real url
 * download live ts pkt to FIFO
 */
int hls_live_play(hls_callback_t hls_ctx_callback, void *user_data)
{
	int i;
	char *live_url = NULL;
	HLSContext *hls_ctx = NULL;
	
	hls_info("get into [hls_live_play] thread\n");
	
	/* create a hls_ctx object */
	hls_ctx = hls_mallocz(sizeof(HLSContext));
	if (hls_ctx == NULL) {
		hls_err("malloc HLSContext failed!\n");
		return HLS_ERR;
	}
	
	hls_dbg("======>pointer [hls_ctx] = %p\n", hls_ctx);
	
	/* call back to main_live.c, get some vars from hls_sys_t to hls_ctx */
	hls_ctx->priv_data = user_data;
	hls_ctx_callback(hls_ctx);
	
	/* get live url */
	live_url = hls_ctx->live_url;

	/* init fifo queue for m3u8 data download */
	http_data_queue_init(&hls_ctx->m3u8_download_q);
	
	sem_init(&hls_ctx->exit_sem, 0, 0);

#ifndef __ANDROID_
	/* notice mongoose proxy to start */
	sem_post(hls_ctx->proxy_start_sem);
#endif

	/* ------------------------------------------------------------------------- */
	/* FIXME before all, we need a url probe, if need rederection */
	//hls_url_detec(hls_ctx, &hls_ctx->var_url_detec, &live_url);
	hls_url_detec(hls_ctx, &hls_ctx->var_url_detec, &hls_ctx->variant_payload, &live_url);
	/**
	 * if live_url is indeed rederection, do not forget free the new url memory 
	 * the memory is in HLSContext struct url_detec.memory[same to live_url]
	 */
	/* ------------------------------------------------------------------------- */

	/* ###careful###
	 * sohu TV need http redirection
	 * first -> get variants */

	hls_dbg("first step...\n\n");
	hls_dbg("%s\n\n", live_url);

	parse_m3u8_playlist(hls_ctx, live_url, NULL, HLS_TRUE);

	/* FIXME some TV source(like sohu) has no variants, but we create it default int the 
	 * parse_m3u8_playlist() func, the variants is live_url(or it's rederection)
	 */

	if (hls_ctx->variants == 0) {
		hls_warn("Empty variants(first step)\n");
		goto fail1;
	}

	/* second -> get segment playlist */
	hls_dbg("second step...\n\n");

	if (HLS_TRUE) {	
		/* FIXME here we just use the first bandwidth url 
		 * need a BandwidthAdaptation() func */
		struct variant *v = hls_ctx->variants[0];

		/* do first url redetection detec (next we can call hls_update_playlist()) */
		char *new_url = v->url;
		/* ###!!!### don't modify var->url, only change the url called by parse_m3u8_playlist() */
		//hls_url_detec(hls_ctx, &v->seg_url_detec, &new_url);
		hls_url_detec(hls_ctx, &v->seg_url_detec, &v->segment_payload, &new_url);

		hls_dbg("%s\n", v->url);

		//if ((parse_m3u8_playlist(hls_ctx, v->url, v, HLS_TRUE) < 0))
		if ((parse_m3u8_playlist(hls_ctx, new_url, v, HLS_TRUE) < 0))
			goto fail1;
	}
#if 0
	if (hls_ctx->n_variants > 1 || hls_ctx->variants[0]->n_segments == 0) {
		for (i = 0; i < hls_ctx->n_variants; i++) {
			struct variant *v = hls_ctx->variants[i];
			if ((parse_m3u8_playlist(hls_ctx, m3u8_payload.memory, v) < 0))
				goto fail1;
		}
	}
#endif
	/* ------------------------------------------------------------------------- */
	/**
	 * next work, hls reload m3u8 playlist
	 * from ffmpeg hls.c
	 */
	if (hls_ctx->variants[0]->n_segments == 0) {
		hls_warn("Empty playlist(second step)\n");
		goto fail2;
	}

	/* if this isn't a live stream, calculate the total duration of the stream */
#if 0
	if (hls_cxt->variants[0]->finished) {
		int64_t duration = 0;
		for (i = 0; i < hls_ctx->variants[0]->n_segments; i++)
			duration += hls_ctx->variants[0]->segments[i]->duration;
		/* TODO give it to a global var */
	}
#endif

	/* if this is a live stream with more than 3 segments, start at the
	 * third last segment. */
	struct variant *v = hls_ctx->variants[0];
	v->cur_seq_no = v->start_seq_no;
	if ((!v->finished) && (v->n_segments > 3)) {
		v->cur_seq_no = v->start_seq_no + v->n_segments - 3;
		hls_dbg("------------->>>start_seq_no => %d <-> n_segments => %d\n", v->start_seq_no, v->n_segments);
		hls_dbg("------------->>>cur_seq_no = %d\n", v->cur_seq_no);
		hls_dbg("first playlist more than 3 segment, srart at the third last segment\n");
	}

	/* now, a variant has been created,
	 * init fifo queue for http url */
	http_data_queue_init(&v->m3u8_playlist_q);

	/* create the ts data download thread */
	if (hls_create_thread(&hls_ctx->th_hls_download, hls_ctx, hls_download) != HLS_SUCCESS)
		goto fail2;

	/* for first playlist update, then begin reloading */
	hls_first_playlist_queue(v);

	/* put hls_cxt to variant's pri_data */
	v->priv_data = hls_ctx;
	
	hls_dbg("get into playlist reloading loop...\n");

#ifndef __THREAD_EXIT_TEST_
	/* go to a while(...) */
	hls_reload_playlist(v);
#endif
	/* never return here except live down or change channel */
	/* ------------------------------------------------------------------------- */

	/* FIXME wait for mongoose data queue get thread down first */
	sem_wait(&hls_ctx->exit_sem);
	sem_destroy(&hls_ctx->exit_sem);

fail2:
#ifdef __THREAD_EXIT_TEST_
	//test return
	hls_ctx->exit_flag = HLS_EXIT;
#endif	
	/** 
	 * waiting for the hls downloading thread down
	 */
	pthread_join(hls_ctx->th_hls_download, NULL);
	
	if (hls_ctx->redection == HLS_REDIRECT) {
		free(hls_ctx->var_url_detec.memory);
		hls_ctx->var_url_detec.memory = NULL;
		hls_ctx->redection = 0;
	}

	/* destroy the m3u8 playlist queue */
	http_data_queue_destroy(&v->m3u8_playlist_q);
	hls_dbg("destroy m3u8_playlist_q success\n");

fail1:
	/* FIX BUG
	 * memory leak, find by valgrind tool 2013-02-22
	 * free the hls's var url detection memory malloced by libcurl(malloc(1))
	 */
	if (hls_ctx->var_url_detec.memory) {
		free(hls_ctx->var_url_detec.memory);
		hls_ctx->var_url_detec.memory = NULL;
	}

	/* do not forget we have not free the variant's memory yet */
	free_variant_list(hls_ctx);
	hls_dbg("free_variant_list success\n");
	
	/* do not forget free buf */
	http_data_queue_destroy(&hls_ctx->m3u8_download_q);
	hls_dbg("destroy m3u8_download_q success\n");
	
	/* free HLSContext *hls_ctx memory */
	free(hls_ctx);

	hls_info(">>>>>>>>>>>>>>>>>>>[hls_live_play thread] exit...\n");

	return 0;
}

