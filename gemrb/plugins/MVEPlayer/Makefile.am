plugin_LTLIBRARIES = libMVEPlayer.la
INCLUDES = $(all_includes)
libMVEPlayer_la_LDFLAGS = -module -version-info 0:0:0 -lSDL @LIBPTHREAD@
libMVEPlayer_la_LIBADD = ../Core/libgemrb_core.la
libMVEPlayer_la_METASOURCES = AUTO
libMVEPlayer_la_SOURCES = mve_audio.cpp MVEPlayerDesc.cpp MVEPlayer.cpp MVEPlay.cpp 
noinst_HEADERS = MVEPlayerDesc.h MVEPlay.h mve_audio.h 
