/*
 * curl_live.h: for HTTP Live Streaming and m3u8 playlist parsing
 * 
 * juguofeng <ju_guofeng@hoperun.com>
 *
 * 2013-01-11:	first version
 *
 */
#ifndef __CURL_LIVE_H
#define __CURL_LIVE_H

#include <stdio.h>
#include <stdlib.h>

/* ---------------------------------------------------------- */
typedef struct http_payload {
	char *memory;
	size_t size;
} http_payload_t;

//int curl_http_download(char *url_live, http_payload_t *http_payload, int *flag);
int curl_http_download(char *url_live, http_payload_t *http_payload);

//int curl_http_header(char *url_live, http_payload_t *http_payload, int *flag);
int curl_http_header(char *url_live, http_payload_t *host_url, http_payload_t *http_payload, int *flag);

void curl_http_init(void);

void curl_http_uninit(void);

#endif
