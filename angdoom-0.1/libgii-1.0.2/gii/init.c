/* $Id$
******************************************************************************

   LibGII initialization.

   Copyright (C) 1997 Jason McMullan	[jmcc@ggi-project.org]
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

#include "config.h"

#include <ggi/gg.h>
#include <ggi/internal/gg_replace.h>
#include <ggi/internal/gii.h>
#include <ggi/internal/gii_debug.h>

#include "plat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>



/* Exported variables */
uint32_t              _giiDebug  = 0;

int                   _gii_threadsafe  = 0;
void                 *_gii_global_lock = NULL;

/* Global variables */
gg_config            _giiconfhandle	= NULL;
void                 *_gii_safe_lock	= NULL;
void                 *_gii_event_lock	= NULL;

/* Static variables */
static int 	      _giiLibIsUp =0 ;
static char           giiconfstub[512] = GIICONFDIR;
static char          *giiconfdir = giiconfstub + GIITAGLEN;

void _giiInitBuiltins(void);
void _giiExitBuiltins(void);

/* 
 * Returns the directory where global config files are kept
 */

const char *giiGetConfDir(void)
{
#if defined(__WIN32__) && !defined(__CYGWIN__)
	/* On Win32 we allow overriding of the compiled in path. */
	const char *envdir = getenv("GGI_CONFDIR");
	if (envdir) return envdir;
#endif
	return giiconfdir;
}


/*
 * Turn on thread safe operation
 */

int giiMTInit(void)
{
	_gii_threadsafe = 1;
	
	return 0;
}

/*
 * Initialize the structures for the library
 */

int giiInit(void)
{
	int err;
	const char *str;
	char *conffile;
	
	if (_giiLibIsUp>0) {
		/* Initialize only at first call. */
		_giiLibIsUp++;
		return 0;
	}

	err = ggInit();
	if (err != GGI_OK) {
		fprintf(stderr, "LibGII: unable to initialize LibGG\n");
		return err;
	}

	err = GGI_ENOMEM;
	if ((_gii_event_lock = ggLockCreate()) == NULL) {
		fprintf(stderr,"LibGII: unable to initialize event mutex.\n");
		goto out_ggexit;
	}

	if ((_gii_safe_lock = ggLockCreate()) == NULL) {
		fprintf(stderr,"LibGII: unable to initialize safe mutex.\n");
		goto out_destroy_event;
	}

	if ((_gii_global_lock = ggLockCreate()) == NULL) {
		fprintf(stderr,"LibGII: unable to initialize global mutex.\n");
		goto out_destroy_safe;
	}

	conffile = malloc(strlen(giiGetConfDir()) + 1
			  + strlen(GIICONFFILE) +1);
	if (conffile == NULL) {
		fprintf(stderr,"LibGII: unable to allocate memory for config filename.\n");
		goto out_destroy_global;
	}
	snprintf(conffile, strlen(giiGetConfDir()) + strlen(GIICONFFILE) + 2,
		"%s%c%s", giiGetConfDir(), CHAR_DIRDELIM, GIICONFFILE);
	if(ggLoadConfig(conffile, &_giiconfhandle)) {
		fprintf(stderr, "LibGII: fatal error - could not load %s\n",
			conffile);
		free(conffile);
		goto out_destroy_global;
	}
	free(conffile);
	
	str = getenv("GII_DEBUGSYNC");
	if (str != NULL) {
		_giiDebug |= DEBUG_SYNC;
	}
	
	str = getenv("GII_DEBUG");
	if (str != NULL) {
		_giiDebug |= atoi(str) & DEBUG_ALL;
		DPRINT_CORE("%s Debugging=%d\n",
			    DEBUG_ISSYNC ? "sync" : "async",
			    _giiDebug);
	}

	_giiInitBuiltins();

	_giiLibIsUp++;
	
	return 0;

  out_destroy_global:
	ggLockDestroy(_gii_global_lock);
  out_destroy_safe:
	ggLockDestroy(_gii_safe_lock);
  out_destroy_event:
	ggLockDestroy(_gii_event_lock);
  out_ggexit:
	ggExit();

	return err;
}

int giiExit(void)
{
	DPRINT_CORE("giiExit() called\n");
	if (!_giiLibIsUp)
		return GGI_ENOTALLOC;
	
	if (_giiLibIsUp > 1) {
		_giiLibIsUp--;
		return _giiLibIsUp;
	}
	
	DPRINT_CORE("giiExit: really destroying.\n");

	_giiExitBuiltins();

	ggFreeConfig(_giiconfhandle);
	ggLockDestroy(_gii_global_lock);
	ggLockDestroy(_gii_safe_lock);
	ggLockDestroy(_gii_event_lock);

	/* Set them back to initialization value.
	 * Otherwise this leads to a memory corruption bug,
	 * when gii is initialized again in the same application.
	 */
	_giiconfhandle = NULL;
	_gii_global_lock = NULL;
	_gii_safe_lock = NULL;
	_gii_event_lock = NULL;

	ggExit();
	_giiLibIsUp=0;
	
	DPRINT_CORE("giiExit: done!\n");
	return 0;
}
