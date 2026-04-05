# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#Don't build if openal is not there
IF(OPENAL_FOUND)
	INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR} ${SDL_INCLUDE_DIR} )
	FILE( GLOB OpenALAudio_files *.cpp )
	ADD_GEMRB_PLUGIN (OpenALAudio ${OpenALAudio_files} )

	TARGET_LINK_LIBRARIES(OpenALAudio ${OPENAL_LIBRARY} Threads::Threads)

	IF(MINGW AND SDL_BACKEND STREQUAL "SDL2")
		TARGET_LINK_LIBRARIES( OpenALAudio imm32 winmm version)
	ENDIF()
ENDIF(OPENAL_FOUND)
