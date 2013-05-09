/* $Id: win32sched.c,v 1.4 2005/01/14 09:32:08 pekberg Exp $
******************************************************************************

   Task scheduler implementation: compiled-in win32 "driver"

   Copyright (C) 2004  Peter Ekberg     [peda@lysator.liu.se]
   Copyright (C) 2004  Brian S. Julin   [skids@users.sourceforge.net]
   Copyright (C) 1998  Steve Cheng      [steve@ggi-project.org]

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
#include <process.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define _GG_THREADS_ABSURD_NTHREADS 64
#define _GG_THREADS_ABSURD_RATE 10000

static struct {
	int rate;		/* Time in 100nsecs between ticks	*/
	HANDLE tickdie[2];	/* Conditions for thread sync		*/
	void *ssmtx;		/* Mutex for start/stop			*/
	int num;		/* Number of entries in next member	*/
	HANDLE *handles;	/* Handles for each thread		*/
	int crashing;		/* Flag: all threads should die ASAP	*/
	void *cmtx;		/* Mutex for crash			*/
	void *waiter;		/* Lock for waiter			*/
	int waiters;		/* Number of waiting waiters		*/
	HANDLE waitersem;	/* Semaphore to find the last waiter	*/
} _gg_task_thread;


/* Test to see if the scheduler is crashing and if so bring it down fast. */
static void _gg_task_thread_crashout(void)
{
	if (!_gg_task_thread.crashing) return;
	/* Scheduler is crashing. 
	 *
	 * "Say Goodbye to All of... THIS... and hello... TO OBLIVION."
	 */
	if (ggTryLock(_gg_task_thread.cmtx)) {
		/* ...but someone else is doing the wetwork. 
		 * We are a scheduler thread, so just exit.
		 */
		_endthreadex(0);
	}
	/* ...and we just got the hot potato! 
	 *
	 * "You die, she dies, everybody dies."
	 */

	SetEvent(_gg_task_thread.tickdie[1]);
	_endthreadex(0);
}


static inline int _gg_task_thread_running(void)
{
	return WaitForSingleObject(_gg_task_thread.tickdie[1], 0)
		== WAIT_TIMEOUT;
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
static void _gg_task_thread_signal(void *unused)
{
	/* Raise a flag and let the threads kill themselves.
	 */
	_gg_task_thread.crashing = 1;

	/* TODO: does win32 have enough to determine what thread
	 * we are in (GetCurrentThreadId) and cancel the others
	 * (GetThreadId, SetEvent, WaitForSingleObject) and then
	 * self-terminate (_endthreadex)?
	 * It might be nice to call the above function here if
	 * the system supports it.
	 */
}

/* There is one thread that does sleeping.  This is the code it runs.	*/
static unsigned __stdcall _gg_task_thread_sleeper(void *context)
{
	int defer = 0;

	/* When first started we don't want an instant tick. */
	ggUSlumber(_gg_task_thread.rate);

	while (1) {
		struct timeval then, now;
		int used;

		ggCurTime(&then);
		_gg_task_thread_crashout();
		if(!_gg_task_thread_running()) _endthreadex(0);

//		printf("sleeper %d run\n", GetCurrentThreadId());fflush(stdout);
		ggLock(_gg_task_thread.waiter);
		defer = _gg_task_tick();
		if(!defer) {
			if(_gg_task_thread.waiters > 1)
				/* More than one waiting waiter, set it up so that
				 * the last waiting waiter knows that it is last
				 * and restores state when it doesn't get this
				 * semaphore.
				 */
				ReleaseSemaphore(_gg_task_thread.waitersem, _gg_task_thread.waiters - 1, NULL);
			if(_gg_task_thread.waiters != 0)
				/* release waiters, the last waiter restores state */
				SetEvent(_gg_task_thread.tickdie[0]);
			else
				/* no waiters, restoring state myself */
				ggUnlock(_gg_task_thread.waiter);
		}
		else
			ggUnlock(_gg_task_thread.waiter);

//		printf("sleeper %d tick\n", GetCurrentThreadId());fflush(stdout);
		if(!defer)
			if(_gg_task_tick_finish()) _endthreadex(0);
		_gg_task_thread_crashout();
		if(!_gg_task_thread_running()) _endthreadex(0);
		/* Nix unwanted sleep. */
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

/* The rest of the threads run this code to synchronize with the above one */
static unsigned __stdcall _gg_task_thread_waiter(void *context)
{
	while (1) {
		_gg_task_thread_crashout();

		ggLock(_gg_task_thread.waiter);
		_gg_task_thread.waiters++;
		ggUnlock(_gg_task_thread.waiter);
//		printf("waiter %d waiting\n", GetCurrentThreadId());fflush(stdout);
		switch(WaitForMultipleObjects(
			2, _gg_task_thread.tickdie, FALSE, INFINITE)) {
		case WAIT_OBJECT_0 + 1: /* die */
			_endthreadex(0);
		/* TODO: case WAIT_FAILED: */
		}
		if(WaitForSingleObject(_gg_task_thread.waitersem, 0) == WAIT_TIMEOUT) {
			/* I am the last waiting waiter, so I get to
			 * restore state for the next tick
			 */
			ResetEvent(_gg_task_thread.tickdie[0]);
			_gg_task_thread.waiters = 0;
			ggUnlock(_gg_task_thread.waiter);
		}
		if(!_gg_task_thread_running()) _endthreadex(0);
		_gg_task_thread_crashout();
//		printf("waiter %d tock\n", GetCurrentThreadId());fflush(stdout);
		if (_gg_task_tock()) _endthreadex(0);
	}
}

/* This code fragment is used by start/stop/exit to kill running threads. */
static int _gg_task_thread_reap(void)
{
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
	if(_gg_task_thread.handles[0] != NULL) {
		WaitForSingleObject(_gg_task_thread.handles[0], INFINITE);
		CloseHandle(_gg_task_thread.handles[0]);
		_gg_task_thread.handles[0] = NULL;
	}

	/* Some threads may have died, some may still be waiting on
	 * the condition.  Some may even still be running tasks, but those
	 * will terminate themselves when they finish the task and notice
	 * the scheduler is trying to stop.
	 *
	 * Send the condition to wake up the ones that are waiting.
	 */
	if(_gg_task_thread.tickdie[1] != NULL)
		SetEvent(_gg_task_thread.tickdie[1]);

	/* Now we wait for them all to die. */
	for (i = 1; i < _gg_task_thread.num; i++) {
		if(_gg_task_thread.handles[i] == NULL)
			continue;
		WaitForSingleObject(_gg_task_thread.handles[i], INFINITE);
		CloseHandle(_gg_task_thread.handles[i]);
		_gg_task_thread.handles[i] = NULL;
	}

	return res;
}

/* Stop the scheduler.  This should not be invoked by scheduler threads. 
 * The caller must ensure that the scheduler will tell threads to die
 * when they are done running their current task.
 */
static int _gg_task_thread_stop(void)
{
	int res;

	/* Wait for any other start/stop calls to finish */
	ggLock(_gg_task_thread.ssmtx);

	res = 0;
	if(!_gg_task_thread_running()) goto done;
	SetEvent(_gg_task_thread.tickdie[1]);
	res = _gg_task_thread_reap();
 done:
	ggUnlock(_gg_task_thread.ssmtx);
	if (res) return GGI_EUNKNOWN;
	return 0;
}

/* Start all threads when scheduler gets first task.  This should
 * not be called by scheduler threads.
 */
static int _gg_task_thread_start(void)
{
	int i;

	/* Wait for any other start/stop calls to finish */
	ggLock(_gg_task_thread.ssmtx);

	/* If the scheduler died (or is currently dying) of its own 
	 * accord, it still has (possibly still running) threads that 
	 * need to be destroyed.
	 */
	if(_gg_task_thread_running()) {
		SetEvent(_gg_task_thread.tickdie[1]);
		_gg_task_thread_reap();
	}
	ResetEvent(_gg_task_thread.tickdie[1]);

	while(WaitForSingleObject(_gg_task_thread.waitersem, 0) == WAIT_OBJECT_0);
	_gg_task_thread.waiters = 0;
	ggUnlock(_gg_task_thread.waiter);

	/* Since all these threads do not do anything until the 
	 * sleeper thread starts, we start them first.  This makes
	 * cleanup in the event of a failure free from the locking 
	 * problems that would result if we allowed the scheduler
	 * to be running while we were creating them.
	 */
	for (i = 1; i < _gg_task_thread.num; i++) {
		_gg_task_thread.handles[i] = (HANDLE)_beginthreadex(
			NULL, 0,
			_gg_task_thread_waiter,
			NULL, 0, NULL);
		if(_gg_task_thread.handles[i] == NULL)
			goto bail;
	}

	/* This thread sleeps, those above wait for it to signal. */
	_gg_task_thread.handles[0] = (HANDLE)_beginthreadex(
		NULL, 0,
		_gg_task_thread_sleeper,
		NULL, 0, NULL);
	if(_gg_task_thread.handles[0] == NULL)
		goto bail;
	ggUnlock(_gg_task_thread.ssmtx);
	return 0;
 bail:
	SetEvent(_gg_task_thread.tickdie[1]);
	_gg_task_thread_reap();
	ggUnlock(_gg_task_thread.ssmtx);
	return GGI_EUNKNOWN;
}


/* Clean up on graceful exit. */
static int _gg_task_thread_exit(void)
{
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
	if (_gg_task_thread.ssmtx != NULL)
		ggLock(_gg_task_thread.ssmtx);

	if (_gg_task_thread.tickdie[1] != NULL) {
		SetEvent(_gg_task_thread.tickdie[1]);
		_gg_task_thread_reap();
		CloseHandle(_gg_task_thread.tickdie[1]);
		_gg_task_thread.tickdie[1] = NULL;
	}
	
	if(_gg_task_thread.handles) free(_gg_task_thread.handles);
	_gg_task_thread.handles = NULL;
	if(_gg_task_thread.tickdie[0] != NULL) {
		CloseHandle(_gg_task_thread.tickdie[0]);
		_gg_task_thread.tickdie[0] = NULL;
	}
	/* TODO: make this unlock unnecessary */
	ggUnlock(_gg_global_mutex);
	ggUnregisterCleanup(_gg_task_thread_signal, NULL);
	ggLock(_gg_global_mutex);
	if(_gg_task_thread.cmtx != NULL) {
		ggLockDestroy(_gg_task_thread.cmtx);
		_gg_task_thread.cmtx = NULL;
	}
	if(_gg_task_thread.waiter != NULL) {
		ggLockDestroy(_gg_task_thread.waiter);
		_gg_task_thread.waiter = NULL;
	}
	if(_gg_task_thread.waitersem != NULL) {
		CloseHandle(_gg_task_thread.waitersem);
		_gg_task_thread.waitersem = NULL;
	}
	if(_gg_task_thread.ssmtx != NULL) {
		ggUnlock(_gg_task_thread.ssmtx);
		ggLockDestroy(_gg_task_thread.ssmtx);
		_gg_task_thread.ssmtx = NULL;
	}
	return 0;
}


/* Initialize threads-based scheduler */
int _gg_task_driver_init(_gg_task_fn *start, _gg_task_fn *stop, 
			 _gg_task_fn *xit, int rate)
{
	int res;
	int i;

	_gg_task_thread.crashing = 0;
	_gg_task_thread.ssmtx = NULL;
	_gg_task_thread.cmtx = NULL;
	_gg_task_thread.tickdie[0] = NULL;
	_gg_task_thread.tickdie[1] = NULL;
	_gg_task_thread.waiter = NULL;
	_gg_task_thread.waitersem = NULL;

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
		malloc(_gg_task_thread.num * sizeof(HANDLE));
	if(_gg_task_thread.handles == NULL) goto bail;
	for(i=0; i<_gg_task_thread.num; i++)
		_gg_task_thread.handles[i] = NULL;

	res = GGI_EUNKNOWN;
	_gg_task_thread.ssmtx = ggLockCreate();
	if(_gg_task_thread.ssmtx == NULL) goto bail;
	_gg_task_thread.cmtx = ggLockCreate();
	if(_gg_task_thread.cmtx == NULL) goto bail;
	_gg_task_thread.tickdie[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(_gg_task_thread.tickdie[0] == NULL) goto bail;
	_gg_task_thread.tickdie[1] = CreateEvent(NULL, TRUE, TRUE, NULL);
	if(_gg_task_thread.tickdie[1] == NULL) goto bail;
	_gg_task_thread.waiter = ggLockCreate();
	if(_gg_task_thread.waiter == NULL) goto bail;
	_gg_task_thread.waitersem = CreateSemaphore(NULL, 0, _gg_task_thread.num, NULL);
	if(_gg_task_thread.waitersem == NULL) goto bail;
	_gg_task_thread.waiters = 0;

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
