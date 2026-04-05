# SPDX-FileCopyrightText: 2014 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.11)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "" FORCE)
string(APPEND CMAKE_CURRENT_SOURCE_DIR "/../../")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
set(BAKE_ICON 0)

project(GemRB)

INCLUDE (config)
