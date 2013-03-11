LOCAL_PATH := $(call my-dir)

# ogg
include $(CLEAR_VARS)

LOCAL_MODULE := ogg
LOCAL_SRC_FILES := libogg.so
LOCAL_C_EXPORT_INCLUDES := include

include $(PREBUILT_SHARED_LIBRARY)

