plugin_LTLIBRARIES = libACMImporter.la
INCLUDES = $(all_includes)
libACMImporter_la_LDFLAGS = -module -version-info 0:0:0 @SDL_LIBS@ @LIBPTHREAD@
libACMImporter_la_METASOURCES = AUTO
libACMImporter_la_SOURCES = unpacker.cpp readers.cpp decoder.cpp ACMImp.cpp
noinst_HEADERS = ACMImp.h decoder.h general.h readers.h unpacker.h
