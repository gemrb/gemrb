plugin_LTLIBRARIES = libMVEPlayer.la
INCLUDES = $(all_includes)
libMVEPlayer_la_LDFLAGS = -module -version-info 0:0:0 -lSDL -lopenal @LIBPTHREAD@
libMVEPlayer_la_LIBADD = ../Core/libgemrb_core.la
libMVEPlayer_la_METASOURCES = AUTO
libMVEPlayer_la_SOURCES = \
	MVEPlay.cpp \
	MVEPlayer.cpp \
	MVEPlayerDesc.cpp \
	decoder16.cpp \
	decoder8.cpp \
	mve_audio.cpp \
	mve_play.cpp \
	mvelib.cpp

noinst_HEADERS = MVEPlayerDesc.h MVEPlay.h decoders.h libmve.h mve_audio.h mvelib.h
