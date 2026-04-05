# SPDX-FileCopyrightText: 2013 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := openal
LOCAL_SRC_FILES := libopenal.so
LOCAL_EXPORT_C_INCLUDES := include/

include $(PREBUILT_SHARED_LIBRARY)
