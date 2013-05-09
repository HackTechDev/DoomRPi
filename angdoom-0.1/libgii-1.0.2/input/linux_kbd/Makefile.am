AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.linux_kbd"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes}

noinst_LTLIBRARIES = liblinux_kbd.la

liblinux_kbd_la_SOURCES = input.c linkey.c

EXTRA_DIST = EXPSYMS linkey.h man.txt

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

