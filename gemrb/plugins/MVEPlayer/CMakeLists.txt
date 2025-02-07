ADD_GEMRB_PLUGIN (MVEPlayer
  mvevideodec8.cpp
  mvevideodec16.cpp
  mveaudiodec.cpp
  MVEPlayer.cpp
)

IF(WIN32)
  TARGET_LINK_LIBRARIES(MVEPlayer kernel32)
ENDIF()
