AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.kii"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = libkii.la

libkii_la_SOURCES = input.c kii.c

EXTRA_DIST = EXPSYMS kii.h

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

