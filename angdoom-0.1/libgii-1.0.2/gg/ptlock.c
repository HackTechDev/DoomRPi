/* $Id: ptlock.c,v 1.7 2005/02/05 08:33:33 cegger Exp $
******************************************************************************

   LibGG - simple locking API implementation using pthreads

   Copyright (C) 1998 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 2004 Brian S. Julin	[skids@users.sourceforge.net]
   
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
#include <stdlib.h>
#include <errno.h>
#include <ggi/internal/gg.h>

#ifdef HAVE_PTHREAD_H
#define __C_ASM_H /* Fix for retarded Digital Unix header */
#include <pthread.h>
#endif

struct _gg_pthread_lock {
	pthread_cond_t cond;
	pthread_mutex_t mtx;
	int users;
	int canceled;
};

/* General note: while the LibGG API specifies that cancelling a thread
 * while it is inside one of the locking functions has undefined 
 * consequences, the below code makes a slight effort to behave well in 
 * such instances.  It's actually a bit silly, but I was bored and still
 * hadn't thought through that part of the API definition.  Anyway, it 
 * just might pull us out of a badly written cleanupfunc nosedive so I 
 * figured I'd just leave it this way -- BSJ.
 */

void  *ggLockCreate(void)
{
	struct _gg_pthread_lock *ret;
	int ct;
	int dummy;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &ct);
	if ((ret = calloc(1, sizeof(struct _gg_pthread_lock))) != NULL) {
		if (pthread_mutex_init(&ret->mtx, NULL) != 0) goto bail0;
		if (pthread_cond_init(&ret->cond, NULL) != 0) goto bail1;
	}
	pthread_setcanceltype(ct, &dummy);
	return (void *) ret;

bail1:
	pthread_mutex_destroy(&ret->mtx);
bail0:
	free((void*)ret);
	pthread_setcanceltype(ct, &dummy);
	return (void *) NULL;
}

int ggLockDestroy(void *lock) {
	pthread_mutex_t *l = &((struct _gg_pthread_lock *)lock)->mtx;
	pthread_cond_t  *c = &((struct _gg_pthread_lock *)lock)->cond;
	int ct;
	int dummy;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &ct);
	if (pthread_mutex_destroy(l) != 0) goto busy;
	if (pthread_cond_destroy(c) != 0) goto busy;
	free(lock);
	pthread_setcanceltype(ct, &dummy);
	return 0;
busy:
	pthread_setcanceltype(ct, &dummy);
	return GGI_EBUSY;
}

/* Cleanup handler to decrement lock->users if cancelled when waiting. */
static void dec(void *thing) { if (*((int *)thing) > 1) (*((int *)thing))--; }

/* This silences some compiler warnings by providing the correct typecasts
 * for using pthread_cleanup_push with _pthread_mutex_unlock.
 */
static void _gg_unlock_pt_void(void *arg) { 
  pthread_mutex_unlock((pthread_mutex_t *)arg);
}

void ggLock(void *lock) {
	pthread_mutex_t *l = &((struct _gg_pthread_lock *)lock)->mtx;
	pthread_cond_t  *c = &((struct _gg_pthread_lock *)lock)->cond;
	int *users = &((struct _gg_pthread_lock *)lock)->users;
	int ct;
	int dummy;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &ct);
	pthread_cleanup_push(_gg_unlock_pt_void, (void *)(l));
	if (pthread_mutex_lock(l)) _gg_death_spiral();
	(*users)++;
	if (*users > 1) {
		pthread_cleanup_push(dec, (void *)users);
		if (pthread_cond_wait(c, l)) _gg_death_spiral();
		pthread_cleanup_pop(0);
	}
	pthread_cleanup_pop(1);
	pthread_setcanceltype(ct, &dummy);
}

void ggUnlock(void *lock) {
	pthread_mutex_t *l = &((struct _gg_pthread_lock *)lock)->mtx;
	pthread_cond_t  *c = &((struct _gg_pthread_lock *)lock)->cond;
	int *users = &((struct _gg_pthread_lock *)lock)->users;
	int ct;
	int dummy;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &ct);
	pthread_cleanup_push(_gg_unlock_pt_void, (void *)l);
	if (pthread_mutex_lock(l)) _gg_death_spiral();
	if (*users && --(*users) && pthread_cond_signal(c)) _gg_death_spiral();
	pthread_cleanup_pop(1);
	pthread_setcanceltype(ct, &dummy);
}


int ggTryLock(void *lock) {
	pthread_mutex_t *l = &((struct _gg_pthread_lock *)lock)->mtx;
	int *users = &((struct _gg_pthread_lock *)lock)->users;
	int ret = 0;
	int ct;
	int dummy;

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &ct);
	pthread_cleanup_push(_gg_unlock_pt_void, (void *)l);
	if (pthread_mutex_lock(l)) _gg_death_spiral();
	if (!*users) (*users)++;
	else ret = GGI_EBUSY;
	pthread_cleanup_pop(1);
	pthread_setcanceltype(ct, &dummy);

	return ret;
}

int _ggInitLocks(void) {
	return 0;
}

void _ggExitLocks(void) {
	return;
}
