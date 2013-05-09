AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.spaceorb"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}
libspaceorb_la_LIBADD = $(SELECTLIBS)

noinst_LTLIBRARIES = libspaceorb.la

libspaceorb_la_SOURCES = input.c

EXTRA_DIST = EXPSYMS

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

