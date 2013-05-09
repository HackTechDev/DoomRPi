AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.touchscreen"'
INCLUDES  = -I. -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = libzaurus.la

libzaurus_la_SOURCES = zaurus.c common.c

EXTRA_DIST = EXPSYMS common.h

MAINTAINERCLEANFILES = Makefile.in
