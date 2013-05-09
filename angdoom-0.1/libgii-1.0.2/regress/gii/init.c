/* $Id: init.c,v 1.4 2005/08/28 11:51:47 cegger Exp $
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
#include <ggi/gii.h>

#include "../testsuite.inc.c"


static void testcase1(const char *desc)
{
	int err;
	
	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;


	err = giiInit();
	if (err != GGI_OK) {
		printfailure("expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}

	err = giiInit();
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


	err = giiExit();
	if (err != 1) {
		printfailure("expected return value: 1\n"
			"actual return value: %i\n", err);
		return;
	}

	err = giiExit();
	if (err != GGI_OK) {
		printfailure("expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}


	printsuccess();
	return;
}


static void testcase3(const char *desc)
{
	int err;
	gii_input_t inp;
	
	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;


	err = giiInit();
	if (err != GGI_OK) {
		printfailure("giiInit: expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}

	inp = giiOpen(NULL);
	if (inp == NULL) {
		printfailure("giiOpen: Couldn\'t open default input.\n");
		err = giiExit();
		if (err != GGI_OK) {
			printfailure("giiExit: expected return value: 0\n"
				"actual return value: %i\n", err);
		}
		return;		
	}

	err = giiClose(inp);
	if (err != GGI_OK) {
		printfailure("giiClose: expected return value: %i\n"
			"actual return value: %i\n",
			GGI_OK, err);
		return;
	}

	err = giiExit();
	if (err != GGI_OK) {
		printfailure("giiExit: expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}


	printsuccess();
	return;
}


static void testcase4(const char *desc)
{
	int err;
	
	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;


	err = giiClose(NULL);
	if (err != GGI_EARGINVAL) {
		printfailure("giiClose: expected return value: %i\n"
			"actual return value: %i\n",
			GGI_EARGINVAL, err);
		return;
	}

	err = giiInit();
	if (err != GGI_OK) {
		printfailure("giiInit: expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}

	err = giiClose(NULL);
	if (err != GGI_EARGINVAL) {
		printfailure("giiClose: expected return value: %i\n"
			"actual return value: %i\n",
			GGI_EARGINVAL, err);
		return;
	}

	err = giiExit();
	if (err != GGI_OK) {
		printfailure("giiExit: expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}


	printsuccess();
	return;
}



int main(int argc, char * const argv[])
{
	parseopts(argc, argv);

	printdesc("Regression testsuite for libgii init/exit handling\n\n");

	testcase1("Check that giiInit() behaves as documented.");
	testcase2("Check that giiExit() behaves as documented.");
	testcase3("Check giiOpen()/giiClose() to open/close a default input.");
	testcase4("Check giiClose() works correct with an invalid argument.");

	printsummary();

	return 0;
}
