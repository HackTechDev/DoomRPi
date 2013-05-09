builtins_libs     =
builtins_flags    =

include $(srcdir)/builtins.am

AM_CPPFLAGS = -DBUILDING_LIBGII -DDEBUG_NAMESPACE='"libgii"'
INCLUDES  = -I$(top_builddir)/include -I$(top_srcdir)/include \
		-I$(top_srcdir)/gg $(extra_includes)

libgii_la_LIBADD = $(builtins_libs) $(top_builddir)/gg/libgg.la $(SELECTLIBS)
lib_LTLIBRARIES = libgii.la

libgii_la_SOURCES = builtins.c dl.c gii.c init.c unix.c
libgii_la_LDFLAGS = $(extra_libraries) $(builtins_flags) \
	-version-info 1:0:0 -no-undefined \
	-export-symbols $(srcdir)/EXPSYMS

EXTRA_DIST = EXPSYMS

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in
