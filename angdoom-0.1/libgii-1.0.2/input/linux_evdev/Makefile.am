AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.linux_evdev"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = liblinux_evdev.la

liblinux_evdev_la_SOURCES = eventparse.c key.c input.c

EXTRA_DIST = EXPSYMS linux_evdev.h man.txt

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

