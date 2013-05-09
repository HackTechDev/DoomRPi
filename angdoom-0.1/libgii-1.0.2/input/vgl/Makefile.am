AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.vgl"'
INCLUDES  = -I$(top_builddir)/include -I$(top_srcdir)/include $(extra_includes)
libvgl_la_LIBADD = -lvgl

noinst_LTLIBRARIES = libvgl.la

libvgl_la_SOURCES = key2event.c input.c

EXTRA_DIST = EXPSYMS keycodes.h input-vgl.h

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in

