lib_LTLIBRARIES = libSDLVideo.la
INCLUDES = $(all_includes)
libSDLVideo_la_LDFLAGS = -module -version-info 0:0:0 -lSDL @LIBPTHREAD@
libSDLVideo_la_LIBADD = ../Core/libcore.la
libSDLVideo_la_METASOURCES = AUTO
libSDLVideo_la_SOURCES = SDLVideoDriver.cpp SDLVideoDC.cpp SDLVideo.cpp 
noinst_HEADERS = SDLVideoDC.h SDLVideoDriver.h 
