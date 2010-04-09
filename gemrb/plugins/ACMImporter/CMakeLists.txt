FILE( GLOB ACMImporter_files *.cpp )

ADD_GEMRB_PLUGIN (ACMImporter ${ACMImporter_files})

TARGET_LINK_LIBRARIES(ACMImporter ${VORBIS_LIBRARY})
