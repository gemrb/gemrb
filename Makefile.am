SUBDIRS = gemrb
EXTRA_DIST = setup.sh 
MAINTAINERCLEANFILES = subdirs configure.files aclocal.m4 configure config.log config.status

install-data-hook:
	rm -rf $(bindir)plugins/libcore.so
	mkdir -p $(bindir)/Cache 2>/dev/null
