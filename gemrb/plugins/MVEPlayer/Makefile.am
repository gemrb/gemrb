lib_LTLIBRARIES = libMVEPlayer.la
INCLUDES = $(all_includes)
libMVEPlayer_la_LDFLAGS = -module -version-info 0:0:0 -lSDL @LIBPTHREAD@
libMVEPlayer_la_LIBADD = ../Core/libcore.la
libMVEPlayer_la_METASOURCES = AUTO
libMVEPlayer_la_SOURCES = mve_audio.cpp MVEPlayerDesc.cpp MVEPlayer.cpp MVEPlay.cpp 
EXTRA_DIST = MVEPlay.cpp MVEPlayer.cpp MVEPlayerDesc.cpp MVEPlayerDesc.h MVEPlay.h mve_audio.cpp mve_audio.h 
