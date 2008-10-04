ADD_GEMRB_PLUGIN ( MVEPlayer decoder16.cpp decoder8.cpp mve_audio.cpp mve_play.cpp mvelib.cpp MVEPlay.cpp MVEPlayer.cpp MVEPlayerDesc.cpp )

INSTALL( TARGETS MVEPlayer DESTINATION ${PLUGIN_INSTALL_DIR} )
