builtins_libs =

include $(srcdir)/builtins.am

AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libggi"' -DBUILDING_LIBGGI
INCLUDES  = -I$(top_srcdir)/include $(extra_includes)

libggi_la_LIBADD = $(builtins_libs) -lgii -lgg
lib_LTLIBRARIES = libggi.la

libggi_la_SOURCES = builtins.c colormap.c db.c dl.c events.c ext.c \
	gc.c init.c internal.c mode.c probe.c stubs.c swar.c unix.c \
	visual.c
libggi_la_LDFLAGS = $(extra_libraries) -version-info 2:2:0 -no-undefined \
	-export-symbols $(srcdir)/EXPSYMS

EXTRA_DIST = EXPSYMS ext.h swar.h

MAINTAINERCLEANFILES = Makefile.in
