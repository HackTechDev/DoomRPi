/* $Id: misc.c,v 1.21 2005/08/26 14:40:05 soyt Exp $
******************************************************************************

   LibGG - Misc utility functions

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
#include <limits.h>

#include <ggi/gg.h>
#include <ggi/internal/gg_debug.h>

#if defined(__WIN32__) && !defined(__CYGWIN__)
#include "misc_win32.c"
#endif /* __WIN32__ && !__CYGWIN__ */

/* FIXME - make unknown systems do the right thing here */

#define APPENDNAME	"/.ggi"
#define APPENDLEN	5

const char *ggGetUserDir(void)
{
	static char curpath[PATH_MAX+1];
	char *ptr;
	size_t len;
	int need_ptr2free = 0;

	ggLock(_gg_global_mutex);

	if (curpath[0] != '\0') {
		/* The user dir has already been calculated */
		ggUnlock(_gg_global_mutex);
		return curpath;
	}

	ptr = getenv("HOME");

	if (ptr == NULL) {
		/* hmm... $HOME is not there... */

#ifndef HAVE_TMP_PATH

#if defined(macintosh) || defined(__darwin__)
#define HAVE_TMP_PATH
		/* Darwin 6.x */
		const char *uid = getenv("UID");
		asprintf(&ptr, "/tmp/%s/Temporary Items", uid);
		need_ptr2free = 1;

#endif	/* MAC OS / DARWIN */

#endif


#ifndef HAVE_TMP_PATH

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)
#define HAVE_TMP_PATH
		/* this hopefully catches all unix environments */
		ptr = "/tmp";

#endif	/* Unix */

#endif


#ifndef HAVE_TMP_PATH

#if defined(__WIN32__) && !defined(__CYGWIN__)
#define HAVE_TMP_PATH
		/* Should catch all win32 platforms */
		if (ptr == NULL)
			ptr = get_personal_folder_path();
		if (ptr == NULL)
			ptr = getenv("TMP");
		if (ptr == NULL)
			ptr = getenv("TEMP");

#endif	/* __WIN32__ && !__CYGWIN__ */

#endif


#ifndef HAVE_TMP_PATH
		/* Catch all other platforms */
#error You need to set the default temporary path for this system
#endif
	}	/* if */

	len = strlen(ptr);
	if (len + APPENDLEN > PATH_MAX) return NULL;

	ggstrlcpy(curpath, ptr, sizeof(curpath));
	ggstrlcpy(curpath+len, APPENDNAME, sizeof(curpath) - len);
	ggUnlock(_gg_global_mutex);

	if (need_ptr2free) {
		free(ptr);
	}

	return curpath;
}



struct gg_observer * ggAddObserver(struct gg_publisher *publisher,
	  ggfunc_observer_update *cb,
	  void *arg)
{
	struct gg_observer * observer;
	
	DPRINT_MISC("ggAddObserver(publisher=%p, update=%p, arg=%p)\n", publisher, cb, arg);
	
	if((observer = calloc(1, sizeof(*observer))) == NULL) {
		DPRINT_MISC("! can not alloc mem for publisher.\n");
		return NULL;
	}
	
	observer->arg = arg;
	observer->update = cb;
	GG_LIST_INSERT_HEAD(&(publisher->observers), observer, _others);
	
	return observer;
}


void ggDelObserver(struct gg_observer *observer)
{
	
	DPRINT_MISC("ggDelObserver(observer=%p)\n", observer);
	
	GG_LIST_REMOVE(observer, _others);
	free(observer);
}


void ggNotifyObservers(struct gg_publisher *publisher, int flag, void *data)
{
	struct gg_observer *curr, *next;
	
	DPRINT_MISC("ggNotifyObservers(publisher=%p, flag=0x%x, data=%p)\n", flag, publisher, data);
	
	for (curr = GG_LIST_FIRST(&(publisher->observers));
	     curr != NULL;
	     curr = next) {
		next = GG_LIST_NEXT(curr, _others);
		if(curr->update(curr->arg, flag, data)) {
			GG_LIST_REMOVE(curr, _others);
			free(curr);
		}
	}
}


void ggClearPublisher(struct gg_publisher *publisher)
{
  	struct gg_observer *curr, *next;

	DPRINT_MISC("ggClearPublisher(publisher=%p)\n", publisher);

	for (curr = GG_LIST_FIRST(&(publisher->observers));
	     curr != NULL;
	     curr = next) {
		next = GG_LIST_NEXT(curr, _others);
		DPRINT_API("! observer update=%p, arg=%p still registered\n",curr->update,curr->arg);
		GG_LIST_REMOVE(curr, _others);
		free(curr);
	}
}
