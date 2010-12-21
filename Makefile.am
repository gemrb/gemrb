SUBDIRS = gemrb

SPEC_FILES = \
        gemrb.spec \
        gemrb.spec.in

EXTRA_DIST = CMakeLists.txt \
	cmake_config.h.in \
	cmake_uninstall.cmake.in \
	$(SPEC_FILES)

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

dist_man6_MANS = gemrb.6.in

install-data-hook:
	rm -f $(DESTDIR)$(bindir)/plugins/libgemrb_core.so

docs:
	doxygen

dist-hook:
	sed -i 's,-Werror,,' $(distdir)/configure.in $(distdir)/configure
