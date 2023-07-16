cmake_minimum_required(VERSION 3.11)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_CXX_STANDARD 11)

# skip verbose make install output
set(CMAKE_INSTALL_MESSAGE "NEVER")

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

if(CMAKE_BUILD_TYPE MATCHES "Rel.*")
	include(CheckIPOSupported)
	check_ipo_supported(RESULT supported)

	if(supported)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE BOOL)
	endif()
endif()

# VCPKG manifest file: platforms/windows/vcpkg.json
# must be declared before PROJECT()
IF(WIN32)
	SET(VCPKG_MANIFEST_DIR "${CMAKE_SOURCE_DIR}/platforms/windows" )
	message(STATUS "VCPKG_MANIFEST_DIR: ${VCPKG_MANIFEST_DIR}")
ENDIF(WIN32)

PROJECT(gemrb)

find_program(SCCACHE_PROGRAM "sccache")
if(SCCACHE_PROGRAM)
	message(STATUS "Using sccache found at ${SCCACHE_PROGRAM} for caching build results")
	set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "" FORCE)
	set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "" FORCE)
endif()

IF(APPLE)
	# favor mac frameworks over unix libraries
	set(CMAKE_FIND_FRAMEWORK FIRST)
	SET(BAKE_ICON 0)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
ELSE(APPLE)
	SET(BAKE_ICON 1)
ENDIF()

# check for RaspberryPi
IF(UNIX)
	FIND_FILE(RPI NAMES bcm_host.h PATHS "/opt/vc/include")
	# By default, Pi0 to Pi3 models use the legacy (Broadcom) GLESv2 drivers, from /opt/vc.
	# Newer models (Pi4) don't support it, using the open source MESA drivers.
	# NOTE: the Pi3B(+) models can also run with open source MESA drivers, but support for it must be explicitely enabled
	IF(RPI AND NOT DISABLE_VIDEOCORE)
		SET(HINT_GLES_LIBNAME brcmGLESv2)
		SET(HINT_GLES_INCDIR "/opt/vc/include")
		SET(HINT_GLES_LIBDIR "/opt/vc/lib")
		SET(OPENGL_BACKEND GLES)
		SET(SDL_BACKEND SDL2)
	ENDIF()
ENDIF()

# Options list
SET(SDL_BACKEND "Auto" CACHE STRING "libSDL version (possible values: Auto, SDL or SDL2)")
SET_PROPERTY(CACHE SDL_BACKEND PROPERTY STRINGS Auto SDL SDL2)

SET(OPENGL_BACKEND "None" CACHE STRING "OpenGL support (requires SDL2, possible values: None, OpenGL, GLES)")
SET_PROPERTY(CACHE OPENGL_BACKEND PROPERTY STRINGS None OpenGL GLES)

SET(PYTHON_VERSION "Auto" CACHE STRING "Python version to use (e.g.: Auto, 3, 3.6)")
SET(SANITIZE "None" CACHE STRING "Sanitizer to use (possible values: None, address, thread, memory, undefined)")

OPTION(USE_SDLMIXER "Enable SDL_mixer support" ON)
OPTION(USE_OPENAL "Enable OpenAL support" ON)
OPTION(USE_LIBVLC "Enable LibVLC support" ON)
OPTION(USE_FREETYPE "Enable FreeType support" ON)
OPTION(USE_PNG "Enable LibPNG support" ON)
OPTION(USE_VORBIS "Enable Vorbis support" ON)
OPTION(DISABLE_WERROR "Do not treat warnings as errors" OFF)

OPTION(USE_SDL_CONTROLLER_API "Enable SDL controller APIs. (disable if you plan on handling controller input in an external program like gptokeyb)" ON)
OPTION(SDL_RESOLUTION_INDEPENDANCE "Scale the window to the size of the screen with pixel scaling." OFF)

#VCPKG dll deployment is circumvented because it doesn't currently work for gemrb
IF(WIN32 AND _VCPKG_INSTALLED_DIR)
	OPTION(VCPKG_AUTO_DEPLOY "Fix VCPKG dependency DLL locations" ON)
	# This variable disables the built in VCPKG deployment script,
	# which makes copies of the DLL files in places they are not ever found by the game
	SET(VCPKG_APPLOCAL_DEPS OFF)
ENDIF()

# try to extract the version from the source
FILE(READ ${CMAKE_CURRENT_SOURCE_DIR}/gemrb/includes/globals.h GLOBALS)
SET(GEMRB_VERSION "")
STRING(REGEX MATCH "define VERSION_GEMRB .([^\"]*)" GEMRB_VERSION "${GLOBALS}")
STRING(REGEX REPLACE "define VERSION_GEMRB .([^\"]*)$" "\\1" GEMRB_VERSION "${GEMRB_VERSION}")
if(GEMRB_VERSION STREQUAL "") # lookup failed
	set(GEMRB_VERSION "unknown")
endif()
message(STATUS "Detected version: ${GEMRB_VERSION}")
unset(GLOBALS)

IF(PREFIX)
	MESSAGE(FATAL_ERROR "CMake started using PREFIX internally, pass CMAKE_INSTALL_PREFIX instead!")
ENDIF(PREFIX)

if (NOT LAYOUT)
	if (WIN32)
		set(LAYOUT "home")
	elseif (APPLE)
		set(LAYOUT "bundle")
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
	SET_PATH( PLUGIN_DIR ${CMAKE_INSTALL_PREFIX}/plugins/ )
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
	SET_PATH( METAINFO_DIR ${CMAKE_INSTALL_PREFIX} )
elseif (${LAYOUT} MATCHES "fhs")
	SET_PATH( LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIBDIR_SUFFIX}/gemrb )
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
	SET_PATH( METAINFO_DIR ${CMAKE_INSTALL_PREFIX}/share/metainfo )
elseif (${LAYOUT} MATCHES "opt")
	SET_PATH( LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIBDIR_SUFFIX} )
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
	SET_PATH( METAINFO_DIR ${CMAKE_INSTALL_PREFIX}/share/metainfo )
else (${LAYOUT} MATCHES "bundle") # Mac or iOS
	SET(CMAKE_INSTALL_RPATH @loader_path/../Frameworks)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET_PATH( BIN_DIR /Applications )
	SET_PATH( LIB_DIR @loader_path/../Frameworks )
	SET_PATH( METAINFO_DIR "" )
endif (${LAYOUT} MATCHES "home")

# convert the slashes for windows' users' convenience
file(TO_NATIVE_PATH "${PLUGIN_DIR}" DEFAULT_PLUGINS_DIR)
file(TO_NATIVE_PATH ./Cache2/ DEFAULT_CACHE_DIR)
if(APPIMAGE)
	file(RELATIVE_PATH DATA_DIR "/" "${DATA_DIR}")
endif()

# check if this is a release version
SET(RC "")
STRING(REGEX MATCH "-git$" RC ${GEMRB_VERSION})
if(RC STREQUAL "")
	set (GIT_VERSION 0)
else()
	set (GIT_VERSION 1)
endif()
MESSAGE(STATUS "Git version bool: ${GIT_VERSION}")

IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	INCLUDE(CheckCXXCompilerFlag)
	if ((NOT DISABLE_WERROR) AND GIT_VERSION)
		string(APPEND CMAKE_CXX_FLAGS " -Werror -Wno-inline -Wno-error=cast-align -Wmissing-declarations")
		if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
			# clang doesn't know as many -Wno-error directives, so we don't set up these in the shared path above
			string(APPEND CMAKE_CXX_FLAGS " -Wcast-align")
			CHECK_CXX_COMPILER_FLAG("-Wno-error=stringop-truncation" NO_STRINGOP_TRUNCATION)
			if (NO_STRINGOP_TRUNCATION)
				string(APPEND CMAKE_CXX_FLAGS " -Wno-error=stringop-truncation -Wno-error=stringop-overflow")
			endif ()
			CHECK_CXX_COMPILER_FLAG("-Wno-error=stringop-overread" NO_STRINGOP_OVERREAD)
			if (NO_STRINGOP_TRUNCATION)
				string(APPEND CMAKE_CXX_FLAGS " -Wno-error=stringop-overread")
			endif ()
		endif ()
	endif ()
	string(APPEND CMAKE_CXX_FLAGS " -Wall -W -Wpointer-arith")
	string(APPEND CMAKE_CXX_FLAGS " -pedantic -Wno-format-y2k -Wno-long-long")
	# only later c++ standards conditionally allow function/object pointer casts
	# gcc pragmas for disabling are broken: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
	if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
		string(APPEND CMAKE_CXX_FLAGS " -Wno-error=pedantic")
	elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6 OR APPLE))
		string(APPEND CMAKE_CXX_FLAGS " -Wno-error=pedantic")
	endif ()
	# mark chars explicitly signed (ARM defaults to unsigned)
	string(APPEND CMAKE_CXX_FLAGS " -fsigned-char")
	# only export symbols explicitly marked to be exported.
	CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" VISIBILITY_HIDDEN)
	IF (VISIBILITY_HIDDEN AND NOT WIN32)
		string(APPEND CMAKE_CXX_FLAGS " -fvisibility=hidden")
	ENDIF ()
	# use a more lenient regex for finding documented switch fallthroughs (gcc7)
	CHECK_CXX_COMPILER_FLAG("-Wimplicit-fallthrough" FALLTHROUGH)
	IF (FALLTHROUGH AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		string(APPEND CMAKE_CXX_FLAGS " -Wimplicit-fallthrough=2")
	ENDIF ()
	if (WIN32)
		# GCC 4.5.0+ has shared libstdc++ without dllimport
		string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--enable-auto-import")
		string(APPEND CMAKE_MODULE_LINKER_FLAGS " -Wl,--enable-auto-import")
	endif (WIN32)
	# Ensure all plugin symbols exist.
	if (NOT APPLE AND NOT UNSAFE_PLUGIN)
		string(APPEND CMAKE_MODULE_LINKER_FLAGS " -Wl,--no-undefined")
	endif (NOT APPLE AND NOT UNSAFE_PLUGIN)

	# Fast math helps us covering some things that need a little more rework soon
	string(APPEND CMAKE_CXX_FLAGS " -ffast-math -frounding-math")
ELSEIF(MSVC)
	string(APPEND CMAKE_CXX_FLAGS " /fp:fast")
ENDIF()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

IF(NOT STATIC_LINK)
	# prevent static libraries from being selected by FIND_LIBRARY
	LIST(REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF()

# Check for all the required and optional dependencies
IF(PYTHON_VERSION STREQUAL "Auto")
	FIND_PACKAGE(PythonLibs 3)
	# Record for reporting later
	SET(PYTHON_VERSION 3)
ELSEIF(PYTHON_VERSION STREQUAL "2")
	MESSAGE(FATAL_ERROR "Python 2 support has been removed, rerun cmake in a clean build dir.")
ELSE()
	FIND_PACKAGE(PythonLibs ${PYTHON_VERSION} REQUIRED)
ENDIF()
IF(PYTHONLIBS_FOUND)
	MESSAGE(STATUS "Looking for Python libraries and headers: found")
ENDIF()

# Autodetection mechanism
# Default priority to SDL
# If both backends are found, preferring SDL2
IF(SDL_BACKEND STREQUAL "Auto")
	MESSAGE(STATUS "Autodetecting SDL backend...")
	IF(WIN32 AND _VCPKG_INSTALLED_DIR)
		find_package(SDL2 CONFIG REQUIRED)
		GET_TARGET_PROPERTY(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
		GET_TARGET_PROPERTY(SDL2_LIBRARY_DEBUG SDL2::SDL2 IMPORTED_IMPLIB_DEBUG)
		GET_TARGET_PROPERTY(SDL2_LIBRARY_RELEASE SDL2::SDL2 IMPORTED_IMPLIB_RELEASE)
		SET(SDL2_LIBRARIES debug ${SDL2_LIBRARY_DEBUG} optimized ${SDL2_LIBRARY_RELEASE})
	ELSE()
		find_package(SDL2)
	ENDIF()
	IF(SDL2_FOUND)
		MESSAGE(STATUS "Found SDL 2.0, switching to SDL2 backend.")
		SET(SDL_BACKEND "SDL2")
	ELSE()
		INCLUDE(FindSDL)
		IF(SDL_FOUND)
			MESSAGE(STATUS "Found SDL 1.2, switching to SDL backend.")
			SET(SDL_BACKEND "SDL")
		ENDIF()
	ENDIF()
	IF(NOT (SDL_FOUND OR SDL2_FOUND))
		MESSAGE(WARNING "No SDL or SDL2 libraries found.")
		MESSAGE(FATAL_ERROR "Please get SDL from www.libsdl.org")
	ENDIF()
ENDIF()

IF(SDL_BACKEND STREQUAL "SDL")
	INCLUDE(FindSDL)
	IF(USE_SDLMIXER)
		find_package(SDL_mixer MODULE)
	ENDIF()
ENDIF()

IF(SDL_BACKEND STREQUAL "SDL2")
	IF(WIN32 AND _VCPKG_INSTALLED_DIR)
		find_package(SDL2 CONFIG REQUIRED)
		GET_TARGET_PROPERTY(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
		GET_TARGET_PROPERTY(SDL2_LIBRARY_DEBUG SDL2::SDL2 IMPORTED_IMPLIB_DEBUG)
		GET_TARGET_PROPERTY(SDL2_LIBRARY_RELEASE SDL2::SDL2 IMPORTED_IMPLIB_RELEASE)
		SET(SDL2_LIBRARIES debug ${SDL2_LIBRARY_DEBUG} optimized ${SDL2_LIBRARY_RELEASE})
	ELSE()
		find_package(SDL2 REQUIRED)
	ENDIF()
	# unify SDL variables, so we don't have to differentiate later
	UNSET(SDL_INCLUDE_DIR CACHE)
	UNSET(SDL_LIBRARY CACHE)
	SET(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIRS})
	SET(SDL_LIBRARY ${SDL2_LIBRARIES})
	SET(SDL_FOUND ${SDL2_FOUND})

	IF(USE_SDLMIXER)
		find_package(SDL2_mixer MODULE)
		# unify variables, so we don't have to differentiate later
		SET(SDL_MIXER_INCLUDE_DIR ${SDL2_MIXER_INCLUDE_DIRS})
		SET(SDL_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARIES})
		SET(SDL_MIXER_VERSION_STRING ${SDL2_MIXER_VERSION_STRING})
		SET(SDL_MIXER_FOUND ${SDL2_MIXER_FOUND})
	ENDIF()
ENDIF()

IF(USE_OPENGL)
	MESSAGE(WARNING "USE_OPENGL has been dropped, use OPENGL_BACKEND instead: None, OpenGL, GLES")
	SET(OPENGL_BACKEND "OpenGL")
ENDIF()

SET(VALID_GL_BACKENDS None OpenGL GLES)
IF(NOT (OPENGL_BACKEND IN_LIST VALID_GL_BACKENDS))
	MESSAGE(FATAL_ERROR "Wrong value passed for OPENGL_BACKEND, use one of: None, OpenGL, GLES")
ENDIF()

IF(NOT SDL_FOUND)
	MESSAGE(WARNING "Looking for SDL: not found!")
	MESSAGE(FATAL_ERROR "Please get SDL from www.libsdl.org")
ENDIF()

IF(SDL_MIXER_FOUND)
	MESSAGE(STATUS "Looking for SDL_mixer: found")
ELSE()
	MESSAGE(WARNING "Looking for SDL_mixer: not found!")
	MESSAGE(WARNING "If you want to build the SDL_mixer plugin, install SDL_mixer first.")
	MESSAGE(WARNING "Make sure you use a version compatible with the chosen SDL version.")
ENDIF()

IF(NOT OPENGL_BACKEND STREQUAL "None")

	IF(SDL_BACKEND STREQUAL "SDL")
		MESSAGE(FATAL_ERROR "SDL2 is required for OpenGL backend support!")
	ENDIF()

	IF(OPENGL_BACKEND STREQUAL "OpenGL")
		ADD_DEFINITIONS("-DUSE_OPENGL_API")
		INCLUDE(FindOpenGL)
		IF(NOT OPENGL_FOUND)
			MESSAGE(FATAL_ERROR "OpenGL library not found!")
		ENDIF()

		# GLEW is required only for Windows
		IF (WIN32)
			find_package(GLEW REQUIRED)
		ENDIF()

	ELSEIF(OPENGL_BACKEND STREQUAL "GLES")
		INCLUDE(FindOpenGLES2)
		IF(NOT OPENGLES2_FOUND)
			MESSAGE(FATAL_ERROR "OpenGLES2 library not found!")
		ENDIF()
	ENDIF()
ENDIF()

find_package(ZLIB REQUIRED)
IF(ZLIB_FOUND)
	MESSAGE(STATUS "Looking for Zlib: found")
ENDIF()

find_package(Iconv REQUIRED)
IF(Iconv_FOUND)
	MESSAGE(STATUS "Looking for iconv: found")
ENDIF()

IF(UNIX)
	SET(CMAKE_THREAD_PREFER_PTHREAD true)
	FIND_PACKAGE(Threads REQUIRED)
ENDIF(UNIX)

IF(USE_OPENAL)
	INCLUDE(FindOpenAL)
	IF(OPENAL_FOUND)
		MESSAGE(STATUS "Looking for OpenAL: found")
	ELSE()
		MESSAGE(WARNING "Looking for OpenAL: not found!")
		MESSAGE(WARNING "If you want to build the OpenAL plugin, get OpenAL from www.openal.org.")
		MESSAGE(WARNING "If it just wasn't found, try setting the OPENALDIR environment variable.")
	ENDIF()
ENDIF()

IF(USE_LIBVLC AND NOT MSVC)
	find_package(LIBVLC MODULE)
	IF(LIBVLC_FOUND)
		MESSAGE(STATUS "Looking for VLC: found")
	ELSE()
		MESSAGE(WARNING "Looking for VLC: not found!")
		MESSAGE(WARNING "If you want to build the VLC plugin, install VLC first.")
	ENDIF()
ENDIF()

IF(USE_FREETYPE)
	INCLUDE(FindFreetype)
	IF(FREETYPE_FOUND)
		MESSAGE(STATUS "Looking for Freetype: found")
	ELSE()
		MESSAGE(WARNING "Looking for Freetype: not found!")
		MESSAGE(WARNING "If you want to build the TTF plugin, install Freetype first.")
		MESSAGE(WARNING "It is required for our demo to work!")
	ENDIF()
ENDIF()

IF(USE_PNG)
	INCLUDE(FindPNG)
	IF(PNG_FOUND)
		MESSAGE(STATUS "Looking for libPNG: found")
	ELSE()
		MESSAGE(WARNING "Looking for libPNG: not found!")
		MESSAGE(WARNING "GemRB will be built without any PNG support. Get it from www.libpng.org" )
		MESSAGE(WARNING "While no original game data is in PNG format, some mod data is and will need conversion.")
		MESSAGE(WARNING "It is required for our demo to work!")
	ENDIF()
ENDIF()

IF(USE_VORBIS)
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
		MESSAGE(WARNING "It is required for our demo to work!")
	ENDIF()
ENDIF()

if(NOT SANITIZE STREQUAL "None" AND NOT SANITIZE STREQUAL "none")
	string(APPEND CMAKE_C_FLAGS " -O0 -g -fsanitize=${SANITIZE} -fno-omit-frame-pointer")
	string(APPEND CMAKE_CXX_FLAGS " -O0 -g -fsanitize=${SANITIZE} -fno-omit-frame-pointer")
	string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=${SANITIZE}")
	string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fsanitize=${SANITIZE}")
	# also CMAKE_MODULE_LINKER_FLAGS for macs?
endif()

# On Release builds cmake automatically defines NDEBUG, so we
# explicitly undefine it:
# This matches stripped (Release) and nonstripped (RelWithDebInfo) builds
if(CMAKE_BUILD_TYPE MATCHES "Rel.*" AND NOT MSVC)
	ADD_DEFINITIONS("-UNDEBUG")
endif()

if (STATIC_LINK)
  if (MSVC)
    unset(STATIC_LINK CACHE)
    MESSAGE(STATUS "Static linking not (yet) supported on this platform.")
  else (MSVC)
    ADD_DEFINITIONS("-DSTATIC_LINK")
  endif (MSVC)
endif (STATIC_LINK)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} gemrb/includes gemrb/core platforms/${CMAKE_SYSTEM_NAME})

# generate config.h
INCLUDE (config)
IF(NOT STATIC_LINK AND NOT HAVE_DLFCN_H AND NOT WIN32)
	MESSAGE(WARNING "Dynamic linking not supported by platform, switching to static!")
	SET(STATIC_LINK)
	ADD_DEFINITIONS("-DSTATIC_LINK")
	# readd, previously removed
	LIST(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF()

IF(APPLE)
	# Make sure we can find the 'ibtool' program. we need it to compile xibs
	find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	IF (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
  		MESSAGE (FATAL_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with 
                    the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	ENDIF ()

	ADD_DEFINITIONS("-x objective-c++")
	INCLUDE_DIRECTORIES(platforms/apple)
	FIND_LIBRARY(COCOA_LIBRARY_PATH Cocoa)
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	# for objective-c++
	STRING(APPEND CMAKE_CXX_FLAGS " -DTARGET_OS_MAC")
	# for pure objective-c
	STRING(APPEND CMAKE_C_FLAGS " -x objective-c -DTARGET_OS_MAC")
ENDIF(APPLE)

IF(VITA)
	INCLUDE(platforms/vita/vita.cmake)
ENDIF(VITA)

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
		elseif (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
			TARGET_LINK_LIBRARIES(${plugin_name} -lbsd)
		endif (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
	endif (STATIC_LINK)

	IF (APPLE)
		SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gemrb/${PROJECT_NAME}.app/Contents/PlugIns)
	ELSE (APPLE)
		IF (NOT STATIC_LINK)
			INSTALL(TARGETS ${plugin_name} DESTINATION ${PLUGIN_DIR})
		ENDIF()
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
	if (NOT HAIKU)
		INSTALL( FILES artwork/gemrb-logo.png DESTINATION ${ICON_DIR} RENAME gemrb.png )
		INSTALL( FILES artwork/logo04-rb_only.svg DESTINATION ${SVG_DIR} RENAME gemrb.svg )
		INSTALL( FILES platforms/linux/gemrb.desktop DESTINATION ${MENU_DIR} )
		INSTALL( FILES platforms/linux/org.gemrb.gemrb.metainfo.xml DESTINATION ${METAINFO_DIR} )
	ENDIF()
	INSTALL( FILES README.md INSTALL COPYING NEWS AUTHORS DESTINATION ${DOC_DIR} )
	INSTALL( FILES admin/extend2da.py DESTINATION ${BIN_DIR}
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
ENDIF()

CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY
)

ADD_CUSTOM_TARGET( uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" )

add_custom_target(appimage
    COMMAND rm -rf ./AppDir GemRB-*-x86_64.AppImage
    COMMAND make install DESTDIR=./AppDir
    COMMAND mv AppDir/usr/usr/share/gemrb AppDir/usr/share/ || true # not always needed
    COMMAND sed -i 's,X-AppImage-Version.*,X-AppImage-Version='`date +%F`-$ENV{GITHUB_SHA}',' AppDir/usr/share/applications/gemrb.desktop
    # bundle core python modules
    # AppRun defaults PYTHONPATH to this destination
    # source is probably available in Python3_STDLIB once we switch to newer cmake
    COMMAND mkdir -p AppDir/usr/share/pyshared
    COMMAND cp -r /usr/lib/python3.8/* AppDir/usr/share/pyshared
    COMMAND rm -rf AppDir/usr/share/pyshared/config-*x86_64-linux-gnu
    COMMAND rm -rf AppDir/usr/share/pyshared/dist-packages
    COMMAND rm -rf AppDir/usr/share/pyshared/test
    COMMAND rm -rf AppDir/usr/share/pyshared/pydoc_data
    COMMAND rm -rf AppDir/usr/share/pyshared/lib2to3
    COMMAND rm -rf AppDir/usr/share/pyshared/config-3*/libpython3*.a
    COMMAND LD_LIBRARY_PATH=./AppDir/usr/lib/gemrb ./linuxdeploy --appdir=./AppDir --output=appimage --custom-apprun="${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux/AppRun.sh"
    WORKING_DIRECTORY ${CMAKE_BUILD_DIR}
)

# make dist for a gzipped tarball of current HEAD
# include fetch_demo_data files manually, since they're not part of the repo
set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${GEMRB_VERSION})
add_custom_target( dist
	COMMAND git archive --worktree-attributes --prefix=${ARCHIVE_NAME}/ -o ${ARCHIVE_NAME}.tar HEAD
	COMMAND tar --append -f ${ARCHIVE_NAME}.tar --transform="s,^,${ARCHIVE_NAME}/," demo/music
	COMMAND gzip --best -c ${ARCHIVE_NAME}.tar > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(fetch-demo-data
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/fetch_demo_data.cmake
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/demo
)
add_dependencies(dist fetch-demo-data)

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
PRINT_OPTION(LAYOUT)
PRINT_OPTION(NOCOLOR)
PRINT_OPTION(STATIC_LINK)
PRINT_OPTION(INSOURCEBUILD)
PRINT_OPTION(DISABLE_WERROR)
PRINT_OPTION(WIN32_USE_STDIO)
PRINT_OPTION(SDL_BACKEND)
PRINT_OPTION(USE_SDL_CONTROLLER_API)
PRINT_OPTION(PYTHON_VERSION)
PRINT_OPTION(OPENGL_BACKEND)
PRINT_OPTION(SANITIZE)
message(STATUS "")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Target bitness: ${CMAKE_SIZEOF_VOID_P}*8")
message(STATUS "")
if ((NOT DISABLE_WERROR) AND GIT_VERSION)
	message(STATUS "Compiler warnings are fatal.")
	message(STATUS "If you hit one, let us know and rerun cmake with -DDISABLE_WERROR=1 to continue.")
	message(STATUS "")
endif()
