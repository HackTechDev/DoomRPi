EXTRA_PROGRAMS = mhub xsendbut
bin_PROGRAMS = @DEMOS@
noinst_PROGRAMS = config cpuinfo demo inputinfo \
		filter scope tasksched

INCLUDES = -I${top_srcdir}/include -I${top_builddir}/include \
	-I${top_builddir} ${extra_includes} ${X_CFLAGS}

LIBGII = ${top_builddir}/gii/libgii.la
LIBGG = ${top_builddir}/gg/libgg.la

LDADD = ${LIBGII}

cpuinfo_LDADD = ${LIBGG}
tasksched_LDADD = ${LIBGG}
scope_LDADD = ${LIBGG}
config_LDADD = ${LIBGG}

xsendbut_LDADD = ${X_LIBS} -lX11 ${X_EXTRA_LIBS}

EXTRA_DIST = mhub.txt xsendbut.txt

MAINTAINERCLEANFILES = Makefile.in
