EXTRA_PROGRAMS =
man_MANS =
noinst_PROGRAMS = config init iter lock parsetarget time

INCLUDES = -I${top_srcdir}/include -I${top_builddir}/include \
        -I${top_builddir} ${extra_includes} ${X_CFLAGS}

EXTRA_DIST = $(man_MANS)

MAINTAINERCLEANFILES = Makefile.in

LIBGG = ${top_builddir}/gg/libgg.la

LDADD = ${LIBGG} ${GGDLLIBS}
