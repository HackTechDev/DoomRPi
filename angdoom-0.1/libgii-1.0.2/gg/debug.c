/* $Id: debug.c,v 1.5 2005/05/01 00:22:51 aldot Exp $
******************************************************************************

   LibGG - Debug output and other cruft

   Copyright (C) 1997 Todd Fries	[toddf@acm.org]
   
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
#include <ggi/internal/gg.h>

#include <stdio.h>
#include <stdarg.h>


void ggDPrintf(int _sync, const char *subsys, const char *form,...)
{
	va_list args;

	fprintf(stderr, "%s: ", subsys);
	va_start(args, form);
	vfprintf(stderr, form, args);
	va_end(args);
	if (_sync) fflush(stderr);
}


/* This function tries to stop or spin execution of the program 
 * to make a tricky (usually locking related) bug easy to find with 
 * a debugger stack trace.
 */
int _gg_death;
void _gg_death_spiral(void) {

	fprintf(stderr, "Code condemned to the spiral of death.\n");

	/* First we try to segfault.   Assignment to a global avoids 
	 * being optimized out at compile time.
	 */
	_gg_death = *((int *)NULL);
	
	/* If that somehow gets ignored we do a friendly infinite loop. */
	while(1) ggUSleep(100000);
}
