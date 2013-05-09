/* $Id: cleanup.c,v 1.10 2004/10/05 08:40:10 pekberg Exp $
******************************************************************************

   LibGG - Functions for adding and removing cleanup callbacks

   Copyright (C) 2004 Brian S. Julin	[skids@users.sourceforge.net]
   Copyright (C) 2004 Peter Ekberg	[peda@lysator.liu.se]
   Copyright (C) 1998 Marcus Sundberg	[marcus@ggi-project.org]

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
#include <ggi/internal/gg.h>
#include "cleanup.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#if defined(__WIN32__) && !defined(__CYGWIN__)
/* sleep is really in seconds and Sleep in milliseconds,
 * but in this file, sleep is only used to take a short
 * nap. Why not make it shorter than one second when we
 * have the opportunity?
 */
#define sleep(x) Sleep((x)*10)
#endif

/* The LibGG cleanup handlers only get run once, after any handler that
 * the application may have installed (for the signal being processed)
 * before it registered any cleanups.
 *
 * That sounds simple, but it is actually a fairly complicated thing to
 * implement, because we must prevent the callbacks from being run 
 * more than once even in threaded, multiprocessor environments.
 *
 * There are lots of OS-specific ways to prevent signal handlers from 
 * being reentered and make signals play nice with threads, but we do 
 * not want to use them because of portability issues and because we want 
 * to touch those settings as little as possible to give the application 
 * the least unexpected results if it uses them when overloading our 
 * signal handler.  In fact we do some gymnastics to try to keep the 
 * settings consistant for the signal handlers of the application which 
 * we overload with the cleanup signal handler.
 * 
 * The below implementation should work in all unthreaded environments 
 * and in POSIX threaded environments (even if the threading library is
 * not pthreads) -- if it doesn't then the environment is broken.  We
 * rely only on documented POSIX behavior and then only in the threaded
 * case.
 *
 * In non-POSIX, threaded environments we do our best, and so
 * double-running of cleanups should be very rare even on those systems.
 */

typedef struct funclist {
	ggcleanup_func	*func;
	void		*arg;
	struct funclist	*next;
} funclist;

static funclist *cleanups = NULL, *free_cus = NULL;
static int nofallback = 0;
static int force_exit = 0;
static int cleanups_grabbed = 0;

/* grab_cleanups_cond is a ggLock that is used as cleanup condition when not
 * using the _gg_signum_dead signal as cleanup mutex. grap_cleanups_cond
 * is used when _gg_signum_dead is zero.
 * Specifically, this is fine on Win32, as there is no restrictions on
 * using ggLocks in a structured exception handler and there is no
 * unused signal available that can be used as mutex. And a structured
 * exception handler is good to have to run cleanups is some extra cases
 * as there are exceptions that do not generate signals.
 */
static void *grab_cleanups_cond = NULL;

static void do_cleanup(funclist *curr)
{
	/* free() is not async safe, so dealloc has to happen later. */
	free_cus = curr;

	while (curr != NULL) {
		curr->func(curr->arg);
		curr = curr->next;
	}
}

static void free_cleanups(void)
{
	funclist *curr = free_cus;

	while (curr != NULL) {
		free_cus = free_cus->next;
		free(curr);
		curr = free_cus;
	}
}


#if defined(HAVE_SIGNAL) || defined(HAVE_SIGACTION)

typedef void (ggsighandler)(int);

typedef struct {
	int		  sig;
	ggsighandler     *oldhandler;
#ifdef HAVE_SIGACTION
	struct sigaction  oldsa;
#endif
} gg_siginfo;

#ifdef HAVE_SIGACTION
#define INITSIG(signam)	\
	{ signam,  SIG_ERR, }
#else
#define INITSIG(signam) \
	{ signam,  SIG_ERR }
#endif

static gg_siginfo siglist[] = {
#ifdef SIGHUP
	INITSIG(SIGHUP),
#endif
#ifdef SIGINT
	INITSIG(SIGINT),
#endif
#ifdef SIGQUIT
	INITSIG(SIGQUIT),
#endif
#ifdef SIGILL
	INITSIG(SIGILL),
#endif
#ifdef SIGTRAP
	INITSIG(SIGTRAP),
#endif
#ifdef SIGABRT
	INITSIG(SIGABRT),
#endif
#ifdef SIGBUS
	INITSIG(SIGBUS),
#endif
#ifdef SIGFPE
	INITSIG(SIGFPE),
#endif
#ifdef SIGSEGV
	INITSIG(SIGSEGV),
#endif
#ifdef SIGPIPE
	INITSIG(SIGPIPE),
#endif
#ifdef SIGALRM
	INITSIG(SIGALRM),
#endif
#ifdef SIGTERM
	INITSIG(SIGTERM),
#endif
#ifdef SIGSTKFLT
	INITSIG(SIGSTKFLT),
#endif
#ifdef SIGIO
	INITSIG(SIGIO),
#endif
#if defined(SIGPOLL) && (SIGPOLL != SIGIO)
	INITSIG(SIGPOLL),
#endif
#ifdef SIGXCPU
	INITSIG(SIGXCPU),
#endif
#ifdef SIGXFSZ
	INITSIG(SIGXFSZ),
#endif
#ifdef SIGVTALRM
	INITSIG(SIGVTALRM),
#endif
#ifdef SIGPROF
	INITSIG(SIGPROF),
#endif
#ifdef SIGPWR
	INITSIG(SIGPWR),
#endif
#ifdef SIGLOST
	INITSIG(SIGLOST),
#endif
#ifdef SIGUNUSED
	INITSIG(SIGUNUSED)
#endif
#ifdef SIGBREAK
	INITSIG(SIGBREAK)
#endif
};

#define SIGLIST_LEN	sizeof(siglist)/sizeof(gg_siginfo)

#ifdef HAVE_SIGACTION

/* Implementation for modern systems.   We assume the system never
 * sets the handler to SIG_DFL here, and we use sigaction().  In
 * the future we may also want to provide support for the three
 * argument form of the signal handler available on some systems
 * and deal with flags like SA_NOMASK.  For now LibGG does not 
 * promise to do so in the API documentation.
 */

static void sighandler(int signum);

static void unregister_sighandler(void)
{
	unsigned int i;

	/* Remove all our signal handlers (not including _gg_signum_dead).
	 * Note individual sighandlers may be removed already by the
	 * handlers themselves, so we try not to assume we have exclusive
	 * access.
	 */
	for (i = 0; i < SIGLIST_LEN; i++) {
		struct sigaction sa;

		/* Skip entries that are not in a registered state. */
		if (siglist[i].oldhandler == SIG_ERR) continue;

		/* Get the current handler.
		 *
		 * Note sleep() is specified as async-signal-safe, which
		 * is not true of ggUSleep(), so we use it here instead.
		 */
		while (sigaction(siglist[i].sig, NULL, &sa) != 0) sleep(1);

		if (sa.sa_handler != sighandler) {
			/* Someone else has changed this signal action, so
			 * we shouldn't touch it.  We leave the LibGG handler
			 * in place so that it will continue to forward
			 * signals to any handlers we overloaded, at least 
			 * until someone unloads our dl, and then there is 
			 * unavoidable trouble.
			 */
			continue;
		}
		while (sigaction(siglist[i].sig, &siglist[i].oldsa, NULL)) 
			sleep(1);
		siglist[i].oldhandler = SIG_ERR;
	}
}		


static void register_sighandler(void)
{
	unsigned int i;

	/* Register our signal handler for all fatal signals. */
	nofallback = 1;

	for (i = 0; i < SIGLIST_LEN; i++) {
		/* Get the current handler.
		 *
		 * Note sleep() is specified as async-signal-safe, which
		 * is not true of ggUSleep(), so we use it here instead.
		 */
		while (sigaction(siglist[i].sig, NULL, &siglist[i].oldsa)) 
			sleep(1);
		if (siglist[i].oldsa.sa_handler == SIG_DFL ||
		    siglist[i].oldsa.sa_handler == SIG_IGN) {
			struct sigaction sa;
			sa.sa_handler = sighandler;
			sa.sa_flags = 0;
			sigemptyset(&sa.sa_mask);
			while(sigaction(siglist[i].sig, &sa, NULL)) sleep(1);
			siglist[i].oldhandler = siglist[i].oldsa.sa_handler;
		}
	}
}

static void do_oload(int signum, int sli) {
	struct sigaction curact;

	/* Run an overloaded signal handler, if any.  
	 * If we are overloaded we keep the parent registered.  
	 * If not, we unregister LibGG for this signal.
	 */

	while (sigaction(signum, NULL, &curact)) sleep(1);

	if (siglist[sli].oldhandler == SIG_DFL) {
		if (curact.sa_handler == sighandler) {
			/* We are not being overloaded, so unregister */
			sigaction(signum, &siglist[sli].oldsa, NULL);
			siglist[sli].oldhandler = SIG_ERR;
		}
		return;
	}

	/* We have an overloaded handler to run.
	 *
	 * We perform a courtesy check to make sure that
	 * a badly written signal handler doesn't steal a 
	 * signal from a signal handler that is overloading us.
	 *
	 * Note sleep() is specified as async-signal-safe, which
	 * is not true of ggUSleep(), so we use it here instead.
	 */
	while (sigaction(signum, NULL, &curact)) sleep(1);
	if (curact.sa_handler == sighandler) {
		/* We are not being overloaded, so unregister. */
		sigaction(signum, &siglist[sli].oldsa, NULL);
		siglist[sli].oldhandler(signum);
		siglist[sli].oldhandler = SIG_ERR;
		return;
	}
	/* We are being overloaded, so we restore the parent signal
	 * handler after calling the handler just in case the handler 
	 * is buggy.
	 */
	siglist[sli].oldhandler(signum);
	while (sigaction(signum, &curact, NULL)) sleep(1);
}


static void sighandler(int signum)
{
	unsigned int sli;
	struct sigaction curact, deadact;
	funclist *run_cus = NULL;

	/* Find the siglist index for this signal. */
	for (sli = 0; sli < SIGLIST_LEN; sli++) {
		if (siglist[sli].sig == signum) goto found;
	}
	/* Uncomment for debug.  Not compiled in normally, because
	 * calling ggDprintf here is technically illegal.
	 *
	 * ggDPrintf("LibGG: Signal %i caught but not ours!\n", sli); 
	 */
	return;
 found:
	if (siglist[sli].oldhandler == SIG_ERR) {
		/* Uncomment for debug.  Not compiled in normally, because
		 * calling ggDPrintf here is technically illegal.
		 *
		 * ggDPrintf("LibGG: Caught unexpected signal (%i)!\n", sli);
		 */
		return;
	}

	/* Check and see if we should ignore this signal, as per manpage */
	if (siglist[sli].oldhandler == SIG_IGN) return;

	/* OK, now we know the cleanup list should be run, but we
	 * have to check if maybe some other thread (or previous
	 * or even future interrupt in this thread) has (using the word 
	 * "has" loosly :-) also reached this point and is intending to 
	 * run the cleanup handlers.
	 *
	 * First we do a short-circuit test that will work most
	 * of the time, and prevent entry into the below code,
	 * by checking if the cleanup list has been emptied.  This
	 * is just an optimization, as the below test is not 
	 * conclusive due to multiprocessor cache issues.
	 */
	run_cus = cleanups;
	cleanups = NULL;
	cleanups_grabbed = 1;
	if (run_cus == NULL) {
		/* Someone else has grabbed the cleanup list.
		 *
		 * Run any overloaded signal handler and we are done.
		 */
	oload:
		do_oload(signum, sli);
		return;
	}
	
	/* It is still possible for someone else to have gotten 
	 * the cleanup list, even in unthreaded environments, so 
	 * now we enter the "black magic" section.
	 *
	 * LibGG uses a special signal (_gg_signum_dead) as an 
	 * async-signal-safe mutex.  The application is not supposed 
	 * to use this signal.  (If the scheduler is signal based, it 
	 * also uses this signal, but that is part of LibGG so we know
	 * the handler will not be altered there.)
	 *
	 * We can do this because sigaction() is defined to be 
	 * async-signal-safe.  Thus it must perform what is for our
	 * purposes an atomic swap of a newly installed signal handler
	 * with the old one.  Only one execution of the following
	 * statement will return the original handler value, the rest 
	 * will return _gg_sigfunc_dead.
	 *
	 * Note sleep() is specified as async-signal-safe, which
	 * is not true of ggUSleep(), so we use it here instead.
	 */
	if(_gg_signum_dead == 0) {
		if(ggTryLock(grab_cleanups_cond))
			goto oload;
	}
	else {
		deadact.sa_handler = _gg_sigfunc_dead;
		deadact.sa_flags = 0;
		sigemptyset(&deadact.sa_mask);
		while (sigaction(_gg_signum_dead, &deadact, &curact)) sleep(1);
		if (curact.sa_handler == deadact.sa_handler) goto oload;
	}

	/* Now we *know* we should run the cleanups, noone else will.
	 * As per manpage, overloaded callbacks are run before we run 
	 * the cleanups.
	 */
	do_oload(signum, sli);
	do_cleanup(run_cus);

	_exit(-1);
}


/* Test for cleanup start from normal (ggExit) context.  See above comments. */
static funclist *grab_cleanup(void) {
	funclist *run_cus = NULL;  
	struct sigaction deadact, curact;

	run_cus = cleanups;
	cleanups = NULL;
	if (run_cus == NULL) return run_cus;

	if(_gg_signum_dead == 0) {
		if(ggTryLock(grab_cleanups_cond))
			return NULL;
	}
	else {
		deadact.sa_handler = _gg_sigfunc_dead;
		deadact.sa_flags = 0;
		sigemptyset(&deadact.sa_mask);
		while (sigaction(_gg_signum_dead, &deadact, &curact))
			ggUSleep(10000);
		if (curact.sa_handler == deadact.sa_handler) return NULL;
	}
	return run_cus;
}

#else


/* Implementation for systems that do not have sigaction() and thus
 * must use signal().  This includes systems that use the old, broken,
 * UNIX style signals, and also those with the better BSD-style signals.
 * However, at least we do not have to worry about signal mask alteration.
 */

static void sighandler(int signum);

static void unregister_sighandler(void)
{
	unsigned int i;

	for (i = 0; i < SIGLIST_LEN; i++) {
		ggsighandler *oldfunc;

		/* Skip entries that are not in a registered state. */
		if (siglist[i].oldhandler == SIG_ERR) continue;

		/* Get the current handler.
		 *
		 * Note sleep() is specified as async-signal-safe, which
		 * is not true of ggUSleep(), so we use it here instead.
		 */
		while (1) {
			oldfunc = 
			  signal(siglist[i].sig, siglist[i].oldhandler);
			if (oldfunc != SIG_ERR) break;
			sleep(1);
		}
		if (oldfunc != sighandler) {
			ggsighandler *ret;

			/* Someone else has changed this signal action, so
			 * we shouldn't touch it.  We leave the LibGG handler
			 * in place so that it will continue to forward
			 * signals to any handlers we overloaded, at least 
			 * until someone unloads our dl, and then there is 
			 * unavoidable trouble.
			 */
			while (1) {
				ret = signal(siglist[i].sig, oldfunc);
				if (ret != SIG_ERR) break;
				sleep(1);
			}
			continue;
		}
		siglist[i].oldhandler = SIG_ERR;
	}
}		


static void register_sighandler(void)
{
	unsigned int i;

	/* Register our signal handler for all fatal signals */
	nofallback = 1;

	for (i = 0; i < SIGLIST_LEN; i++) {
		ggsighandler *oldfunc;

		/* Note sleep() is specified as async-signal-safe, which
		 * is not true of ggUSleep(), so we use it here instead.
		 */
		while (1) {
			oldfunc = signal(siglist[i].sig, sighandler);
			if (oldfunc != SIG_ERR) break;
			sleep(1);
		}
		siglist[i].oldhandler = oldfunc;
	}
}


static void do_oload(int signum, int sli, ggsighandler *curfunc) {

	/* Run an overloaded signal handler, if any.  
	 * If we are overloaded we keep the parent registered.  
	 * If not, we unregister LibGG for this signal.
	 */
	if (siglist[sli].oldhandler == SIG_DFL) {
		/* We are not overloading another handler */

		if (curfunc == sighandler) goto unreg;
		if (curfunc == SIG_DFL) goto unreg;

		/* Restore the parent. */
		while(1) {
			ggsighandler *ret;
			ret = signal(signum, curfunc);
			if (ret != SIG_ERR) break;
			sleep(1);
		}
		return;

	unreg:
		/* We only want to run once, so stay unregistered */
		siglist[sli].oldhandler = SIG_ERR;
		return;
	}

	/* We have an overloaded handler to run. */
	if (curfunc == SIG_DFL) goto noparent;
	if (curfunc == sighandler) goto noparent;

	/*
	 * We install the parent temporarily in hopes that the
	 * handler we call will take the hint and not install
	 * itself.  This could cause reentry (except on BSD-style
	 * systems) but it is the best we can do.
	 *
	 * Note sleep() is specified as async-signal-safe, which
	 * is not true of ggUSleep(), so we use it here instead.
	 */
	while (1) {
		ggsighandler *ret;
		ret = signal(signum, curfunc);
		if (ret != SIG_ERR) break;
		sleep(1);
	}
	siglist[sli].oldhandler(signum);

	/* We reinstall the parent here, in case the handler we just
	 * called was buggy.
	 */
	while (1) {
		ggsighandler *ret;
		ret = signal(signum, curfunc);
		if (ret != SIG_ERR) break;
		sleep(1);
	}
	return;

 noparent:
	/* We are not being overloaded, so we put our overloaded
	 * handler back into our slot.  It should put itself in
	 * based on receiving the SIG_DFL.  Even buggy handlers
	 * must do so.
	 */
	siglist[sli].oldhandler(signum);
	siglist[sli].oldhandler = SIG_ERR;
}

static int cleanup_latch = 0;

static void sighandler(int signum)
{
	unsigned int sli;
	ggsighandler *curfunc, *deadfunc;
	funclist *run_cus = NULL;
	int cleanup_latch_check;

	/* We have to put a SIG_DFL back in ASAP in case
	 * the person overloading us (if any) followed our
	 * manpage advise :-)
	 *
	 * Note sleep() is specified as async-signal-safe, which
	 * is not true of ggUSleep(), so we use it here instead.
	 */
	while(1) {
		curfunc = signal(signum, SIG_DFL);
		if (curfunc != SIG_ERR) break;
		sleep(1);
	}

	/* Find the siglist index for this signal. */
	for (sli = 0; sli < SIGLIST_LEN; sli++) {
		if (siglist[sli].sig == signum) goto found;
	}
	/* Uncomment for debug.  Not compiled in normally, because
	 * calling ggDprintf here is technically illegal.
	 *
	 * ggDPrintf(1, "LibGG", "Signal %i caught but not ours!\n", signum);
	 */
	goto restore;
 found:
	if (siglist[sli].oldhandler == SIG_ERR) {
		/* Uncomment for debug.  Not compiled in normally, because
		 * calling ggDPrintf here is technically illegal.
		 *
		 * ggDPrintf(1, "LibGG",
		 *	 "Caught unexpected signal (%i)!\n", signum);
		 */
		goto restore;
	}

	/* Check and see if we should ignore this signal, as per manpage */
	if (siglist[sli].oldhandler == SIG_IGN) goto restore;

	/* OK, now we know the cleanup list should be run, but we
	 * have to check if maybe some other thread (or previous
	 * or even future interrupt in this thread) has (using the word 
	 * "has" loosly :-) also reached this point and is intending to 
	 * run the cleanup handlers.
	 *
	 * First we do a short-circuit test that will work most
	 * of the time, and prevent entry into the below code,
	 * by checking if the cleanup list has been emptied.
	 * Unlike the sigaction()-based code above, we employ
	 * an extra latch helps us in non-threaded environments,
	 * because in this case interruptions must occur at one
	 * point or another and will run all the way through before 
	 * returning control, so either the latch or cleanups/run_cus 
	 * will catch.  See further below for why we do this.
	 */
	cleanup_latch_check = cleanup_latch;
	cleanup_latch = 1;
	if (cleanup_latch_check) goto oload;
	run_cus = cleanups;
	cleanups = NULL;
	cleanups_grabbed = 1;
	if (run_cus == NULL) {
		/* Someone else has grabbed the cleanup list.
		 *
		 * Run any overloaded signal handler and we are done.
		 */
	oload:
		do_oload(signum, sli, curfunc);
		return;
	}
	
	/* It is still possible for someone else to have gotten 
	 * the cleanup list (but only on multiprocessor machines 
	 * when using threads) so now we enter the "black magic" section.
	 *
	 * LibGG uses a special signal (_gg_signum_dead) as an 
	 * async-signal-safe mutex.  The application is not supposed 
	 * to use this signal.  (If the scheduler is signal based, it 
	 * also uses this signal.)
	 *
	 * We can do this because signal() is almost certainly
	 * async-signal-safe.  Thus it must perform what is for our
	 * purposes an atomic swap of a newly installed signal handler
	 * with the old one, minus the fact that SIG_DFL may turn up 
	 * on classic UNIX systems if the scheduler received a signal 
	 * and is at a particularly inopportune place in its code.
	 *
	 * This is a bit trickier than the corresponding 
	 * sigaction()-based case above.  We can still guarantee 
	 * cleanups will only be run once but we have to rely on
	 * the fact that we forbid people to run threads without
	 * a threaded LibGG.  
	 *
	 * When restricted thus, if we have threads, _gg_signum_dead
	 * should not be receiving *any* signals, because the scheduler
	 * will be running off threads, not signals.  If we don't have
	 * threads, then the only way for overlap to occur is
	 * through stacked interrupts, and there is no multiprocessor
	 * write cache issue due to context switch barriers.  So this 
	 * case is filtered out by the double-latch code above.
	 *
	 * Note sleep() is specified as async-signal-safe, which
	 * is not true of ggUSleep(), so we use it here instead.
	 */
	if(_gg_signum_dead == 0) {
		if(ggTryLock(grab_cleanups_cond))
			goto oload;
	}
	else {
		while (1) {
			deadfunc = signal(_gg_signum_dead, _gg_sigfunc_dead);
			if (deadfunc != SIG_ERR) break;
			sleep(1);
		}
		if (deadfunc == _gg_sigfunc_dead) goto oload;
	}

	/* Now we *know* we should run the cleanups, noone else will.
	 * As per manpage, overloaded callbacks are run before we run 
	 * the cleanups.
	 */
	do_oload(signum, sli, curfunc);
	do_cleanup(run_cus);

	_exit(-1);

 restore:
	/* We are not overloading another handler */
	if (curfunc == sighandler || curfunc == SIG_DFL) {
		/* We only want to run once, so stay unregistered */
		siglist[sli].oldhandler = SIG_ERR;
		return;
	}

	/* Restore the parent. */
	while(1) {
		ggsighandler *ret;
		ret = signal(signum, curfunc);
		if (ret != SIG_ERR) break;
		sleep(1);
	}
	return;
}

/* Test for cleanup start from normal (ggExit) context.  See above comments. */
static funclist *grab_cleanup(void) {
	funclist *run_cus = NULL;  
	ggsighandler *deadfunc;
	int cleanup_latch_check;	

	cleanup_latch_check = cleanup_latch;
	cleanup_latch = 1;
	if (cleanup_latch_check) return run_cus;
	run_cus = cleanups;
	cleanups = NULL;
	if (run_cus == NULL) return run_cus;
	if(_gg_signum_dead == 0) {
		if(ggTryLock(grab_cleanups_cond))
			return NULL;
	}
	else {
		while (1) {
			deadfunc = signal(_gg_signum_dead, _gg_sigfunc_dead);
			if (deadfunc != SIG_ERR) break;
			ggUSleep(10000);
		}
		if (deadfunc == _gg_sigfunc_dead) return NULL;
	}
	return run_cus;
}

#endif /* HAVE_SIGACTION */

#else /* HAVE_SIGNAL || HAVE_SIGACTION */

#define register_sighandler() /* empty */
#define unregister_sighandler() /* empty */
#define grab_cleanup()	cleanups

#endif /* HAVE_SIGNAL || HAVE_SIGACTION */			

/* Cleanup function for atexit, for fallback mode.
 * (Global lock and force_exit really do not matter at this point.)
 */
static void do_graceful_cleanup(void) { 
	do_cleanup(cleanups); 
	free_cleanups(); 
}

int ggRegisterCleanup(ggcleanup_func *func, void *arg)
{
	int ret = 0;
	funclist *newlist;

	ggLock(_gg_global_mutex);
	ret = _gg_register_os_cleanup();
	if (ret) goto out;
	register_sighandler();
	if (!nofallback) {
		if (atexit(do_graceful_cleanup) != 0) {
			ret = GGI_EUNKNOWN;
			goto out;
		}
		nofallback = 1;
	}
	if ((newlist = malloc(sizeof(funclist))) == NULL) {
		ret = GGI_ENOMEM;
	out:
		ggUnlock(_gg_global_mutex);
		return ret;
	}
	newlist->func = func;
	newlist->arg = arg;
	
	/* Insert first in list */
	newlist->next = cleanups;
	cleanups = newlist; /* TODO: possible corruption due to split write */
	ggUnlock(_gg_global_mutex); /* memory barrier */
	if (cleanups_grabbed) {
		/* It is possible that the grabbed cleanups included ours,
		 * so don't dealloc.  Inconsequential because we are crashing. 
		 */
		cleanups = NULL;
		ret = GGI_EBUSY;
	}
	return ret;
}


int ggUnregisterCleanup(ggcleanup_func *func, void *arg)
{
	funclist *curr = cleanups, *prev = NULL;
	void *barrier; /* Used only as a memory write barrier */

	ggLock(_gg_global_mutex);
	while (curr != NULL) {
		if (curr->func == func && curr->arg == arg) goto remove;
		prev = curr;
		curr = curr->next;
	}
	ggUnlock(_gg_global_mutex);
	if (cleanups_grabbed) return GGI_EBUSY;
	return GGI_ENOTALLOC;

 remove:
	barrier = ggLockCreate();
	if (barrier == NULL) return GGI_ENOMEM;
	ggLock(barrier);
	/* When cleanups_grabbed tests postive, we are crashing so no
	 * need to worry about memory leaks.
	 */
	if (cleanups_grabbed) return GGI_EBUSY;
	if (curr == cleanups) {
		ggUnlock(barrier);
		if (cleanups_grabbed) return GGI_EBUSY;
		cleanups = curr->next;      /* TODO: split-write corruption */
	}
	else {
		ggUnlock(barrier);
		if (cleanups_grabbed) return GGI_EBUSY;
		prev->next = curr->next;    /* TODO: split-write corruption */
	}
	ggLock(barrier);
	if (cleanups_grabbed) return GGI_EBUSY;
	free(curr);
	ggUnlock(barrier);
	if (cleanups_grabbed) return GGI_EBUSY;
	if (cleanups == NULL) {
		unregister_sighandler();
		_gg_unregister_os_cleanup();
	}
	ggLockDestroy(barrier);
	ggUnlock(_gg_global_mutex);
	return 0;
}


void ggCleanupForceExit(void)
{
	force_exit = 1;
}


/* Reset all globals during ggInit */
void _gg_init_cleanups(void) {
	/* ggInit will take care of the _gg_*_dead stuff itself as 
	 * that involves the scheduler.  We just take care of the
	 * statics from this file.
	 */
	cleanups = NULL;
	free_cus = NULL;
	force_exit = 0; /* Silly really, but JIC :-) */
	/* Don't alter nofallback as it persists across deinit/init */
	cleanups_grabbed = 0;
#if defined(HAVE_SIGNAL) || defined(HAVE_SIGACTION)
#ifndef HAVE_SIGACTION
	cleanup_latch = 0;
#endif
#endif	
	if(_gg_signum_dead == 0) {
		if(grab_cleanups_cond != NULL)
			ggLockDestroy(grab_cleanups_cond);
		grab_cleanups_cond = ggLockCreate();
		/* TODO: fail ggInit if this lock is not created */
	}
}


/* Wrapper to call from ggExit for graceful library de-initialization.
 * Note we expect _gg_global_mutex to be obtained by the caller, and
 * we are in normal execution context, not in a signal handler.
 */
int _gg_do_graceful_cleanup(void) {
	funclist *run_cus;

	run_cus = grab_cleanup();
	do_cleanup(run_cus);
	free_cleanups();
	return force_exit;
}
