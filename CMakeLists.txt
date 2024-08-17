CMAKE_MINIMUM_REQUIRED(VERSION 3.19)

SET(CMAKE_CXX_STANDARD 14)

LIST(APPEND CMAKE_MODULE_PATH
	"${CMAKE_SOURCE_DIR}/cmake"
	"${CMAKE_SOURCE_DIR}/cmake/modules/"
)

INCLUDE(Validations)
INCLUDE(Helpers)

# skip verbose make install output
SET(CMAKE_INSTALL_MESSAGE "NEVER")

CACHE_BUILD_TYPE()

if(CMAKE_BUILD_TYPE MATCHES "Rel.*")
	ENABLE_LINK_TIME_OPTIMIZATIONS()
	# On Release builds cmake automatically defines NDEBUG,
	# so we explicitly undefine it:
	# This matches stripped (Release) and nonstripped (RelWithDebInfo) builds
	ADD_DEFINITIONS("-UNDEBUG")
endif()

SET(CMAKE_PROJECT_INCLUDE_BEFORE
	"${CMAKE_SOURCE_DIR}/cmake/IncludeBeforeProject.cmake"
)


PROJECT(gemrb)
IF(APPLE)
	enable_language(OBJC OBJCXX)
ENDIF()

# use relative rpaths for build relocation / reproducibility
SET(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# Options list
SET(SDL_BACKEND_DESCRIPTION "libSDL version (possible values: Auto, SDL or SDL2)")
SET(SDL_BACKEND "Auto" CACHE STRING ${SDL_BACKEND_DESCRIPTION})
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
OPTION(USE_TESTS "Enable building and running of tests" OFF)
OPTION(USE_VORBIS "Enable Vorbis support" ON)
OPTION(DISABLE_WERROR "Do not treat warnings as errors" OFF)
OPTION(USE_TRACY "Build with Tracy support" OFF)

OPTION(USE_SDL_CONTROLLER_API "Enable SDL controller APIs. (disable if you plan on handling controller input in an external program like gptokeyb)" ON)

#VCPKG dll deployment is circumvented because it doesn't currently work for gemrb
IF(WIN32 AND _VCPKG_INSTALLED_DIR)
	OPTION(VCPKG_AUTO_DEPLOY "Fix VCPKG dependency DLL locations" ON)
	# This variable disables the built in VCPKG deployment script,
	# which makes copies of the DLL files in places they are not ever found by the game
	SET(VCPKG_APPLOCAL_DEPS OFF)
ENDIF()

READ_GEMRB_VERSION()
CONFIGURE_DIRECTORY_LAYOUT()

# convert the slashes for windows' users' convenience
file(TO_NATIVE_PATH "${PLUGIN_DIR}" DEFAULT_PLUGINS_DIR)
file(TO_NATIVE_PATH ./Cache2/ DEFAULT_CACHE_DIR)
if(APPIMAGE)
	file(RELATIVE_PATH DATA_DIR "/" "${DATA_DIR}")
endif()

CHECK_IS_RELEASE_VERSION()

IF(NOT STATIC_LINK)
	# prevent static libraries from being selected by FIND_LIBRARY
	LIST(REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF()

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_BINARY_DIR}
	gemrb/includes
	gemrb/core
	platforms/${CMAKE_SYSTEM_NAME}
)

INCLUDE(config)
CONFIGURE_LINKING()
CONFIGURE_COMPILER()
CONFIGURE_PYTHON()
CONFIGURE_SDL(${SDL_BACKEND})
CONFIGURE_OPENGL(${OPENGL_BACKEND} ${SDL_BACKEND})
INCLUDE(FindDependencies)
CONFIGURE_FOR_SANITIZE(${SANITIZE})
CONFIGURE_TARGET_PLATFORM_SPECIFICS()
CONFIGURE_FMT_INCLUDES()

# The engine does not work without threads, enable them globally.
FIND_PACKAGE(Threads REQUIRED)

# Tests (need init'd before subdirectories)
IF (USE_TESTS)
	FIND_PACKAGE(GTest REQUIRED)
	INCLUDE(CTest)
	ENABLE_TESTING()
	LIST(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
ENDIF()

IF(USE_TRACY)
	INCLUDE(FetchContent)

	IF (NOT STATIC_LINK)
		SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
	ENDIF()

	FETCHCONTENT_DECLARE(
		tracy
		GIT_REPOSITORY https://github.com/wolfpld/tracy.git
		GIT_TAG v0.10
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)
	FETCHCONTENT_MAKEAVAILABLE(tracy)

	ADD_DEFINITIONS("-DUSE_TRACY")
	INCLUDE_DIRECTORIES(${tracy_SOURCE_DIR}/public)
ENDIF()

ADD_SUBDIRECTORY(gemrb)
INSTALL_APP_RESOURCES()
MAKE_UNINSTALL_TARGET()
MAKE_DIST_TARGET()
MAKE_APPIMAGE_TARGET()

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
PRINT_OPTION(STATIC_LINK)
PRINT_OPTION(INSOURCEBUILD)
PRINT_OPTION(DISABLE_WERROR)
PRINT_OPTION(SDL_BACKEND)
PRINT_OPTION(USE_SDL_CONTROLLER_API)
PRINT_OPTION(PYTHON_VERSION)
PRINT_OPTION(OPENGL_BACKEND)
PRINT_OPTION(SANITIZE)
PRINT_OPTION(USE_TESTS)
PRINT_OPTION(USE_TRACY)
message(STATUS "")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Target bitness: ${CMAKE_SIZEOF_VOID_P}*8")
message(STATUS "")
IF((NOT DISABLE_WERROR) AND (NOT IS_RELEASE_VERSION))
	message(STATUS "Compiler warnings are fatal.")
	message(STATUS "If you hit one, let us know and rerun cmake with -DDISABLE_WERROR=1 to continue.")
	message(STATUS "")
ENDIF()
