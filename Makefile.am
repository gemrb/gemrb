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

dist_man6_MANS = gemrb.6

install-data-hook:
	rm -f $(DESTDIR)$(bindir)/plugins/libgemrb_core.so

docs:
	doxygen
