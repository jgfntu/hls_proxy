// Copyright (c) 2004-2011 Sergey Lyubka
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>

#include "mongoose.h"

#include <sys/wait.h>
#include <unistd.h>
#define WINCDECL

#define MAX_OPTIONS 40

#ifndef __ANDROID_
static int exit_flag;
#else
extern sem_t hls_live_sem;
#endif

static char server_name[40];        // Set by init_server_name()
static struct mg_context *ctx;      // Set by start_mongoose()

static HLSContext *hls_ctx_g = NULL;

#ifndef __ANDROID_
static void WINCDECL signal_handler(int sig_num)
{
	exit_flag = sig_num;
}
#endif

static void die(const char *fmt, ...)
{
	va_list ap;
	char msg[200];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	hls_err("%s\n", msg);

	exit(EXIT_FAILURE);
}

static void verify_document_root(const char *root)
{
	const char *p, *path;
	char buf[PATH_MAX];
	struct stat st;

	path = root;
	if ((p = strchr(root, ',')) != NULL && (size_t) (p - root) < sizeof(buf)) {
		memcpy(buf, root, p - root);
		buf[p - root] = '\0';
		path = buf;
	}

	if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
		die("Invalid root directory: [%s]: %s", root, strerror(errno));
	}
}

static char *sdup(const char *str)
{
	char *p;
	if ((p = (char *) malloc(strlen(str) + 1)) != NULL) {
		strcpy(p, str);
	}
	return p;
}

static void set_option(char **options, const char *name, const char *value)
{
	int i;

	if (!strcmp(name, "document_root") || !(strcmp(name, "r"))) {
		verify_document_root(value);
	}

	for (i = 0; i < MAX_OPTIONS - 3; i++) {
		if (options[i] == NULL) {
			options[i] = sdup(name);
			options[i + 1] = sdup(value);
			options[i + 2] = NULL;
			break;
		}
	}

	if (i == MAX_OPTIONS - 3) {
		die("%s", "Too many options specified");
	}
}

static void init_server_name(void)
{
	snprintf(server_name, sizeof(server_name), "Mongoose web server v. %s",
			mg_version());
}

static void *mongoose_callback(enum mg_event ev, struct mg_connection *conn)
{
	hls_dbg("mongoose callback to proxy_live\n");
	
	if (ev == MG_EVENT_LOG) {
		hls_dbg("%s\n", (const char *)mg_get_request_info(conn)->ev_data);
	}
	
	if (ev == MG_NEW_REQUEST) {
		hls_info("mongoose proxy get queue from hls\n");
		mg_get_info_from_hls(conn, hls_ctx_g);
	}

	// Returning NULL marks request as not handled, signalling mongoose to
	// proceed with handling it.
	return NULL;
}

static void start_mongoose(void)
{
#ifndef __ANDROID_
	const char *options[] = {
		"listening_ports", "8080",
		"document_root", ".",
		NULL
	};
	/* Setup signal handler: quit on Ctrl-C */
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
#else
	const char *options[] = {
		"listening_ports", "8080",
		"document_root", "/mnt/sdcard/",
		NULL
	};
#endif

	/* Start Mongoose, and put hls_sys_t hls_glb->hls_ctx->m3u8_download_q to mongoose */
	ctx = mg_start(&mongoose_callback, NULL, (const char **) options);

	if (ctx == NULL) {
		die("%s", "Failed to start Mongoose.");
	}
}

int set_ctx_to_mg(void *data)
{
	HLSContext *ctx = (HLSContext *)data;
	hls_ctx_g = ctx;
	
	return 0;
}

/**
 * start mongoose thread
 */
int start_proxy(void)
{
	init_server_name();
	
	start_mongoose();

	hls_info("%s started on port(s) %s with web root [%s]\n",
			server_name, mg_get_option(ctx, "listening_ports"),
			mg_get_option(ctx, "document_root"));
			
#ifndef __ANDROID_
	while (exit_flag == 0) {
		sleep(1);
	}
	hls_info("\nExiting on signal %d, waiting for all threads to finish...\n",
		exit_flag);
#else
	/* wait here for destroy signal */
	sem_wait(&hls_live_sem);
	hls_info("\nExiting on signal sem_post, waiting for all threads to finish...\n");
#endif

	fflush(stdout);
	mg_stop(ctx);
	hls_info("%s", " done.\n");

	hls_info(">>>>>>>>>>>>>>>>>>>[mongoose proxy thread] exit...\n");

	return EXIT_SUCCESS;
}
