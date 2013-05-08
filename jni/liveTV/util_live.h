/*
 * util_live.h: util method for hls proxy project
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *			<ju_guofeng@odc.hoperun.com>
 *
 *	2013-01-11:	first version
 *	2013-01-12:	add some string util from ffmpeg libavutil
 */

#ifndef __UTIL_LIVE_H
#define __UTIL_LIVE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "config_jni.h"

#ifdef __ANDROID_
#include "libhls_jni.h"
#include "libhls_event.h"
#endif

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;

#define HLS_ERR		-1
#define HLS_FALSE	0
#define HLS_TRUE	1
#define HLS_NOCHANGE 2
#define HLS_CHANGE	3
#define HLS_SUCCESS 4
#define HLS_REDIRECT 302
#define HLS_EXIT HLS_TRUE
#define HLS_M3U8	5
#define HLS_FLV		6

/* debug test */
//#define __THREAD_EXIT_TEST_

/* ----------------------------------------------------------------- */

#define __HLS_LOG_ON
#ifndef __ANDROID_
/* TODO 
 * (better moving somewhere else, and can be modified for android)
 * log print system
 */
#ifdef __HLS_LOG_ON
	#define hls_dbg(fmt, args...) printf(fmt, ##args)
	#define hls_info(fmt, args...) printf(fmt, ##args)
	#define hls_warn(fmt, args...) printf(fmt, ##args)
	#define hls_err(fmt, args...) fprintf(stderr, fmt, ##args)
#else
	#define hls_dbg(fmt, args...)
	#define hls_info(fmt, args...)
	#define hls_warn(fmt, args...)
	#define hls_err(fmt, args...)
#endif
#else
#include <android/log.h>
#ifndef LOG_TAG
#define LOG_TAG "hlsjni"
#endif
	#define hls_dbg(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
	#define hls_err(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
	#define hls_info(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
	#define hls_warn(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif

/* hls thread */
int hls_create_thread(pthread_t *thread, void *arg, void *thread_func);

/* ----------------------------------------------------------------- */

/* ----------------------------------------------------------------- */
/* 						from vlc project							 */
/* ----------------------------------------------------------------- */
#define HLS_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
/* vlc use 4096 */
#define HLS_MAX_URL_SIZE 1024	/* 4096 */

typedef int64_t mtime_t;

/* ----------------------------------------------------------------- */

/**
 * Waits until a deadline (possibly later due to OS scheduling).
 * @param deadline timestamp to wait for (see mdate())
 */
void mwait(mtime_t deadline);

/* ----------------------------------------------------------------- */
/* 						from ffmpeg libavutil						 */
/* ----------------------------------------------------------------- */
/**
 * Return non-zero if pfx is a prefix of str. If it is, *ptr is set to
 * the address of the first character in str after the prefix.
 *
 * @param str input string
 * @param pfx prefix to test
 * @param ptr updated if the prefix is matched inside str
 * @return non-zero if the prefix matches, zero otherwise
 */
int hls_strstart(const char *str, const char *pfx, const char **ptr);

/**
 * Return non-zero if pfx is a prefix of str independent of case. If
 * it is, *ptr is set to the address of the first character in str
 * after the prefix.
 *
 * @param str input string
 * @param pfx prefix to test
 * @param ptr updated if the prefix is matched inside str
 * @return non-zero if the prefix matches, zero otherwise
 */
int hls_stristart(const char *str, const char *pfx, const char **ptr);

/**
 * Locate the first case-independent occurrence in the string haystack
 * of the string needle.  A zero-length string needle is considered to
 * match at the start of haystack.
 *
 * This function is a case-insensitive version of the standard strstr().
 *
 * @param haystack string to search in
 * @param needle   string to search for
 * @return         pointer to the located match within haystack
 *                 or a null pointer if no match
 */
char *hls_stristr(const char *haystack, const char *needle);

/**
 * Copy the string src to dst, but no more than size - 1 bytes, and
 * null-terminate dst.
 *
 * This function is the same as BSD strlcpy().
 *
 * @param dst destination buffer
 * @param src source string
 * @param size size of destination buffer
 * @return the length of src
 *
 * @warning since the return value is the length of src, src absolutely
 * _must_ be a properly 0-terminated string, otherwise this will read beyond
 * the end of the buffer and possibly crash.
 */
size_t hls_strlcpy(char *dst, const char *src, size_t size);

/**
 * Append the string src to the string dst, but to a total length of
 * no more than size - 1 bytes, and null-terminate dst.
 *
 * This function is similar to BSD strlcat(), but differs when
 * size <= strlen(dst).
 *
 * @param dst destination buffer
 * @param src source string
 * @param size size of destination buffer
 * @return the total length of src and dst
 *
 * @warning since the return value use the length of src and dst, these
 * absolutely _must_ be a properly 0-terminated strings, otherwise this
 * will read beyond the end of the buffer and possibly crash.
 */
size_t hls_strlcat(char *dst, const char *src, size_t size);

/* ----------------------------------------------------------------- */

/**
 * Callback function type for hls_parse_key_value.
 *
 * @param key a pointer to the key
 * @param key_len the number of bytes that belong to the key, including the '='
 *                char
 * @param dest return the destination pointer for the value in *dest, may
 *             be null to ignore the value
 * @param dest_len the length of the *dest buffer
 */
typedef void (*hls_parse_key_val_cb)(void *context, const char *key,
                                    int key_len, char **dest, int *dest_len);
                                    
/**
 * Parse a string with comma-separated key=value pairs. The value strings
 * may be quoted and may contain escaped characters within quoted strings.
 *
 * @param str the string to parse
 * @param callback_get_buf function that returns where to store the
 *                         unescaped value string.
 * @param context the opaque context pointer to pass to callback_get_buf
 */
void hls_parse_key_value(const char *str, hls_parse_key_val_cb callback_get_buf,
                        void *context);

/*
 * Convert a relative url into an absolute url, given a base url.
 *
 * @param buf the buffer where output absolute url is written
 * @param size the size of buf
 * @param base the base url, may be equal to buf.
 * @param rel the new url, which is interpreted relative to base
 */
void hls_make_absolute_url(char *buf, int size, const char *base,
                          const char *rel);

/**
 * Add an element to a dynamic array.
 *
 * @param tab_ptr Pointer to the array.
 * @param nb_ptr  Pointer to the number of elements in the array.
 * @param elem    Element to be added.
 */
void hls_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem);


#define dynarray_add(tab, nb_ptr, elem)\
do {\
    __typeof__(tab) _tab = (tab);\
    __typeof__(elem) _elem = (elem);\
    (void)sizeof(**_tab == _elem); /* check that types are compatible */\
    hls_dynarray_add(_tab, nb_ptr, _elem);\
} while(0)

int64_t hls_gettime(void);

#define mdate(void) hls_gettime(void)

/**
 * Allocate a block of size bytes with alignment suitable for all
 * memory accesses (including vectors if available on the CPU) and
 * zero all the bytes of the block.
 * @param size Size in bytes for the memory block to be allocated.
 * @return Pointer to the allocated block, NULL if it cannot be allocated.
 * @see av_malloc()
 */
void *hls_mallocz(size_t size);

void hls_freep(void *arg);
/* ----------------------------------------------------------------- */

#endif
