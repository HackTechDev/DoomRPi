/* $Id: evtimestamp.c,v 1.4 2004/08/08 20:50:58 cegger Exp $
******************************************************************************

   evtimestamp - Regression tests for event timestamps

   Copyright (C) 2004      Peter Ekberg		[peda@lysator.liu.se]

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
#include <ggi/internal/gii.h>

#include "../testsuite.inc.c"


static void testcase1(const char *desc)
{
	gii_event ev, old;
	int i, j;

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	_giiEventBlank(&ev, sizeof(gii_any_event));

	for(i = 0; i < 30; ++i) {
		for(j = 0; j < 10000; ++j) {
			old = ev;
			_giiEventBlank(&ev, sizeof(gii_any_event));
			if(ev.any.time.tv_sec > old.any.time.tv_sec)
				continue;
			if(ev.any.time.tv_sec == old.any.time.tv_sec
			   && ev.any.time.tv_usec > old.any.time.tv_usec)
			   	continue;

			printfailure("Event timestamps are not "
				"strictly monotonic.");
			return;
		}
		if(i < 99)
			ggUSleep(100000);
	}

	printsuccess();
	return;
}


int main(int argc, char * const argv[])
{
	parseopts(argc, argv);

	printdesc("Regression testsuite for event timestamps.\n\n");

	testcase1("Checks if event timestamps are strictly monotonic.");

	printsummary();

	return 0;
}
