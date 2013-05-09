/* $Id: init.c,v 1.28 2005/08/18 10:44:56 cegger Exp $
******************************************************************************

   LibGG - Init/Exit functions

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
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <ggi/internal/gg.h>
#include <ggi/internal/gg_debug.h>

uint32_t         _ggDebug   = 0;

void		*_gg_global_mutex = NULL;
static int	 _ggLibIsUp = 0;

/* Globals used for scheduler and/or async-signal-safe mutex trick */
#if defined(HAVE_SIGNAL) || defined(HAVE_SIGACTION)

int		_gg_signum_dead;                    /* Reserved signal      */
void _gg_sigfunc_dead(int unused) {
	/* Dummy handler installed when dying */
	return;
}

#endif

gg_option _gg_optlist[] =
	{
		{ "signum", "" },        /* Signal scheduler should use     */
		{ "schedthreads", "" },  /* Number of threads in scheduler  */
		{ "schedhz", "" },       /* scheduler granularity           */
		{ "banswar", "" }        /* report certain SWARs as missing */
	};

/* Option indices are in ggi/internal/gg.h */

#define NUM_OPTS        (sizeof(_gg_optlist)/sizeof(gg_option))

int ggInit(void)
{
	int ret;
	char * str;
	
	/* Initialize only at first call. */
	if (_ggLibIsUp > 0) {
		/* Lock serves as memory write barrier for increment. */
		ggLock(_gg_global_mutex);
		_ggLibIsUp++;
		ggUnlock(_gg_global_mutex);
		return 0;
	}
	
	str = getenv("GG_DEBUGSYNC");
	if (str != NULL) {
		_ggDebug |= DEBUG_SYNC;
	}
	
	str = getenv("GG_DEBUG");
	if (str != NULL) {
		_ggDebug |= atoi(str) & DEBUG_ALL;
		DPRINT_CORE("- %s debugging=%d\n",
			    DEBUG_ISSYNC ? "sync" : "async",
			    _ggDebug);
	}
	
	/* We don't need to lock here because it is illegal to use
	 * LibGG API until the first call to ggInit() returns, and
	 * specifically illegal to call two overlapping ggInit() calls
	 * until first ggInit() call returns.  Would be kinda hard
	 * to lock anyway, considering the lock is not there yet :-).
	 */

	/* Implement GG_OPTS environment variable */
	ggstrlcpy(_gg_optlist[_GG_OPT_SIGNUM].result, "no",
		  GG_MAX_OPTION_RESULT);
	ggstrlcpy(_gg_optlist[_GG_OPT_SCHEDTHREADS].result, "1",
		  GG_MAX_OPTION_RESULT );
	ggstrlcpy(_gg_optlist[_GG_OPT_SCHEDHZ].result, "60",
		  GG_MAX_OPTION_RESULT);
	ggstrlcpy(_gg_optlist[_GG_OPT_BANSWAR].result, "no",
		  GG_MAX_OPTION_RESULT);
	if (getenv("GG_OPTS") != NULL) {
		/* Apply defaults.  Do this every time in case LibGG exited
		 * and setenv() used, then LibGG reinitialized.
		 */
		if (ggParseOptions(getenv("GG_OPTS"),
				   _gg_optlist, NUM_OPTS) == NULL) {
			fprintf(stderr, "LibGG: error in $GG_OPTS\n");
			return GGI_EARGINVAL;
		}
        }

	/* parse flags that are easiest done here due to code layout. */
	if (_gg_optlist[_GG_OPT_BANSWAR].result[0] != 'n') {
		errno = 0;
#if defined(GG_HAVE_INT64) && defined(HAVE_STRTOULL) && !defined(__STRICT_ANSI__)
		swars_enabled = ~strtoull(_gg_optlist[_GG_OPT_BANSWAR].result,
					  NULL, 16);
#else
		swars_enabled = ~strtoul(_gg_optlist[_GG_OPT_BANSWAR].result,
					 NULL, 16);
#endif
		if (errno) {
			fprintf(stderr, "LibGG: bad -banswar\n");
			exit (-1);
		}
	}

#ifdef SIGPROF
#if defined(HAVE_SIGNAL) || defined(HAVE_SIGACTION)
	_gg_signum_dead = SIGPROF;
	if (_gg_optlist[_GG_OPT_SIGNUM].result[0] != 'n') {
		int res;
		errno = 0;
		res = (int)strtoul(_gg_optlist[_GG_OPT_SIGNUM].result,NULL,10);
#ifdef HAVE_SIGACTION
		do {
			struct sigaction sa;
			int res2;

			sa.sa_flags = 0;
			sa.sa_handler = SIG_IGN;
			sigemptyset(&sa.sa_mask);
			res2 = sigaction(res, &sa, NULL);
			if (res2) res = -1;
		} while(0);
#else
		do {
			void (*res2)(int);
			res2 = signal(res, SIG_IGN);
			if (res2 == SIG_ERR) res = -1;
		} while(0);
#endif
		if (errno || res < 0) {
			fprintf(stderr, "LibGG: bad -signum\n");
			exit(-1);
		}
	}
#endif
#else
/* No SIGPROF signal! Ok on Win32 as there is structured exception handling */
#ifndef __WIN32__
#warning The SIGPROF signal is not available, what to do?
#endif
#endif /* SIGPROF */

	ret = _ggInitLocks();
	if (ret != 0) {
		fprintf(stderr, "LibGG: mutex init failed\n");
		return ret;
	}

	if ((_gg_global_mutex = ggLockCreate()) == NULL) {
		_ggExitLocks();
		return GGI_EUNKNOWN;
	}

	/* Lock serves as memory barrier for everything above. */
	ggLock(_gg_global_mutex);
	_gg_init_cleanups();
	_ggTaskInit();
	_ggScopeInit();
	_ggLibIsUp++;
	ggUnlock(_gg_global_mutex);

	return 0;
}

int ggExit(void)
{
	/* We voluntarily test this before using the global lock.
	 * We do not have to because _ggLibIsUp will always be positive
	 * when it is legal API use to call ggExit.  This just reduces
	 * the chances that misuse will cause a disaster.
	 */
	if (!_ggLibIsUp) return GGI_ENOTALLOC;

	/* Lock provides memory barrier. */
	ggLock(_gg_global_mutex);
	_ggLibIsUp--;
	if (_ggLibIsUp > 0) {
		ggUnlock(_gg_global_mutex);
		return _ggLibIsUp;
	}

	_ggScopeExit();

	/* Past this point API says it is illegal to call LibGG API
	 * functions, including ggExit(), so it is not our fault
	 * if things blow up, but we do our best.
	 */
	_ggTaskExit();

	/* If a graceful cleanup is _NOT_ possible, we try to shut down
	 * as nicely as possible. However this is _NOT_ a normal condition,
	 * and we should clearly indicate this to the user.
	 */
	if ( _gg_do_graceful_cleanup() ) {
#ifdef HAVE__EXIT
	        _exit(123);	/* weird exit code to hint the user here */
#elif defined(HAVE_RAISE) && defined(SIGSEGV)
		raise(SIGSEGV);
#elif defined(HAVE_RAISE) && defined(SIGKILL)
		raise(SIGKILL);
#elif defined(HAVE_RAISE)
		raise(9);
#elif defined(HAVE_ABORT)
		abort();
#endif
	}

	ggUnlock(_gg_global_mutex);

	ggLockDestroy(_gg_global_mutex);
	_ggExitLocks();

	/* Reset global variables to initialization value
	 * to prevent a possible memory corruption bugs
	 * when libgg gets re-initialized within one application.
	 */
	_gg_global_mutex = NULL;

	return 0;
}
