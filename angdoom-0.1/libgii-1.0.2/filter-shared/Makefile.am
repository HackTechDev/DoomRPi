filterdir=$(top_builddir)/filter
filterlibdir = ${libdir}/${ggi_subdir}/filter

EXTRA_LTLIBRARIES = mouse.la keytrans.la save.la tcp.la
filterlib_LTLIBRARIES = @FILTERMODULES@

libgii = ${top_builddir}/gii/libgii.la ${top_builddir}/gg/libgg.la

mouse_la_LIBADD = ${top_builddir}/filter/mouse/libfiltermouse.la ${libgii}
mouse_la_SOURCES =
mouse_la_LDFLAGS = -rpath $(filterlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/filter/mouse/EXPSYMS

keytrans_la_LIBADD = ${top_builddir}/filter/keytrans/libkeytrans.la ${libgii}
keytrans_la_SOURCES =
keytrans_la_LDFLAGS = -rpath $(filterlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/filter/keytrans/EXPSYMS

save_la_LIBADD = ${top_builddir}/filter/save/libsave.la ${libgii}
save_la_SOURCES =
save_la_LDFLAGS = -rpath $(filterlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/filter/save/EXPSYMS

tcp_la_LIBADD = ${top_builddir}/filter/tcp/libfiltertcp.la $(top_builddir)/input/tcp/libtcpconv.la ${libgii}
tcp_la_SOURCES =
tcp_la_LDFLAGS = -rpath $(filterlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/filter/tcp/EXPSYMS

DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in
