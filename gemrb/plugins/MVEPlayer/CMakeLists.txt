# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

ADD_GEMRB_PLUGIN (MVEPlayer
  mvevideodec8.cpp
  mvevideodec16.cpp
  mveaudiodec.cpp
  MVEPlayer.cpp
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(MVEPlayer kernel32)
ENDIF()
