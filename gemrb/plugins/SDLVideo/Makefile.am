lib_LTLIBRARIES = libSDLVideo.la
INCLUDES = $(all_includes)
#LDFLAGS =     
libSDLVideo_la_LDFLAGS = -module -version-info 0:0:0 -lSDL @LIBS@
libSDLVideo_la_LIBADD = ../Core/libcore.la
libSDLVideo_la_METASOURCES = AUTO
libSDLVideo_la_SOURCES = SDLVideoDriver.cpp SDLVideoDC.cpp SDLVideo.cpp 
EXTRA_DIST = SDLVideo.cpp SDLVideoDC.cpp SDLVideoDC.h SDLVideoDriver.cpp SDLVideoDriver.h 
