/* $Id: sigsched.c,v 1.12 2005/07/30 11:46:40 soyt Exp $
******************************************************************************

   Task scheduler implementation using various alarm/timer/signal mechanisms.

   Copyright (C) 1998  Steve Cheng     [steve@ggi-project.org]
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

#include "plat.h"
#include <ggi/internal/gg.h>
#include <ggi/internal/gg_replace.h>
#include <ggi/internal/gg_debug.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/wait.h>

#define _GG_SIG_ABSURD_RATE 100

struct {
	int rate;		 /* rate in usecs                 */
        int pid;		 /* pid of child process, if any  */
	int done;	 	 /* Set when the scheduler should terminate */
	void *block;		 /* Lock against double-entry into scheduler */
} _gg_task_sig;


static void _gg_task_handler(int unused);
static int _gg_sig_restore(void) {
#ifdef HAVE_SIGACTION
	struct sigaction sa;
	int res;

	do {
		res = sigaction(_gg_signum_dead, NULL, &sa);
	} while (res == -1 && errno == EINTR);
	assert(res != -1);

	/* Check if program is crashing/in cleanups */
	if (sa.sa_handler == _gg_sigfunc_dead) return -1;

	/* Check if we need to restore the handler (very unlikely since
	 * system supports sigaction, but what the heck...) 
	 */
	if (sa.sa_handler == SIG_DFL) {
#ifdef SA_RESTART
		sa.sa_flags = SA_RESTART;
#else
		sa.sa_flags = 0;
#endif
		res = sigemptyset(&sa.sa_mask);
		assert(res == 0);
		sa.sa_handler = _gg_task_handler;
		do {
			res = sigaction(_gg_signum_dead, &sa, NULL);
		} while (res == -1 && errno == EINTR);
		assert(res != -1);
#ifdef HAVE_SIGINTERRUPT
		res = siginterrupt(_gg_signum_dead, 0);
		assert(res == 0);
#endif
	}
#else  /* HAVE_SIGACTION */
	/* There is no way to harmlessly check for SIG_DFL, so we
	 * just reinstall the handler unconditionally.
	 */
	res = signal(_gg_signum_dead, SIG_IGN);
	assert(res != SIG_ERR);
	if (res == _gg_sigfunc_dead) {
		res = signal(_gg_signum_dead, _gg_sigfunc_dead);
		assert(res != SIG_ERR);
		return -1;
	}
	res = signal(_gg_signum_dead, _gg_task_handler);
	assert(res != SIG_ERR);
#ifdef HAVE_SIGINTERRUPT
	res = siginterrupt(_gg_signum_dead, 0);
	assert(res == 0);
#endif

#endif  /* not HAVE_SIGACTION */
	return 0;
}


static void _gg_task_handler(int unused) {

	if (_gg_sig_restore()) return; /* Prevent death by SIG_DFL */

	/* Old mansync code used to try to use sigpending or timevals
	 * to prevent handler runs from hogging CPU.  We just chalk that
	 * up to be the fault of the application (the application can 
	 * tell in the task handler if tasks are being missed and
	 * those tasks that might cause such a problem should be 
	 * coded to ease off in such a case.)
	 * 
	 * As such, all we have to do is prevent reentry.
	 */
	if (ggTryLock(_gg_task_sig.block)) return;

	if (_gg_task_sig.done) goto skip;
	if (_gg_task_tick()) goto skip;  /* Locked out by main app. */
	_gg_task_tick_finish();
 skip:
	ggUnlock(_gg_task_sig.block);
}

static int _gg_task_sig_start(void) {
	_gg_task_sig.done = 0;
	return 0;
}

static int _gg_task_sig_stop(void) {
	_gg_task_sig.done = 1;
	return 0;
}

static void _gg_task_sig_cleanup(void *unused) {
	_gg_task_sig.done = 1;
	if(_gg_task_sig.pid > 0) {
		kill(_gg_task_sig.pid, SIGKILL);
		waitpid(_gg_task_sig.pid, NULL, 0); /* async-signal-safe? */
		_gg_task_sig.pid = 0;
	}
}

/* Deinitialize signal-based scheduler */
static int _gg_task_sig_exit(void) {
	int ret;

	_gg_task_sig.done = 1;
	if(_gg_task_sig.pid > 0) {
		kill(_gg_task_sig.pid, SIGKILL);
		waitpid(_gg_task_sig.pid, NULL, 0);
		_gg_task_sig.pid = 0;
	}

	ggUnlock(_gg_global_mutex); /* TODO: nix the need for this unlock */
	ret = ggUnregisterCleanup(_gg_task_sig_cleanup, NULL);
	ggLock(_gg_global_mutex); /* TODO: nix the need for this lock */
	ggUnlock(_gg_task_sig.block);
	ggLockDestroy(_gg_task_sig.block);
	return 0;
}

/* Initialize signal-based scheduler */
int _gg_task_driver_init(_gg_task_fn *start, _gg_task_fn *stop, 
                         _gg_task_fn *xit, int rate) {

	/* Minimum scheduler rate is 1HZ.  Because I'm lazy. */
	if (rate < 1) return GGI_EARGINVAL;
	if (rate > _GG_SIG_ABSURD_RATE) return GGI_EARGINVAL;
	_gg_task_sig.rate = 1000000/rate;

	/* This bounds check also prevents string truncation below. */
	if (_gg_signum_dead > 1000) return GGI_ENOSPACE;
	if (_gg_signum_dead < 1) return GGI_ENOSPACE;

	_gg_task_sig.done = 1;

	_gg_task_sig.block = ggLockCreate();
	if (!_gg_task_sig.block) return -1;

	if (_gg_sig_restore()) goto bail0;

	_gg_task_sig.pid = 0;
	ggUnlock(_gg_global_mutex); /* TODO: nix the need for this unlock */
	if (ggRegisterCleanup(_gg_task_sig_cleanup, NULL)) goto bail1;
	ggLock(_gg_global_mutex); /* TODO: nix the need for this lock */

        _gg_task_sig.pid = fork();
	if (_gg_task_sig.pid == 0) {
		char usecs[8], sig[5];

		snprintf(usecs, 8, "%7.7i", _gg_task_sig.rate);
		snprintf(sig, 5, "%4.4i", _gg_signum_dead);

#define GGTICK	INSTALL_PREFIX"/bin/ggtick"

		DPRINT_CORE("- try to run %s\n", GGTICK);
		execl(GGTICK, "ggtick", usecs, sig, (void *)NULL);

		DPRINT_CORE("! ggtick exec failed!\n");
		goto bail2;
	}
	if (_gg_task_sig.pid < 0) {
		DPRINT_CORE("! scheduler fork failed!\n");
		goto bail2;
	}
	
	*start = _gg_task_sig_start;
	*stop = _gg_task_sig_stop;
	*xit = _gg_task_sig_exit;
	
	return 0;
 bail2:
	ggUnlock(_gg_global_mutex); /* TODO: nix the need for this lock */
	ggUnregisterCleanup(_gg_task_sig_cleanup, NULL);
 bail1:
	ggLock(_gg_global_mutex); /* TODO: nix the need for this lock */
 bail0:
	ggLockDestroy(_gg_task_sig.block);
	return -1;
}
