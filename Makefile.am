SUBDIRS = gemrb
EXTRA_DIST = admin/setup.sh 
MAINTAINERCLEANFILES = subdirs configure.files aclocal.m4 configure config.h.in config.log config.status stamp-h.in admin/config.sub admin/install-sh admin/missing admin/py-compile admin/config.guess admin/depcomp admin/ltmain.sh admin/mkinstalldirs

dist_man1_MANS = gemrb.1

install-data-hook:
	rm -f $(bindir)/plugins/libgemrb_core.so
	mkdir -p $(bindir)/Cache 2>/dev/null
	rm -rf $(bindir)/Cache/*
