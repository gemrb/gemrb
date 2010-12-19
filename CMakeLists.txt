cmake_minimum_required(VERSION 2.4.4)
if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
      cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)
# allow empty else and endif constructs (available by default since 2.6.0)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

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
	else (WIN32)
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
else (${LAYOUT} MATCHES "opt")
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
endif (${LAYOUT} MATCHES "home")


IF(CMAKE_COMPILER_IS_GNUCXX)
	if ( NOT DISABLE_WERROR )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
	endif ( NOT DISABLE_WERROR )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wpointer-arith -Wcast-align")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wno-format-y2k -Wno-long-long -fno-strict-aliasing")
	# only export symbols explicitly marked to be exported.
	INCLUDE(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" VISIBILITY_HIDDEN)
	IF (VISIBILITY_HIDDEN)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	ENDIF (VISIBILITY_HIDDEN)
	if (WIN32)
		# GCC 4.5.0+ has shared libstdc++ without dllimport
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-auto-import")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--enable-auto-import")
	endif (WIN32)
	# Ensure all plugin symbols exist.
	if (NOT APPLE AND NOT UNSAFE_PLUGIN)
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
	endif (NOT APPLE AND NOT UNSAFE_PLUGIN)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# Check for all the required and optional dependencies
INCLUDE(FindPythonLibs)
IF(PYTHONLIBS_FOUND)
	MESSAGE(STATUS "Looking for Python libraries and headers: found")
ELSE()
	MESSAGE(SEND_ERROR "Looking for Python libraries and headers: not found!")
	MESSAGE(FATAL_ERROR "Please get them (www.python.org)")
ENDIF()

INCLUDE(FindSDL)
IF(SDL_FOUND)
	MESSAGE(STATUS "Looking for SDL: found")
ELSE()
	MESSAGE(SEND_ERROR "Looking for SDL: not found!")
	MESSAGE(FATAL_ERROR "Please get it from www.libsdl.org")
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

INCLUDE(FindSDL_mixer)
IF(SDLMIXER_FOUND)
	MESSAGE(STATUS "Looking for SDL_mixer: found")
ELSE()
	MESSAGE(WARNING "Looking for SDL_mixer: not found!")
	MESSAGE(WARNING "If you want to build the SDL_mixer plugin, install SDL_mixer first.")
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

ADD_DEFINITIONS("-DHAVE_CONFIG_H")

# On Release builds cmake automatically defines NDEBUG, so we
# explicitly undefine it:
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
	ADD_DEFINITIONS("-UNDEBUG")
endif()

if (STATIC_LINK)
	if (NOT WIN32 AND NOT APPLE)
		ADD_DEFINITIONS("-DSTATIC_LINK")
	else (NOT WIN32 AND NOT APPLE)
		unset(STATIC_LINK CACHE)
		MESSAGE(STATUS "Static linking not (yet) supported on this platform.")
	endif (NOT WIN32 AND NOT APPLE)
endif (STATIC_LINK)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} gemrb/includes gemrb/core)

#Platform checks
INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("long int" SIZEOF_LONG_INT)

INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS("snprintf" HAVE_SNPRINTF)
CHECK_FUNCTION_EXISTS("strndup" HAVE_STRNDUP)

#Unneeded on windows
IF(NOT WIN32)
	INCLUDE (CheckCXXSourceCompiles)
	CHECK_CXX_SOURCE_COMPILES("typedef void *(* voidvoid)(void);

		void *object = 0;
		voidvoid function;
		function = (voidvoid) object;
		" PERMITS_OBJECT_TO_FUNCTION_CAST)

	IF( NOT PERMITS_OBJECT_TO_FUNCTION_CAST )
		SET(HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST 1)
	ENDIF( NOT PERMITS_OBJECT_TO_FUNCTION_CAST )
ENDIF(NOT WIN32)

IF(APPLE)
 FIND_LIBRARY(SDL_MAIN_LIBRARY_PATH SDLmain)
 FIND_LIBRARY(COCOA_LIBRARY_PATH Cocoa)
ENDIF(APPLE)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake_config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h ESCAPE_QUOTES)

#Plugin addition macro
MACRO(ADD_GEMRB_PLUGIN plugin_name)
	if (STATIC_LINK)
		ADD_LIBRARY(${plugin_name} STATIC ${ARGN})
		set(plugins "${plugins};${plugin_name}" PARENT_SCOPE)
	else (STATIC_LINK)
		ADD_LIBRARY(${plugin_name} MODULE ${ARGN})
		if (NOT UNSAFE_PLUGIN)
			TARGET_LINK_LIBRARIES(${plugin_name} gemrb_core ${CMAKE_THREAD_LIBS_INIT})
		endif (NOT UNSAFE_PLUGIN)
		INSTALL(TARGETS ${plugin_name} DESTINATION ${PLUGIN_DIR})
	endif (STATIC_LINK)
	SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX ""
		INSTALL_RPATH ${LIB_DIR}
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gemrb/plugins)
ENDMACRO(ADD_GEMRB_PLUGIN)

#gemrb overrides macro
MACRO(ADD_GEMRB_OVERRIDE game_name)
	FILE( GLOB FILES_TO_INSTALL *.2da *.bmp *.ini *.chu *.ids *.bcs *.vvc *.mos *.spl *.wav *.pro)
	INSTALL( FILES ${FILES_TO_INSTALL} DESTINATION ${DATA_DIR}/override/${game_name} )
ENDMACRO(ADD_GEMRB_OVERRIDE )

# also put the chosen paths in the man page (Ubuntu)
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/gemrb.6.in"
  "${CMAKE_CURRENT_BINARY_DIR}/gemrb.6"
  IMMEDIATE @ONLY
)

ADD_SUBDIRECTORY( gemrb )
INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/gemrb.6" DESTINATION ${MAN_DIR} )
INSTALL( FILES artwork/gemrb-logo.png DESTINATION ${ICON_DIR} RENAME gemrb.png )
INSTALL( FILES artwork/logo04-rb_only.svg DESTINATION ${SVG_DIR} RENAME gemrb.svg )
INSTALL( FILES gemrb.desktop DESTINATION ${MENU_DIR} )
INSTALL( FILES README INSTALL COPYING NEWS AUTHORS DESTINATION ${DOC_DIR} )

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
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
    | sed '/CMAKE_CXX_FLAGS/ s,-Werror,,'
    | gzip --best > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

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
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
