SUBDIRS = $(GGSUBDIRS)

AM_CPPFLAGS = -DBUILDING_LIBGG -DDEBUG_NAMESPACE='"libgg"'
INCLUDES  = -I$(top_builddir)/include -I$(top_srcdir)/include $(extra_includes)
libgg_la_LIBADD = $(GGDLLIBS) $(REPLACELIB)


lib_LTLIBRARIES = libgg.la

mutex_src = task.c
if PTLOCK
mutex_src += ptlock.c ptsched.c
else
if WIN32LOCK
mutex_src += win32lock.c win32sched.c
else
mutex_src += gglock.c sigsched.c
endif # WIN32LOCK
endif # PTLOCK

memdebug_src =
if MEMDEBUG
memdebug_src += memdebug.c
endif

dl_src =
if OS_DARWIN
dl_src += dl_darwin.c
endif # OS_DARWIN
if OS_WIN32_MINGW
dl_src += dl_win32.c
endif # OS_WIN32_MINGW


cleanup_src = cleanup.c
if OS_WIN32_MINGW
cleanup_src += cleanup_win32.c
else
cleanup_src += cleanup_stubs.c
endif # OS_WIN32_MINGW



cpuid_src = cpuid.c
if ARCH_ALPHA
cpuid_src += cpuid_alpha.c
endif # ARCH_ALPHA
if ARCH_IA32
cpuid_src += cpuid_ia32.c
endif # ARCH_IA32
if ARCH_PPC
cpuid_src += cpuid_ppc.c
endif # ARCH_PPC
if ARCH_SPARC
cpuid_src += cpuid_sparc.c
endif # ARCH_SPARC


EXTRA_DIST =

include $(srcdir)/quad/quad.am

if NEED_QUAD
extra_src = $(quad_src)
else   
extra_src =   
endif # NEED_QUAD

libgg_la_SOURCES = $(cleanup_src) $(cpuid_src) conf.c debug.c dl.c \
		init.c misc.c parse.c string.c time.c \
		$(extra_src) $(dl_src) $(memdebug_src) $(mutex_src)

libgg_la_LDFLAGS = $(extra_libraries) -version-info 1:0:0 -no-undefined \
	-export-symbols $(top_builddir)/gg/EXPSYMS

EXTRA_libgg_la_SOURCES = gglock.c ptlock.c memdebug.c dl_darwin.c \
			cpuid_alpha.c cpuid_ia32.c cpuid_ppc.c cpuid_sparc.c \
			dl_win32.c win32lock.c task.c ptsched.c win32sched.c \
			sigsched.c cleanup_stubs.c cleanup_win32.c

EXTRA_DIST += EXPSYMS.in EXPSYMS.quad cleanup.h cpuid.h gglock.h \
	misc_win32.c plat.h

DISTCLEANFILES = Makefile .deps EXPSYMS
MAINTAINERCLEANFILES = Makefile.in

DIST_SUBDIRS = lib ggtick
