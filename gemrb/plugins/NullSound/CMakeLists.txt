ADD_LIBRARY(NullSound MODULE NullSnd.cpp NullSound.cpp NullSoundCD.cpp )
TARGET_LINK_LIBRARIES(NullSound gemrb_core)

INSTALL( TARGETS NullSound DESTINATION ${PLUGIN_INSTALL_DIR} )