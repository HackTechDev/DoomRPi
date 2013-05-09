/* $Id: plat.h,v 1.31 2005/07/31 15:31:10 soyt Exp $
******************************************************************************

   LibGG internal definitions

   Copyright (C) 1998	Marcus Sundberg		[marcus@ggi-project.org]

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#ifndef _GG_INTERNAL_PLAT_H
#define _GG_INTERNAL_PLAT_H

#include "config.h"
#include <ggi/system.h>

/* gg_dlhand is only needed for dl_win32 and dl_darwin */
typedef void * gg_dlhand;
/* XXX this is just here not to break the darwin build. */
#define GG_MODULE_GLOBAL 1

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

/* Mutex for protecting LibGG global data */
extern void	*_gg_global_mutex;

#if defined(__GNUC__) && !defined(inline)
# define inline __inline__
#endif

#if (defined(__SVR4) || defined(__svr4) || defined(SVR4) \
	|| defined(svr4) || defined(__SYSTYPE_SVR4) || defined(__hpux__)) \
	&& !defined(__svr4__)
# define __svr4__	1
#endif

#if (defined(__APPLE__) && defined(__MACH__)) && !defined(__darwin__)
# define __darwin__	1
#endif


#if defined(__WIN32__) && !defined(__CYGWIN__)
# define CHAR_DIRDELIM '\\'
# define STRING_DIRDELIM "\\"
#else
# define CHAR_DIRDELIM '/'
# define STRING_DIRDELIM "/"
#endif


#if !defined(PATH_MAX)
# ifdef _POSIX_PATH_MAX
#  define PATH_MAX _POSIX_PATH_MAX
# else
#  define PATH_MAX 4096 /* Should be enough for most systems */
# endif
#endif


#if !defined(_POSIX_SOURCE)

/* *BSD system */
# if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) \
	|| defined(__DragonFly__)
#   define _POSIX_C_SOURCE	199506L
# endif

/* SYSTEM V Release IV systems (almost all commercial unices) */
# if defined(__svr4__)
#   define _POSIX_SOURCE
# endif

/* other posix systems */
# if defined(__CYGWIN__)
#   define _POSIX_SOURCE
# endif

#endif	/* _POSIX_SOURCE && _POSIX_C_SOURCE */

#if defined(_POSIX_C_SOURCE)
#   define _XOPEN_SOURCE	600
#endif


#ifndef HAVE_ARCH

#if defined(__WIN32__) && !defined(__CYGWIN__)
#define HAVE_ARCH
# include <windows.h>
# define GGOPENLIB(filename)	ggWin32DLOpen((filename))
# define GGOPENLIBGLOBAL(filename) GGOPENLIB(filename)
# define GGGETSYM(handle,sym)	GetProcAddress((handle),(sym))
# define GGCLOSELIB(handle)	FreeLibrary((handle))
# define GGDLERROR()		ggWin32DLError()

/* forward declarations
 */

#include <ggi/gg.h>

__BEGIN_DECLS

gg_dlhand ggWin32DLOpen(const char *filename);
const char *ggWin32DLError(void);

__END_DECLS

#endif	/* __WIN32__ */

#endif


#ifndef HAVE_ARCH

#if defined(macintosh) || defined(__darwin__)
# define HAVE_ARCH
# ifdef HAVE_MACH_O_DYLD_H
#  include <mach-o/dyld.h>
# endif
# define GGOPENLIB(filename)	ggDarwinDLOpen((filename), 0)
# define GGOPENLIBGLOBAL(filename) ggDarwinDLOpen((filename), GG_MODULE_GLOBAL)
# define GGGETSYM(handle,sym)	ggDarwinDLSym((handle),(sym))
# define GGCLOSELIB(handle)	ggDarwinDLClose((handle))
# define GGDLERROR()		ggDarwinDLError()

# define GG_SYMPREFIX		"_"

/* forward declarations
 */

#include <ggi/gg.h>

__BEGIN_DECLS

gg_dlhand ggDarwinDLOpen(const char *filename, int flags);
void *ggDarwinDLSym(gg_dlhand handle, const char *symbol);
void ggDarwinDLClose(gg_dlhand handle);
const char *ggDarwinDLError(void);

__END_DECLS

#endif	/* MAC OS/DARWIN */

#endif


#ifndef HAVE_ARCH

#if defined(HAVE_DLOPEN)
#define HAVE_ARCH
/* should catch all unix system with a native dlopen() implementation
 */
# ifdef HAVE_DLFCN_H
#  include <dlfcn.h>
# endif
# ifdef RTLD_LAZY
#  define GGOPENLIB(filename)		dlopen((filename), RTLD_LAZY)
#  ifdef RTLD_GLOBAL
#   define GGOPENLIBGLOBAL(filename)	dlopen((filename), RTLD_LAZY | RTLD_GLOBAL)
#  else
#   define GGOPENLIBGLOBAL(filename)	dlopen((filename), RTLD_LAZY)
#  endif /* RTLD_GLOBAL */
# elif defined(DL_LAZY)
#  define GGOPENLIB(filename)		dlopen((filename), DL_LAZY)
#  define GGOPENLIBGLOBAL(filename)	dlopen((filename), DL_LAZY)
# else
#  error This system has an unknown dlopen() call !!!
# endif /* RTLD_LAZY */
# define GGGETSYM(handle,sym)	dlsym((handle),(sym))
# define GGCLOSELIB(handle)	dlclose((handle))
# define GGDLERROR()		dlerror()
#endif

#ifndef HAVE_ARCH
# error Unsupported plattform!
#endif

#endif


/* Remark: NetBSD 1.4.x is marked as dead. NetBSD >= 1.5.x
 * does no longer need the GG_SYMPREFIX.
 */

#if defined(SYMBOL_UNDERSCORE) || defined(__OS2__) || \
	(defined(__OpenBSD__) && !defined(__ELF__))
# define GG_SYMPREFIX		"_"
#endif


#endif /* _GG_INTERNAL_PLAT_H */
