/**
 * libhls_jni.c : used for android jni
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *
 * 2013-01-26:	first version
 *
 */
#ifndef __LIBHLS_JNI_H
#define __LIBHLS_JNI_H

#include <jni.h>
#include <android/log.h>

#include "util_live.h"

/* ------------------------------------------------------------------------------------- */
/* java call native */

//JNIEXPORT jstring JNICALL Java_com_hoperun_mediaplayerdemo_MyGridActivity_hlsProxyStart
//  (JNIEnv *env, jclass cls, jbyteArray url);

/* ------------------------------------------------------------------------------------- */
/* native call java */

int hls_event_callback(int event);

#endif
