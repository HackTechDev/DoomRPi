/* $Id: parsetarget.c,v 1.9 2005/06/09 05:15:40 cegger Exp $
******************************************************************************

   parsetarget - Regression test for ggParseTarget

   Copyright (C) 2004 Christoph Egger

   This software is placed in the public domain and can be used freely
   for any purpose. It comes without any kind of warranty, either
   expressed or implied, including, but not limited to the implied
   warranties of merchantability or fitness for a particular purpose.
   Use it at your own risk. the author is not responsible for any damage
   or consequences raised by use or inability to use this program.

******************************************************************************
*/

#include "config.h"

#include <ggi/internal/gg.h>
#include <string.h>

#include "../testsuite.inc.c"


static void testcase1(const char *desc)
{
	char buffer[1024];
	const char *str = " \t";
	const char *retstr;
	const char *expected = "";

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	/* Testcase: Passing a string with whitespaces only
	 * expects to return \0
	 */

	retstr = ggParseTarget(str, buffer, sizeof(buffer));

	if (strncmp(expected, retstr, 1) != 0) {
		printfailure("expected return value: \"%s\"\n"
			"actual return value: \"%s\"\n",
			expected, retstr);
		return;
	}

	printsuccess();
	return;
}


static void testcase2(const char *desc)
{
	char buffer[3];
	const char *str = " \tdisplay-multi:(display-x:-noshm):(display-aa)";
	const char *retstr;
	const char *expected = "d";

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	/* Testcase: Passing a string with an too short
	 * target buffer expects to fill out the buffer
	 * as much as possible, but not complete.
	 */

	retstr = ggParseTarget(str, buffer, sizeof(buffer));

	if (retstr != NULL) {
		printfailure("expected return value: \"%s\"\n"
			"actual return value: \"%s\"\n",
			"NULL", (retstr) ? retstr : "NULL");
		return;
	}

	if (strncmp(expected, buffer, 3) != 0) {
		printfailure("expected target value: \"%s\"\n"
			"actual target value: \"%s\"\n",
			expected, buffer);
		return;
	}

	printsuccess();
	return;
}



static void testcase3(const char *desc)
{
	char buffer[1024];
	const char *str = " \tdisplay-multi:(display-x:-noshm):(display-aa)";
	const char *retstr;
	const char *expected = "display-x:-noshm";

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	/* Testcase: Passing a string with valid arguments
	 * expects to return ":"
	 */

	retstr = ggParseTarget(&str[16], buffer, sizeof(buffer));

	if (retstr == NULL || strncmp(":", retstr, 1) != 0) {
		printfailure("expected return value: \"%s\"\n"
			"actual return value: \"%s\"\n",
			":", (retstr) ? retstr : "NULL");
		return;
	}

	if (strncmp(expected, buffer, 1024) != 0) {
		printfailure("expected target value: \"%s\"\n"
			"actual target value: \"%s\"\n",
			expected, buffer);
		return;
	}

	printsuccess();
	return;
}



static void testcase4(const char *desc)
{
	char buffer[1024];
	const char *str = " \tdisplay-multi:(display-x:-noshm)):(display-aa)";
	const char *retstr;
	const char *expected = "display-multi:(display-x:-noshm)";

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	/* Testcase: Passing a string with syntax errors
	 */

	retstr = ggParseTarget(str, buffer, sizeof(buffer));

	if (retstr != NULL) {
		printfailure("expected return value: \"%s\"\n"
			"actual return value: \"%s\"\n",
			"NULL", (retstr) ? retstr : "NULL");
		return;
	}

	if (strncmp(expected, buffer, 1024) != 0) {
		printfailure("expected target value: \"%s\"\n"
			"actual target value: \"%s\"\n",
			expected, buffer);
		return;
	}

	printsuccess();
	return;
}


static void testcase5(const char *desc)
{
	char buffer[1024];
	const char *str = " \tdisplay-multi:((display-x:-noshm):()display-aa)";
	const char *retstr;
	const char *expected = "display-multi:((display-x:-noshm):()display-aa)";

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	/* Testcase: Passing a string with syntax errors
	 */

	retstr = ggParseTarget(str, buffer, sizeof(buffer));

	if (retstr == NULL || strncmp("", retstr, 1) != 0) {
		printfailure("expected return value: \"%s\"\n"
			"actual return value: \"%s\"\n",
			"", (retstr) ? retstr : "NULL");
		return;
	}

	if (strncmp(expected, buffer, 1024) != 0) {
		printfailure("expected target value: \"%s\"\n"
			"actual target value: \"%s\"\n",
			expected, buffer);
		return;
	}

	printsuccess();
	return;
}




int main(int argc, char *argv[])
{
	parseopts(argc, argv);

	printdesc("Regression testsuite for libgg's internal ggParseTarget() function.\n\n");

	testcase1("Passes a string with whitespaces only. Expected to return \\0.");
	testcase2("Passes a string with an too short target buffer. "
		"This expects to fill up the buffer as much as possible, "
		"but not completely - buffer overflow protection.");
	testcase3("Passes a string with valid arguments.");
	testcase4("Passes a string with syntax error: "
		"A closing bracket doesn\'t match an opening bracket.");
	testcase5("Passes a string with syntax error: "
		"Number of opening and closing brackets matches, but wrong ordered.");

	printsummary();

	return 0;
}

