plugin_LTLIBRARIES = libMVEPlayer.la
INCLUDES = $(all_includes)
libMVEPlayer_la_LDFLAGS = -module -version-info 0:0:0 @LIBPTHREAD@
libMVEPlayer_la_METASOURCES = AUTO
libMVEPlayer_la_SOURCES = \
	MVEPlay.cpp \
	MVEPlayer.cpp \
	MVEPlayerDesc.cpp \
	mvevideodec8.cpp \
	mvevideodec16.cpp \
	mveaudiodec.cpp \
	mve_player.cpp

noinst_HEADERS = MVEPlayerDesc.h MVEPlay.h gstmvedemux.h mve.h mve_player.h
