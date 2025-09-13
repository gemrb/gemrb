# Xbox platform configuration for NXDK toolchain
# Based on vita.cmake implementation

ADD_DEFINITIONS("-DXBOX")

# Xbox-specific compiler flags for NXDK
SET(CMAKE_DL_LIBS "")
SET(XBOX_FLAGS "-march=pentium3 -msse -mfpmath=sse -O2 -fomit-frame-pointer -DNDEBUG")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${XBOX_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${XBOX_FLAGS}")

# Xbox-specific library linking for NXDK
# These libraries are typically provided by NXDK
# Note: Actual library names may vary based on NXDK version
SET(XBOX_LIBRARIES_SDL_COMMON "-lSDL -lxboxkrnl -lhal -lusb")
SET(XBOX_LIBRARIES_SDL "${XBOX_LIBRARIES_SDL_COMMON}")

# Xbox filesystem paths - using standard Xbox drive mappings
SET(DEFAULT_CACHE_DIR "E:\\GemRB\\Cache2\\")
SET(DATA_DIR "E:\\GemRB\\")
SET(SYSCONF_DIR "E:\\GemRB")

include_directories(AFTER SYSTEM "${CMAKE_CURRENT_LIST_DIR}")

# Xbox uses SDL 1.2 with NXDK
IF (SDL_BACKEND STREQUAL "SDL")
	SET(CMAKE_CXX_STANDARD_LIBRARIES "${XBOX_LIBRARIES_SDL}")
ENDIF ()

# Force static linking for Xbox
SET(STATIC_LINK ON CACHE BOOL "Static linking required for Xbox" FORCE)

# Disable features that may not work well on Xbox's limited hardware
SET(USE_OPENAL OFF CACHE BOOL "OpenAL not available on Xbox" FORCE)
SET(USE_LIBVLC OFF CACHE BOOL "LibVLC not available on Xbox" FORCE)
SET(OPENGL_BACKEND "None" CACHE STRING "OpenGL not available on Xbox" FORCE)

# Memory optimizations for Xbox's 64MB RAM limit
ADD_DEFINITIONS("-DXBOX_MEMORY_OPTIMIZED")

# When using NXDK toolchain, SDL detection should be handled by the toolchain
# This message helps users understand they need the proper NXDK environment
IF(NOT DEFINED ENV{NXDK_DIR} AND XBOX)
	MESSAGE(WARNING "Building for Xbox without NXDK_DIR environment variable set.")
	MESSAGE(WARNING "Make sure you're using the NXDK toolchain file:")
	MESSAGE(WARNING "  -DCMAKE_TOOLCHAIN_FILE=$NXDK_DIR/share/toolchain-nxdk.cmake")
ENDIF()