# MSVC VCPKG Dependency DLL script
# The existing default behaviour when using VCPKG copies the DLL files to the wrong place
# Because of the way that GemRB is split into plugins, VCPKG makes copies of the DLL files in the /plugins/ directory
# There is no way to override the output directory without changing the layout which has existed from the beginning
# So the VCPKG DLL helper script is disabled and substituted here

# This does not autodetect anything, it just relies on an expected set of filenames if using Visual Studio+VCPKG libraries
# If the dll files cannot be found here, nothing happens.

MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring rules for VCPKG dependency deployment")

SET(VCPKG_DATAROOT "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

# we either want SDL1 or 2, no need to copy both
IF(SDL_BACKEND MATCHES SDL2)
	SET (DLL_SDL_VER 2)
ENDIF()

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
	SET(DLL_SET_DBG "d") # all the debug dll's just have 'd' somewhere in the filename if they are different at all
	SET(DLL_DIR "${VCPKG_DATAROOT}/debug/bin/")
ELSE()
	UNSET(DLL_SET_DBG)
	SET(DLL_DIR "${VCPKG_DATAROOT}/bin/")
ENDIF()

# lists of dll files to be deployed to the build/install directory for Win32 builds
# libcharset is just a relic of the current iconv build
LIST(APPEND DLL_SET
	${VCPKG_DATAROOT}/bin/SDL${DLL_SDL_VER}.dll # Cmake doesn't actually find the sdl debug libs
	${DLL_DIR}python27.dll
	${DLL_DIR}OpenAL32.dll
	${DLL_DIR}zlib${DLL_SET_DBG}1.dll
	${DLL_DIR}vorbisfile.dll
	${DLL_DIR}ogg.dll
	${DLL_DIR}vorbis.dll
	${DLL_DIR}libpng16${DLL_SET_DBG}.dll
	${DLL_DIR}freetype${DLL_SET_DBG}.dll
	${DLL_DIR}bz2${DLL_SET_DBG}.dll
	${DLL_DIR}libcharset.dll
	${DLL_DIR}libiconv.dll )

FOREACH(ENTRY IN LISTS DLL_SET)

	IF(NOT EXISTS ${ENTRY})
		LIST(REMOVE_ITEM DLL_SET ${ENTRY})
		CONTINUE()
	ENDIF()

ENDFOREACH()

ADD_CUSTOM_COMMAND(TARGET gemrb POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	${DLL_SET}
	${CMAKE_BINARY_DIR}/gemrb/)

# if a user decides to install, they also need a copy of the dll in their game directory.
INSTALL(FILES ${DLL_SET} DESTINATION ${BIN_DIR})

# the ogg plugin doesn't actually find the vorbisfile-1.dll built by vcpkg
# after checking dependencies with dumpbin, turns out it is looking for a vorbisfile.dll
# this may be because the .lib file differs in name from the .dll file
IF(EXISTS ${DLL_DIR}vorbisfile-1.dll )

	ADD_CUSTOM_COMMAND(TARGET gemrb POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${DLL_DIR}vorbisfile-1.dll
		${CMAKE_BINARY_DIR}/gemrb/vorbisfile.dll)

	INSTALL(FILES ${DLL_DIR}vorbisfile-1.dll DESTINATION ${BIN_DIR} RENAME vorbisfile.dll)

ENDIF()

# copy over python core modules, so the buildbot binaries work without python installed
INSTALL(DIRECTORY ${VCPKG_DATAROOT}/share/python2/Lib DESTINATION ${BIN_DIR})

MESSAGE(STATUS "Dependency DLL's will be copied to the build and install directory")
MESSAGE(STATUS "Disable option VCPKG_AUTO_DEPLOY to skip this")
