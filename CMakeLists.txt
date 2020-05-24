cmake_minimum_required(VERSION 3.1)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_CXX_STANDARD 11)

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

# Options list
SET(SDL_BACKEND "Auto" CACHE STRING "libSDL version (possible values: Auto, SDL or SDL2)")
SET_PROPERTY(CACHE SDL_BACKEND PROPERTY STRINGS Auto SDL SDL2)

SET(OPENGL_BACKEND "None" CACHE STRING "OpenGL support (requires SDL2, possible values: None, OpenGL, GLES)")
SET_PROPERTY(CACHE OPENGL_BACKEND PROPERTY STRINGS None OpenGL GLES)

OPTION(USE_SDLMIXER "Enable SDL_mixer support" ON)
OPTION(USE_OPENAL "Enable OpenAL support" ON)
OPTION(USE_LIBVLC "Enable LibVLC support" ON)
OPTION(USE_FREETYPE "Enable FreeType support" ON)
OPTION(USE_PNG "Enable LibPNG support" ON)
OPTION(USE_VORBIS "Enable Vorbis support" ON)
OPTION(USE_ICONV "Enable Iconv support" ON)

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
	SET(CMAKE_INSTALL_PREFIX ${PREFIX})
	# cmake 3 makes internal use of this variable ...
	UNSET(PREFIX)
	UNSET(PREFIX CACHE)
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
# convert the slashes for windows' users' convenience
file(TO_NATIVE_PATH ${PLUGIN_DIR} DEFAULT_PLUGINS_DIR)
file(TO_NATIVE_PATH ./Cache2/ DEFAULT_CACHE_DIR)

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
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-error=inline -Wno-error=cast-align -Wmissing-declarations")
		if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
			# clang doesn't know as many -Wno-error directives, so we don't set up these in the shared path above
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align -Winline")
			CHECK_CXX_COMPILER_FLAG("-Wno-error=stringop-truncation" NO_STRINGOP_TRUNCATION)
			if (NO_STRINGOP_TRUNCATION)
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=stringop-truncation")
			endif ()
		else()
			# hit in python2 includes
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-register")
		endif ()
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wpointer-arith ")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wno-format-y2k -Wno-long-long")
	# only later c++ standards conditionally allow function/object pointer casts
	# gcc pragmas for disabling are broken: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
	if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=pedantic")
	elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6 OR APPLE))
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=pedantic")
	endif ()
	# mark chars explicitly signed (ARM defaults to unsigned)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
	# only export symbols explicitly marked to be exported.
	CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" VISIBILITY_HIDDEN)
	IF (VISIBILITY_HIDDEN AND NOT WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	ENDIF ()
	CHECK_CXX_COMPILER_FLAG("-fno-stack-protector" STACK_PROTECTOR)
	IF (STACK_PROTECTOR)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector")
	ENDIF ()
	# use a more lenient regex for finding documented switch fallthroughs (gcc7)
	CHECK_CXX_COMPILER_FLAG("-Wimplicit-fallthrough" FALLTHROUGH)
	IF (FALLTHROUGH AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wimplicit-fallthrough=2")
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
ENDIF()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

IF(NOT STATIC_LINK)
	# prevent static libraries from being selected by FIND_LIBRARY
	LIST(REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF()

# Check for all the required and optional dependencies
FIND_PACKAGE(PythonLibs 2.3 REQUIRED)
IF(PYTHONLIBS_FOUND)
	MESSAGE(STATUS "Looking for Python libraries and headers: found")
ELSE()
	MESSAGE(SEND_ERROR "Looking for Python libraries and headers: not found!")
	MESSAGE(FATAL_ERROR "Please get them (www.python.org)")
ENDIF()

# Autodetection mechanism
# Default priority to SDL
# If found both backends and USE_SDL2 is active, switching to SDL2
IF(SDL_BACKEND STREQUAL "Auto")
	MESSAGE(STATUS "Autodetecting SDL backend...")
	INCLUDE(FindSDL)
	INCLUDE(FindSDL2)
	IF(SDL_FOUND)
		MESSAGE(STATUS "Found SDL 1.2, switching to SDL backend.")
		SET(SDL_BACKEND "SDL")
	ENDIF()
	IF((NOT SDL_FOUND AND SDL2_FOUND) OR (SDL_FOUND AND SDL2_FOUND AND USE_SDL2))
		MESSAGE(STATUS "Found SDL 2.0, switching to SDL2 backend.")
		SET(SDL_BACKEND "SDL2")
	ENDIF()
	IF(NOT (SDL_FOUND OR SDL2_FOUND))
		MESSAGE(WARNING "No SDL or SDL2 libraries found.")
		MESSAGE(FATAL_ERROR "Please get SDL from www.libsdl.org")
	ENDIF()
ENDIF()

IF((SDL_BACKEND STREQUAL "SDL") AND NOT USE_SDL2)
	INCLUDE(FindSDL)
	IF(USE_SDLMIXER)
		find_package(SDL_mixer MODULE)
	ENDIF()
ENDIF()

IF((SDL_BACKEND STREQUAL "SDL2") OR USE_SDL2)
	INCLUDE(FindSDL2)
	# unify SDL variables, so we don't have to differentiate later
	SET(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
	SET(SDL_LIBRARY ${SDL2_LIBRARY})
	SET(SDL_FOUND ${SDL2_FOUND})

	IF(USE_SDLMIXER)
		find_package(SDL2_mixer MODULE)
		# unify variables, so we don't have to differentiate later
		SET(SDL_MIXER_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIRS})
		SET(SDL_MIXER_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIRS})
		SET(SDL_MIXER_VERSION_STRING ${SDL2_MIXER_VERSION_STRING})
		SET(SDL_MIXER_FOUND ${SDL2_MIXER_FOUND})
	ENDIF()
ENDIF()

# backward compatibility notice - remove USE_SDL2 and OPENGL_BACKEND in the future (added mid 2016)
IF(USE_SDL2)
	MESSAGE(WARNING "USE_SDL2 has been deprecated, use SDL_BACKEND instead: Auto, SDL, SDL2.")
ENDIF()
IF(USE_OPENGL)
	MESSAGE(WARNING "USE_OPENGL has been deprecated, use OPENGL_BACKEND instead: None, OpenGL, GLES.")
ENDIF()

IF(SDL_FOUND)
	MESSAGE(STATUS "Looking for SDL: found")
	# unify SDL variables, so we don't have to differentiate later
	MESSAGE(STATUS "Looking for libSDL: found")
ELSE()
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

IF((NOT OPENGL_BACKEND STREQUAL "None") OR USE_OPENGL)
	IF(SDL_BACKEND STREQUAL "SDL")
		MESSAGE(FATAL_ERROR "SDL2 is required for the OpenGL driver!")
	ENDIF()
	INCLUDE(FindOpenGL)
	IF(NOT OPENGL_FOUND)
		MESSAGE(FATAL_ERROR "OpenGL library not found!")
	ENDIF()

	IF((OPENGL_BACKEND STREQUAL "OpenGL") OR (USE_OPENGL EQUAL 1))
		ADD_DEFINITIONS("-DUSE_GL")
		# Don't delete this - there is code that relies on -DUSE_OPENGL=1
		SET(USE_OPENGL 1)
		INCLUDE(FindGLEW)
		IF(GLEW_FOUND)
			MESSAGE(STATUS "Looking for Glew: found")
		ELSE()
			MESSAGE(SEND_ERROR "Looking for Glew: not found!")
			MESSAGE(FATAL_ERROR "Please install the Glew library and headers first!")
		ENDIF()
	ENDIF()

	IF((OPENGL_BACKEND STREQUAL "GLES") OR (USE_OPENGL EQUAL 2))
		# Don't delete this - there is code that relies on -DUSE_OPENGL=2
		SET(USE_OPENGL 2)
		# use GLES instead of regular GL
		SET(GLEW_LIBRARIES "")
		find_path(GLES2_H gl2.h PATH_SUFFIXES GLES2)
		IF(APPLE)
			find_path(GLES2_H gl.h PATH_SUFFIXES OpenGLES/ES2)
		ENDIF()
		IF(GLES2_H)
			MESSAGE(STATUS "Looking for GLES2: found")
		ELSE()
			MESSAGE(SEND_ERROR "Looking for GLES2: not found!")
			MESSAGE(FATAL_ERROR "Please install the GLES2 library and headers first!")
		ENDIF()
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
		IF(USE_ICONV)
			INCLUDE(FindICONV)
			IF(NOT ICONV_FOUND)
				MESSAGE(WARNING "Iconv not found. TTF plugin will not be able to translate non-unicode compatible TLK encodings.")
			ELSE()
				set(HAVE_ICONV 1)
			ENDIF()
		ENDIF()
	ELSE()
		MESSAGE(WARNING "Looking for Freetype: not found!")
		MESSAGE(WARNING "If you want to build the TTF plugin, install Freetype first.")
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
	ENDIF()
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
		elseif (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
			TARGET_LINK_LIBRARIES(${plugin_name} -lbsd)
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
	if (NOT HAIKU)
		INSTALL( FILES artwork/gemrb-logo.png DESTINATION ${ICON_DIR} RENAME gemrb.png )
		INSTALL( FILES artwork/logo04-rb_only.svg DESTINATION ${SVG_DIR} RENAME gemrb.svg )
		INSTALL( FILES gemrb.desktop DESTINATION ${MENU_DIR} )
	ENDIF()
ENDIF()
INSTALL( FILES README.md INSTALL COPYING NEWS AUTHORS DESTINATION ${DOC_DIR} )
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
	COMMAND git archive --worktree-attributes --prefix=${ARCHIVE_NAME}/ HEAD
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
PRINT_OPTION(LAYOUT)
PRINT_OPTION(NOCOLOR)
PRINT_OPTION(STATIC_LINK)
PRINT_OPTION(INSOURCEBUILD)
PRINT_OPTION(DISABLE_WERROR)
PRINT_OPTION(WIN32_USE_STDIO)
PRINT_OPTION(SDL_BACKEND)
PRINT_OPTION(OPENGL_BACKEND)
message(STATUS "")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Target bitness: ${CMAKE_SIZEOF_VOID_P}*8")
message(STATUS "")
if ((NOT DISABLE_WERROR) AND GIT_VERSION)
	message(STATUS "Compiler warnings are fatal.")
	message(STATUS "If you hit one, let us know and rerun cmake with -DDISABLE_WERROR=1 to continue.")
	message(STATUS "")
endif()
