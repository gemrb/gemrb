# SPDX-FileCopyrightText: 2013 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
# APP_PROJECT_PATH := $(call my-dir)/..
APP_STL := gnustl_static
APP_MODULES := SDL2 ogg vorbis openal python freetype2-static png main
APP_CPPFLAGS += -frtti
