bin_PROGRAMS = gemrb
data_DATA = GemRB.cfg
gemrb_SOURCES = GemRB.cpp 

gemrb_LDADD =  ./plugins/Core/libcore.la

SUBDIRS = plugins includes GUIScripts override Config
EXTRA_DIST = GemRB.cfg GemRB.cpp 
INCLUDES= \
	$(all_includes) \
	$(PYTHON_INCLUDES)

gemrb_LDFLAGS = $(all_libraries) 

install-data-local:
	$(INSTALL_DATA) GemRB.cfg* $(bindir)
