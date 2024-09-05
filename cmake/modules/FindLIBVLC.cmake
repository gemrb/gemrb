
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
  "/Applications/VLC.app/Contents/MacOS/include"
  "/Applications/VLC.app/Contents/MacOS/include/vlc"
  #mingw
  c:/msys/local/include
  "C:/Program Files (x86)/VideoLAN/VLC/sdk/include"
  "C:/Program Files/VideoLAN/VLC/sdk/include"
  NO_DEFAULT_PATH
  )
FIND_PATH(LIBVLC_INCLUDE_DIR vlc.h)

#Put here path to custom location
#example: /home/user/vlc/lib etc..
FIND_LIBRARY(LIBVLC_LIBRARY NAMES vlc libvlc PATHS
  "$ENV{LIBVLC_LIBRARY_PATH}"
  "$ENV{LIB_DIR}/lib"
  "/Applications/VLC.app/Contents/MacOS/lib"
  "/Applications/VLC.app/Contents/MacOS/plugins"
  #mingw
  c:/msys/local/lib
  "C:/Program Files (x86)/VideoLAN/VLC/sdk/lib"
  "C:/Program Files/VideoLAN/VLC/sdk/lib"
  NO_DEFAULT_PATH
  )
FIND_LIBRARY(LIBVLC_LIBRARY NAMES vlc libvlc)

IF (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARY)
  # we require this function from 2.0
  execute_process(
    COMMAND grep -sq libvlc_video_set_format_callbacks "${LIBVLC_INCLUDE_DIR}/vlc/libvlc_media_player.h" "${LIBVLC_INCLUDE_DIR}/libvlc_media_player.h"
    RESULT_VARIABLE LIBVLC_GOOD
    OUTPUT_VARIABLE TM
  )
  #message("1111111: ${LIBVLC_LIBRARY} + ${LIBVLC_INCLUDE_DIR} : ${LIBVLC_GOOD} ~ ${TM}")
  IF (LIBVLC_GOOD EQUAL 0)
     SET(LIBVLC_FOUND TRUE)
  ELSE (LIBVLC_GOOD EQUAL 0)
     message ("The VLCPlayer plugin requires at least VLC 2.0.0 to build, skipping!")
  ENDIF (LIBVLC_GOOD EQUAL 0)
ENDIF (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARY)

IF (LIBVLC_FOUND)
   IF (NOT LIBVLC_FIND_QUIETLY)
      MESSAGE(STATUS "Found LibVLC include-dir path: ${LIBVLC_INCLUDE_DIR}")
      MESSAGE(STATUS "Found LibVLC library path:${LIBVLC_LIBRARY}")
   ENDIF (NOT LIBVLC_FIND_QUIETLY)
ELSE (LIBVLC_FOUND)
   IF (LIBVLC_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find LibVLC")
   ENDIF (LIBVLC_FIND_REQUIRED)
ENDIF (LIBVLC_FOUND)
