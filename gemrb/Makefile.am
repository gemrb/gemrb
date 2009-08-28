bin_PROGRAMS = gemrb
sysconf_DATA = GemRB.cfg.sample GemRB.cfg.subdir.sample
gemrb_SOURCES = GemRB.cpp 

gemrb_LDADD =  ./plugins/Core/libgemrb_core.la

SUBDIRS = plugins includes GUIScripts override docs
EXTRA_DIST = GemRB.cfg* plugins-prepare.sh CMakeLists.txt
INCLUDES= \
	$(all_includes) \
	$(PYTHON_INCLUDES)

gemrb_LDFLAGS = $(all_libraries) 

#install-data-local:
#	$(INSTALL_DATA) GemRB.cfg* $(sysconfdir)
