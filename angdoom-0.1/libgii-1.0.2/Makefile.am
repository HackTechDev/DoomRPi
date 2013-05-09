# multiple AUTOMAKE_OPTIONS lines only confuses automake 1.6
AUTOMAKE_OPTIONS = 1.4 foreign

SUBDIRS = include gg input filter gii input-shared filter-shared regress demos doc m4 dist

giiconfdatadir = ${sysconfdir}/${ggi_subdir}
giiconfdata_DATA = libgii.conf

DISTCLEANFILES = libgii.conf config.lt

MAINTAINERCLEANFILES = configure aclocal.m4 acinclude.m4 autom4te.cache \
	Makefile.in config.h.in

EXTRA_DIST = ChangeLog.1999 ChangeLog FAQ INSTALL.autoconf \
	autogen.sh checkversion.sh
