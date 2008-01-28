INCLUDE_DIRECTORIES( ${OPENAL_INCLUDE_DIR} )

ADD_LIBRARY( MVEPlayer MODULE decoder16.cpp decoder8.cpp mve_audio.cpp mve_play.cpp mvelib.cpp MVEPlay.cpp MVEPlayer.cpp MVEPlayerDesc.cpp )

TARGET_LINK_LIBRARIES( MVEPlayer gemrb_core ${OPENAL_LIBRARY} )

INSTALL( TARGETS MVEPlayer DESTINATION ${PLUGIN_INSTALL_DIR} )