/* $Id: task.c,v 1.7 2005/07/29 16:40:52 soyt Exp $
******************************************************************************

   LibGG - Configuration handling

   Copyright (C) 2004 Brian S. Julin

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
#include <string.h>

static struct {
  /* Rate of scheduler in HZ */
  int			rate;

  /* Lock keeps threading out of AddTask/DelTask */
  void			*editlock;

  /* Lock used to signal when tick arrives during AddTask/DelTask */
  void			*deadlock;

  /* Pointer into double linked ring-list of all tasks */
  struct gg_task	*all;

  /* Pointer to tasks with earliest expiring deadline */
  struct gg_task	*dl;

  /* Current tick.  Note does not increment when no tasks registered. */
  int			currtick;

  /* Functions to start and stop the scheduler */
  _gg_task_fn		start, stop, xit;
} _gg_task_sched;

uint32_t ggTimeBase(void) {
	return 1000000 / _gg_task_sched.rate;
}

void _ggTaskExit(void) {
	struct gg_task *task;

	/* The only way editlock can get locked on us is by 
	 * another thread or by a signal handler, both of which 
	 * will return us control, so it is safe to spin.
	 */
	ggLock(_gg_task_sched.editlock);
 again:
	task = _gg_task_sched.all;
	if (task == NULL) goto done;

	/* Make sure our list/ring entry points don't point to
	 * the removed event.
	 */
	if (_gg_task_sched.all == task) _gg_task_sched.all = task->next;
	if (_gg_task_sched.all == task) _gg_task_sched.all = NULL;
	if (_gg_task_sched.dl == task) _gg_task_sched.dl = task->nextdl;
	if (_gg_task_sched.dl == task) _gg_task_sched.dl = NULL;

	/* Unlink from ring */
	task->next->last = task->last;
	task->last->next = task->next;
	task->next = task->last = NULL;
	if (task->nextdl) {
		task->nextdl->lastdl = task->lastdl;
		task->lastdl->nextdl = task->nextdl;
		task->nextdl = task->lastdl = NULL;
	}

	/* Remove the lock, unless the task is currently running. */
	if (!ggTryLock(task->exelock)) {
		/* The task was not running, and we locked the task.  Since 
		 * we also have editlock, we know nothing will come along and 
		 * use it so it is safe to (unlock it and then) delete it.
		 */
		ggUnlock(task->exelock);
		ggLockDestroy(task->exelock);
		task->exelock = NULL;
	}
	/* Otherwise we let code elsewhere detect that the lock needs to
	 * be removed (it checks if task->next has been NULLed out.)
	 */
	goto again;

 done:
	/* deadlock is normally locked.  If the tick comes while we
	 * (or some other thread) have editlock locked up, then 
	 * it will unlock deadlock.  Since we just cleared all the 
	 * tasks, though, we do not have to run the tick, but to
	 * be consistant we should leave deadlock locked.
	 */
	ggTryLock(_gg_task_sched.deadlock);
	ggUnlock(_gg_task_sched.editlock);

	/* Now we let the driver stop itself */
	_gg_task_sched.stop();

	/* We have the driver do any final tidying up. */
	_gg_task_sched.xit();

	/* And now we are finally ready to clean up our own mess. */
	ggUnlock(_gg_task_sched.editlock); /* may have been locked since */
	ggLockDestroy(_gg_task_sched.editlock);
	_gg_task_sched.editlock = NULL;
	ggUnlock(_gg_task_sched.deadlock);
	ggLockDestroy(_gg_task_sched.deadlock);
	_gg_task_sched.deadlock = NULL;
	return;
}

int _ggTaskInit(void) {
	int rate;
	memset (&_gg_task_sched, 0, sizeof(_gg_task_sched));

	/* This option defaults to "60" so it should have a number
	 * unless the user screwed up GG_OPTS.
	 *
	 * For now we must have >= 1HZ.
	 */
	rate = strtoul(_gg_optlist[_GG_OPT_SCHEDHZ].result, NULL, 10);
	if (rate < 1) return GGI_EARGINVAL;
	_gg_task_sched.rate = rate;

	_gg_task_sched.editlock = ggLockCreate();
	if(_gg_task_sched.editlock == NULL) return GGI_ENOMEM;
	_gg_task_sched.deadlock = ggLockCreate();
	if(_gg_task_sched.deadlock == NULL) {
		ggLockDestroy(_gg_task_sched.editlock);
		_gg_task_sched.editlock = NULL;
		return GGI_ENOMEM;
	}
	ggLock(_gg_task_sched.deadlock);

	if (_gg_task_driver_init(&_gg_task_sched.start, &_gg_task_sched.stop, 
				 &_gg_task_sched.xit, rate)) {
		ggLockDestroy(_gg_task_sched.editlock);
		_gg_task_sched.editlock = NULL;
		ggLockDestroy(_gg_task_sched.deadlock);
		_gg_task_sched.deadlock = NULL;
		return GGI_EARGINVAL;
	}
	return 0;
}

static inline int deadline(struct gg_task *t) {
	int elapsed;
	if (t->lasttick > _gg_task_sched.currtick) {
		elapsed = 
		  GG_SCHED_TICK_WRAP - t->lasttick + _gg_task_sched.currtick;
	}
	else elapsed = _gg_task_sched.currtick - t->lasttick;

	if (t->pticks < elapsed) return 0;
	return t->pticks - elapsed;
}

/* Find and group together the task(s) with the earliest impending deadline.
 * Assumes editlock is already acquired.
 */
static void _gg_task_build_dl(void) {
	int edl, tdl;
	struct gg_task *task;

	edl = GG_SCHED_TICK_WRAP - 1;

	task = _gg_task_sched.all;
	if (task == NULL) return;	/* No tasks */

	do {
		tdl = deadline(task);
		if (tdl < edl) edl = tdl;
		task = task->next;
	} while (task != _gg_task_sched.all);

	/* SUBTLE: We assume here that if there are no two existing rings 
	 * with the same deadline. However, there may be unjoined entries
	 * to add to the edl ring.
	 */
	task = _gg_task_sched.all;
	do {
		tdl = deadline(task);
		if (tdl == edl) {
			if (task->nextdl == NULL) {
		  		if (_gg_task_sched.dl != NULL) {
					task->lastdl = 
					  _gg_task_sched.dl->lastdl;
					task->nextdl = _gg_task_sched.dl;
					_gg_task_sched.dl->lastdl->nextdl = 
					  task;
					_gg_task_sched.dl->lastdl = task;
				} else {
					task->lastdl = task->nextdl = task;
				}
			}
			_gg_task_sched.dl = task;
		}
		task = task->next;
	} while(task != _gg_task_sched.all);
}

/* Run tasks in the task list.  Assumes editlock is already acquired.
 * May let go of editlock but will re-acquire editlock before exit.
 */
static void _gg_task_run(void) {
	struct gg_task *task;

	if (_gg_task_sched.dl == NULL) return;
	if (deadline(_gg_task_sched.dl) != 0) return;

 again:
	/* Remove a task from the dl list, retaining a handle to it */
	task = _gg_task_sched.dl;
	if (task == NULL) return;
	task->lastdl->nextdl = task->nextdl;
	task->nextdl->lastdl = task->lastdl;
	if (task->nextdl == task) _gg_task_sched.dl = NULL;
	else _gg_task_sched.dl = task->nextdl;
	task->nextdl = task->lastdl = NULL;

	/* Check for misbehaved tasks still running from a previous tick.  
	 * We don't run them.  They miss ticks for their crime.  The
	 * thread running them will take care of the rest so we just
	 * move on.
	 */
	if (ggTryLock(task->exelock)) goto again;

	/* Now that our task has been removed from the dl list, and 
	 * is locked to tell any future ticks that it is running,
	 * we can let other threads (or Add/Del) into the scheduler 
	 * so they can find a task to run.
	 */
	ggUnlock(_gg_task_sched.editlock);


	task->lasttick = _gg_task_sched.currtick;
	if (task->ncalls >= 0) {
		if (task->ncalls == 1) task->ncalls = -1;
		if (task->ncalls > 1) task->ncalls--;
		task->cb(task);

		/* Check if the task has been deleted.  
		 * Note we must unlock exelock to safely destroy it
		 * but we do not have to acquire editlock for that
		 * because ggDelTask will have already removed the task
		 * from .dl and .all, so nothing else will try
		 * to grab it.
		 */
		if (task->next == NULL) {
			ggUnlock(task->exelock);
			ggLockDestroy(task->exelock);
			task->exelock = NULL;
			ggLock(_gg_task_sched.editlock);
			goto again;
		}
		
		/* Check if we just ran a misbehaved task.
		 * If so, we check to see if it is in the dl ring, and
		 * we remove it.  We update the lasttick field but
		 * we do not touch the ncalls field.  The task
		 * can check those itself to see if it missed ticks.
		 */
		if (task->lasttick != _gg_task_sched.currtick) {
			fprintf(stderr, "bad task\n");
			ggLock(_gg_task_sched.editlock);
			task->lasttick = _gg_task_sched.currtick;
			goto remove;
		}
	}
	ggLock(_gg_task_sched.editlock);

	if (task->ncalls < 0) {
	remove:
		/* If it has been picked up by a new dl list, take it out */
		if (task->nextdl != NULL) {
			task->lastdl->nextdl = task->nextdl;
			task->nextdl->lastdl = task->lastdl;
			if (_gg_task_sched.dl == task) {
				if (task->nextdl == task) 
					_gg_task_sched.dl = NULL;
				else _gg_task_sched.dl = task->nextdl;
			}
			task->nextdl = task->lastdl = NULL;
		}
	}

	if (task->ncalls < 0) {
		/* Remove the task entirely from the scheduler */
		task->last->next = task->next;
		task->next->last = task->last;
		if (_gg_task_sched.all == task) {
			if (task->next == task) _gg_task_sched.all = NULL;
			else _gg_task_sched.all = task->next;
		}
		task->next = task->last = NULL;
		ggUnlock(task->exelock);
		ggLockDestroy(task->exelock);
		task->exelock = NULL;
		goto again;
	}

	ggUnlock(task->exelock);
	goto again;
}

/* Used to begin a tick by the primary thread or by a signal handler 
 * Note this leaves the editlock locked, and *must* be followed by
 * calling gg_task_tick_finish or by explicitly unlocking by something 
 * else inside this file.
 */
int _gg_task_tick(void) {

	/* If ggAddTask or ggDelTask are editing, we let them finish. */
	if (ggTryLock(_gg_task_sched.editlock)) {
	        fprintf(stderr, "defer)");
		/* Tell ggAddTask/ggDelTask that they have to start the tick */
		ggUnlock(_gg_task_sched.deadlock);
		return 1;
	}
	_gg_task_sched.currtick++;
	_gg_task_sched.currtick %= GG_SCHED_TICK_WRAP;
	/* There may still be leftover tasks running from the last tick, 
	 * if there are other threads, but they will have been removed 
	 * from the dl list, so it is safe to build the next one now.
	 */
	_gg_task_build_dl();
	return 0;
}

/* Used to proceed with tick by the primary thread or signal handler 
 * Note this assumes the editlock is held (locked by _gg_task_tick.)
 */
int _gg_task_tick_finish(void) {
	int ret;

	ret = 0;
	_gg_task_run();
	if (_gg_task_sched.all == NULL) ret = 1;
	ggUnlock(_gg_task_sched.editlock);
	return ret;
}

/* Used by secondary threads (if any) to join a tick in-progress. */
int _gg_task_tock(void) {
	int ret;

	/* The edit lock will be held by one of two owners:
	 *
	 * 1) If ggAddTask or ggDelTask are editing, we wait for them to 
	 * finish (and start the tick).
	 *
	 * 2) If _gg_task_tick has it, the tick has already started
	 * and we wait for the primary thread to let go, which it will
	 * do when it finds a task to run.
	 */
	ret = 0;
	ggLock(_gg_task_sched.editlock);
	_gg_task_run();
	if (_gg_task_sched.all == NULL) ret = 1;
	ggUnlock(_gg_task_sched.editlock);
	return ret;
}

int ggAddTask(struct gg_task *task) {
	int err, dlnew, dlcurr;

	if (task == NULL) return GGI_EARGREQ;

	err = GGI_EARGINVAL;

	if (task->cb == NULL) goto bail0;
	if (task->pticks < 1) goto bail0;
	if (task->pticks >= GG_SCHED_TICK_WRAP) goto bail0;
	if (task->ncalls < 0) goto bail0;

	/* This keeps a task from being added more than once */
	if (task->exelock != NULL) return GGI_EBUSY;

	err = GGI_ENOMEM;

	task->exelock = ggLockCreate();
	if (task->exelock == NULL) goto bail0;

	/* The only way editlock can get locked on us is by 
	 * another thread or by a signal handler, both of which 
	 * will return us control, so it is safe to spin.
	 */
	ggLock(_gg_task_sched.editlock);

	task->lasttick = _gg_task_sched.currtick;

	if (_gg_task_sched.all == NULL) {
		/* We are the only task. */
		task->last = task->next = task;
		_gg_task_sched.all = task;
		task->nextdl = task->lastdl = task;
		_gg_task_sched.dl = task;
		/* Start the scheduler mechanism up */
		_gg_task_sched.start();
		goto skip;
	} 

	task->last = _gg_task_sched.all->last;
	task->next = _gg_task_sched.all;
	_gg_task_sched.all->last->next = task;
	_gg_task_sched.all->last = task;
	_gg_task_sched.all = task;

	if (_gg_task_sched.dl == NULL) {
		/* Will be added when the dl list is built. */
		task->nextdl = task->lastdl = NULL;
		goto skip;
	}
	dlnew = deadline(task);
	dlcurr = deadline(_gg_task_sched.dl);
	if (dlnew < dlcurr) {
		/* We are the single earliest scheduled task. */
		task->nextdl = task->lastdl = task;
		_gg_task_sched.dl = task;
	} else if (dlnew == dlcurr) {
		/* We have the same deadline as the current dl list */
		task->lastdl = _gg_task_sched.dl->lastdl;
		task->nextdl = _gg_task_sched.dl;
		_gg_task_sched.dl->lastdl->nextdl = task;
		_gg_task_sched.dl->lastdl = task;
		_gg_task_sched.dl = task;
	} else {
		/* This task will be added to the dl list later */
		task->nextdl = task->lastdl = NULL;
	}

 skip:
	/* deadlock is normally locked.  If the tick comes while we
	 * (or some other thread) have editlock locked up, then 
	 * it will unlock deadlock.
	 */
	if (!ggTryLock(_gg_task_sched.deadlock)) {
		/* We locked deadlock.  That means we have the 
		 * responsibility to run the tick.
		 */
		_gg_task_sched.currtick++;
		_gg_task_sched.currtick %= GG_SCHED_TICK_WRAP;
		/* There may be leftover tasks running from the last tick, 
		 * if there are other threads, but they will have been removed 
		 * from the dl list, so it is safe to build the next one now.
		 */
		_gg_task_build_dl();
		_gg_task_run();
	}
	ggUnlock(_gg_task_sched.editlock);
	return 0;
	/* bail1: */
	ggUnlock(_gg_task_sched.editlock);
	ggLockDestroy(task->exelock);
	task->exelock = NULL;
 bail0:
	return err;
}


int ggDelTask(struct gg_task *task) {

	if (task == NULL) return GGI_EARGREQ;
	if (task->exelock == NULL) return GGI_EARGINVAL;

	/* The only way editlock can get locked on us is by 
	 * another thread or by a signal handler, both of which 
	 * will return us control, so it is safe to spin.
	 */
	ggLock(_gg_task_sched.editlock);

	/* Make sure our list/ring entry points don't point to
	 * the removed event.
	 */
	if (_gg_task_sched.all == task) _gg_task_sched.all = task->next;
	if (_gg_task_sched.all == task) _gg_task_sched.all = NULL;
	if (_gg_task_sched.dl == task) _gg_task_sched.dl = task->nextdl;
	if (_gg_task_sched.dl == task) _gg_task_sched.dl = NULL;

	/* Unlink from ring */
	task->next->last = task->last;
	task->last->next = task->next;
	task->next = task->last = NULL;
	if (task->nextdl != NULL) {
		task->nextdl->lastdl = task->lastdl;
		task->lastdl->nextdl = task->nextdl;
		task->nextdl = task->lastdl = NULL;
	}

	/* Remove the lock, unless the task is currently running. */
	if (!ggTryLock(task->exelock)) {
		/* The task was not running, and we locked the task.  Since 
		 * we also have editlock, we know nothing will come along and 
		 * use it so it is safe to (unlock it and then) delete it.
		 */
		ggUnlock(task->exelock);
		ggLockDestroy(task->exelock);
		task->exelock = NULL;
	}
	/* Otherwise we let code elsewhere detect that the lock needs to
	 * be destroyed (it checks if task->next has been NULLed out.)
	 */

	if (_gg_task_sched.all == NULL) goto stop;

	/* deadlock is normally locked.  If the tick comes while we
	 * (or some other thread) have editlock locked up, then 
	 * it will unlock deadlock.
	 */
	if (!ggTryLock(_gg_task_sched.deadlock)) {
		/* We locked deadlock.  That means we have the 
		 * responsibility to run the tick.
		 */
		_gg_task_sched.currtick++;
		_gg_task_sched.currtick %= GG_SCHED_TICK_WRAP;
		/* There may be leftover tasks running from the last tick, 
		 * if there are other threads, but they will have been removed 
		 * from the dl list, so it is safe to build the next one now.
		 */
		_gg_task_build_dl();
		_gg_task_run();
	}
	ggUnlock(_gg_task_sched.editlock);
	return 0;
 stop:
	/* deadlock is normally locked.  If the tick comes while we
	 * (or some other thread) have editlock locked up, then 
	 * it will unlock deadlock.  Since we just cleared all the 
	 * tasks, we do not have to run the tick, but to be consistant 
	 * we should leave deadlock locked.
	 */
	ggTryLock(_gg_task_sched.deadlock);
	ggUnlock(_gg_task_sched.editlock);
	_gg_task_sched.stop();
	ggTryLock(_gg_task_sched.deadlock);
	ggUnlock(_gg_task_sched.editlock);
	return 0;
}


