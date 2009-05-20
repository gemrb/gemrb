ADD_GEMRB_PLUGIN ( MVEPlayer mvevideodec8.cpp mvevideodec16.cpp mveaudiodec.cpp mve_player.cpp MVEPlay.cpp MVEPlayer.cpp MVEPlayerDesc.cpp )

INSTALL( TARGETS MVEPlayer DESTINATION ${PLUGIN_INSTALL_DIR} )
