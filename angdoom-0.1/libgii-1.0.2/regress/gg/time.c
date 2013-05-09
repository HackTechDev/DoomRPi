/* $Id: time.c,v 1.12 2005/07/29 16:40:59 soyt Exp $
******************************************************************************

   time - Regression test for ggCurTime & friends

   Copyright (C) 2004 Peter Ekberg

   This software is placed in the public domain and can be used freely
   for any purpose. It comes without any kind of warranty, either
   expressed or implied, including, but not limited to the implied
   warranties of merchantability or fitness for a particular purpose.
   Use it at your own risk. the author is not responsible for any damage
   or consequences raised by use or inability to use this program.

******************************************************************************
*/

#include "config.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <ggi/internal/gg.h>

#include "../testsuite.inc.c"


static void testcase1(const char *desc)
{
	struct timeval prev, now;
	int i;

	uint32_t expected_pause = 1000000;
	uint32_t actual_pause;
	uint32_t step_estimate;

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	/* Testcase: Check if time goes forward for over one minute.
	 */

	/* Spin until ggCurTime changes to estimate the clock step, if any.
	 */
	ggCurTime(&now);
	do  {
		prev = now;
		ggCurTime(&now);
	} while(now.tv_sec == prev.tv_sec && now.tv_usec == prev.tv_usec);
	step_estimate = (now.tv_sec - prev.tv_sec) * 1000000 + (now.tv_usec - prev.tv_usec);

	for(i = 0; i < 65; ++i) {
		ggUSlumber(expected_pause);
		ggCurTime(&now);

		if (now.tv_sec < prev.tv_sec) {
			printfailure("Time went backwards.\n"
				"ggCurTime() probably incorrectly implemented.\n");
			return;
		}

		actual_pause = (now.tv_sec - prev.tv_sec) * 1000000 + (now.tv_usec - prev.tv_usec);
		if (actual_pause < expected_pause - step_estimate) {
			printfailure("expected pause: %i microseconds\n"
					"actual pause: %i microseconds\n"
					"step estimate: %i microseconds\n"
					"ggUSlumber() probably incorrectly implemented\n",
					expected_pause, actual_pause, step_estimate);
			return;
		}

		prev = now;
	}

	printsuccess();
	return;
}


static void testcase2(const char *desc)
{
	struct timeval tv1, tv3;
	time_t t2, t4;

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	/* Testcase: Check if time() and ggCurTime() agrees.
	 */

	ggCurTime(&tv1);
	t2 = time(NULL);
	ggCurTime(&tv3);
	t4 = time(NULL);

	if(tv1.tv_sec == t2)
		goto success;
	if(tv1.tv_sec > t2)
		goto failure;
	/* Double check if the second happened to change
	 * between the calls.
	 */
	if(t2 == tv3.tv_sec)
		goto success;
	if(t2 > tv3.tv_sec)
		goto failure;
	/* Tripple check if the second happened to change
	 * between the calls twice in a row (loaded machine?).
	 */
	if(tv3.tv_sec == t4)
		goto success;
	if(tv3.tv_sec > t4)
		goto failure;

	/* Not very likely to get here, but at least time did not go
	 * backwards, so succeed.
	 */
success:
	printsuccess();
	return;
	
failure:
	printfailure("time() and ggCurTime() disagrees.\n"
		"ggCurTime() probably incorrectly implemented.\n");
}


int main(int argc, char * const argv[])
{
	parseopts(argc, argv);

	printdesc("Regression testsuite for ggCurTime(3) & friends.\n\n");

	testcase1("Check if time goes forward for over one minute.");
	testcase2("Check if time() and ggCurTime() agrees.");

	printsummary();

	return 0;
}
