# SPDX-FileCopyrightText: 2013 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := png
LOCAL_SRC_FILES := libpng.a
LOCAL_EXPORT_C_INCLUDES := include/

include $(PREBUILT_STATIC_LIBRARY)
