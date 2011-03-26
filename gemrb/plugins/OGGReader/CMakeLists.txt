IF (VORBIS_LIBRARY)
	FILE( GLOB OGGReader_files *.cpp )

	# include the second parent of vorbisfile.h
	get_filename_component(OGG_INCLUDE ${VORBIS_FILE} PATH)
	include_directories(${OGG_INCLUDE})

	ADD_GEMRB_PLUGIN (OGGReader ${OGGReader_files})

	TARGET_LINK_LIBRARIES(OGGReader ${VORBIS_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
ENDIF (VORBIS_LIBRARY)
