/* $Id: ptsched.c,v 1.8 2005/01/14 09:32:08 pekberg Exp $
******************************************************************************

   Task scheduler implementation: compiled-in libpthreads "driver"

   Copyright (C) 2004  Brian S. Julin   [skids@users.sourceforge.net]
   Copyright (C) 1998  Steve Cheng   [steve@ggi-project.org]

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
#include <ggi/internal/gg.h>
#include <ggi/internal/gg_replace.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pthread.h>

/* TODO: better, perhaps OS-specific values here?  Not urgent.
 * (Note that PTHREADS_THREADS_MAX documented in manpages does not 
 *  actually exist as a define in LinuxThreads !!)
 */
#define _GG_THREADS_ABSURD_NTHREADS 64
#define _GG_THREADS_ABSURD_RATE 10000

static struct {
	int rate;		/* Time in usecs between ticks		*/
	pthread_cond_t tick;	/* Condition for thread syncronization	*/
	pthread_mutex_t mtx;	/* Mutex for above member		*/
	pthread_mutex_t ssmtx;	/* Mutex for start/stop			*/
	int num;		/* Number of entries in next member	*/
	pthread_t *handles;	/* Handles for each thread		*/
	int running;		/* Flag: there are threads running	*/
	int crashing;		/* Flag: all threads should die ASAP	*/
	pthread_mutex_t cmtx;	/* Mutex for crash			*/
	/* I would expect better from POSIX, no portable invalid values */
	int tick_valid, mtx_valid, ssmtx_valid, cmtx_valid; /* :-( */
} _gg_task_thread;


/* Test to see if the scheduler is crashing and if so bring it down fast. */
static void _gg_task_thread_crashout(void) {
	int i, suicide = 0;
	pthread_t myid;

	if (!_gg_task_thread.crashing) return;
	/* Scheduler is crashing. 
	 *
	 * "Say Goodbye to All of... THIS... and hello... TO OBLIVION."
	 */
	if (pthread_mutex_trylock(&_gg_task_thread.cmtx)) {
		/* ...but someone else is doing the wetwork. 
		 * Sit around and wait for death.  That will either
		 * happen because we get canceled (we are a scheduler
		 * thread) or because exit is called.
		 */
		while (1) ggUSleep(1000000);
	}
	/* ...and we just got the hot potato! 
	 *
	 * "You die, she dies, everybody dies."
	 */

	myid = pthread_self();
	for (i = _gg_task_thread.num - 1; i >= 0; i--) {
		if (myid != _gg_task_thread.handles[i])
			pthread_cancel(_gg_task_thread.handles[i]);
	}
	for (i = _gg_task_thread.num - 1; i >= 0; i--) {
		if (myid != _gg_task_thread.handles[i])
			pthread_join(_gg_task_thread.handles[i], NULL);
		else suicide = 1;
	}
	if (suicide) pthread_exit(NULL);
}


/* This is the cleanup handler which may be called in async-signal 
 * context, and to add to the excitement, may in fact be running 
 * inside one of the scheduler threads.
 *
 * The good news is that this only gets called when the program
 * is about to crash so we can run roughshod over many of the
 * mutexes and don't have to clean up memory allocs.
 *
 * It is also never called more than once, so we don't have
 * to worry about reentry.
 */
static void _gg_task_thread_signal(void *unused) {
	/* The only completely portable POSIX way to deal with this
	 * is to raise a flag and let the threads kill themselves.
	 */
	_gg_task_thread.crashing = 1;

	/* TODO: many POSIX implementations do have async-signal-safe
	 * versions of functions that are not so specified by POSIX.
	 * Some may have enough to determine what thread we are in
	 * (pthread_self) and cancel the others (pthread_cancel, 
	 * pthread_join) and then self-terminate (pthread_exit)
	 * so it might be nice to call the above function here when
	 * the system supports it.
	 */
}


/* There is one thread that does sleeping.  This is the code it runs.	*/
static void * _gg_task_thread_sleeper(void *myid) {
	int done = 0;
	int defer = 0;

	/* When first started we don't want an instant tick. */
	ggUSlumber(_gg_task_thread.rate);
	
	while (1) {
		struct timeval then, now;
		int used;

		ggCurTime(&then);
		_gg_task_thread_crashout();
		pthread_mutex_lock(&_gg_task_thread.mtx);
		if (!_gg_task_thread.running) done = 1;
		else {
			defer = _gg_task_tick();
			if (!defer) 
				pthread_cond_broadcast(&_gg_task_thread.tick);
		}
		pthread_mutex_unlock(&_gg_task_thread.mtx);
		if (defer) {
			if (done) return myid;
			if (!_gg_task_thread.running) return myid;
			_gg_task_thread_crashout();
		}
		else {
			if (_gg_task_tick_finish()) return myid;
			if (done) return myid;
			if (!_gg_task_thread.running) return myid; 
			_gg_task_thread_crashout();
		}
		ggCurTime(&now);

		/* We skip any missed beats without counting them. */
		if (then.tv_usec > now.tv_usec) {
			used = 1000000 - then.tv_usec + now.tv_usec;
		} else used = now.tv_usec - then.tv_usec; 

		/* Since the .rate is >= 1 HZ this keeps our tempo
		 * irrespective of overflow into tv_secs.
		 */
                used %= _gg_task_thread.rate;
		ggUSlumber(_gg_task_thread.rate - used);
	}
}

/* This silences some compiler warnings by providing the correct typecasts
 * for using pthread_cleanup_push with _pthread_mutex_unlock.
 */
static void _gg_unlock_pt_void(void *arg) {
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}


/* The rest of the threads run this code to syncronize with the above one */
static void * _gg_task_thread_waiter(void *myid) {
	int done = 0;

	while (1) {
		/* pthread_cond_wait is a cancellation point and so
		 * we must ensure that .mtx gets unlocked if the thread is 
		 * cancelled.
		 */
		_gg_task_thread_crashout();
		pthread_cleanup_push(_gg_unlock_pt_void,
				     (void *)(&_gg_task_thread.mtx));
		pthread_mutex_lock(&_gg_task_thread.mtx);
		if (!_gg_task_thread.running) {
			/* push/pop must be cleanly nested.       */
			done = 1;  /* Do not rely on .running, volatile. */
		} else {
			/* push/pop must be cleanly nested. */
			pthread_cond_wait(&_gg_task_thread.tick, &_gg_task_thread.mtx);
		}
		pthread_cleanup_pop(1);
		if (done) return myid;
		if (!_gg_task_thread.running) return myid;
		_gg_task_thread_crashout();
		if (_gg_task_tock()) return myid;
	}
}

/* This code fragment is used by start/stop/exit to kill running threads. */
static int _gg_task_thread_reap(void) {
	int res = 0, i;

	/* Tasks may still be running, so we cannot just cancel the
	 * threads.  Cancelling them would do no good anyway if they
	 * were spinning in code without cancellation points, plus they
	 * might have all sorts of locking issues.  So we must wait for 
	 * them to finish, which they will do of their own accord once
	 * the scheduler core informs them there are no tasks (we assume
	 * the caller has ensured that it will.)
	 *
	 * First we kill the main thread.
	 */
	res |= pthread_join(_gg_task_thread.handles[0], NULL);

	/* Some threads may have died, some may still be waiting on
	 * the condition.  Some may even still be running tasks, but those
	 * will terminate themselves when they finish the task and notice
	 * the scheduler is trying to stop.
	 *
	 * Send the condition to wake up the ones that are waiting.
	 */
	res |= pthread_mutex_lock(&_gg_task_thread.mtx);
	res |= pthread_cond_broadcast(&_gg_task_thread.tick);
	res |= pthread_mutex_unlock(&_gg_task_thread.mtx);

	/* Now we wait for them all to die. */
	for (i = 1; i < _gg_task_thread.num; i++)
		res |= pthread_join(_gg_task_thread.handles[i], NULL);

	return res;
}

/* Stop the scheduler.  This should not be invoked by scheduler threads. 
 * The caller must ensure that the scheduler will tell threads to die
 * when they are done running their current task.
 */
static int _gg_task_thread_stop(void) {
	int res;

	/* Wait for any other start/stop calls to finish */
	pthread_mutex_lock(&_gg_task_thread.ssmtx);

	res = 0;
	if(!_gg_task_thread.running) goto done;
	_gg_task_thread.running = 0;
	res = _gg_task_thread_reap();
 done:
	pthread_mutex_unlock(&_gg_task_thread.ssmtx);
	if (res) return GGI_EUNKNOWN;
	return 0;
}

/* Start all threads when scheduler gets first task.  This should
 * not be called by scheduler threads.
 */
static int _gg_task_thread_start(void) {
	int i;

	/* Wait for any other start/stop calls to finish */
	pthread_mutex_lock(&_gg_task_thread.ssmtx);

	/* If the scheduler died (or is currently dying) of its own 
	 * accord, it still has (possibly still running) threads that 
	 * need to be destroyed.
	 */
	if (_gg_task_thread.running) {
		_gg_task_thread.running = 0;
		_gg_task_thread_reap();
	}

	/* Since all these threads do not do anything until the 
	 * sleeper thread starts, we start them first.  This makes
	 * cleanup in the event of a failure free from the locking 
	 * problems that would result if we allowed the scheduler
	 * to be running while we were creating them.
	 */
	for (i = 1; i < _gg_task_thread.num; i++) {
		if(pthread_create(_gg_task_thread.handles + i, NULL, 
				  _gg_task_thread_waiter,
				  _gg_task_thread.handles + i))
			goto bail;
	}

	/* This thread sleeps, those above wait for it to signal. */
	if(pthread_create(_gg_task_thread.handles, NULL, 
			  _gg_task_thread_sleeper, 
			  _gg_task_thread.handles))
		goto bail;
	_gg_task_thread.running = 1;
	pthread_mutex_unlock(&_gg_task_thread.ssmtx);
	return 0;
 bail:
	while(i-- > 1) {
		pthread_cancel(_gg_task_thread.handles[i]);
		pthread_join(_gg_task_thread.handles[i], NULL);
	};
	pthread_mutex_unlock(&_gg_task_thread.ssmtx);
	return GGI_EUNKNOWN;
}


/* Clean up on graceful exit. */
static int _gg_task_thread_exit(void) {
	/* We are being called gracefully through ggExit(), or
	 * by something else in this file, but not from a cleanup
	 * handler.  So, we have full access to pthreads API calls.
	 *
	 * First we make sure that other threads cannot try to start
	 * or stop the scheduler while we are destroying it.  This
	 * also ensures that a failing call to start doesn't leave
	 * us with a partially running scheduler by waiting for it
	 * to clean up after itself.
	 */
	if (_gg_task_thread.ssmtx_valid)
		pthread_mutex_lock(&_gg_task_thread.ssmtx);

	if (_gg_task_thread.running) {
		_gg_task_thread.running = 0;
		_gg_task_thread_reap();
	}

	if(_gg_task_thread.handles) free(_gg_task_thread.handles);
	_gg_task_thread.handles = NULL;
	if(_gg_task_thread.tick_valid) {
		_gg_task_thread.tick_valid = 0;
		pthread_cond_destroy(&_gg_task_thread.tick);
	}
	if(_gg_task_thread.mtx_valid) {
		_gg_task_thread.mtx_valid = 0;
		pthread_mutex_destroy(&_gg_task_thread.mtx);
	}
	/* TODO: make this unlock unnecessary */
	ggUnlock(_gg_global_mutex);
	ggUnregisterCleanup(_gg_task_thread_signal, NULL);
	ggLock(_gg_global_mutex);
	if(_gg_task_thread.cmtx_valid) {
		_gg_task_thread.cmtx_valid = 0;
		pthread_mutex_destroy(&_gg_task_thread.cmtx);
	}
	if(_gg_task_thread.ssmtx_valid) {
		pthread_mutex_unlock(&_gg_task_thread.ssmtx);
		_gg_task_thread.ssmtx_valid = 0;
		pthread_mutex_destroy(&_gg_task_thread.ssmtx);
	}
	return 0;
}


/* Initialize threads-based scheduler */
int _gg_task_driver_init(_gg_task_fn *start, _gg_task_fn *stop, 
			 _gg_task_fn *xit, int rate) {
	int res;

	_gg_task_thread.running = 0;
	_gg_task_thread.crashing = 0;
	_gg_task_thread.ssmtx_valid = 0;
	_gg_task_thread.mtx_valid = 0;
	_gg_task_thread.tick_valid = 0;
	_gg_task_thread.cmtx_valid = 0;

	/* Minimum scheduler rate is 1HZ.  Because I'm lazy. */
	if (rate < 1) return GGI_EARGINVAL;
	if (rate > _GG_THREADS_ABSURD_RATE) return GGI_EARGINVAL;
	_gg_task_thread.rate = 1000000/rate;

	_gg_task_thread.num = 1;
	if (_gg_optlist[_GG_OPT_SCHEDTHREADS].result[0] != 'n') {
		unsigned long nthreads;
		nthreads = 
		  strtoul(_gg_optlist[_GG_OPT_SCHEDTHREADS].result, NULL, 10);
		if (nthreads > _GG_THREADS_ABSURD_NTHREADS) 
			return GGI_EARGINVAL;
		if (nthreads < 1) return GGI_EARGINVAL;
		_gg_task_thread.num = (int)nthreads;
	}

	res = GGI_ENOMEM;
	_gg_task_thread.handles = 
		malloc(_gg_task_thread.num * sizeof(pthread_t));
	if(_gg_task_thread.handles == NULL) goto bail;

	res = GGI_EUNKNOWN;
	if(pthread_mutex_init(&_gg_task_thread.ssmtx, NULL)) goto bail;
	_gg_task_thread.ssmtx_valid = 1;
	if(pthread_mutex_init(&_gg_task_thread.cmtx, NULL)) goto bail;
	_gg_task_thread.cmtx_valid = 1;
	if(pthread_mutex_init(&_gg_task_thread.mtx, NULL)) goto bail;
	_gg_task_thread.mtx_valid = 1;
	if(pthread_cond_init(&_gg_task_thread.tick, NULL)) goto bail;
	_gg_task_thread.tick_valid = 1;

	*start = _gg_task_thread_start;
	*stop = _gg_task_thread_stop;
	*xit = _gg_task_thread_exit;
	/* TODO: make this unlock unneccesary */
	ggUnlock(_gg_global_mutex);
	ggRegisterCleanup(_gg_task_thread_signal, NULL);
	ggLock(_gg_global_mutex);
	return 0;
 bail:
	_gg_task_thread_exit();
	return res;
}
