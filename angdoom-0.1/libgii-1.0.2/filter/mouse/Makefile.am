AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.filter.mouse"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = libfiltermouse.la

libfiltermouse_la_SOURCES = filter.c

giiconfdatadir = ${sysconfdir}/${ggi_subdir}/filter
giiconfdata_DATA = mouse

EXTRA_DIST = EXPSYMS man.txt mouse

MAINTAINERCLEANFILES = Makefile.in

