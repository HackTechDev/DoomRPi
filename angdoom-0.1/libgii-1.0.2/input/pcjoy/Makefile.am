AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.pcjoy"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = libpcjoy.la

libpcjoy_la_SOURCES = input.c

EXTRA_DIST = EXPSYMS

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

