lib_LTLIBRARIES = libGUIScript.la
INCLUDES = $(all_includes) \
	 $(PYTHON_INCLUDES)
#LDFLAGS =     
libGUIScript_la_LDFLAGS = -module -version-info 0:0:0 -lpython2.3 @LIBPTHREAD@
libGUIScript_la_LIBADD = ../Core/libcore.la
libGUIScript_la_METASOURCES = AUTO
libGUIScript_la_SOURCES = GUIScriptEngine.cpp GUIScriptCD.cpp GUIScript.cpp 
EXTRA_DIST = GUIScript.cpp GUIScript.h GUIScriptCD.cpp GUIScriptCD.h GUIScriptEngine.cpp 
