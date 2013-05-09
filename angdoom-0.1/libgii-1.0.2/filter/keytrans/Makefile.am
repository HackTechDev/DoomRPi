AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.filter.keytrans"'
INCLUDES = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = libkeytrans.la

libkeytrans_la_SOURCES = filter.c

giiconfdatadir = ${sysconfdir}/${ggi_subdir}/filter
giiconfdata_DATA = keytrans

EXTRA_DIST = EXPSYMS keytrans man.txt

MAINTAINERCLEANFILES = Makefile.in
