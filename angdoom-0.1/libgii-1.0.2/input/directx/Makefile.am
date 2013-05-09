AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.directx"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

libdxinput_la_LIBADD = -ldinput -lgdi32 -luser32 -lole32

noinst_LTLIBRARIES = libdxinput.la

libdxinput_la_SOURCES = input.c di.c dxguid.c

EXTRA_DIST = EXPSYMS dxinput.h dxinputkb.h man.txt

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

