plugin_LTLIBRARIES = libOpenALAudio.la
INCLUDES = $(all_includes)
libOpenALAudio_la_LDFLAGS = -module -version-info 0:0:0 @SDL_LIBS@ @LIBPTHREAD@ @OPENAL_LIBS@
libOpenALAudio_la_METASOURCES = AUTO
libOpenALAudio_la_SOURCES = OpenALAudio.cpp AmbientMgrAL.cpp StackLock.cpp
noinst_HEADERS = OpenALAudio.h AmbientMgrAL.h StackLock.h
