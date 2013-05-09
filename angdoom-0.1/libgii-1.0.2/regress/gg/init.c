/* $Id: init.c,v 1.2 2005/08/16 22:47:06 cegger Exp $
******************************************************************************

   init - Regression tests for init/exit handling

   Copyright (C) 2005 by Christoph Egger

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

#include "../testsuite.inc.c"


static void testcase1(const char *desc)
{
	int err;
	
	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;


	err = ggInit();
	if (err != GGI_OK) {
		printfailure("expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}

	err = ggInit();
	if (err != GGI_OK) {
		printfailure("expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}


	printsuccess();
	return;
}


static void testcase2(const char *desc)
{
	int err;
	
	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;


	err = ggExit();
	if (err != 1) {
		printfailure("expected return value: 1\n"
			"actual return value: %i\n", err);
		return;
	}

	err = ggExit();
	if (err != GGI_OK) {
		printfailure("expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}


	printsuccess();
	return;
}


int main(int argc, char * const argv[])
{
	parseopts(argc, argv);

	printdesc("Regression testsuite for libgg init/exit handling\n\n");

	testcase1("Check that ggInit() behaves as documented.");
	testcase2("Check that ggExit() behaves as documented.");


	printsummary();

	return 0;
}
