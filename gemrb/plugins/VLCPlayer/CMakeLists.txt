# SPDX-FileCopyrightText: 2012 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

IF (LIBVLC_FOUND)
	INCLUDE_DIRECTORIES( ${LIBVLC_INCLUDE_DIR} )
	ADD_GEMRB_PLUGIN ( VLCPlayer VLCPlayer.cpp )
	TARGET_LINK_LIBRARIES( VLCPlayer ${LIBVLC_LIBRARY} )
ENDIF (LIBVLC_FOUND)
