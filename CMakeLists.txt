cmake_minimum_required(VERSION 2.6.0)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# prevent in-source builds
IF(NOT INSOURCEBUILD AND (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR}))
	MESSAGE(FATAL_ERROR "
		CMake generation for this project is not allowed within the source directory!
		Remove the CMake cache files and try again from another folder, e.g.:
		  rm -r CMakeCache.txt CMakeFiles/
		  mkdir build
		  cd build
		  cmake ..
		If you really want an in-source build, pass -DINSOURCEBUILD=1"
	)
ENDIF()

# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Release prior to calling PROJECT()
IF(DEFINED CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE(DEFINED CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF(DEFINED CMAKE_BUILD_TYPE)

PROJECT(gemrb)
# try to extract the version from the source
execute_process(
	COMMAND sed -n "s/\#define VERSION_GEMRB .\\([^\"]*\\).$/\\1/p" ${CMAKE_CURRENT_SOURCE_DIR}/gemrb/includes/globals.h
	OUTPUT_VARIABLE GEMRB_VERSION
	RESULT_VARIABLE RC
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(${RC} GREATER 0) # lookup failed
	set(GEMRB_VERSION "unknown")
endif(${RC} GREATER 0)
message("Detected version: ${GEMRB_VERSION}")

IF(PREFIX)
	SET(PREFIX CACHE PATH "Abbreviation for CMAKE_INSTALL_PREFIX.")
	SET(CMAKE_INSTALL_PREFIX ${PREFIX})
ENDIF(PREFIX)

if (NOT LAYOUT)
	if (WIN32)
		set(LAYOUT "home")
	elseif (APPLE)
		set(LAYOUT "bundle")
		# favor mac frameworks over unix libraries
		set(CMAKE_FIND_FRAMEWORK FIRST)
	else (APPLE)
		set(LAYOUT "fhs")
	endif (WIN32)
endif (NOT LAYOUT)

SET(LAYOUT "${LAYOUT}" CACHE STRING "Directory layout.")

# macro that sets a default (path) if one wasn't specified
MACRO(SET_PATH variable default)
	IF(NOT ${variable})
		SET(${variable} ${default})
	ENDIF(NOT ${variable})
ENDMACRO(SET_PATH)

if (${LAYOUT} MATCHES "home")
	SET_PATH( PLUGIN_DIR ${CMAKE_INSTALL_PREFIX}/plugins )
	SET_PATH( DATA_DIR ${CMAKE_INSTALL_PREFIX} )
	SET_PATH( MAN_DIR ${CMAKE_INSTALL_PREFIX}/man/man6 )
	SET_PATH( BIN_DIR ${CMAKE_INSTALL_PREFIX} )
	SET_PATH( SYSCONF_DIR ${CMAKE_INSTALL_PREFIX} )
	SET_PATH( LIB_DIR ${CMAKE_INSTALL_PREFIX} )
	SET_PATH( DOC_DIR ${CMAKE_INSTALL_PREFIX}/doc )
	SET_PATH( ICON_DIR ${CMAKE_INSTALL_PREFIX} )
	SET_PATH( SVG_DIR ${CMAKE_INSTALL_PREFIX} )
	SET_PATH( MENU_DIR ${CMAKE_INSTALL_PREFIX} )
	SET_PATH( EXAMPLE_CONF_DIR ${CMAKE_INSTALL_PREFIX} )
elseif (${LAYOUT} MATCHES "fhs")
	SET_PATH( LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib/gemrb )
	SET_PATH( PLUGIN_DIR ${LIB_DIR}/plugins )
	SET_PATH( DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/gemrb )
	SET_PATH( MAN_DIR ${CMAKE_INSTALL_PREFIX}/share/man/man6 )
	SET_PATH( BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin )
	IF( NOT SYSCONF_DIR )
		if ( ${CMAKE_INSTALL_PREFIX} STREQUAL "/usr" )
			SET( SYSCONF_DIR /etc/gemrb )
		else ()
			SET( SYSCONF_DIR ${CMAKE_INSTALL_PREFIX}/etc/gemrb )
		endif ()
	ENDIF( NOT SYSCONF_DIR )
	SET_PATH( DOC_DIR ${CMAKE_INSTALL_PREFIX}/share/doc/gemrb )
	SET_PATH( ICON_DIR ${CMAKE_INSTALL_PREFIX}/share/pixmaps )
	SET_PATH( SVG_DIR ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps )
	SET_PATH( MENU_DIR ${CMAKE_INSTALL_PREFIX}/share/applications )
	SET_PATH( EXAMPLE_CONF_DIR ${SYSCONF_DIR} )
elseif (${LAYOUT} MATCHES "opt")
	SET_PATH( LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib )
	SET_PATH( PLUGIN_DIR ${LIB_DIR}/plugins )
	SET_PATH( DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/ )
	SET_PATH( MAN_DIR ${CMAKE_INSTALL_PREFIX}/man/man6 )
	SET_PATH( BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin )
	SET_PATH( SYSCONF_DIR ${CMAKE_INSTALL_PREFIX}/etc )
	SET_PATH( DOC_DIR ${CMAKE_INSTALL_PREFIX}/share/doc/gemrb )
	SET_PATH( ICON_DIR ${CMAKE_INSTALL_PREFIX}/share/pixmaps )
	SET_PATH( SVG_DIR ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps )
	SET_PATH( MENU_DIR ${CMAKE_INSTALL_PREFIX}/share/applications )
	SET_PATH( EXAMPLE_CONF_DIR ${SYSCONF_DIR} )
else (${LAYOUT} MATCHES "bundle") # Mac or iOS
	SET(CMAKE_INSTALL_RPATH @loader_path/../Frameworks)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
	# most paths are irrelevant since the items will be bundled with application
	SET_PATH( BIN_DIR /Applications )
	# TODO: these should be copied during build and not install.
	SET_PATH( PLUGIN_DIR "${BIN_DIR}/${PROJECT_NAME}.app/Contents/Plugins" )
	SET_PATH( DOC_DIR "${BIN_DIR}/${PROJECT_NAME}.app/Contents/Resources" )
	SET_PATH( LIB_DIR @loader_path/../Frameworks )
endif (${LAYOUT} MATCHES "home")

# check if this is a release version
execute_process(
	COMMAND echo ${GEMRB_VERSION}
	COMMAND grep -q git # cmake creates a pipe by itself
	RESULT_VARIABLE RC
	OUTPUT_QUIET
)
if(${RC} GREATER 0) # no match or error
	set (GIT_VERSION 0)
else()
	set (GIT_VERSION 1)
endif()

IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	if ((NOT DISABLE_WERROR) AND GIT_VERSION)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-error=cast-align")
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wpointer-arith -Wcast-align")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wno-format-y2k -Wno-long-long -fno-strict-aliasing")
	# only export symbols explicitly marked to be exported.
	INCLUDE(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" VISIBILITY_HIDDEN)
	IF (VISIBILITY_HIDDEN AND NOT WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	ENDIF ()
	if (WIN32)
		# GCC 4.5.0+ has shared libstdc++ without dllimport
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-auto-import")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--enable-auto-import")
	endif (WIN32)
	# Ensure all plugin symbols exist.
	if (NOT APPLE AND NOT UNSAFE_PLUGIN)
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
	endif (NOT APPLE AND NOT UNSAFE_PLUGIN)
	# GNU systems need to define the Mersenne exponent for the RNG to compile w/o warning
	ADD_DEFINITIONS("-DSFMT_MEXP=19937")
ENDIF()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

# Check for all the required and optional dependencies
IF((CMAKE_MAJOR_VERSION GREATER 1) AND (CMAKE_MINOR_VERSION GREATER 7))
	FIND_PACKAGE(PythonLibs 2.3 REQUIRED)
ELSE((CMAKE_MAJOR_VERSION GREATER 1) AND (CMAKE_MINOR_VERSION GREATER 7))
	INCLUDE(FindPythonLibs)
ENDIF((CMAKE_MAJOR_VERSION GREATER 1) AND (CMAKE_MINOR_VERSION GREATER 7))
IF(PYTHONLIBS_FOUND)
	MESSAGE(STATUS "Looking for Python libraries and headers: found")
ELSE()
	MESSAGE(SEND_ERROR "Looking for Python libraries and headers: not found!")
	MESSAGE(FATAL_ERROR "Please get them (www.python.org)")
ENDIF()

INCLUDE(FindSDL)
IF(SDL_FOUND)
	MESSAGE(STATUS "Looking for SDL1: found")
ELSE()
	MESSAGE(WARNING "Looking for SDL1: not found!")
ENDIF()

# look for sdl2 if sdl1 was not found or a preference was stated
IF(USE_SDL2 OR NOT SDL_FOUND)
	INCLUDE(FindSDL2)
ENDIF()
IF(SDL2_FOUND)
	MESSAGE(STATUS "Looking for SDL2: found")
	# unify SDL variables, so we don't have to differentiate later
	SET(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
	SET(SDL_LIBRARY ${SDL2_LIBRARY})
	# skip SDL_FOUND for later use
ELSE()
	MESSAGE(WARNING "Looking for SDL2: not found!")
	IF(NOT SDL_FOUND)
		MESSAGE(FATAL_ERROR "Please get SDL from www.libsdl.org")
	ENDIF()
ENDIF()

IF(USE_OPENGL)
	IF(NOT SDL2_FOUND)
		MESSAGE(FATAL_ERROR "SDL2 is required for the OpenGL driver!")
	ENDIF()
	INCLUDE(FindOpenGL)
	IF(NOT OPENGL_FOUND)
		MESSAGE(FATAL_ERROR "OpenGL library not found!")
	ENDIF()

	IF(USE_OPENGL EQUAL 1)
		ADD_DEFINITIONS("-DUSE_GL")
		INCLUDE(FindGLEW)
		IF(GLEW_FOUND)
			MESSAGE(STATUS "Looking for Glew: found")
		ELSE()
			MESSAGE(SEND_ERROR "Looking for Glew: not found!")
			MESSAGE(FATAL_ERROR "Please install the Glew library and headers first!")
		ENDIF()
	ELSE()
		# use GLES instead of regular GL
		# TODO: check for all the stuff GLES needs
	ENDIF()
ENDIF()

INCLUDE(FindZLIB)
IF(ZLIB_FOUND)
	MESSAGE(STATUS "Looking for Zlib: found")
ELSE()
	MESSAGE(SEND_ERROR "Looking for Zlib: not found!")
	MESSAGE(FATAL_ERROR "Please install the Zlib library and headers first!")
ENDIF()

IF(UNIX)
	SET(CMAKE_THREAD_PREFER_PTHREAD true)
	FIND_PACKAGE(Threads REQUIRED)
ENDIF(UNIX)

INCLUDE(FindOpenAL)
IF(OPENAL_FOUND)
	MESSAGE(STATUS "Looking for OpenAL: found")
ELSE()
	MESSAGE(WARNING "Looking for OpenAL: not found!")
	MESSAGE(WARNING "If you want to build the OpenAL plugin, get OpenAL from www.openal.org.")
	MESSAGE(WARNING "If it just wasn't found, try setting the OPENALDIR environment variable.")
ENDIF()

IF(SDL2_FOUND)
	INCLUDE(FindSDL2_mixer)
	# unify variables, so we don't have to differentiate later
	SET(SDL_MIXER_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIRS})
	SET(SDL_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARIES})
	SET(SDL_MIXER_VERSION_STRING ${SDL2_MIXER_VERSION_STRING})
	SET(SDL_MIXER_FOUND ${SDL2_MIXER_FOUND})
ELSE()
	INCLUDE(FindSDL_mixer)
ENDIF()
IF(SDL_MIXER_FOUND)
	MESSAGE(STATUS "Looking for SDL_mixer: found")
ELSE()
	MESSAGE(WARNING "Looking for SDL_mixer: not found!")
	MESSAGE(WARNING "If you want to build the SDL_mixer plugin, install SDL_mixer first.")
	MESSAGE(WARNING "Make sure you use a version compatible with the chosen SDL version.")
ENDIF()

INCLUDE(FindLIBVLC)
IF(LIBVLC_FOUND)
	MESSAGE(STATUS "Looking for VLC: found")
ELSE()
	MESSAGE(WARNING "Looking for VLC: not found!")
	MESSAGE(WARNING "If you want to build the VLC plugin, install VLC first.")
ENDIF()

INCLUDE(FindFreetype)
IF(FREETYPE_FOUND)
	MESSAGE(STATUS "Looking for Freetype: found")
	INCLUDE(FindICONV)
	IF(NOT ICONV_FOUND)
		MESSAGE(WARNING "Iconv not found. TTF plugin will not be able to translate non-unicode compatible TLK encodings.")
	ELSE()
		set(HAVE_ICONV)
	ENDIF()
ELSE()
	MESSAGE(WARNING "Looking for Freetype: not found!")
	MESSAGE(WARNING "If you want to build the TTF plugin, install Freetype first.")
ENDIF()

INCLUDE(FindPNG)
IF(PNG_FOUND)
	MESSAGE(STATUS "Looking for libPNG: found")
ELSE()
	MESSAGE(WARNING "Looking for libPNG: not found!")
	MESSAGE(WARNING "GemRB will be built without any PNG support. Get it from www.libpng.org" )
	MESSAGE(WARNING "While no original game data is in PNG format, some mod data is and will need conversion.")
ENDIF()

FIND_LIBRARY(VORBIS_LIBRARY vorbisfile)
IF(VORBIS_LIBRARY)
	find_path(VORBIS_FILE vorbisfile.h PATH_SUFFIXES vorbis)
	IF(VORBIS_FILE)
		MESSAGE(STATUS "Looking for Ogg Vorbis support: found")
	ELSE()
		unset(VORBIS_LIBRARY) # disable the build for this plugin
	ENDIF()
ENDIF()
IF(NOT VORBIS_LIBRARY)
	MESSAGE(WARNING "Looking for Ogg Vorbis support: not found!")
	MESSAGE(WARNING "While no original game data is in OGG format, some mod data is and will need conversion.")
ENDIF()

# On Release builds cmake automatically defines NDEBUG, so we
# explicitly undefine it:
# This matches stripped (Release) and nonstripped (RelWithDebInfo) builds
if(CMAKE_BUILD_TYPE MATCHES "Rel.*" AND NOT MSVC)
	ADD_DEFINITIONS("-UNDEBUG")
endif()

if (STATIC_LINK)
	if (NOT WIN32)
		ADD_DEFINITIONS("-DSTATIC_LINK")
	else (NOT WIN32)
		unset(STATIC_LINK CACHE)
		MESSAGE(STATUS "Static linking not (yet) supported on this platform.")
	endif (NOT WIN32)
endif (STATIC_LINK)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} gemrb/includes gemrb/core)

# generate config.h
INCLUDE (config)

IF(APPLE)
	# Make sure we can find the 'ibtool' program. we need it to compile xibs
	find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	IF (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
  		MESSAGE (FATAL_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with 
                    the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	ENDIF ()

	ADD_DEFINITIONS("-x objective-c++")
	INCLUDE_DIRECTORIES(apple)
	FIND_LIBRARY(COCOA_LIBRARY_PATH Cocoa)
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	# for objective-c++
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTARGET_OS_MAC")
	# for pure objective-c
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -x objective-c -DTARGET_OS_MAC")
ENDIF(APPLE)

#Plugin addition macro
MACRO(ADD_GEMRB_PLUGIN plugin_name)
	SET (PLUGIN_BUILD_FILES ${ARGN})
	if (${ARGV1} STREQUAL "COCOA")
		LIST(REMOVE_ITEM PLUGIN_BUILD_FILES "COCOA")
		#this is an Apple thing
		if (APPLE)
			SET_SOURCE_FILES_PROPERTIES(
				CocoaWrapper.m
				PROPERTIES LANGUAGE C
			)
			message(STATUS "Will link ${plugin_name} plugin to: ${BUNDLE_LOADER}")
			SET (PLUGIN_BUILD_FILES ${PLUGIN_BUILD_FILES} CocoaWrapper.m)
		endif (APPLE)
	endif (${ARGV1} STREQUAL "COCOA")

	if (STATIC_LINK)
		ADD_LIBRARY(${plugin_name} STATIC ${PLUGIN_BUILD_FILES})
		set(plugins "${plugins};${plugin_name}" PARENT_SCOPE)
	else (STATIC_LINK)
		ADD_LIBRARY(${plugin_name} MODULE ${PLUGIN_BUILD_FILES})
		if (NOT UNSAFE_PLUGIN)
			TARGET_LINK_LIBRARIES(${plugin_name} gemrb_core ${CMAKE_THREAD_LIBS_INIT})
		endif (NOT UNSAFE_PLUGIN)
		if (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
			TARGET_LINK_LIBRARIES(${plugin_name} -shared-libgcc)
		elseif (CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
			TARGET_LINK_LIBRARIES(${plugin_name} -lc)
		endif (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
		INSTALL(TARGETS ${plugin_name} DESTINATION ${PLUGIN_DIR})
	endif (STATIC_LINK)

	IF (APPLE)
		SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gemrb/${PROJECT_NAME}.app/Contents/PlugIns)
	ELSE (APPLE)
		SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX ""
			INSTALL_RPATH ${LIB_DIR}
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gemrb/plugins)
	ENDIF (APPLE)
ENDMACRO(ADD_GEMRB_PLUGIN)

# also put the chosen paths in the man page (Ubuntu)
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/gemrb.6.in"
	"${CMAKE_CURRENT_BINARY_DIR}/gemrb.6"
	IMMEDIATE @ONLY
)

ADD_SUBDIRECTORY( gemrb )
IF (NOT APPLE)
	INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/gemrb.6" DESTINATION ${MAN_DIR} )
	INSTALL( FILES artwork/gemrb-logo.png DESTINATION ${ICON_DIR} RENAME gemrb.png )
	INSTALL( FILES artwork/logo04-rb_only.svg DESTINATION ${SVG_DIR} RENAME gemrb.svg )
	INSTALL( FILES gemrb.desktop DESTINATION ${MENU_DIR} )
ENDIF()
INSTALL( FILES README INSTALL COPYING NEWS AUTHORS DESTINATION ${DOC_DIR} )
INSTALL( FILES admin/extend2da.py DESTINATION ${BIN_DIR}
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY
)

# copy the variable, since the file uses @VERSION@
set(VERSION ${GEMRB_VERSION})
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/gemrb.spec.in"
	"${CMAKE_CURRENT_BINARY_DIR}/gemrb.spec"
	IMMEDIATE @ONLY
)

ADD_CUSTOM_TARGET( uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" )

# make dist for a gzipped tarball of current HEAD
set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${GEMRB_VERSION})
add_custom_target( dist
	COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
		| gzip --best > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# pretty-print options macro
# as of 2.8 cmake does not support anything like EVAL
MACRO(PRINT_OPTION option)
if (${option})
	message(STATUS "  ${option}: ${${option}}")
else()
	message(STATUS "  ${option}: disabled")
endif()
ENDMACRO(PRINT_OPTION)

message(STATUS "")
message(STATUS "These are the configured paths:")
message(STATUS "  PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  LIB_DIR: ${LIB_DIR}")
message(STATUS "  PLUGIN_DIR: ${PLUGIN_DIR}")
message(STATUS "  BIN_DIR: ${BIN_DIR}")
message(STATUS "  DATA_DIR: ${DATA_DIR}")
message(STATUS "  MAN_DIR: ${MAN_DIR}")
message(STATUS "  SYSCONF_DIR: ${SYSCONF_DIR}")
message(STATUS "  DOC_DIR: ${DOC_DIR}")
message(STATUS "  ICON_DIR: ${ICON_DIR}")
message(STATUS "  SVG_DIR: ${SVG_DIR}")
message(STATUS "  MENU_DIR: ${MENU_DIR}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  LAYOUT: ${LAYOUT}")
PRINT_OPTION(NOCOLOR)
PRINT_OPTION(STATIC_LINK)
PRINT_OPTION(INSOURCEBUILD)
PRINT_OPTION(DISABLE_WERROR)
PRINT_OPTION(WIN32_USE_STDIO)
PRINT_OPTION(USE_SDL2)
PRINT_OPTION(USE_OPENGL)
message(STATUS "")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
