AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.mouse"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}
libmouse_la_LIBADD = $(SELECTLIBS)

noinst_LTLIBRARIES = libmouse.la

libmouse_la_SOURCES = input.c packetparse.c

EXTRA_DIST = EXPSYMS man.txt mouse.h

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

