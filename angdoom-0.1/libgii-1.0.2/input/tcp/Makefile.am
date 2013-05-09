AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.tcp"'
INCLUDES  = -I$(top_builddir)/include \
	-I$(top_srcdir)/common -I$(top_srcdir)/include \
	$(extra_includes)

libtcpconv_la_LIBADD = $(NETLIBS)
EXTRA_LTLIBRARIES = libtcpconv.la

libtcpconv_la_SOURCES = libtcp.c



libtcp_la_LIBADD = $(SELECTLIBS)

noinst_LTLIBRARIES = @TCPCONV@ libtcp.la

libtcp_la_SOURCES = input.c

EXTRA_DIST = EXPSYMS libtcp.h man.txt

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in
