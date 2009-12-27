ADD_GEMRB_PLUGIN ( BIKPlayer BIKPlay.cpp BIKPlayer.cpp BIKPlayerDesc.cpp dct.cpp fft.cpp GetBitContext.cpp mem.cpp rational.cpp rdft.cpp )

INSTALL( TARGETS BIKPlayer DESTINATION ${PLUGIN_INSTALL_DIR} )
