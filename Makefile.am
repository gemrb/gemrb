SUBDIRS = gemrb
EXTRA_DIST = setup.sh 
MAINTAINERCLEANFILES = subdirs configure.in configure.files

install-data-hook:
	rm $(libdir)/libcore.la
	rm $(libdir)/libcore.so*
	mkdir $(bindir)/Cache
