inputdir=$(top_builddir)/input
inputlibdir = ${libdir}/${ggi_subdir}/input

EXTRA_LTLIBRARIES = null.la file.la ipaq_touchscreen.la zaurus.la \
	tcp.la stdin.la x.la mouse.la linux_mouse.la linux_joy.la linux_kbd.la \
	linux_evdev.la spaceorb.la vgl.la dxinput.la pcjoy.la lk201.la \
	kii.la cocoa.la quartz.la
inputlib_LTLIBRARIES = @INPUTMODULES@

libgii = ${top_builddir}/gii/libgii.la ${top_builddir}/gg/libgg.la

null_la_LIBADD = ${top_builddir}/input/null/libnull.la ${libgii}
null_la_SOURCES =
null_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/null/EXPSYMS

file_la_LIBADD = ${top_builddir}/input/file/libfile.la ${libgii}
file_la_SOURCES =
file_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/file/EXPSYMS

ipaq_touchscreen_la_LIBADD = ${top_builddir}/input/ipaq_touchscreen/libipaq_touchscreen.la ${libgii}
ipaq_touchscreen_la_SOURCES =
ipaq_touchscreen_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/ipaq_touchscreen/EXPSYMS

zaurus_la_LIBADD = ${top_builddir}/input/touchscreen/libzaurus.la ${libgii}
zaurus_la_SOURCES =
zaurus_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/touchscreen/EXPSYMS

tcp_la_LIBADD = ${top_builddir}/input/tcp/libtcp.la $(top_builddir)/input/tcp/libtcpconv.la ${libgii}
tcp_la_SOURCES =
tcp_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/tcp/EXPSYMS

stdin_la_LIBADD = ${top_builddir}/input/stdin/libstdin.la ${libgii}
stdin_la_SOURCES =
stdin_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/stdin/EXPSYMS

x_la_LIBADD = ${top_builddir}/input/x/libx.la ${libgii}
x_la_SOURCES =
x_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_builddir}/input/x/EXPSYMS

mouse_la_LIBADD = ${top_builddir}/input/mouse/libmouse.la ${libgii}
mouse_la_SOURCES =
mouse_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/mouse/EXPSYMS

linux_mouse_la_LIBADD = ${top_builddir}/input/linux_mouse/liblinux_mouse.la ${libgii}
linux_mouse_la_SOURCES =
linux_mouse_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/linux_mouse/EXPSYMS

linux_joy_la_LIBADD = ${top_builddir}/input/linux_joy/liblinux_joy.la ${libgii}
linux_joy_la_SOURCES =
linux_joy_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/linux_joy/EXPSYMS

linux_kbd_la_LIBADD = ${top_builddir}/input/linux_kbd/liblinux_kbd.la ${libgii}
linux_kbd_la_SOURCES =
linux_kbd_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/linux_kbd/EXPSYMS

linux_evdev_la_LIBADD = ${top_builddir}/input/linux_evdev/liblinux_evdev.la ${libgii}
linux_evdev_la_SOURCES =
linux_evdev_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/linux_evdev/EXPSYMS

spaceorb_la_LIBADD = ${top_builddir}/input/spaceorb/libspaceorb.la ${libgii}
spaceorb_la_SOURCES =
spaceorb_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/spaceorb/EXPSYMS

vgl_la_LIBADD = ${top_builddir}/input/vgl/libvgl.la ${libgii}
vgl_la_SOURCES =
vgl_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/vgl/EXPSYMS

dxinput_la_LIBADD = ${top_builddir}/input/directx/libdxinput.la ${libgii}
dxinput_la_SOURCES =
dxinput_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/directx/EXPSYMS

pcjoy_la_LIBADD = ${top_builddir}/input/pcjoy/libpcjoy.la ${libgii}
pcjoy_la_SOURCES =
pcjoy_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/pcjoy/EXPSYMS

lk201_la_LIBADD = ${top_builddir}/input/lk201/liblk201.la ${libgii}
lk201_la_SOURCES =
lk201_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/lk201/EXPSYMS

kii_la_LIBADD = ${top_builddir}/input/kii/libkii.la ${libgii}
kii_la_SOURCES =
kii_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/kii/EXPSYMS

cocoa_la_LIBADD = ${top_builddir}/input/cocoa/libcocoa.la ${libgii}
cocoa_la_SOURCES =
cocoa_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/cocoa/EXPSYMS

quartz_la_LIBADD = ${top_builddir}/input/quartz/libquartz.la ${libgii}
quartz_la_SOURCES =
quartz_la_LDFLAGS = -rpath $(inputlibdir) ${extra_libraries} \
	-shared -module -no-undefined -avoid-version \
	-export-symbols ${top_srcdir}/input/quartz/EXPSYMS


DISTCLEANFILES = Makefile .deps
MAINTAINERCLEANFILES = Makefile.in
