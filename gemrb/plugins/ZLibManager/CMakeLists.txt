# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
ADD_GEMRB_PLUGIN (ZLibManager ZLibManager.cpp)

TARGET_LINK_LIBRARIES( ZLibManager ${ZLIB_LIBRARY} Threads::Threads )
