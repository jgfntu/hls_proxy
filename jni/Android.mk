#
#2013-01-21		add by juguofeng	just for android commanline test 
#

LOCAL_PATH := $(call my-dir)

LIVETVROOT := $(LOCAL_PATH)/liveTV
LOCALLIBROOT := $(LOCAL_PATH)/arm-linux-androideabi

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))

