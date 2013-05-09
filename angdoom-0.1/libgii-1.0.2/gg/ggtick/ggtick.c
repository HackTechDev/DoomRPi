/* $Id: ggtick.c,v 1.6 2005/01/14 09:32:08 pekberg Exp $
******************************************************************************

   Code for slave process for periodic signal-based task scheduler.

   Copyright (C) 1998  Steve Cheng     [steve@ggi-project.org]
   Copyright (C) 1998 Marcus Sundberg  [marcus@ggi-project.org]
   Copyright (C) 2004  Brian S. Julin  [skids@users.sourceforge.net]

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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ggi/internal/gg_replace.h>

#include "../time.c"


int main (int argc, char **argv) {
	unsigned long parm;
	int usecs, sig;
	pid_t parent;

	errno = 0;

	if (argc != 3) goto usage;
	parm = strtoul(argv[1], NULL, 10);

	if (errno) goto usage;
	if (parm < 10) goto usage;
	if (parm > 1000000) goto usage;
	
	usecs = (int)parm;

	parm = strtoul(argv[2], NULL, 10);
	if (errno) goto usage;
	if (parm < 0) goto usage;

	sig = (int)parm;

	parent = getppid();

	/* Note we are not linking to libgg for ggUSleep, rather
	 * we are compiling it in, so we do not ggInit().
	 */
	while (1) {
		ggUSlumber(usecs);
		if (kill(parent, sig)) {
			fprintf(stderr, "Failed to kill(%i,%i) (error %i).\n", 
				(int)parent, sig, errno);
		}
	}

 usage:
	fprintf(stderr, "usage: ggtick usecs signal\n");
	fprintf(stderr, "usecs -- microseconds between signals\n");
	fprintf(stderr, "         (range 10 to 1000000)\n");
	fprintf(stderr, "signal -- integer value of signal to send\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "WARNING: this program kill()s its parent!\n");
	exit (-1);
}
