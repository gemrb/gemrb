plugin_LTLIBRARIES = libGUIScript.la
INCLUDES = $(all_includes) \
	 $(PYTHON_INCLUDES)
libGUIScript_la_LDFLAGS = -module -version-info 0:0:0 @PYTHON_LIBS@ @LIBPTHREAD@
libGUIScript_la_METASOURCES = AUTO
libGUIScript_la_SOURCES = GUIScript.cpp
noinst_HEADERS = GUIScript.h
