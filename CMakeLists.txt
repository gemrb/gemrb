cmake_minimum_required(VERSION 2.4.4)
if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
      cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Release prior to calling PROJECT()
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
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

if (${LAYOUT} MATCHES "home")
	IF( NOT PLUGIN_DIR )
		SET( PLUGIN_DIR ${CMAKE_INSTALL_PREFIX}/plugins )
	ENDIF( NOT PLUGIN_DIR )
	IF( NOT DATA_DIR )
		SET( DATA_DIR ${CMAKE_INSTALL_PREFIX} )
	ENDIF( NOT DATA_DIR )
	IF( NOT MAN_DIR )
		SET( MAN_DIR ${CMAKE_INSTALL_PREFIX}/man/man6 )
	ENDIF( NOT MAN_DIR )
	IF( NOT BIN_DIR )
		SET( BIN_DIR ${CMAKE_INSTALL_PREFIX} )
	ENDIF( NOT BIN_DIR )
	IF( NOT SYSCONF_DIR )
		SET( SYSCONF_DIR ${CMAKE_INSTALL_PREFIX} )
	ENDIF( NOT SYSCONF_DIR )
	IF( NOT LIB_DIR )
		SET( LIB_DIR ${CMAKE_INSTALL_PREFIX} )
	ENDIF( NOT LIB_DIR )
elseif (${LAYOUT} MATCHES "fhs")
	IF( NOT LIB_DIR )
		SET( LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib/gemrb )
	ENDIF( NOT LIB_DIR )
	IF( NOT PLUGIN_DIR )
		SET( PLUGIN_DIR ${LIB_DIR}/plugins )
	ENDIF( NOT PLUGIN_DIR )
	IF( NOT DATA_DIR )
		SET( DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/gemrb )
	ENDIF( NOT DATA_DIR )
	IF( NOT MAN_DIR )
		SET( MAN_DIR ${CMAKE_INSTALL_PREFIX}/share/man/man6 )
	ENDIF( NOT MAN_DIR )
	IF( NOT BIN_DIR )
		SET( BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin )
	ENDIF( NOT BIN_DIR )
	IF( NOT SYSCONF_DIR )
		SET( SYSCONF_DIR ${CMAKE_INSTALL_PREFIX}/etc/gemrb )
	ENDIF( NOT SYSCONF_DIR )
else (${LAYOUT} MATCHES "opt")
	IF( NOT LIB_DIR )
		SET( LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib )
	ENDIF( NOT LIB_DIR )
	IF( NOT PLUGIN_DIR )
		SET( PLUGIN_DIR ${LIB_DIR}/plugins )
	ENDIF( NOT PLUGIN_DIR )
	IF( NOT DATA_DIR )
		SET( DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/ )
	ENDIF( NOT DATA_DIR )
	IF( NOT MAN_DIR )
		SET( MAN_DIR ${CMAKE_INSTALL_PREFIX}/man/man6 )
	ENDIF( NOT MAN_DIR )
	IF( NOT BIN_DIR )
		SET( BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin )
	ENDIF( NOT BIN_DIR )
	IF( NOT SYSCONF_DIR )
		SET( SYSCONF_DIR ${CMAKE_INSTALL_PREFIX}/etc )
	ENDIF( NOT SYSCONF_DIR )
endif (${LAYOUT} MATCHES "home")


IF(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -W -Wpointer-arith -Wcast-align")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wno-format-y2k -Wno-long-long -fno-strict-aliasing")
	# only export symbols explicitly marked to be exported.
	INCLUDE(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" VISIBILITY_HIDDEN)
	IF (VISIBILITY_HIDDEN)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	ENDIF (VISIBILITY_HIDDEN)
	# Ensure all plugin symbols exist.
	if (NOT APPLE)
		set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined")
	endif (NOT APPLE)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

MESSAGE(STATUS "Looking for Python libraries")
INCLUDE(FindPythonLibs)
IF(PYTHON_LIBRARY)
 MESSAGE(STATUS "Python Libraries Found")
ELSE(PYTHON_LIBRARY)
 MESSAGE(FATAL_ERROR "Unable to find Python development libraries. Please get it from www.python.org")
ENDIF(PYTHON_LIBRARY)

MESSAGE(STATUS "Looking for openAL libraries")
INCLUDE(FindOpenAL)
IF(OPENAL_FOUND)
 MESSAGE(STATUS "openal found")
ELSE(OPENAL_FOUND)
 MESSAGE(ERROR "unable to find openal. please set OPENALDIR environment variable or get it from www.openal.org")
ENDIF(OPENAL_FOUND)

MESSAGE(STATUS "Looking for SDL")
INCLUDE(FindSDL)
IF(SDL_FOUND)
 MESSAGE(STATUS "SDL found")
ELSE(SDL_FOUND)
 MESSAGE(FATAL_ERROR "unable to find SDL. please get it from www.libsdl.org")
ENDIF(SDL_FOUND)

MESSAGE( STATUS "Looking for Zlib" )
#SET(ZLIB_FIND_REQUIRED TRUE)
INCLUDE(FindZLIB)

MESSAGE( STATUS "Looking for libPNG" )
INCLUDE(FindPNG)
IF(PNG_FOUND)
 MESSAGE( STATUS "libPNG found" )
ELSE(PNG_FOUND)
 MESSAGE( STATUS "WARNING : GemRB will be built without any PNG support. Get it from www.libpng.org" )
ENDIF(PNG_FOUND)

IF(UNIX)
 MESSAGE(STATUS "Looking for dl library")
 FIND_LIBRARY(DLD_LIBRARY_PATH dl dld)
 IF(DLD_LIBRARY_PATH)
  MESSAGE(STATUS "dl library found")
 ELSE(DLD_LIBRARY_PATH)
  MESSAGE(FATAL_ERROR "dl library not found")
 ENDIF(DLD_LIBRARY_PATH)
ENDIF(UNIX)

MESSAGE(STATUS "Looking for Ogg Vorbis support")
FIND_LIBRARY(VORBIS_LIBRARY vorbisfile)

ADD_DEFINITIONS("-DHAVE_CONFIG_H")

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

IF(VORBIS_LIBRARY)
 SET(HAS_VORBIS_SUPPORT 1)
ELSE(VORBIS_LIBRARY)
 SET(VORBIS_LIBRARY "")
ENDIF(VORBIS_LIBRARY)

IF(APPLE)
 FIND_LIBRARY(SDL_MAIN_LIBRARY_PATH SDLmain)
 FIND_LIBRARY(COCOA_LIBRARY_PATH Cocoa)
ENDIF(APPLE)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

#Plugin addition macro
MACRO(ADD_GEMRB_PLUGIN plugin_name)
	ADD_LIBRARY(${plugin_name} MODULE ${ARGN})
	SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX ""
		INSTALL_RPATH ${LIB_DIR}
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gemrb/plugins)
	TARGET_LINK_LIBRARIES(${plugin_name} gemrb_core)
	INSTALL(TARGETS ${plugin_name} DESTINATION ${PLUGIN_DIR})
ENDMACRO(ADD_GEMRB_PLUGIN)

#gemrb overrides macro
MACRO(ADD_GEMRB_OVERRIDE game_name)
FILE( GLOB FILES_TO_INSTALL *.2da *.bmp *.ini *.chu *.ids *.bcs *.vvc *.mos *.spl *.wav *.pro)
INSTALL( FILES ${FILES_TO_INSTALL} DESTINATION ${DATA_DIR}/override/${game_name} )
ENDMACRO(ADD_GEMRB_OVERRIDE )

ADD_SUBDIRECTORY( gemrb )
INSTALL( FILES gemrb.6 DESTINATION ${MAN_DIR} )

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
    | gzip --best > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
