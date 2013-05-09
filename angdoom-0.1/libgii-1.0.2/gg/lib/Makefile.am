
AM_CPPFLAGS = -DBUILDING_LIBGG
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include $(extra_includes)
libreplace_la_LIBADD = $(LTLIBOBJS)

noinst_LTLIBRARIES = libreplace.la

libreplace_la_SOURCES =

libreplace_la_LDFLAGS = -static $(extra_libraries)

EXTRA_DIST = getopt.c memcmp.c snprintf.c strtoul.c

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in
