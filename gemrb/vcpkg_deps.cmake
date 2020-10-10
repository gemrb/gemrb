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
SET(DLL_DIR_DEBUG ${VCPKG_DATAROOT}/debug/bin)
SET(DLL_DIR_RELEASE ${VCPKG_DATAROOT}/bin)

# lists of dll files to be deployed to the build/install directory for Win32 builds
# libcharset is just a relic of the current iconv build
LIST(APPEND DLL_SET_DEBUG
	brotlicommon.dll
	brotlidec.dll
	bz2d.dll
	freetyped.dll
	glew32d.dll
	libcharset.dll
	libiconv.dll
	libpng16d.dll
	ogg.dll
	OpenAL32.dll
	python27_d.dll
	SDL.dll
	SDL2d.dll
	SDL2_mixer.dll
	vorbis.dll
	vorbisfile.dll
	zlibd1.dll )

LIST(APPEND DLL_SET_RELEASE
	brotlicommon.dll
	brotlidec.dll
	bz2.dll
	freetype.dll
	glew32.dll
	libcharset.dll
	libiconv.dll
	libpng16.dll
	ogg.dll
	OpenAL32.dll
	python27.dll
	SDL.dll
	SDL2.dll
	SDL2_mixer.dll
	vorbis.dll
	vorbisfile.dll
	zlib1.dll )

FOREACH(ENTRY IN LISTS DLL_SET_DEBUG)
	IF(EXISTS ${DLL_DIR_DEBUG}/${ENTRY})
		LIST(APPEND DLL_PATHS_DEBUG ${DLL_DIR_DEBUG}/${ENTRY})
	ENDIF()
ENDFOREACH()

FOREACH(ENTRY IN LISTS DLL_SET_RELEASE)
	IF(EXISTS ${DLL_DIR_RELEASE}/${ENTRY})
		LIST(APPEND DLL_PATHS_RELEASE ${DLL_DIR_RELEASE}/${ENTRY})
	ENDIF()
ENDFOREACH()

# a custom target which copies the dll files to the build directory if needed, useful for rapid testing
IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	SET(DLL_SET ${DLL_PATHS_DEBUG} )
ELSE()
	SET(DLL_SET ${DLL_PATHS_RELEASE} )
ENDIF()

ADD_CUSTOM_COMMAND(TARGET gemrb POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	${DLL_SET}
	${CMAKE_BINARY_DIR}/gemrb/)


# if a user decides to install, they also need a copy of the dll in their game directory.
INSTALL(FILES ${DLL_PATHS_DEBUG} CONFIGURATIONS Debug DESTINATION ${BIN_DIR})
INSTALL(FILES ${DLL_PATHS_RELEASE} CONFIGURATIONS Release RelWithDebInfo DESTINATION ${BIN_DIR})

# copy over python core modules, so the buildbot binaries work without python installed

#this copies the modules bundled with the standard python installer if found
#otherwise, this uses the vcpkg bundled python modules if detected
#site.py is searched for by name because the vcpkg uninstall process doesn't properly purge the folder
#so if you switch between them for any reason, it will try to copy the wrong folder

GET_FILENAME_COMPONENT(PYTHON_PARENT_DIR ${PYTHON_INCLUDE_DIR} DIRECTORY)

IF(EXISTS ${PYTHON_PARENT_DIR}/Lib/site.py )
	INSTALL(DIRECTORY ${PYTHON_PARENT_DIR}/Lib DESTINATION ${BIN_DIR})
ELSEIF(EXISTS ${VCPKG_DATAROOT}/share/python2/Lib/site.py)
	INSTALL(DIRECTORY ${VCPKG_DATAROOT}/share/python2/Lib DESTINATION ${BIN_DIR})
ENDIF()

MESSAGE(STATUS "Dependency DLL's will be copied to the build and install directory")
MESSAGE(STATUS "Disable option VCPKG_AUTO_DEPLOY to skip this")
