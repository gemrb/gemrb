SUBDIRS = gemrb
EXTRA_DIST = admin/setup.sh 
MAINTAINERCLEANFILES = subdirs configure.files aclocal.m4 configure config.log config.status

dist_man1_MANS = gemrb.1

install-data-hook:
	rm -f $(bindir)plugins/libcore.so
	mkdir -p $(bindir)/Cache 2>/dev/null
	rm -rf $(bindir)/Cache/*
