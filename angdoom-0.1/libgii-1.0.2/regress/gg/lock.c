/* $Id: lock.c,v 1.6 2004/08/08 20:50:58 cegger Exp $
******************************************************************************

   lock - Regression tests for ggLock & friends

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

#include <ggi/gg.h>

#include "../testsuite.inc.c"


static void testcase1(const char *desc)
{
	void *lock;
	int result;
	
	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	lock = ggLockCreate();
	if(lock == NULL) {
		printfailure("Failed to create lock.");
		return;
	}

	ggLock(lock);

	result = ggTryLock(lock);
	if(result == 0) {
		printfailure("Lock should have been locked already.");
		return;
	}
	printassert(result == GGI_EBUSY, "Lock not busy. result: %i\n", result);

	ggUnlock(lock);

	result = ggTryLock(lock);
	printassert(result != GGI_EBUSY, "Lock shouldn't be busy. result: %i\n", result);
	if (result != 0) {
		printfailure("Locking failed.\n"
			"expected result: 0\n"
			"actual result: %i\n", result);
		return;
	}

	ggUnlock(lock);
	result = ggLockDestroy(lock);
	if(result != 0) {
		printfailure("Failed to destroy lock.");
		return;
	}

	printsuccess();
	return;
}


int main(int argc, char * const argv[])
{
	parseopts(argc, argv);

	printdesc("Regression testsuite for ggLock(3) & friends.\n\n");

	testcase1("Checks behaviour of locking functions, whether they match documentation.");

	printsummary();

	return 0;
}
