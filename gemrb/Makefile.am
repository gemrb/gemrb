bin_PROGRAMS = gemrb
sysconf_DATA = GemRB.cfg
gemrb_SOURCES = GemRB.cpp 

gemrb_LDADD =  ./plugins/Core/libgemrb_core.la $(LIBDL)

SUBDIRS = plugins includes GUIScripts override Config
EXTRA_DIST = GemRB.cfg* plugins-prepare.sh
INCLUDES= \
	$(all_includes) \
	$(PYTHON_INCLUDES)

gemrb_LDFLAGS = $(all_libraries) 

#install-data-local:
#	$(INSTALL_DATA) GemRB.cfg* $(sysconfdir)
