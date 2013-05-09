AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.filter.tcp"'
INCLUDES = -I$(top_builddir)/include \
	-I$(top_srcdir)/input/tcp -I$(top_srcdir)/include \
	$(extra_includes)
libfiltertcp_la_LIBADD = $(SELECTLIBS)

noinst_LTLIBRARIES = libfiltertcp.la

libfiltertcp_la_SOURCES = filter.c

EXTRA_DIST = EXPSYMS man.txt

CLEANFILES = libtcp.c
MAINTAINERCLEANFILES = Makefile.in

