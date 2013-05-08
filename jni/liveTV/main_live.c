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
#include <stdio.h>
#include <stdlib.h>

#include "proxy_live.h"
#include "main_live.h"

/* -------------------------------------------------- */
/*				global struct						  */
/* -------------------------------------------------- */

#ifndef __ANDROID_
static sem_t proxy_start_sem;
#endif
static pthread_t th_hls_mongoose;
static int n_hls_channel;

#ifndef __ANDROID_
/* define an active live_thread chain list */
typedef struct thread_list {
	pthread_t **th;
	int n_ths;
	/* need a mutex lock, because active_live_th is a global var */
	pthread_mutex_t mutex;
} thread_list_t;

static thread_list_t active_live_th;

static int new_active_thread(thread_list_t *act_th, pthread_t *th)
{
	pthread_mutex_lock(&act_th->mutex);
	
	/* append hls_obj_th to global struct thread_list, need a mutex lock */
	dynarray_add(&act_th->th, &act_th->n_ths, th);
	
	pthread_mutex_unlock(&act_th->mutex);
	
	return 0;
}

static void free_actth_list(thread_list_t *act_th)
{
	/* thread obj list th was a memory of hls_obj_t, no need by free().
	 * it will free when free(hls_obj) in the thread func */
	
	hls_freep(&act_th->th);		/* malloced at dynarray_add() */
	act_th->n_ths = 0;
}

static void pthread_list_join(thread_list_t *act_th)
{
	int i;
	
	//hls_dbg("=================================================\n");
	//hls_dbg("--------act_th->n_ths = %d\n", act_th->n_ths);
	
	/* FIX BUG thread[0] has been joined, so skip it, find by valgrind tool 2013-02-22 */
	i = 1;

	for (i; i < act_th->n_ths; i++) {
		pthread_t th = *act_th->th[i];
		hls_info(">>>>>>>>>>>>>>>>>>>pthread join [%d]\n", i);
		pthread_join(th, NULL);
	}
}
#endif

#ifdef __ANDROID_
/* total exit sem_t */
sem_t hls_live_sem;
#endif

/* TODO */
static hls_obj_t *pre_hls_obj = NULL;

/* -------------------------------------------------- */

const char software_version[] = "0.5.0";

/**
 * The following is intended to be output as part of the main help text for
 * each program. ``program_name`` is thus the name of the program.
 */
#define REPORT_VERSION(program_name) \
  hls_dbg("\r--------------------------------------------------------------\n"	\
  		"\rHLS proxy version %s, %s built %s %s\n"	\
  		"\r--------------------------------------------------------------\n", \
         software_version, (program_name), \
         __DATE__, __TIME__)

/* -------------------------------------------------- */

int hls_which_live_src(char *url)
{
	/* TODO now just for LETV(CCTV1 & CCTV3...) */
	
	int ret = 0;
	
	if (!strncmp(url, LETV_SRC, strlen(LETV_SRC))) {
		ret = LETV;
		hls_dbg("Live TV source is [channel => LETV]\n");
	}
	
	return ret;
}

/**
 * callback by hls m3u8 parser module
 */
static void hls_ctx_callback(struct HLSContext *hls_ctx)
{
	/* get hls_sys_t back */
	hls_obj_t *hls_obj = hls_ctx->priv_data;
	/* get hls_ctx object back */
	hls_obj->hls_ctx = hls_ctx;
	
	/* FIXME save org live url */
	hls_ctx->src_flag = hls_which_live_src(hls_obj->live_url);
	hls_ctx->org_url = hls_obj->live_url;
	
	hls_ctx->live_url = hls_obj->live_url;
#ifndef __ANDROID_
	hls_ctx->proxy_start_sem = hls_obj->proxy_start_sem;
#endif
}

static void hls_obj_fill(hls_obj_t *hls_obj)
{
#ifndef __ANDROID_
	hls_obj->proxy_start_sem = &proxy_start_sem;
#endif
}

/*
 * m3u8 parsing thread
 */
static void *hls_live_thread(void *arg)
{
	hls_obj_t *hls_obj = (hls_obj_t *)arg;

	hls_info("get into %s\n", __func__);

#ifndef __ANDROID_
	/* put active live thread to chain list */
	new_active_thread(&active_live_th, &hls_obj->th_hls_live);
#endif

	/*
	 * in the while(...)
	 */
	hls_live_play(hls_ctx_callback, hls_obj);

	/* ###do some free### */
#ifdef __ANDROID_
	free(hls_obj->live_url);
#endif
	free(hls_obj);
	
	pthread_exit(NULL);
	
	hls_info("get out of %s\n", __func__);
}

/*
 * mongoose server thread
 */
static void *hls_mongoose_thread(void *arg)
{
	hls_obj_t *hls_obj = (hls_obj_t *)arg;

	hls_info("get into %s\n", __func__);

#ifndef __ANDROID_
	sem_wait(hls_obj->proxy_start_sem);
	set_ctx_to_mg(hls_obj->hls_ctx);
#else
	/* FIXME just put here */
	hls_event_send(NULL, /* init success */libhls_MediaPlayerOpening);	
#endif
	/*
	 * in the while(...)
	 */
#ifndef __THREAD_EXIT_TEST_
	/* give ts data queue to mongoose */
	start_proxy();
#endif

	pthread_exit(NULL);
	
	hls_info("get out of %s\n", __func__);
}

/**
 * create a virtual file for mongoose proxy
 */
int hls_create_virtual_file(char *file_name)
{
	FILE *vir_file = NULL;

	vir_file = fopen(file_name, "w+");
	if (vir_file == NULL) {
		hls_err("create virtual file failed\n");
		return HLS_ERR;
	}
	
	fclose(vir_file);
	
	return HLS_SUCCESS;
}

/**
 *	is flv streaming media ?
 */
static int isFLVLiveStreaming(char *s)
{
	const char *peek = s;

	int size = strlen(s);
	if (size < 3)
		return HLS_FALSE;

	if (memcmp(peek, "FLV", 3) != 0)
		return HLS_FALSE;
	
	return HLS_TRUE;
}

/**
 * probe streaming media protocol
 */
static int stream_protocol_probe(char *url)
{
	/* call curl get some data for probe */
	http_payload_t probe_payload;
	int ret = HLS_FALSE;
	
	curl_probe_download(url, &probe_payload);
	
	//hls_dbg("===> probe payload size = %d\n", probe_payload.size);
	//hls_dbg("%s\n", probe_payload.memory);
	
	/* this proxy only used for HLS m3u8 */
	if (/* m3u8 */isHTTPLiveStreaming(probe_payload.memory) == HLS_TRUE)
		ret = HLS_M3U8;
	else if (/* flv */isFLVLiveStreaming(probe_payload.memory) == HLS_TRUE)
		ret = HLS_FLV;
		
	free(probe_payload.memory);
	
	return ret;
}

#ifndef __ANDROID_
/**
 * main func by linux working, not support TV channel change
 */
/* ---------------------------------------------------------------- */
int main(int argc, char *argv[])
{
	/* ---------------------------------------------------------------- */
	/* TODO waiting a better place */
	
	/* PPTV */
	//char live_url[] = "http://web-play.pptv.com/web-m3u8-300164.m3u8";
	//char live_url[] = "http://web-play.pptv.com/web-m3u8-300166.m3u8";
	//char live_url[] = "http://web-play.pptv.com/web-m3u8-300169.m3u8";
	/* -------------------------------------------------------------------------------------------------------------- */
	/* next sohu */
	//char live_url[] = "http://gslb.tv.sohu.com/live?cid=60&type=hls";
	//char live_url[] = "http://gslb.tv.sohu.com/live?cid=59&type=hls";
	//char live_url[] = "http://gslb.tv.sohu.com/live?cid=51&type=hls";
	//char live_url[] = "http://gslb.tv.sohu.com/live?cid=53&type=hls";
	//char live_url[] = "http://gslb.tv.sohu.com/live?cid=3&type=hls";		//dongfang weishi
	/* -------------------------------------------------------------------------------------------------------------- */
	/* yangshi */
	//char live_url[] = "http://t.live.cntv.cn/m3u8/cctv-4.m3u8";
	//char live_url[] = "http://vdn.apps.cntv.cn/api/getLiveUrlCommonRedirectApi.do?channel=pa://cctv_p2p_hdcctv6&type=ipad";
	//char live_url[] = "http://rtmp.cntv.lxdns.com/live/cctv-fengyunzuqiu/playlist.m3u8";
	/* LETV */
	//char live_url[] = "http://live.gslb.letv.com/gslb?stream_id=cctv9&tag=live&ext=m3u8&sign=live_ipad";
	/* -------------------------------------------------------------------------------------------------------------- */
	/* QQTV */
	//char live_url[] = "http://zb.v.qq.com:1863/?progid=2084914015&ostype=ios";
	//char live_url[] = "http://zb.v.qq.com:1863/?progid=2674956498&ostype=ios";	//jiangsu weishi 1
	//char live_url[] = "http://zb.v.qq.com:1863/?progid=1258215255&ostype=ios";		//jiangsu weishi 2
	//char live_url[] = "http://liveipad.wasu.cn/jsws_ipad/z.m3u8";					//jiangsu weishi 3
	/* -------------------------------------------------------------------------------------------------------------- */
	/* mix source */
	//char live_url[] = "http://liveipad.wasu.cn/ahws_ipad/z.m3u8";
	//char live_url[] = "http://live.gslb.letv.com/gslb?stream_id=dongnan&tag=live&ext=m3u8&sign=live_ipad";
	//char live_url[] = "http://itv.hdpfans.com/live?vid=43";							/* not hls */
	//char live_url[] = "http://live.gslb.letv.com/gslb?stream_id=hunan&tag=live&ext=m3u8&sign=live_ipad";
	
	//char live_url[] = "http://114.112.34.103:82/live/5/45/3bde7498f8b64e738f7e5b4938415b3d.m3u";
	//char live_url[] = "http://url.52itv.cn/live/70473843713237686132364D";			/* do not work ??? */
	//char live_url[] = "http://114.112.34.103:82/live/5/45/cc535b8712c347ceb850ddd27ea60045.m3u";
	//char live_url[] = "http://live1.ms.tvb.com/tvb/tv/jade/033.m3u8";
	//char live_url[] = "http://live.gslb.letv.com/gslb?stream_id=bjkaku&tag=live&ext=m3u8&sign=live_ipad";
	//char live_url[] = "http://live.gslb.letv.com/gslb?stream_id=btv6_800&tag=live&ext=m3u8";
	/* -------------------------------------------------------------------------------------------------------------- */
	/* CUTV */
	//char live_url[] = "http://tsl2.hls.cutv.com/cutvChannelLive/2YeHjQB.m3u8";			/* this channel cause segment fault */
	//char live_url[] = "http://tsl5.hls.cutv.com/cutvChannelLive/Qg929n9.m3u8";
	
	/* some flv source */
	char live_url[] = "http://v4.cztv.com/channels/107/500.flv/live";
	//char live_url[] = "http://125.211.216.198/channels/hljtv/wypd/flv:sd/live?1342275792593";
	
	/* bug source test */
	//char live_url[] = "http://114.112.34.103:82/live/5/45/3bde7498f8b64e738f7e5b4938415b3d.m3u";
	
	/* -------------------------------------------------------------------------------------------------------------- */	
	
	REPORT_VERSION("httplive");
	
	/* init libcurl (for protocol probe) */
	curl_http_init();

	/* TODO first most important thing, probe the streaming media protocol */
	switch(stream_protocol_probe(live_url)) {
		case HLS_M3U8 : /* go on this proxy */
			hls_dbg("===> hls protocol\n");
			break;
		case HLS_FLV : /* use another method */
			hls_dbg("===> flv protocol\n");
			goto fail0;
			break;
		default:
			hls_dbg("===> unknow protocol\n");
			goto fail0;
			break;
	}
	
	/* create a virtual network file livetv.ts */
	char *file_name = "livetv.ts";
	if (hls_create_virtual_file(file_name) != HLS_SUCCESS)
		goto fail1;

	/* -------------------------------------------------------------------------------------------------------------- */
	/**
	 * step 1 -> create hls_glb
	 */
	hls_obj_t *hls_obj = hls_mallocz(sizeof(hls_obj_t));		/* FIXME do not forget memory free */
	if (hls_obj == NULL) {
		hls_err("malloc hls_hls_obj_t failed!\n");
		return HLS_ERR;
	}
	hls_obj_fill(hls_obj);
	hls_obj->pre_hls_obj = NULL;
	pre_hls_obj = hls_obj;

	/* init some global structs and flags */
	sem_init(&proxy_start_sem, 0, 0);
	pthread_mutex_init(&active_live_th.mutex, NULL);
	
	/**
	 * step 2 -> get url request
	 */
	/* put live_url to struct hls_glb */
	hls_info("Playback ===> [%s]\n", live_url);
	hls_obj->live_url = live_url;
	/* create the hls m3u8 playlist parsing thread */
	if (hls_create_thread(&hls_obj->th_hls_live, hls_obj, hls_live_thread) != HLS_SUCCESS)
		goto fail1;
	
	/**
	 * step 3 -> start mongoose proxy
	 */
	/* create the hls mongoose server thread */
	if (hls_create_thread(&th_hls_mongoose, hls_obj, hls_mongoose_thread) != HLS_SUCCESS)
		goto fail2;
	
	/**
	 * ...............................................
	 */
	
	/* waiting here for TV channel change */
	
	/* FIXME waiting for the hls_live_thread return, muse be current one, and will be 
	 * changed when new channel arrive 
	 * [may need an active thread_obj chain, and join in the for(;;)]
	 */
	/* do it first (because act_th->n_ths adds at the hls_live_thread func, sometimes main thread may
	 * has been running here, but the hls_live_thread has not been running, n_ths is 0) */
	hls_info(">>>>>>>>>>>>>>>>>>>pthread join [0]\n");
	pthread_join(hls_obj->th_hls_live, NULL);
	/* then try again, and will be more, because of channel change */
	pthread_list_join(&active_live_th);
	free_actth_list(&active_live_th);
	
	hls_info(">>>>>>>>>>>>>>>>>>>[waiting for mongoose thread] exit...\n");
	
	/* TODO waiting for the hls_mongoose_thread return */
	pthread_join(th_hls_mongoose, NULL);

fail2:
	/* destroy some global flag */
	sem_destroy(&proxy_start_sem);
	/* clean up libcurl */
	curl_http_uninit();
	pthread_mutex_destroy(&active_live_th.mutex);
	
	hls_info(">>>>>>>>>>>>>>>>>>>[main thread] exit...\n");
	
	return HLS_SUCCESS;
	
fail1:
	/* destroy some global flag */
	sem_destroy(&proxy_start_sem);
	/* clean up libcurl */
	curl_http_uninit();
	pthread_mutex_destroy(&active_live_th.mutex);
	return HLS_ERR;

fail0:
	/* clean up libcurl */
	curl_http_uninit();
	return HLS_ERR;
}
#else
/**
 * called by java, first init an original TV channel, and start the 
 * mongoose proxy, when data is arrive, notice the mediaPlayer playbacking.
 * the call hls_proxy_prepare() for channel changing.
 */
//int hls_proxy_main(char *hls_url, int url_size)
int hls_proxy_main(void)
{
	REPORT_VERSION("libhls_jni");
	
	char *file_name = "/mnt/sdcard/livetv.ts";
	if (hls_create_virtual_file(file_name) != HLS_SUCCESS)
		goto fail1;

	/* init libcurl */
	curl_http_init();
	sem_init(&hls_live_sem, 0, 0);

	/* [android mode] start mongoose web server */
	if (hls_create_thread(&th_hls_mongoose, NULL, hls_mongoose_thread) != HLS_SUCCESS)
		goto fail1;

	/* waiting for the hls_mongoose_thread return */
	pthread_join(th_hls_mongoose, NULL);

	hls_info(">>>>>>>>>>>>>>>>>>>[main thread] exit...\n");
	
	sem_destroy(&hls_live_sem);
	curl_http_uninit();

	return HLS_SUCCESS;

fail1:
	return HLS_ERR;
}
#endif
/* ---------------------------------------------------------------- */	

#ifdef __ANDROID_
/**
 * prepare the next TV channel's data, then notice MediaPlayer to close the
 * current channel, and setDataSource again to play the next channel
 */ 
int hls_proxy_prepare(char *hls_url, int url_size)
{
	char *live_url = hls_mallocz(HLS_MAX_URL_SIZE);
	
	memcpy(live_url, hls_url, url_size);
	
	hls_info("hls start preparing ===> %s\n", live_url);
	
	/**
	 * step 1 -> create hls_obj
	 */
	hls_obj_t *hls_obj = hls_mallocz(sizeof(hls_obj_t));
	if (hls_obj == NULL) {
		hls_err("malloc hls_hls_obj_t failed!\n");
		return HLS_ERR;
	}
	hls_obj_fill(hls_obj);
	hls_obj->pre_hls_obj = pre_hls_obj;		/* first one if NULL */
	pre_hls_obj = hls_obj;
	
	/**
	 * step 2 -> get url request
	 */
	/* put live_url to struct hls_obj */
	hls_info("Playback ===> [%s]\n", live_url);
	hls_obj->live_url = live_url;
	/* create the hls m3u8 playlist parsing thread */
	if (hls_create_thread(&hls_obj->th_hls_live, hls_obj, hls_live_thread) != HLS_SUCCESS)
		return HLS_ERR;
	
	return HLS_SUCCESS;
}

int hls_event_send(void *priv_data, int event)
{
	int ret;

	switch (event) {
		case libhls_MediaPlayerPlaying :
			hls_info(">>>>>>>>>>>>>>>>>>>[stop the previous channel thread]\n");
			/* get previous hls_obj */
			hls_obj_t *hls_obj = (hls_obj_t *)priv_data;
#ifndef __ANDROID_	
			if (hls_obj->pre_hls_obj) {	/* if hls_obj->pre_hls_obj == NULL, means the first channel */
#endif
				/* get previous hls_ctx */
				//HLSContext *hls_ctx = hls_obj->pre_hls_obj->hls_ctx;

				/* kill current hls thread and connection */
				//if (hls_ctx) {
				/* FIXME do not set it here, let mongoose do it */
				//hls_ctx->exit_flag = HLS_EXIT;
				/* set new hls_obj to mongoose */
				set_ctx_to_mg(hls_obj->hls_ctx);
				//}
#ifndef __ANDROID_	
			}
#endif
			break;
		case libhls_MediaPlayerOpening :
			hls_info(">>>>>>>>>>>>>>>>>>>[libhlsjni has initialized]\n");
			break;
		default :
			hls_warn("not support event yet!!!\n");
			break;
	}

	/* notice for new connection */
	ret = hls_event_callback(event);

	return ret;
}

/**
 * post a sem to mongoose to stop mongoose, then total elf return
 */
int hls_proxy_uninit(void)
{
	sem_post(&hls_live_sem);
}

/* ----------------------------------------- */
pthread_t th_main;

typedef struct default_url {
	char *hls_url;
	int url_size;
} default_url_t;

/**
 * create main thread
 */
static void *hls_main_thread(void *arg)
{
	hls_info("get into %s\n", __func__);
	
	hls_proxy_main();

	hls_info("get out of %s\n", __func__);

	pthread_exit(NULL);
	
}

/**
 * create a main thread, called by java
 */
int hls_proxy_init(void)
{
	hls_create_thread(&th_main, NULL, hls_main_thread);

	return 0;
}
#endif

