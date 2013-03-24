/*
 * util_live.c: util method for hls proxy project
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *			<ju_guofeng@odc.hoperun.com>
 *
 *	2013-01-11:	first version
 *	2013-01-12:	add some string util from ffmpeg libavutil
 */

//#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util_live.h"

int hls_strstart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && *pfx == *str) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr)
        *ptr = str;
    return !*pfx;
}

int hls_stristart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && toupper((unsigned)*pfx) == toupper((unsigned)*str)) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr)
        *ptr = str;
    return !*pfx;
}

char *hls_stristr(const char *s1, const char *s2)
{
    if (!*s2)
        return (char*)(intptr_t)s1;

    do {
        if (hls_stristart(s1, s2, NULL))
            return (char*)(intptr_t)s1;
    } while (*s1++);

    return NULL;
}

size_t hls_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = 0;
    while (++len < size && *src)
        *dst++ = *src++;
    if (len <= size)
        *dst = 0;
    return len + strlen(src) - 1;
}

size_t hls_strlcat(char *dst, const char *src, size_t size)
{
    size_t len = strlen(dst);
    if (size <= len + 1)
        return len + strlen(src);
    return len + hls_strlcpy(dst + len, src, size - len);
}

#if 0
size_t hls_strlcatf(char *dst, size_t size, const char *fmt, ...)
{
    int len = strlen(dst);
    va_list vl;

    va_start(vl, fmt);
    len += vsnprintf(dst + len, size > len ? size - len : 0, fmt, vl);
    va_end(vl);

    return len;
}
#endif

void hls_parse_key_value(const char *str, hls_parse_key_val_cb callback_get_buf,
                        void *context)
{
    const char *ptr = str;

    /* Parse key=value pairs. */
    for (;;) {
        const char *key;
        char *dest = NULL, *dest_end;
        int key_len, dest_len = 0;

        /* Skip whitespace and potential commas. */
        while (*ptr && (isspace(*ptr) || *ptr == ','))
            ptr++;
        if (!*ptr)
            break;

        key = ptr;

        if (!(ptr = strchr(key, '=')))
            break;
        ptr++;
        key_len = ptr - key;

        callback_get_buf(context, key, key_len, &dest, &dest_len);
        dest_end = dest + dest_len - 1;

        if (*ptr == '\"') {
            ptr++;
            while (*ptr && *ptr != '\"') {
                if (*ptr == '\\') {
                    if (!ptr[1])
                        break;
                    if (dest && dest < dest_end)
                        *dest++ = ptr[1];
                    ptr += 2;
                } else {
                    if (dest && dest < dest_end)
                        *dest++ = *ptr;
                    ptr++;
                }
            }
            if (*ptr == '\"')
                ptr++;
        } else {
            for (; *ptr && !(isspace(*ptr) || *ptr == ','); ptr++)
                if (dest && dest < dest_end)
                    *dest++ = *ptr;
        }
        if (dest)
            *dest = 0;
    }
}

void hls_make_absolute_url(char *buf, int size, const char *base,
                          const char *rel)
{
    char *sep, *path_query;
    /* Absolute path, relative to the current server */
    if (base && strstr(base, "://") && rel[0] == '/') {
        if (base != buf)
            hls_strlcpy(buf, base, size);
        sep = strstr(buf, "://");
        if (sep) {
            /* Take scheme from base url */
            if (rel[1] == '/')
                sep[1] = '\0';
            else {
                /* Take scheme and host from base url */
                sep += 3;
                sep = strchr(sep, '/');
                if (sep)
                    *sep = '\0';
            }
        }
        hls_strlcat(buf, rel, size);
        return;
    }
    /* If rel actually is an absolute url, just copy it */
    if (!base || strstr(rel, "://") || rel[0] == '/') {
        hls_strlcpy(buf, rel, size);
        return;
    }
    if (base != buf)
        hls_strlcpy(buf, base, size);

    /* Strip off any query string from base */
    path_query = strchr(buf, '?');
    if (path_query != NULL)
        *path_query = '\0';

    /* Is relative path just a new query part? */
    if (rel[0] == '?') {
        hls_strlcat(buf, rel, size);
        return;
    }

    /* Remove the file name from the base url */
    sep = strrchr(buf, '/');
    if (sep)
        sep[1] = '\0';
    else
        buf[0] = '\0';
    while (hls_strstart(rel, "../", NULL) && sep) {
        /* Remove the path delimiter at the end */
        sep[0] = '\0';
        sep = strrchr(buf, '/');
        /* If the next directory name to pop off is "..", break here */
        if (!strcmp(sep ? &sep[1] : buf, "..")) {
            /* Readd the slash we just removed */
            hls_strlcat(buf, "/", size);
            break;
        }
        /* Cut off the directory name */
        if (sep)
            sep[1] = '\0';
        else
            buf[0] = '\0';
        rel += 3;
    }
    hls_strlcat(buf, rel, size);
}

/* add one element to a dynamic array */
void hls_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem)
{
    /* see similar ffmpeg.c:grow_array() */
    int nb, nb_alloc;
    intptr_t *tab;

    nb = *nb_ptr;
    tab = *(intptr_t**)tab_ptr;
    if ((nb & (nb - 1)) == 0) {
        if (nb == 0)
            nb_alloc = 1;
        else
            nb_alloc = nb * 2;
        /* FIXME modified from ffmpeg, may be error */
        //tab = av_realloc(tab, nb_alloc * sizeof(intptr_t));
        tab = realloc(tab, nb_alloc * sizeof(intptr_t));
        *(intptr_t**)tab_ptr = tab;
    }
    tab[nb++] = (intptr_t)elem;
    *nb_ptr = nb;
}

/* get local time (microseconds) */
int64_t hls_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* malloc and memset */
void *hls_mallocz(size_t size)
{
	void *ptr = malloc(size);
	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}

/* wait (microseconds) */
void mwait(mtime_t deadline)
{
	deadline -= mdate();
	if (deadline > 0)
		usleep(deadline);
}

/*
 * hls thread
 */
int hls_create_thread(pthread_t *thread, void *arg, void *thread_func)
{
	pthread_t *th = thread;
	void *hls_arg = arg;

	if(pthread_create(th, NULL, thread_func, hls_arg) != 0) {
		hls_err("hls pthread_create failed");
		return HLS_ERR;
	}

	hls_info("hls create a new thread...\n");

	return HLS_SUCCESS;
}

void hls_freep(void *arg)
{
	void **ptr = (void **)arg;
	free(*ptr);
	*ptr = NULL;
}
