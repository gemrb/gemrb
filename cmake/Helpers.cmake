FUNCTION(SET_INTERNAL VAR_NAME VALUE)
	SET(${VAR_NAME} ${VALUE} CACHE INTERNAL "")
ENDFUNCTION()

FUNCTION(ADD_FLAG_IF_SUPPORTED)
	SET(options "")
	SET(oneValueArgs FLAG VALUE)
	SET(multiValueArgs "")
	CMAKE_PARSE_ARGUMENTS("" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	IF(DEFINED _VALUE) # so passing 0 works
		SET(TEST_FLAG "${_FLAG}=${_VALUE}")
	ELSE ()
		SET(TEST_FLAG "${_FLAG}")
	ENDIF ()

	STRING(REGEX REPLACE "[^a-zA-Z0-9]" "_" HAS_FLAG ${TEST_FLAG})
	CHECK_CXX_COMPILER_FLAG(${TEST_FLAG} ${HAS_FLAG})

	IF(${HAS_FLAG})
		IF(DEFINED _VALUE)
			SET(_FLAG "${_FLAG}=${_VALUE}")
		ENDIF ()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_FLAG}" PARENT_SCOPE)
	ENDIF ()
ENDFUNCTION()

FUNCTION(USE_SCCACHE_IF_AVAILABLE)
	FIND_PROGRAM(SCCACHE_PROGRAM "sccache")
	IF(SCCACHE_PROGRAM)
		MESSAGE(STATUS "Using sccache found at ${SCCACHE_PROGRAM} for caching build results")
		SET(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "" FORCE)
		SET(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "" FORCE)
	ENDIF()
ENDFUNCTION()

FUNCTION(CONFIGURE_COMPILER)
	INCLUDE(CheckCXXCompilerFlag)
	IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		IF((NOT DISABLE_WERROR) AND (NOT IS_RELEASE_VERSION))
			STRING(JOIN " "
				CMAKE_CXX_FLAGS
				${CMAKE_CXX_FLAGS}
				"-Werror"
				"-Wno-inline"
				"-Wno-error=cast-align"
				"-Wmissing-declarations"
			)
		ELSEIF(IS_RELEASE_VERSION)
			ADD_FLAG_IF_SUPPORTED(
				FLAG "-ffile-prefix-map"
				VALUE "${CMAKE_SOURCE_DIR}=."
			)		
		ENDIF ()

		STRING(JOIN " "
			CMAKE_CXX_FLAGS
			${CMAKE_CXX_FLAGS}
			"-Wall"
			"-W"
			"-Wpointer-arith"
			"-Wno-format-y2k"
			"-Wno-long-long"
		)

		STRING(APPEND CMAKE_CXX_FLAGS " -pedantic")
		# mark chars explicitly signed (ARM defaults to unsigned)
		ADD_FLAG_IF_SUPPORTED(FLAG "-fsigned-char")
		# only export symbols explicitly marked to be exported.
		# BSDs crash with it #2074
		IF (NOT (BSD AND CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
			ADD_FLAG_IF_SUPPORTED(FLAG "-fvisibility=hidden")
		ENDIF ()
		# Fast math helps us covering some things that need a little more rework soon
		ADD_FLAG_IF_SUPPORTED(FLAG "-ffast-math")
		ADD_FLAG_IF_SUPPORTED(FLAG "-frounding-math")
		ADD_FLAG_IF_SUPPORTED(FLAG "-fno-strict-aliasing")

		IF(WIN32)
			# GCC 4.5.0+ has shared libstdc++ without dllimport
			STRING(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--enable-auto-import")
			STRING(APPEND CMAKE_MODULE_LINKER_FLAGS " -Wl,--enable-auto-import")
		ENDIF (WIN32)
		# Ensure all plugin symbols exist.
		IF(NOT APPLE AND NOT UNSAFE_PLUGIN)
			string(APPEND CMAKE_MODULE_LINKER_FLAGS " -Wl,--no-undefined")
		ENDIF (NOT APPLE AND NOT UNSAFE_PLUGIN)
	ENDIF ()

	IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		STRING(APPEND CMAKE_CXX_FLAGS " -Wcast-align")
		ADD_FLAG_IF_SUPPORTED(
			FLAG "-Wno-error"
			VALUE "stringop-truncation"
		)
		ADD_FLAG_IF_SUPPORTED(
			FLAG "-Wno-error"
			VALUE "stringop-overflow"
		)
		ADD_FLAG_IF_SUPPORTED(
			FLAG "-Wno-error"
			VALUE "stringop-overread"
		)

		IF(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
			# too much noise from false positives
			STRING(APPEND CMAKE_CXX_FLAGS " -Wno-stringop-truncation -Wno-stringop-overflow -Wno-stringop-overread")
		ENDIF ()
		# only later c++ standards conditionally allow function/object pointer casts
		# gcc pragmas for disabling are broken: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
		IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
			STRING(APPEND CMAKE_CXX_FLAGS " -Wno-error=pedantic")
		ENDIF ()

		ADD_FLAG_IF_SUPPORTED(FLAG "-Wimplicit-fallthrough" VALUE "2")
		ADD_FLAG_IF_SUPPORTED(FLAG "-Woverloaded-virtual" VALUE "0")
	ENDIF ()

	IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6 OR APPLE)
			STRING(APPEND CMAKE_CXX_FLAGS " -Wno-error=pedantic")
		ENDIF ()
		ADD_FLAG_IF_SUPPORTED(FLAG "-Wno-nan-infinity-disabled")
	ENDIF ()

	IF(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		# this subtly breaks things, HoW crashes on first area entry
		#STRING(APPEND CMAKE_CXX_FLAGS " /fp:fast")
		ADD_COMPILE_OPTIONS(/MP2) # manually enable some file-level parallelism
	ENDIF()

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" PARENT_SCOPE)
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}" PARENT_SCOPE)
	USE_SCCACHE_IF_AVAILABLE()
ENDFUNCTION()

FUNCTION(CONFIGURE_PYTHON)
	IF(PYTHON_VERSION STREQUAL "Auto")
		FIND_PACKAGE(Python 3 COMPONENTS Development REQUIRED)
		# Record for reporting later
		SET_INTERNAL(PYTHON_VERSION 3)
	ELSEIF(PYTHON_VERSION STREQUAL "2")
		MESSAGE(FATAL_ERROR "Python 2 support has been removed, rerun cmake in a clean build dir.")
	ELSE()
		FIND_PACKAGE(Python ${PYTHON_VERSION} COMPONENTS Development REQUIRED)
	ENDIF()
	IF(Python_FOUND)
		MESSAGE(STATUS "Looking for Python libraries and headers: found")
		INCLUDE_DIRECTORIES(SYSTEM ${Python_INCLUDE_DIRS})
		SET_INTERNAL(PYTHON_INCLUDE_DIRS "${Python_INCLUDE_DIRS}")
		SET_INTERNAL(PYTHON_LIBRARIES "${Python_LIBRARIES}")
	ENDIF()
ENDFUNCTION()

FUNCTION(CONFIGURE_SDL SDL_BACKEND)
	# Autodetection mechanism
	# Default priority to SDL
	# If both backends are found, preferring SDL2
	IF(SDL_BACKEND STREQUAL "Auto")
		MESSAGE(STATUS "Autodetecting SDL backend...")
		IF(WIN32 AND _VCPKG_INSTALLED_DIR)
			FIND_PACKAGE(SDL2 CONFIG REQUIRED)
			GET_TARGET_PROPERTY(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
			GET_TARGET_PROPERTY(SDL2_LIBRARY_DEBUG SDL2::SDL2 IMPORTED_IMPLIB_DEBUG)
			GET_TARGET_PROPERTY(SDL2_LIBRARY_RELEASE SDL2::SDL2 IMPORTED_IMPLIB_RELEASE)
			SET(SDL2_LIBRARIES debug ${SDL2_LIBRARY_DEBUG} optimized ${SDL2_LIBRARY_RELEASE})
		ELSE()
			FIND_PACKAGE(SDL2)
		ENDIF()
		IF(SDL2_FOUND)
			SET(SDL_BACKEND "SDL2" CACHE STRING ${SDL_BACKEND_DESCRIPTION} FORCE)
		ELSE()
			INCLUDE(FindSDL)
			IF(SDL_FOUND)
				SET(SDL_BACKEND "SDL" CACHE STRING ${SDL_BACKEND_DESCRIPTION} FORCE)
			ENDIF()
		ENDIF()
	ELSEIF(SDL_BACKEND STREQUAL "SDL")
		INCLUDE(FindSDL)
		IF(SDL_FOUND)
			SET_INTERNAL(SDL_FOUND ${SDL_FOUND})
		ENDIF()
	ELSEIF(SDL_BACKEND STREQUAL "SDL2")
		IF(WIN32 AND _VCPKG_INSTALLED_DIR)
			FIND_PACKAGE(SDL2 CONFIG REQUIRED)
			GET_TARGET_PROPERTY(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
			GET_TARGET_PROPERTY(SDL2_LIBRARY_DEBUG SDL2::SDL2 IMPORTED_IMPLIB_DEBUG)
			GET_TARGET_PROPERTY(SDL2_LIBRARY_RELEASE SDL2::SDL2 IMPORTED_IMPLIB_RELEASE)
			SET(SDL2_LIBRARIES debug ${SDL2_LIBRARY_DEBUG} optimized ${SDL2_LIBRARY_RELEASE})
		ELSE()
			FIND_PACKAGE(SDL2 REQUIRED)
		ENDIF()
	ENDIF()

	IF(NOT (SDL_FOUND OR SDL2_FOUND))
		MESSAGE(WARNING "Looking for SDL: not found!")
		MESSAGE(FATAL_ERROR "Please get SDL from www.libsdl.org")
	ENDIF()

		# unify SDL variables, so we don't have to differentiate later
		UNSET(SDL_INCLUDE_DIR CACHE)
		UNSET(SDL_LIBRARY CACHE)

		IF(SDL2_FOUND)
			MESSAGE(STATUS "Found SDL 2.0, switching to SDL2 backend.")
			SET_INTERNAL(SDL_FOUND ${SDL2_FOUND})
			SET_INTERNAL(SDL_INCLUDE_DIR "${SDL2_INCLUDE_DIRS}")
			SET_INTERNAL(SDL_LIBRARY "${SDL2_LIBRARIES}")
		ELSE()
			MESSAGE(STATUS "Found SDL 1.2, switching to SDL backend.")
			SET_INTERNAL(SDL_FOUND ${SDL_FOUND})
			SET_INTERNAL(SDL_INCLUDE_DIR "${SDL_INCLUDE_DIRS}")
			SET_INTERNAL(SDL_LIBRARY "${SDL_LIBRARIES}")
		ENDIF()

		IF(USE_SDLMIXER)
			IF(SDL2_FOUND)
				FIND_PACKAGE(SDL2_mixer MODULE)
				# unify variables, so we don't have to differentiate later
				SET_INTERNAL(SDL_MIXER_INCLUDE_DIR ${SDL2_MIXER_INCLUDE_DIRS})
				SET_INTERNAL(SDL_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARIES})
				SET(SDL_MIXER_VERSION_STRING ${SDL2_MIXER_VERSION_STRING})
				SET(SDL_MIXER_FOUND ${SDL2_MIXER_FOUND})
			ELSE()
				FIND_PACKAGE(SDL_mixer MODULE)
				SET_INTERNAL(SDL_MIXER_INCLUDE_DIR ${SDL_MIXER_INCLUDE_DIRS})
				SET_INTERNAL(SDL_MIXER_LIBRARIES ${SDL_MIXER_LIBRARIES})
				SET_INTERNAL(SDL_MIXER_VERSION_STRING ${SDL_MIXER_VERSION_STRING})
				SET_INTERNAL(SDL_MIXER_FOUND ${SDL_MIXER_FOUND})
			ENDIF()
		ENDIF()

	IF(SDL_MIXER_FOUND)
		MESSAGE(STATUS "Looking for SDL_mixer: found")
	ELSEIF(USE_SDLMIXER)
		MESSAGE(WARNING "Looking for SDL_mixer: not found!")
		MESSAGE(WARNING "If you want to build the SDL_mixer plugin, install SDL_mixer first.")
		MESSAGE(WARNING "Make sure you use a version compatible with the chosen SDL version.")
	ENDIF()

	IF(APPLE AND SDL2_LIBDIR)
		SET_INTERNAL(SDL2_LIBDIR ${SDL2_LIBDIR})
	ENDIF()
ENDFUNCTION()

FUNCTION(CONFIGURE_OPENGL OPENGL_BACKEND SDL_BACKEND)
	IF(USE_OPENGL)
		MESSAGE(WARNING "USE_OPENGL has been dropped, use OPENGL_BACKEND instead: None, OpenGL, GLES")
		SET(OPENGL_BACKEND "OpenGL")
	ENDIF()

		IF(OPENGL_BACKEND STREQUAL "None")
			RETURN()
		ENDIF()

	SET(VALID_GL_BACKENDS None OpenGL GLES)
	IF(NOT (OPENGL_BACKEND IN_LIST VALID_GL_BACKENDS))
		MESSAGE(FATAL_ERROR "Wrong value passed for OPENGL_BACKEND, use one of: None, OpenGL, GLES")
	ENDIF()

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
				FIND_PACKAGE(GLEW REQUIRED)
			ENDIF()
		ELSEIF(OPENGL_BACKEND STREQUAL "GLES")
			INCLUDE(FindOpenGLES2)
			IF(NOT OPENGLES2_FOUND)
				MESSAGE(FATAL_ERROR "OpenGLES2 library not found!")
			ENDIF()
		ENDIF()
		SET_INTERNAL(OPENGL_LIBRARIES "${OPENGL_LIBRARIES}")
		SET_INTERNAL(OPENGLES2_LIBRARIES "${OPENGLES2_LIBRARIES}")
		SET_INTERNAL(GLEW_LIBRARIES "${GLEW_LIBRARIES}")
ENDFUNCTION()

FUNCTION(CONFIGURE_FOR_SANITIZE SANITIZE)
	STRING(TOLOWER ${SANITIZE} SANITIZE_LOWERCASE)
	IF(NOT SANITIZE_LOWERCASE STREQUAL "none")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -fsanitize=${SANITIZE} -fno-omit-frame-pointer" PARENT_SCOPE)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fsanitize=${SANITIZE} -fno-omit-frame-pointer" PARENT_SCOPE)
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${SANITIZE}" PARENT_SCOPE)
		SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=${SANITIZE}" PARENT_SCOPE)
		# also CMAKE_MODULE_LINKER_FLAGS for macs?
	ENDIF()
ENDFUNCTION()

FUNCTION(SET_PATH variable default)
	IF(NOT ${variable})
		SET(${variable} ${default} CACHE INTERNAL "")
	ENDIF()
ENDFUNCTION(SET_PATH)

FUNCTION(CONFIGURE_DIRECTORY_LAYOUT)
	IF (NOT LAYOUT)
		IF (WIN32)
			set(LAYOUT "home")
		ELSEIF (APPLE)
			set(LAYOUT "bundle")
		ELSE (APPLE)
			set(LAYOUT "fhs")
		ENDIF (WIN32)
	ENDIF (NOT LAYOUT)

	SET(LAYOUT "${LAYOUT}" CACHE STRING "Directory layout.")

	IF (${LAYOUT} MATCHES "home")
		SET_PATH(PLUGIN_DIR ${CMAKE_INSTALL_PREFIX}/plugins/)
		SET_PATH(DATA_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(MAN_DIR ${CMAKE_INSTALL_PREFIX}/man/man6)
		SET_PATH(BIN_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(SYSCONF_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(LIB_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(DOC_DIR ${CMAKE_INSTALL_PREFIX}/doc)
		SET_PATH(ICON_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(SVG_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(MENU_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(EXAMPLE_CONF_DIR ${CMAKE_INSTALL_PREFIX})
		SET_PATH(METAINFO_DIR ${CMAKE_INSTALL_PREFIX})
	ELSEIF (${LAYOUT} MATCHES "fhs")
		SET_PATH(LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIBDIR_SUFFIX}/gemrb)
		SET_PATH(PLUGIN_DIR ${LIB_DIR}/plugins)
		SET_PATH(DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/gemrb)
		SET_PATH(MAN_DIR ${CMAKE_INSTALL_PREFIX}/share/man/man6)
		SET_PATH(BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
		IF(NOT SYSCONF_DIR)
			IF(${CMAKE_INSTALL_PREFIX} STREQUAL "/usr")
				SET_PATH(SYSCONF_DIR /etc/gemrb)
			ELSE()
				SET_PATH(SYSCONF_DIR ${CMAKE_INSTALL_PREFIX}/etc/gemrb)
			ENDIF()
		ENDIF()
		SET_PATH(DOC_DIR ${CMAKE_INSTALL_PREFIX}/share/doc/gemrb)
		SET_PATH(ICON_DIR ${CMAKE_INSTALL_PREFIX}/share/pixmaps)
		SET_PATH(SVG_DIR ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
		SET_PATH(MENU_DIR ${CMAKE_INSTALL_PREFIX}/share/applications)
		SET_PATH(EXAMPLE_CONF_DIR ${SYSCONF_DIR})
		SET_PATH(METAINFO_DIR ${CMAKE_INSTALL_PREFIX}/share/metainfo)
	ELSEIF (${LAYOUT} MATCHES "opt")
		SET_PATH(LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIBDIR_SUFFIX})
		SET_PATH(PLUGIN_DIR ${LIB_DIR}/plugins)
		SET_PATH(DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/)
		SET_PATH(MAN_DIR ${CMAKE_INSTALL_PREFIX}/man/man6)
		SET_PATH(BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
		SET_PATH(SYSCONF_DIR ${CMAKE_INSTALL_PREFIX}/etc)
		SET_PATH(DOC_DIR ${CMAKE_INSTALL_PREFIX}/share/doc/gemrb)
		SET_PATH(ICON_DIR ${CMAKE_INSTALL_PREFIX}/share/pixmaps)
		SET_PATH(SVG_DIR ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
		SET_PATH(MENU_DIR ${CMAKE_INSTALL_PREFIX}/share/applications)
		SET_PATH(EXAMPLE_CONF_DIR ${SYSCONF_DIR})
		SET_PATH(METAINFO_DIR ${CMAKE_INSTALL_PREFIX}/share/metainfo)
	ELSE (${LAYOUT} MATCHES "bundle") # Mac or iOS
		SET(CMAKE_INSTALL_RPATH @loader_path/../Frameworks)
		SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
		SET_PATH(BIN_DIR /Applications)
		SET_PATH(LIB_DIR @loader_path/../Frameworks)
		SET_PATH(METAINFO_DIR "")
	ENDIF (${LAYOUT} MATCHES "home")
ENDFUNCTION()

FUNCTION(READ_GEMRB_VERSION)
	# try to extract the version from the source
	FILE(READ ${CMAKE_CURRENT_SOURCE_DIR}/gemrb/core/InterfaceConfig.h GLOBALS)
	SET(GEMRB_VERSION "")
	STRING(REGEX MATCH "define VERSION_GEMRB .([^\"]*)" GEMRB_VERSION "${GLOBALS}")
	STRING(REGEX REPLACE "define VERSION_GEMRB .([^\"]*)$" "\\1" GEMRB_VERSION "${GEMRB_VERSION}")
	IF(GEMRB_VERSION STREQUAL "") # lookup failed
		set(GEMRB_VERSION "unknown")
	ENDIF()
	message(STATUS "Detected version: ${GEMRB_VERSION}")
	SET_INTERNAL(GEMRB_VERSION ${GEMRB_VERSION})
	unset(GLOBALS)
ENDFUNCTION()

FUNCTION(CACHE_BUILD_TYPE)
	# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
	# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
	# to Release prior to calling PROJECT()
	SET(BUILD_TYPE RelWithDebInfo)
	SET(BUILD_TYPE_DESCRIPTION
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	)
	IF(DEFINED CMAKE_BUILD_TYPE)
		SET(BUILD_TYPE ${CMAKE_BUILD_TYPE})
	ENDIF()

	SET(CMAKE_BUILD_TYPE ${BUILD_TYPE} CACHE STRING ${BUILD_TYPE_DESCRIPTION})
ENDFUNCTION()

FUNCTION(ENABLE_LINK_TIME_OPTIMIZATIONS)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT supported)

	IF(supported)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE BOOL "")
	ENDIF()
ENDFUNCTION()

FUNCTION(CHECK_IS_RELEASE_VERSION)
	SET(RC "")
	STRING(REGEX MATCH "-git$" RC ${GEMRB_VERSION})
	IF(RC STREQUAL "")
		SET(IS_RELEASE_VERSION 1)
	ELSE()
		SET(IS_RELEASE_VERSION 0)
	ENDIF()
	MESSAGE(STATUS "IS_RELEASE_VERSION: ${IS_RELEASE_VERSION}")
	SET_INTERNAL(IS_RELEASE_VERSION ${IS_RELEASE_VERSION})
ENDFUNCTION()

FUNCTION(ADD_GEMRB_PLUGIN plugin_name)
	SET(PLUGIN_BUILD_FILES ${ARGN})
	IF(${ARGV1} STREQUAL "COCOA")
		LIST(REMOVE_ITEM PLUGIN_BUILD_FILES "COCOA")
		#this is an Apple thing
		IF(APPLE)
			message(STATUS "Will link ${plugin_name} plugin to: ${BUNDLE_LOADER}")
			SET (PLUGIN_BUILD_FILES ${PLUGIN_BUILD_FILES} CocoaWrapper.m)
		ENDIF(APPLE)
	ENDIF (${ARGV1} STREQUAL "COCOA")

	IF(STATIC_LINK)
		ADD_LIBRARY(${plugin_name} STATIC ${PLUGIN_BUILD_FILES})
		SET(plugins "${plugins};${plugin_name}" CACHE INTERNAL "")
	ELSE(STATIC_LINK)
		ADD_LIBRARY(${plugin_name} MODULE ${PLUGIN_BUILD_FILES})
		IF(NOT UNSAFE_PLUGIN)
			TARGET_LINK_LIBRARIES(${plugin_name} gemrb_core ${CMAKE_THREAD_LIBS_INIT})
		ENDIF (NOT UNSAFE_PLUGIN)
		IF (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
			TARGET_LINK_LIBRARIES(${plugin_name} -shared-libgcc)
		ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
			TARGET_LINK_LIBRARIES(${plugin_name} -lc)
		ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
			TARGET_LINK_LIBRARIES(${plugin_name} -lbsd)
		ENDIF (CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
	ENDIF (STATIC_LINK)
	target_compile_definitions(${plugin_name} PRIVATE _USE_MATH_DEFINES)

	IF(APPLE)
		SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gemrb/${PROJECT_NAME}.app/Contents/PlugIns)
	ELSE (APPLE)
		IF(NOT STATIC_LINK)
			INSTALL(TARGETS ${plugin_name} DESTINATION ${PLUGIN_DIR})
		ENDIF()
		SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX ""
			INSTALL_RPATH ${LIB_DIR}
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gemrb/plugins)
	ENDIF(APPLE)
ENDFUNCTION(ADD_GEMRB_PLUGIN)

FUNCTION(ADD_GEMRB_PLUGIN_TEST plugin_name)
	# TODO: think of a way to add additional libs for linking
	IF (BUILD_TESTING)
		ADD_EXECUTABLE(Test_${plugin_name} ${ARGN})
		target_compile_definitions(Test_${plugin_name} PRIVATE _USE_MATH_DEFINES)

		TARGET_LINK_LIBRARIES(Test_${plugin_name} GTest::gtest GTest::gtest_main gemrb_core)

		# copy test executables to where the core lib is
		IF (WIN32)
			ADD_CUSTOM_COMMAND(TARGET Test_${plugin_name} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Test_${plugin_name}> $<TARGET_FILE_DIR:gemrb_core>
			)

			# 3rd party DLLs are somewhere in the build directory
			IF (MSVC)
				ADD_TEST(NAME Test_${plugin_name}
					COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/gemrb;$ENV{PATH}" $<TARGET_FILE_DIR:gemrb_core>/$<TARGET_FILE_NAME:Test_${plugin_name}>
					WORKING_DIRECTORY $<TARGET_FILE_DIR:gemrb_core>
				)
			ELSE ()
				ADD_TEST(NAME Test_${plugin_name}
					COMMAND $<TARGET_FILE_DIR:gemrb_core>/$<TARGET_FILE_NAME:Test_${plugin_name}>
					WORKING_DIRECTORY $<TARGET_FILE_DIR:gemrb_core>
				)
			ENDIF()
		# set env to where the core lib is
		ELSE()
			ADD_TEST(NAME Test_${plugin_name}
				COMMAND $<TARGET_FILE:Test_${plugin_name}>
				WORKING_DIRECTORY $<TARGET_FILE_DIR:gemrb_core>
			)
			SET_PROPERTY(TEST Test_${plugin_name} PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:gemrb_core>")
			SET_PROPERTY(TEST Test_${plugin_name} PROPERTY ENVIRONMENT "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:gemrb_core>")
		ENDIF()
	ENDIF()
ENDFUNCTION()

# pretty-print options macro
# as of 2.8 cmake does not support anything like EVAL
MACRO(PRINT_OPTION option)
if (${option})
	message(STATUS "  ${option}: ${${option}}")
else()
	message(STATUS "  ${option}: disabled")
endif()
ENDMACRO(PRINT_OPTION)

FUNCTION(CONFIGURE_APPLE_SPECIFICS)
	# favor mac frameworks over unix libraries
	SET(CMAKE_FIND_FRAMEWORK FIRST)
	SET_INTERNAL(BAKE_ICON 0)
	SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
	# Make sure we can find the 'ibtool' program. we need it to compile xibs
	FIND_PROGRAM(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	IF (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
		MESSAGE (FATAL_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with 
					the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	ENDIF ()
	INCLUDE_DIRECTORIES(platforms/apple)
	FIND_LIBRARY(COCOA_LIBRARY_PATH Cocoa)
	FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
	add_compile_definitions(TARGET_OS_MAC)
ENDFUNCTION()

FUNCTION(CONFIGURE_VITA_SPECIFICS)
	INCLUDE(platforms/vita/vita.cmake)
ENDFUNCTION()

FUNCTION(CONFIGURE_UNIX_SPECIFICS)
	SET_INTERNAL(CMAKE_THREAD_PREFER_PTHREAD true)
ENDFUNCTION()

FUNCTION(CONFIGURE_RPI_SPECIFICS)
	# check for RaspberryPi; on newer 64 bit RaspberryPi OS there is no /opt/vc/, check for raspi.list in /etc/apt/sources.list.d/
	FIND_FILE(RPI NAMES bcm_host.h raspi.list PATHS "/opt/vc/include" "/etc/apt/sources.list.d/")

	IF(RPI AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		MESSAGE(FATAL_ERROR
			"GCC v12/13 have shown to cause a mysterious and severe bug on RPi (see issue #2088)."
			" Please use Clang in the meantime, or check if a newer GCC version may work again."
		)
	ENDIF()

	# By default, Pi0 to Pi3 models use the legacy (Broadcom) GLESv2 drivers, from /opt/vc.
	# Newer models (Pi4) don't support it, using the open source MESA drivers.
	# NOTE: the Pi3B(+) models can also run with open source MESA drivers, but support for it must be explicitely enabled
	IF(RPI AND NOT DISABLE_VIDEOCORE)
		SET_INTERNAL(HINT_GLES_LIBNAME brcmGLESv2)
		SET_INTERNAL(HINT_GLES_INCDIR "/opt/vc/include")
		SET_INTERNAL(HINT_GLES_LIBDIR "/opt/vc/lib")
		SET_INTERNAL(OPENGL_BACKEND GLES)
		SET(SDL_BACKEND "SDL2" CACHE STRING ${SDL_BACKEND_DESCRIPTION} FORCE)
	ENDIF()
ENDFUNCTION()

FUNCTION(CONFIGURE_TARGET_PLATFORM_SPECIFICS)
	SET_INTERNAL(BAKE_ICON 1)
	IF(APPLE)
		CONFIGURE_APPLE_SPECIFICS()
	ENDIF()

	IF(VITA)
		CONFIGURE_VITA_SPECIFICS()
	ENDIF()

	IF(UNIX)
		CONFIGURE_UNIX_SPECIFICS()
		CONFIGURE_RPI_SPECIFICS()
	ENDIF()
ENDFUNCTION()

FUNCTION(CONFIGURE_LINKING)
	IF(STATIC_LINK)
		IF(MSVC)
			UNSET(STATIC_LINK CACHE)
			MESSAGE(STATUS "Static linking not (yet) supported on this platform.")
		ELSE()
			ADD_DEFINITIONS("-DSTATIC_LINK")
		ENDIF()
	ELSE()
		IF(NOT HAVE_DLFCN_H AND NOT WIN32)
			MESSAGE(WARNING "Dynamic linking not supported by platform, switching to static!")
			SET(STATIC_LINK 1 CACHE FORCE "")
			ADD_DEFINITIONS("-DSTATIC_LINK")
			LIST(FIND CMAKE_FIND_LIBRARY_SUFFIXES ".a" FOUND)
			IF(NOT (FOUND EQUAL "-1"))
				LIST(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a")
			ENDIF()
		ENDIF()
	ENDIF()
ENDFUNCTION()

FUNCTION(CONFIGURE_FMT_INCLUDES)
	# supress static analysis of fmt
	INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/includes/fmt)
	# ensure fmt gets included properly
	ADD_DEFINITIONS("-DFMT_HEADER_ONLY -DFMT_EXCEPTIONS=0")
ENDFUNCTION()

FUNCTION(MAKE_UNINSTALL_TARGET)
	CONFIGURE_FILE(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
		@ONLY
	)
	ADD_CUSTOM_TARGET(
		uninstall "${CMAKE_COMMAND}"
			-P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	)
ENDFUNCTION()

FUNCTION(MAKE_DIST_TARGET)
	# make dist for a gzipped tarball of current HEAD
	set(PKG_NAME ${CMAKE_PROJECT_NAME}-${GEMRB_VERSION})
	ADD_CUSTOM_TARGET(dist
		COMMAND
			git archive --worktree-attributes --prefix=${PKG_NAME}/ -o ${PKG_NAME}.tar HEAD
		COMMAND
			tar --append -f ${PKG_NAME}.tar --transform="s,^,${PKG_NAME}/," demo/music
		COMMAND
			gzip --best -c ${PKG_NAME}.tar > ${CMAKE_BINARY_DIR}/${PKG_NAME}.tar.gz
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)

	# include fetch_demo_data files manually, since they're not part of the repo
	ADD_CUSTOM_TARGET(fetch-demo-data
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/fetch_demo_data.cmake
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/demo
	)
	ADD_DEPENDENCIES(dist fetch-demo-data)
ENDFUNCTION()

FUNCTION(MAKE_APPIMAGE_TARGET)
	SET(SHARE "AppDir/usr/share")
	ADD_CUSTOM_TARGET(appimage
		COMMAND
			rm -rf ./AppDir GemRB-*-x86_64.AppImage
		COMMAND
			make install DESTDIR=./AppDir
		COMMAND
			mv AppDir/usr/usr/share/gemrb ${SHARE}/ || true # not always needed
		COMMAND
			sed -i 's,X-AppImage-Version.*,X-AppImage-Version='`date +%F`-$ENV{GITHUB_SHA}',' ${SHARE}/applications/gemrb.desktop
		# bundle core python modules
		# AppRun defaults PYTHONPATH to this destination
		# source is probably available in Python3_STDLIB once we switch to newer cmake
		COMMAND
			mkdir -p ${SHARE}/pyshared
		COMMAND
			cp -r /usr/lib/python3.8/* ${SHARE}/pyshared
		COMMAND
			rm -rf ${SHARE}/pyshared/config-*x86_64-linux-gnu
		COMMAND
			rm -rf ${SHARE}/pyshared/dist-packages
		COMMAND
			rm -rf ${SHARE}/pyshared/test
		COMMAND
			rm -rf ${SHARE}/pyshared/pydoc_data
		COMMAND
			rm -rf ${SHARE}/pyshared/lib2to3
		COMMAND
			rm -rf ${SHARE}/pyshared/config-3*/libpython3*.a
		COMMAND
			LD_LIBRARY_PATH=./AppDir/usr/lib/gemrb ./linuxdeploy --appdir=./AppDir --output=appimage --custom-apprun="${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux/AppRun.sh"
		WORKING_DIRECTORY ${CMAKE_BUILD_DIR}
	)
ENDFUNCTION()

FUNCTION(INSTALL_APP_RESOURCES)

	CONFIGURE_FILE(
		"${CMAKE_CURRENT_SOURCE_DIR}/gemrb.6.in"
		"${CMAKE_CURRENT_BINARY_DIR}/gemrb.6"
		@ONLY
	)

	IF (NOT APPLE)
		INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/gemrb.6" DESTINATION ${MAN_DIR})
		SET(ADMIN_PATH ${CMAKE_SOURCE_DIR}/admin)
		IF (NOT HAIKU)
			SET(ARTWORK_PATH ${CMAKE_SOURCE_DIR}/artwork)
			SET(LINUX_PATH ${CMAKE_SOURCE_DIR}/platforms/linux)

			INSTALL(FILES ${ARTWORK_PATH}/gemrb-logo.png DESTINATION ${ICON_DIR} RENAME gemrb.png)
			INSTALL(FILES ${ARTWORK_PATH}/logo04-rb_only.svg DESTINATION ${SVG_DIR} RENAME gemrb.svg)
			INSTALL(FILES ${LINUX_PATH}/gemrb.desktop DESTINATION ${MENU_DIR})
			INSTALL(FILES ${LINUX_PATH}/org.gemrb.gemrb.metainfo.xml DESTINATION ${METAINFO_DIR})
		ENDIF()
		INSTALL(FILES ${CMAKE_SOURCE_DIR}/README.md INSTALL COPYING NEWS AUTHORS DESTINATION ${DOC_DIR})
		INSTALL(FILES ${ADMIN_PATH}/extend2da.py DESTINATION ${BIN_DIR}
			PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	ENDIF()
ENDFUNCTION()
