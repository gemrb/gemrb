
# CMake module to search for LIBVLC (VLC library)
# Author: Rohit Yadav <rohityadav89@gmail.com>
#
# If it's found it sets LIBVLC_FOUND to TRUE
# and following variables are set:
#    LIBVLC_INCLUDE_DIR
#    LIBVLC_LIBRARY


# FIND_PATH and FIND_LIBRARY normally search standard locations
# before the specified paths. To search non-standard paths first,
# FIND_* is invoked first with specified paths and NO_DEFAULT_PATH
# and then again with no specified paths to search the default
# locations. When an earlier FIND_* succeeds, subsequent FIND_*s
# searching for the same item do nothing.

#Put here path to custom location
#example: /home/user/vlc/include etc..
FIND_PATH(LIBVLC_INCLUDE_DIR vlc/vlc.h
  "$ENV{LIBVLC_INCLUDE_PATH}"
  "$ENV{LIB_DIR}/include"
  "$ENV{LIB_DIR}/include/vlc"
  "/usr/include"
  "/usr/include/vlc"
  "/usr/local/include"
  "/usr/local/include/vlc"
  #mingw
  c:/msys/local/include
  NO_DEFAULT_PATH
  )
FIND_PATH(LIBVLC_INCLUDE_DIR vlc.h)

#Put here path to custom location
#example: /home/user/vlc/lib etc..
FIND_LIBRARY(LIBVLC_LIBRARY NAMES vlc PATHS
  "$ENV{LIBVLC_LIBRARY_PATH}"
  "$ENV{LIB_DIR}/lib"
  #mingw
  c:/msys/local/lib
  NO_DEFAULT_PATH
  )
FIND_LIBRARY(LIBVLC_LIBRARY NAMES vlc)
FIND_LIBRARY(LIBVLCCORE_LIBRARY NAMES vlccore PATHS
  "$ENV{LIBVLC_LIBRARY_PATH}"
  "$ENV{LIB_DIR}/lib"
  #mingw
  c:/msys/local/lib
  NO_DEFAULT_PATH
  )
FIND_LIBRARY(LIBVLCCORE_LIBRARY NAMES vlccore)

IF (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARY AND LIBVLCCORE_LIBRARY)
   SET(LIBVLC_FOUND TRUE)
ENDIF (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARY AND LIBVLCCORE_LIBRARY)

IF (LIBVLC_FOUND)
   IF (NOT LIBVLC_FIND_QUIETLY)
      MESSAGE(STATUS "Found LibVLC include-dir path: ${LIBVLC_INCLUDE_DIR}")
      MESSAGE(STATUS "Found LibVLC library path:${LIBVLC_LIBRARY}")
      MESSAGE(STATUS "Found LibVLCcore library path:${LIBVLCCORE_LIBRARY}")
   ENDIF (NOT LIBVLC_FIND_QUIETLY)
ELSE (LIBVLC_FOUND)
   IF (LIBVLC_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find LibVLC")
   ENDIF (LIBVLC_FIND_REQUIRED)
ENDIF (LIBVLC_FOUND)
