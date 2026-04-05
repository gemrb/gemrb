# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

IF (VORBIS_LIBRARY)
	FILE( GLOB OGGReader_files *.cpp )

	# include the second parent of vorbisfile.h
	get_filename_component(OGG_INCLUDE ${VORBIS_FILE} PATH)
	include_directories(${OGG_INCLUDE})

	ADD_GEMRB_PLUGIN (OGGReader ${OGGReader_files})

	TARGET_LINK_LIBRARIES(OGGReader ${VORBIS_LIBRARY} Threads::Threads)
ENDIF (VORBIS_LIBRARY)
