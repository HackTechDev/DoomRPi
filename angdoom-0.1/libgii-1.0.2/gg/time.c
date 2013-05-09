/* $Id: time.c,v 1.9 2005/07/29 16:40:52 soyt Exp $
******************************************************************************

   platform abstraction of time related functions.

   Copyright (C) 2004  Brian S. Julin  [skids@users.sourceforge.net]

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

#include "config.h"
#include <ggi/gg.h>
#include <ggi/system.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef GG_CURTIME_USE_GETSYSTEMTIMEASFILETIME
#include <windows.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/* Subtract timeval tv1 from timeval tv2, leaving the result in tv2. */
#define TVDIF(tv1, tv2) \
tv2.tv_sec -= tv1.tv_sec;				\
if (tv2.tv_usec < tv1.tv_usec) {			\
	tv2.tv_sec--;					\
	tv2.tv_usec += 1000000 - tv1.tv_usec;		\
} else tv2.tv_usec -= tv1.tv_usec;


/* Only one of the GG_USLEEP_USE_* macros will be defined by autoconf */

#ifdef GG_USLEEP_USE_USLEEP

/* LibC usleep implementation -- we must deal with different flavors */

#define GG_USLEEP_OK

#ifdef GG_USLEEP_999999

/* We have a pedantic usleep, so we have to split up calls into 1 sec chunks */

int ggUSleep (int32_t usecs) {
	struct timeval tv1, tv2;
	int usecs2;

	ggCurTime(&tv1);
	usecs2 = usecs;
	/* We could check each call for interrupt, but why bother,
	 * since ggUSleep is interruptible.  Plus we don't have to
	 * care about GG_USLEEP_VOID this way.
	 */
	while (usecs2 >= 1000000) {
		usleep(999999);
		usecs2 -= 999999;
	}
	usleep(usecs2);
	ggCurTime(&tv2);

	TVDIF(tv1, tv2);

	if (tv2.tv_sec < usecs / 1000000) return -1;
	if (tv2.tv_usec < usecs % 1000000) return -1;
	return 0;
}

#else  /* not GG_USLEEP 999999 -- still have to adjust return type/value. */

#ifdef GG_USLEEP_VOID

int ggUSleep (int32_t usecs) {
	struct timeval tv1, tv2;

	ggCurTime(&tv1);
	usleep(usecs);
	ggCurTime(&tv2);

	TVDIF(tv1, tv2);

	if (tv2.tv_sec < usecs / 1000000) return -1;
	if (tv2.tv_usec < usecs % 1000000) return -1;
	return 0;
}

#else /* not GG_USLEEP_VOID -- simple C typecasting should suffice. */

int ggUSleep (int32_t usecs) {
	return (int)usleep(usecs);
}

#endif /* not GG_USLEEP_VOID */
#endif /* not GG_USLEEP_999999 */
#endif /* GG_USLEEP_USE_USLEEP */


#ifdef GG_USLEEP_USE_W32SLEEP

/* Win32 implementation, when no usleep() is available i.e. mingw */

#define GG_USLEEP_OK

/* windows.h is included above */

int ggUSleep (int32_t usecs) {
	Sleep((usecs + 999) / 1000);
	return 0;
}

#endif

#ifdef GG_USLEEP_USE_SELECT

/* Unix select used as sleep -- when select doesn't choke on an empty fdset. */

#define GG_USLEEP_OK

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

int ggUSleep(int32_t usecs) {
	struct timeval tv;
	tv.tv_sec = usecs / 1000000; 
	tv.tv_usec = usecs % 1000000;
	select(0, NULL, NULL, NULL, &tv);
	if (tv.tv_usec || tv.tv_sec) return -1;
	return 0;
}

#endif /* GG_USLEEP_USE_SELECT */

#ifndef GG_USLEEP_OK
#error You need to implement ggUSleep on this platform!
#endif



/* Only one of the GG_CURTIME_USE_* macros will be defined by autoconf */

#ifdef GG_CURTIME_USE_GETTIMEOFDAY

/* Headers already included by gg.h since struct timeval needed there */

#define GG_CURTIME_OK
int ggCurTime(struct timeval *tv) { 
	return(gettimeofday((tv), NULL));
}

#endif

#ifdef GG_CURTIME_USE_GETSYSTEMTIMEASFILETIME

/* windows.h included above on w32.
 * Other headers already included by gg.h since struct timeval needed there. 
 */

#define GG_CURTIME_OK
int ggCurTime(struct timeval *tv) { 
	FILETIME ftim;

	GetSystemTimeAsFileTime(&ftim);

	(tv)->tv_sec =  (((LARGE_INTEGER *)(void *)(&ftim))->QuadPart
				- GG_UINT64_C(116444736000000000)) / 10000000;
	(tv)->tv_usec = (((LARGE_INTEGER *)(void *)(&ftim))->QuadPart
				% 10000000) / 10;

	return 0;
}

#endif

#ifndef GG_CURTIME_OK
#error You need to implement ggCurTime() for this system
#endif


/* Uninterruptible sleep is now easily implemented based on the above work. */
void ggUSlumber(int32_t usecs) {
	struct timeval tv1, tv2;

	ggCurTime(&tv1);

	while (ggUSleep(usecs)) {
		ggCurTime(&tv2);
		TVDIF(tv1, tv2);
		if (tv2.tv_sec > usecs / 1000000) return;
		if (tv2.tv_sec) usecs -= 1000000 * tv2.tv_sec;
		if (tv2.tv_usec > usecs) return;
		usecs -= tv2.tv_usec;
		ggCurTime(&tv1);
	}
}
