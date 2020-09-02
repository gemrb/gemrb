LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := png
LOCAL_SRC_FILES := libpng.a
LOCAL_EXPORT_C_INCLUDES := include/

include $(PREBUILT_STATIC_LIBRARY)
