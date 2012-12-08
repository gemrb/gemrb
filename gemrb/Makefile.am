bin_PROGRAMS = gemrb
sysconf_DATA = GemRB.cfg.sample.in
gemrb_SOURCES = GemRB.cpp 

gemrb_LDADD =  ./core/libgemrb_core.la

SUBDIRS = core plugins includes GUIScripts override docs unhardcoded
EXTRA_DIST = GemRB.cfg* plugins-prepare.sh CMakeLists.txt

gemrb_LDFLAGS = $(all_libraries) @LIBPTHREAD@

#install-data-local:
#	$(INSTALL_DATA) GemRB.cfg* $(sysconfdir)
