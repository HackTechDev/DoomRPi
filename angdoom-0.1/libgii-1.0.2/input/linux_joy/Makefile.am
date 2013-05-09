AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.linux_joy"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = liblinux_joy.la

liblinux_joy_la_SOURCES = input.c

EXTRA_DIST = EXPSYMS

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

