INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = libcocoa.la

libcocoa_la_SOURCES = input.m cocoa.m
libcocoa_la_LDFLAGS = -framework ApplicationServices \
		-framework Cocoa


EXTRA_DIST = EXPSYMS keysyms.h cocoa.h

AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.cocoa"' -no-cpp-precomp -I/System/Library/Frameworks

MAINTAINERCLEANFILES = Makefile.in

