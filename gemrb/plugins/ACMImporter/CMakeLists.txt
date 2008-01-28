INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR} ${SDL_INCLUDE_DIR} )

ADD_LIBRARY(ACMImporter MODULE ACMImp.cpp ACMImporter.cpp AmbientMgrAL.cpp decoder.cpp readers.cpp riffhdr.cpp StackLock.cpp unpacker.cpp ACMImporterCD.cpp)

TARGET_LINK_LIBRARIES(ACMImporter gemrb_core ${OPENAL_LIBRARY} ${SDL_LIBRARY})

INSTALL( TARGETS ACMImporter DESTINATION ${PLUGIN_INSTALL_DIR} )