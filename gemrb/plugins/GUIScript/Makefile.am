plugin_LTLIBRARIES = libGUIScript.la
INCLUDES = $(all_includes) \
	 $(PYTHON_INCLUDES)
libGUIScript_la_LDFLAGS = -module -version-info 0:0:0 -lpython2.3 @LIBPTHREAD@
libGUIScript_la_LIBADD = ../Core/libgemrb_core.la
libGUIScript_la_METASOURCES = AUTO
libGUIScript_la_SOURCES = GUIScriptEngine.cpp GUIScriptCD.cpp GUIScript.cpp 
noinst_HEADERS = GUIScript.h GUIScriptCD.h
