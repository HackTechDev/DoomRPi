/* $Id: filter.c,v 1.6 2005/08/01 09:26:29 pekberg Exp $
******************************************************************************

   filter.c - sample code to demonstrate filter usage

   Copyright (C) 1998	Andreas Beck		[becka@ggi-project.org]
   Copyright (C) 1999	Marcus Sundberg		[marcus@ggi-project.org]

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

/* Include the LibGII declarations.
 */
#include <ggi/gii.h>

/* Include the necessary headers used for e.g. error-reporting.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void test_showevent(gii_event *ev)
{
	unsigned int y;
	
	printf("o:%8x ", ev->any.origin);
	switch (ev->any.type) {
		case evKeyPress:
			printf("KeyPress    ");goto showkey;
		case evKeyRepeat:
			printf("KeyRepeat   ");goto showkey;
		case evKeyRelease:
			printf("KeyRelease  ");
			showkey:
			printf("sym=%4x label=%4x button=%4x\n",
				ev->key.sym,ev->key.label,ev->key.button);
			break;
		case evValAbsolute:
			printf("ValAbsolute ");goto showval;
		case evValRelative:
			printf("ValRelative ");
			showval:
			for(y=0;y<ev->val.count;y++)
				printf("[%2d]=%d",ev->val.first+y,ev->val.value[y]);
			printf("\n");
			break;
		case evPtrAbsolute:
			printf("PtrAbsolute ");goto showptr;
		case evPtrRelative:
			printf("PtrRelative ");
			showptr:
			printf("x=%4x y=%4x z=%4x w=%4x\n",
				ev->pmove.x,ev->pmove.y,ev->pmove.z,ev->pmove.wheel);
			break;
		default:
			printf("event: type=%d\n",ev->any.type);
			for(y=0;y<ev->any.size;y++)
				printf("%02x  ",((unsigned char *)ev)[y]);
			printf("\n");
	}
}

static void test_gii_poll(gii_input_t inp, struct timeval *tval)
{
	gii_event_mask	mask;
	gii_event	event;

	mask = giiEventPoll(inp, emAll, tval);
	printf("0x%8x=giiEventPoll(%p,emAll,&{%ld,%ld});\n",
		mask, (void *)inp,
	       (long)tval->tv_sec, (long)tval->tv_usec);
	if (mask) 
	{
		printf("0x%8x ",mask);
		giiEventRead(inp,&event,emAll);
		test_showevent(&event);
	}
	while (mask) {
		struct timeval	tv= {0,0};
		mask = giiEventPoll(inp, emAll, &tv);
		if (mask) {
			printf("0x%8x ",mask);
			giiEventRead(inp,&event,emAll);
			test_showevent(&event);
		}
	}
}

/* The main routine.
 * It will just open a simple gii_input and test some filters.
 */
int main(int argc, char **argv)
{
	/* First we define a bunch of variables we will access throughout the
	 * main() function. Most of them are pretty meaningless loop-counters
	 * and helper-variables.
	 */
	int x;
	struct timeval tval;

 	gii_input_t inp;
 	gii_input_t filt;
	
	/* Initialize the GII library. This must be called before any other 
	 * GII function. 
	 */
	if (giiInit() != 0) {
		fprintf(stderr, "%s: unable to initialize libgii, exiting.\n",
			argv[0]);
		exit(1);
	}

	/* Open some input.
	 */
	inp = giiOpen("input-stdin", NULL);
	if (inp == NULL) {
		fprintf(stderr, "%s: unable to open stdin-source, exiting.\n",
			argv[0]);
		giiExit();
		exit(1);
	}

	printf("Doing test of input-stdin.\n");
	for(x = 0; x < 5; x++) {
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp, &tval);
	}
	printf("Now adding save and mouse filters.\n");

	filt=giiOpen("filter-mouse", NULL);
	if (filt == NULL) {
		fprintf(stderr,	"%s: warning: unable to open filter-mouse.\n",argv[0]);
	}
	inp=giiJoinInputs(inp,filt);
	filt=giiOpen("filter-save:evlog", NULL);
	if (filt == NULL) {
		fprintf(stderr,	"%s: warning: unable to open filter-save.\n",argv[0]);
	}
	inp=giiJoinInputs(inp,filt);

	for (x = 0; x < 10; x++) {
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp,&tval);
	}
	giiClose(inp);

	/* Now close down LibGII. */
	giiExit();	

	/* Terminate the program.*/
	return 0;
}
