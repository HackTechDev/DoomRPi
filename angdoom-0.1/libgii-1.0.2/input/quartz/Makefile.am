INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = libquartz.la

libquartz_la_SOURCES = event.c input.c quartz.c
libquartz_la_LDFLAGS = -framework ApplicationServices \
		-framework Carbon

EXTRA_DIST = EXPSYMS keysyms.h man.txt quartz.h
AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.quartz"' -no-cpp-precomp -I/System/Library/Frameworks

MAINTAINERCLEANFILES = Makefile.in

