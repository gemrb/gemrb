SUBDIRS = gemrb
EXTRA_DIST = setup.sh 
MAINTAINERCLEANFILES = subdirs configure.in configure.files

install-data-hook:
	rm $(libdir)/libcore.la
	rm $(libdir)/libcore.so*
	if test -x $(bindir)/Cache; then 
		rm -rf $(bindir)/Cache
	fi
	mkdir $(bindir)/Cache
