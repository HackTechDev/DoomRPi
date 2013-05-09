/* $Id: gglock.c,v 1.3 2004/03/08 06:36:28 cegger Exp $
******************************************************************************

   LibGG - Mutex implementation using test & set

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

#include "plat.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SCHED_H
# include <sched.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <ggi/internal/gg.h>

#include "gglock.h"


void *ggLockCreate(void)
{
	int *ret;

	if ((ret = malloc(sizeof(int))) != NULL) {
		*ret = 0;
	}
	return (void *) ret;
}


int ggLockDestroy(void *lock)
{
	/* If lock is locked, or NULL, it's better to crash hard than allow
	   a broken app continue to run. */
	free(lock);
	return 0;
}


void ggLock(void *lock)
{
	int *lck = lock;
	int i = 0;

	while (testandset(lck)) {
		if (i < GGLOCK_COUNT) {
			i++;
#ifdef HAVE_SCHED_YIELD
			sched_yield();
#endif
		} else {
#ifdef HAVE_NANOSLEEP
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = (GGLOCK_SLEEP_US*1000);
			nanosleep(&ts, NULL);
#else
			ggUSleep(GGLOCK_SLEEP_US);
#endif
			i = 0;
		}
	}

	return;
}

void ggUnlock(void *lock)
{
	*((int*)lock) = 0;

	return;
}


int ggTryLock(void *lock)
{
	int *lck = lock;

	if (testandset(lck)) return GGI_EBUSY;

	return 0;
}


int _ggInitLocks(void) {
	return 0;
}

void _ggExitLocks(void) {
	return;
}
