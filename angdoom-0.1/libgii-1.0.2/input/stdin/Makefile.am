AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.stdin"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}
libstdin_la_LIBADD = $(SELECTLIBS)

noinst_LTLIBRARIES = libstdin.la

libstdin_la_SOURCES = input.c

EXTRA_DIST = EXPSYMS

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

