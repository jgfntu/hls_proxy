/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* Example source code to show how the callback function can be used to
 * download data into a chunk of memory instead of storing it in a file.
 */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/*
 * modified by juguofeng
 * for HTTP Live Streaming and m3u8 playlist parsing
 * 
 * juguofeng <ju_guofeng@hoperun.com>
 *
 * 2013-01-11:	first version
 *
 */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "curl_live.h"
#include "util_live.h"


/*
 * libcurl callback func for http payload data parsing
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	http_payload_t *mem = (http_payload_t *)userp;

	//hls_dbg("callback write...\n");	
	
	/* TODO
	 * waiting for a better method
	 * (and careful: if realloc failed, we may lost the original malloced memory,
	 * really not a good idea)
	 */
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		hls_err("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

/*
 * func for http downloading by libcurl
 */
int curl_http_download(char *url_live, http_payload_t *http_payload)
{
	CURL *curl_handle;
	
	//char *redirect_url;
	//int response_code;

	http_payload_t *chunk = http_payload;

	chunk->memory = malloc(1);  	/* will be grown as needed by the realloc above */
	chunk->size = 0;    			/* no data at this point */

	//curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url_live);
	
	/* FIXME for LETV's relative url cast 
	 * rederection call curl_http_header(), this fun has rederection inside */
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
	
	/* add http GET/ range context */
	curl_easy_setopt(curl_handle, CURLOPT_RANGE,"0-");
	
	/* some servers don't like requests that are made without a user-agent
     * field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
	//curl_easy_setopt(curl_handle, CURLOPT_FORBID_REUSE, 1);

	/* some servers don't like requests that are made without a user-agent
	   field, so we provide one */
	//curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* this fun will not return until the http payload is all download */
	curl_easy_perform(curl_handle);

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	/*
	 * Now, our chunk.memory points to a memory block that is chunk.size
	 * bytes big and contains the remote file.
	 *
	 * Do something nice with it!
	 *
	 * You should be aware of the fact that at this point we might have an
	 * allocated data block, and nothing has yet deallocated that data. So when
	 * you're done with it, you should free() it as a nice application.
	 */

	//hls_dbg("%lu bytes retrieved\n", (long)chunk->size);

	/* FIXME
	 * modified by juguofeng
	 * do not free it here only in this project 
	 */
	//if(chunk->memory)
	//	free(chunk->memory);
	
	/* we're done with libcurl, so clean it up */
	//curl_global_cleanup();

	return 0;
}

/**
 * detec http header, no http body data
 */
//int curl_http_header(char *url_live, http_payload_t *http_payload, int *flag)
int curl_http_header(char *url_live, http_payload_t *host_url, http_payload_t *http_payload, int *flag)
{
	CURL *curl_handle;
	
	char *redirect_url;
	//int response_code;
	long response_code;

	//http_payload_t *chunk = http_payload;
	http_payload_t *chunk = host_url;

	/* FIXME  may cause memory lost
	 * if http_payload->memory already has a memory, the free job must 
	 * give to the pointer which it give to */
	chunk->memory = malloc(1);  	/* will be grown as needed by the realloc above */
	chunk->size = 0;    			/* no data at this point */
	
	//hls_dbg("[curl get request]===>%s\n", url_live);

	/* FIXME 
	 * QQTV's source do not support HTTP HEAD/ request, so we must request HTTP GET/
	 * but in curl_http_header(), we do not send http body out */
	//http_payload_t body_duty;
	//body_duty.memory = malloc(1);
	//body_duty.size = 0;
	http_payload_t *chunk1 = http_payload;
	chunk1->memory = malloc(1);
	chunk1->size = 0;	

	//curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url_live);
	
	/* do not download body */
	//curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);
	
	/* FIXME we need get the rederection url, so not do it here */
	//curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	//curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&body_duty);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk1);
	
	/* add http GET/ range context */
	curl_easy_setopt(curl_handle, CURLOPT_RANGE,"0-");
	
	/* some servers don't like requests that are made without a user-agent
     * field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
	//curl_easy_setopt(curl_handle, CURLOPT_FORBID_REUSE, 1);

	/* some servers don't like requests that are made without a user-agent
	   field, so we provide one */
	//curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* this fun will not return until the http payload is all download */
	curl_easy_perform(curl_handle);
	
	/* get http response code */
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
	//hls_dbg("response_code = %d\n", response_code);
	
	/* for QQTV souct, release the body_duty.memory, do not send data out */
	//free(body_duty.memory);
	
	if (response_code == HLS_REDIRECT) {
		/* get redirect_url 2013-01-19 */
		curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &redirect_url);
		hls_dbg("redirect_url >>> %s\n", redirect_url);
		
		chunk->memory = realloc(chunk->memory, strlen(redirect_url) + 1);
		if (chunk->memory == NULL) {
			/* out of memory! */
			hls_err("not enough memory (realloc returned NULL)\n");
			exit(EXIT_FAILURE);
		}

		memcpy(&(chunk->memory[chunk->size]), redirect_url, strlen(redirect_url));
		chunk->size += strlen(redirect_url);
		chunk->memory[chunk->size] = 0;
		
		*flag = HLS_REDIRECT;
	} //else if (response_code != 200)
		//*flag = 0;

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	/*
	 * Now, our chunk.memory points to a memory block that is chunk.size
	 * bytes big and contains the remote file.
	 *
	 * Do something nice with it!
	 *
	 * You should be aware of the fact that at this point we might have an
	 * allocated data block, and nothing has yet deallocated that data. So when
	 * you're done with it, you should free() it as a nice application.
	 */

	//hls_dbg("%lu bytes retrieved\n", (long)chunk->size);

	/* FIXME
	 * modified by juguofeng
	 * do not free it here only in this project 
	 */
	//if(chunk->memory)
	//	free(chunk->memory);
	
	/* we're done with libcurl, so clean it up */
	//curl_global_cleanup();

	return 0;
}

/* ------------------------------------------------------------------------ */
/*
 * libcurl callback func for http payload data parsing
 */
static size_t probe_WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	http_payload_t *mem = (http_payload_t *)userp;

	//hls_dbg("callback write...\n");	
	
	/* TODO
	 * waiting for a better method
	 * (and careful: if realloc failed, we may lost the original malloced memory,
	 * really not a good idea)
	 */
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		hls_err("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	
	/* FIXME not a good idea */
	//return realsize;
}

/*
 * func for http probe downloading by libcurl
 */
int curl_probe_download(char *url_live, http_payload_t *http_payload)
{
	CURL *curl_handle;
	
	//char *redirect_url;
	//int response_code;

	http_payload_t *chunk = http_payload;

	chunk->memory = malloc(1);  	/* will be grown as needed by the realloc above */
	chunk->size = 0;    			/* no data at this point */

	//curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url_live);
	
	/* FIXME for LETV's relative url cast 
	 * rederection call curl_http_header(), this fun has rederection inside */
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, probe_WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
	
	/* add http GET/ range context */
	curl_easy_setopt(curl_handle, CURLOPT_RANGE,"0-1023");		/* ??? range do not work ??? */
	
	/* some servers don't like requests that are made without a user-agent
     * field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* this fun will not return until the http payload is all download */
	curl_easy_perform(curl_handle);

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	return 0;
}
/* ------------------------------------------------------------------------ */

void curl_http_init(void)
{
	curl_global_init(CURL_GLOBAL_ALL);
}

void curl_http_uninit(void)
{
	curl_global_cleanup();
}

