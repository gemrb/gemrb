plugin_LTLIBRARIES = libBIKPlayer.la
INCLUDES = $(all_includes)
libBIKPlayer_la_LDFLAGS = -module -version-info 0:0:0 @LIBPTHREAD@
libBIKPlayer_la_METASOURCES = AUTO
libBIKPlayer_la_SOURCES = \
	BIKPlay.cpp \
	BIKPlayer.cpp \
	BIKPlayerDesc.cpp \
	GetBitContext.cpp \
	dct.cpp \
	fft.cpp \
	rdft.cpp \
	rational.cpp \
	mem.cpp

noinst_HEADERS = BIKPlay.h BIKPlayerDesc.h GetBitContext.h binkdata.h common.h rational.h dsputil.h
