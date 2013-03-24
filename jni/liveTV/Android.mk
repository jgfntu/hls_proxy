#
#2013-01-21		add by juguofeng	just for android commanline test 
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

#LOCAL_MODULE := httplive
LOCAL_MODULE := hlsjni

LOCAL_C_INCLUDES += \
    $(LOCALLIBROOT)/include

LOCAL_SRC_FILES := main_live.c proxy_live.c mongoose.c m3u8_live.c curl_live.c util_live.c fifo_live.c \
				libhls_jni.c

LOCAL_LDLIBS := -L$(LOCALLIBROOT)/lib \
	-ldl -llog -ldl -lcurl

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_EXECUTABLE)
