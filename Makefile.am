SUBDIRS = gemrb
EXTRA_DIST = setup.sh 
MAINTAINERCLEANFILES = subdirs configure.in configure.files

install-data-hook:
	rm -rf $(bindir)plugins/libcore.so
	mkdir -p $(bindir)/Cache 2>/dev/null
