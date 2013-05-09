AM_CPPFLAGS = -DDEBUG_NAMESPACE='"libgii.input.x"'
INCLUDES  = -I${top_srcdir}/include -I${top_builddir}/include ${extra_includes} \
	 ${X_CFLAGS}
libx_la_LIBADD = ${X_LIBS} -lX11 ${XF86DGA_LIBS} ${X_EXTRA_LIBS}

noinst_LTLIBRARIES = libx.la

xf86dga_src =
if INPUT_XF86DGA
xf86dga_src += xf86dga.c
endif

libx_la_SOURCES = input.c xwin.c xev.c ${xf86dga_src}
EXTRA_libx_la_SOURCES = xf86dga.c

EXTRA_DIST = EXPSYMS.in man.txt xev.h

DISTCLEANFILES = Makefile .deps EXPSYMS
MAINTAINERCLEANFILES = Makefile.in

