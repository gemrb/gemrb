lib_LTLIBRARIES = libACMImporter.la
INCLUDES = $(all_includes)
libACMImporter_la_LDFLAGS = -module -version-info 0:0:0 -lopenal -lSDL @LIBPTHREAD@
libACMImporter_la_LIBADD = ../Core/libcore.la
libACMImporter_la_METASOURCES = AUTO
libACMImporter_la_SOURCES = unpacker.cpp riffhdr.cpp readers.cpp decoder.cpp ACMImporterCD.cpp ACMImporter.cpp ACMImp.cpp 
noinst_HEADERS = ACMImp.h ACMImporterCD.h decoder.h general.h readers.h riffhdr.h unpacker.h 
