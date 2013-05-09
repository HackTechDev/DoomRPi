EXTRA_PROGRAMS =
man_MANS =
noinst_PROGRAMS = evtimestamp init

INCLUDES = -I${top_srcdir}/include -I${top_builddir}/include \
        -I${top_builddir} ${extra_includes} ${X_CFLAGS}

EXTRA_DIST = $(man_MANS)

MAINTAINERCLEANFILES = Makefile.in

LIBGII = ${top_builddir}/gii/libgii.la

LDADD = ${LIBGII} ${GGDLLIBS}
