AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.file"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}
libfile_la_LIBADD = $(SELECTLIBS)

noinst_LTLIBRARIES = libfile.la

libfile_la_SOURCES = input.c

EXTRA_DIST = EXPSYMS man.txt

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

