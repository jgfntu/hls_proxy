/**
 * libhls_jni.c : used for android jni
 *
 * juguofeng<ju_guofeng@hoperun.com>
 *
 * 2013-01-26:	first version
 *
 */

#include "libhls_jni.h"
#include "main_live.h"

/* ------------------------------------------------------------------------------------- */
/* Pointer to the Java virtual machine
 * Note: It's okay to use a static variable for the VM pointer since there
 * can only be one instance of this shared library in a single VM
 */
JavaVM *hls_vm;

jclass hls_event_cls = NULL;
jobject hls_event_obj = NULL;
jmethodID hls_event_cbk = NULL;

/**
 * our own method
 */
jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    /* Keep a reference on the Java VM */
    hls_vm = vm;

    hls_info("JNI interface loaded.");
    
    return JNI_VERSION_1_2;
}

void JNI_OnUnload(JavaVM *vm, void *reserved)
{
    hls_info("JNI interface unloaded.");
}

/* ------------------------------------------------------------------------------------- */
/* java call native */

/**
 * give native a play url, then hls proxy start buffing data, but not playing
 */
//void Java_com_hoperun_mediaplayerdemo_MyGridActivity_hlsProxyInit(JNIEnv *env, jclass cls, jbyteArray url)
void Java_com_hoperun_media_proxy_MediaProxy_hlsProxyInit(JNIEnv *env, jclass cls)
{
	//unsigned char *hls_url = (*env)->GetByteArrayElements(env, url, NULL);
	//int url_size = (*env)->GetArrayLength(env, url);

	hls_info("get into libhlsjni.so...\n");

	/* call to hls proxy lib */
	//hls_proxy_init(hls_url, url_size);	
	hls_proxy_init();	
}

void Java_com_hoperun_media_proxy_MediaProxy_hlsProxyUninit(JNIEnv *env, jclass cls)
{
	hls_err("-------------->destroy the native thread\n");

	/* call to hls proxy lib */
	hls_proxy_uninit();	
}

void Java_com_hoperun_media_proxy_MediaProxy_hlsProxyPrepare(JNIEnv *env, jclass cls, jbyteArray url)
{
	unsigned char *hls_url = (*env)->GetByteArrayElements(env, url, NULL);
	int url_size = (*env)->GetArrayLength(env, url);
	
	/* call to hls proxy lib */
	hls_proxy_prepare(hls_url, url_size);
}

void Java_com_hoperun_media_proxy_MediaProxy_setEventManager(JNIEnv *env, jobject thiz, jobject eventManager)
{
    if (hls_event_obj != NULL) {
        (*env)->DeleteGlobalRef(env, hls_event_obj);
        hls_event_obj = NULL;
    }

    jclass cls = (*env)->GetObjectClass(env, eventManager);
    if (!cls) {
        hls_err("setEventManager: failed to get class reference\n");
        return;
    }

    jmethodID methodID = (*env)->GetMethodID(env, cls, "callback", "(I)V");
    if (!methodID) {
        hls_err("setEventManager: failed to get the callback method");
        return;
    }

    hls_event_obj = (*env)->NewGlobalRef(env, eventManager);
}

void Java_com_hoperun_media_proxy_MediaProxy_detachEventManager(JNIEnv *env, jobject thiz)
{
    if (hls_event_obj != NULL) {
        (*env)->DeleteGlobalRef(env, hls_event_obj);
        hls_event_obj = NULL;
    }
}

/* ------------------------------------------------------------------------------------- */
/* native call java */

int hls_event_callback(int event)
{
	JNIEnv *env;
	int ret;
	
	hls_info("get into hls_event_callback method\n");
	
	if (hls_event_obj == NULL)
        return HLS_ERR;
	
	int isAttached = HLS_FALSE;
	
	int status = (*hls_vm)->GetEnv(hls_vm, (void**)&env, JNI_VERSION_1_2);
    if (status < 0) {
        hls_info("hls_event_callback: failed to get JNI environment, "
             "assuming native thread");
        /* FIXME doing this for what??? */
        status = (*hls_vm)->AttachCurrentThread(hls_vm, &env, NULL);
        if (status < 0)
            return HLS_ERR;
        isAttached = HLS_TRUE;
    }
    
    if (env == NULL) {
    	hls_err("get jni env failed\n");
    	ret = HLS_ERR;
		goto end;
    }
	
	/* From VLC libvlc_jni.c */
	
	/* Get the object class */
    hls_event_cls = (*env)->GetObjectClass(env, hls_event_obj);
    if (!hls_event_cls) {
        hls_err("EventManager: failed to get class reference");
        ret = HLS_ERR;
        goto end;
    }

    /* Find the callback ID */
    hls_event_cbk = (*env)->GetMethodID(env, hls_event_cls, "callback", "(I)V");
    if (hls_event_cbk) {
        (*env)->CallVoidMethod(env, hls_event_obj, hls_event_cbk, event);
        ret = HLS_SUCCESS;
    } else {
        hls_err("EventManager: failed to get the callback method");
        ret = HLS_ERR;
    }
    
end:
    if (isAttached)
        (*hls_vm)->DetachCurrentThread(hls_vm);
    
    hls_info("get out of hls_event_callback method\n");
    
    return ret;
}




