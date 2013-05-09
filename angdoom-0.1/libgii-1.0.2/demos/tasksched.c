/* $Id: tasksched.c,v 1.10 2005/06/09 05:23:51 cegger Exp $
******************************************************************************

   tasksched - tests the LibGG task scheduler

   Copyright (C) 2002 Christoph Egger	[Christoph_Egger@t-online.de]

   This software is placed in the public domain and can be used freely
   for any purpose. It comes without any kind of warranty, either
   expressed or implied, including, but not limited to the implied
   warranties of merchantability or fitness for a particular purpose.
   Use it at your own risk. the author is not responsible for any damage
   or consequences raised by use or inability to use this program.

******************************************************************************
*/

#include "config.h"
#include <ggi/gg.h>
#include <stdio.h>
#include <string.h>

#define HAVE_NON_GG_SLEEP

/* #define HAVE_NON_GG_SLEEP to let the program run a bit after exiting
 * LibGG, e.g. to verify the scheduler shutdown without leaving threads
 * running.  Of course that will only work if the related code works on 
 * your OS.
 */

#ifdef HAVE_NON_GG_SLEEP
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#ifdef GG_USLEEP_USE_W32SLEEP
/* sleep does not exist on win32, use Sleep */
#include <windows.h>
#define sleep(x) Sleep(1000*(x))
#endif /* GG_USLEEP_USE_W32SLEEP */

static struct gg_task Hlo, el, space, World, foobar, normal;
static const char *tstr[4] = { "Hlo", "el", " ", "World\n"};

static int str_task(struct gg_task *task)
{
	const char *str;
	int n;
	
	n = task->ncalls + 1;
	if (!n) n = 1;
	str = (const char *)task->hook;
	if (n > (int)(strlen(str))) return 0;
	fprintf(stderr, "%1.1s", str + strlen(str) - n);
	return 0;
}

static int chr_task(struct gg_task *task)
{
	const char *str = (const char *)task->hook;
	fprintf(stderr, "%1.1s", str);
	return 0;
}

int main(void)
{
	int i, j;

	if (ggInit()) {
		fprintf(stderr, "Failure initializing LibGG.\n");
		exit(-1);
	}

	i = ggTimeBase();
	if ((i > 1000000) || (i < 1))
		fprintf(stderr,
			"Bad scheduler timebase retrieved (%i)\n", i);
	if (i < 10000)
		fprintf(stderr, 
			"Test may not work well with short (%i) timebases\n",
			i);

	fprintf(stderr, "Should print \"Hello World\" on the next line ");
	fprintf(stderr, "(be patient afterwards.)\n");

	memset(&Hlo, 0, sizeof(struct gg_task));
	memset(&el, 0, sizeof(struct gg_task));
	memset(&space, 0, sizeof(struct gg_task));
	memset(&World, 0, sizeof(struct gg_task));
	memset(&foobar, 0, sizeof(struct gg_task));
	memset(&normal, 0, sizeof(struct gg_task));

	Hlo.pticks = 2;
	Hlo.ncalls = 3;
	Hlo.hook = (void *)tstr[0];
	Hlo.cb = str_task;

	el.pticks = 1;
	el.ncalls = 3;
	el.hook = (void *)tstr[1];
	el.cb = str_task;

	space.pticks = 7;
	space.ncalls = 1;
	space.hook = (void *)tstr[2];
	space.cb = str_task;

	World.pticks = 1;
	World.ncalls = 13;
	World.hook = (void *)tstr[3];
	World.cb = str_task;

	ggAddTask(&Hlo);
	ggAddTask(&el);
	ggAddTask(&space);
	ggAddTask(&World);
	
	ggUSlumber(17000000);
	fprintf(stderr, "Dots should follow, with no exclamation points.\n");

	foobar.pticks = 100;
	foobar.ncalls = 0;
	foobar.hook = &"!";
	foobar.cb = chr_task;	

	normal.pticks = 1;
	normal.ncalls = 0;
	normal.hook = &".";
	normal.cb = chr_task;

	/* Make sure adding same task twice fails */
	if (ggAddTask(&foobar)) fprintf(stderr, "add failed!");
	if (!ggAddTask(&foobar)) fprintf(stderr, "add should have failed!");;
	ggUSlumber(100000);
	ggDelTask(&foobar);

	/* Stress test Add/Del */

	ggAddTask(&normal);
	for (i = 0; i < 100; i++) {
		for (j = 0; j < 10000; j++) {
			ggAddTask(&foobar);
			ggDelTask(&foobar);
		}
		ggUSlumber(i * 4389);
	}

	fprintf(stderr, "\nKill now to test ungraceful death, or wait\n");
	ggUSlumber(15000000);

	/* TODO: test manipulation of task period/ncalls from within handler */
	/* TODO: test task addition from within handler */
	/* TODO: test task deletion from within handler */

	if (ggExit()) {
	  fprintf(stderr, "Failure deinitializing LibGG.\n");
	  exit(-1);
	}
	fprintf(stderr, "LibGG exited.\n");

#ifdef HAVE_NON_GG_SLEEP
	sleep(10);
#endif
	return 0;
}	/* main */
