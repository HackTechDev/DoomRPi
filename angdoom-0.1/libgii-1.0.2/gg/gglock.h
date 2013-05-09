/* $Id: gglock.h,v 1.2 2005/07/31 15:31:10 soyt Exp $
******************************************************************************

   LibGG - test & set implementation and constants for gglock.c

   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]

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

#ifndef _GGLOCK_H
#define _GGLOCK_H

#include "plat.h"

#ifdef __GNUC__

#if	defined(__i386__) || defined(__i386) || defined(i386) || \
	defined(__i486__) || defined(__i486) || defined(i486) || \
	defined(__i586__) || defined(__i586) || defined(i586) || \
	defined(__i686__) || defined(__i686) || defined(i686)

static inline int
testandset(int *lock)
{
	int ret;

	__asm__ __volatile__(
		"xchgl %0, %1"
		: "=r"(ret), "=m"(*lock)
		: "0"(1), "m"(*lock)
		: "memory"
		);

	return ret;
}
#define HAVE_TESTANDSET

#elif	defined(__alpha__) || defined (__alpha) || defined(alpha)

static inline int
testandset(int *lock)
{
	long ret;
	long tmp;

	__asm__ __volatile__(
		"1:\t"
		"ldl_l %0,%3\n\t"
		"bne   %0,2f\n\t"
		"or    $31,1,%1\n\t"
		"stl_c %1,%2\n\t"
		"beq   %1,1b\n"
		"2:\tmb"
		: "=&r"(ret), "=&r"(tmp), "=m"(*lock)
		: "m"(*lock)
		: "memory"
		);

	return (int)ret;
}
#define HAVE_TESTANDSET

#elif	defined(__arm__) || defined (__arm) || defined(arm)

static inline int
testandset(int *lock)
{
	register unsigned int ret;

	__asm__ __volatile__(
		"swp %0, %1, [%2]"
		: "=r"(ret)
		: "0"(1), "r"(lock)
		);

	return ret;
}
#define HAVE_TESTANDSET

#elif	defined(__m68k__) || defined (__m68k) || defined(m68k)

static inline int
testandset(int *lock)
{
	char ret;

	__asm__ __volatile__(
		"tas %1\n\t"
		"sne %0"
		: "=dm"(ret), "=m"(*lock)
		: "m"(*lock)
		: "cc"
		);

	return ret;
}
#define HAVE_TESTANDSET

#elif	defined(__mips__) || defined (__mips) || defined(mips)

#warning Does the 'sc' instruction mean syscall as on PowerPC?
#warning Then you are out of luck if this is not Linux...

static inline int
testandset(int *lock)
{
	long ret;
	long tmp;

	__asm__ __volatile__(
		".set    mips2\n"
		"1:\t"
		"ll     %0,%3\n\t"
		"bnez   %0,2f\n\t"
		".set   noreorder\n\t"
		"li     %1,1\n\t"
		".set   reorder\n\t"
		"sc     %1,%2\n\t"
		"beqz   %1,1b\n"
		"2:\t"
		".set   mips0"
		: "=&r"(ret), "=&r"(tmp), "=m"(*lock)
		: "m"(*lock)
		: "memory"
		);

	return (int) ret;
}
#define HAVE_TESTANDSET

#elif	defined(__powerpc__) || defined (__powerpc) || defined(powerpc)

static inline int
testandset(int *lock)
{
        int ret, tmp;

        __asm__ __volatile__(
		"1:\t"
		"lwarx   %0,0,%3\n\t"
		"or      %1,%0,%2\n\t"
		"stwcx.  %1,0,%3\n\t"
		"bne     1b"
		: "=&r"(ret), "=&r"(tmp)
		: "r"(1), "r"(lock)
		: "cc"
		);

        return ret;
}
#define HAVE_TESTANDSET

#elif	defined(__sparc__) || defined (__sparc) || defined(sparc)

static inline int
testandset(int *lock)
{
	int ret;

	__asm__ __volatile__(
		"ldstub %1,%0"
		: "=r"(ret), "=m"(*lock)
		: "m"(*lock)
		);

  return ret;
}
#define HAVE_TESTANDSET
#endif /* Supported platforms */

#endif /* __GNUC__ */


/* C implementation - UNSAFE */
#ifndef HAVE_TESTANDSET
static inline int
testandset(int *lock)
{
	int ret;

	ret = (*lock != 0);
	*lock = 1;

	return ret;
}
#define HAVE_TESTANDSET
#endif


/* Default values
 */
#ifndef GGLOCK_SLEEP_US
# define GGLOCK_SLEEP_US	2001
#endif

#ifndef GGLOCK_COUNT
# ifndef HAVE_SCHED_YIELD
/* Do more iterations before sleeping */
#  define GGLOCK_COUNT		1000
# else
#  define GGLOCK_COUNT		50
# endif
#endif

#endif /* _GGLOCK_H */
