SUBDIRS = gemrb
EXTRA_DIST = admin/setup.sh 
MAINTAINERCLEANFILES = \
	aclocal.m4 \
	config.h.in \
	config.log \
	config.status \
	configure \
	configure.files \
	stamp-h.in \
	subdirs \
	admin/config.guess \
	admin/config.sub \
	admin/depcomp \
	admin/install-sh \
	admin/ltmain.sh \
	admin/missing \
	admin/mkinstalldirs \
	admin/py-compile

dist_man1_MANS = gemrb.1

install-data-hook:
	rm -f $(bindir)/plugins/libgemrb_core.so
	mkdir -p $(bindir)/Cache 2>/dev/null
	rm -rf $(bindir)/Cache/*
