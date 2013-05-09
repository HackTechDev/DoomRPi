/* $Id: demo.c,v 1.11 2005/08/01 09:26:29 pekberg Exp $
******************************************************************************

   demo.c - Author 1998 Andreas Beck   becka@ggi-project.org

   This is a demonstration of LibGII's functions and can be used as a
   reference programming example.

     This software is placed in the public domain and can be used freely
     for any purpose. It comes without any kind of warranty, either
     expressed or implied, including, but not limited to the implied
     warranties of merchantability or fitness for a particular purpose.
     Use it at your own risk. the author is not responsible for any damage
     or consequences raised by use or inability to use this program.

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
	switch(ev->any.type) {
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
		for(y = 0; y < ev->val.count; y++)
			printf("[%2u]=%d",ev->val.first+y,ev->val.value[y]);
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
		for(y = 0; y < ev->any.size; y++)
			printf("%02x  ",((unsigned char *)ev)[y]);
		printf("\n");
	}
}

static void test_gii_poll(gii_input_t inp, struct timeval *tval)
{
	gii_event_mask	mask;
	gii_event	event;
	gii_cmddata_getdevinfo devinfo;

	mask = giiEventPoll(inp,emAll,tval);
	printf("0x%8x=giiEventPoll(%p,emAll,&{%d,%d});\n",mask,
		(void *)inp, (int)tval->tv_sec,(int)tval->tv_usec);
	if (mask) {
		printf("0x%8x ",mask);
		giiEventRead(inp,&event,emAll);
		test_showevent(&event);
	}
	while(mask) {
		struct timeval	tv= {0,0};
		mask = giiEventPoll(inp,emAll,&tv);
		if (mask) {
			printf("0x%8x ",mask);
			giiEventRead(inp,&event,emAll);
			test_showevent(&event);
			printf("getdevinfo returned: %d\n",giiQueryDeviceInfo(inp,event.any.origin,&devinfo));
			printf("%s (%s): %x %x %x\n",devinfo.longname,
				devinfo.shortname,devinfo.can_generate,
				devinfo.num_buttons,devinfo.num_axes);
		}
	}
}

/* The main routine.
 * It will just open a gii_input and do some basic tests.
 */
int main(int argc, char **argv)
{
	/* First we define a bunch of variables we will access throughout the
	 * main() function. Most of them are pretty meaningless loop-counters
	 * and helper-variables.
	 */
	int x, res;
	struct timeval tval;

	gii_input_t inp,inp2;
	gii_event	event;

	/* Initialize the GII library. This must be called before any other
	 * GII function.
	 */
	if (giiInit() != 0) {
		fprintf(stderr, "%s: unable to initialize libgii, exiting.\n",
			argv[0]);
		exit(1);
	}

	/* First test the nulldevice ...
	 */
	inp = giiOpen("input-null", NULL);
	if (inp == NULL) {
		fprintf(stderr,
			"%s: unable to open null-source, exiting.\n",
			argv[0]);
		giiExit();
		exit(1);
	}
	printf("Doing test of input-null.\n");
	for(x = 0; x < 5; x++) {
		event.any.type=evKeyPress;
		event.any.size=sizeof(gii_key_event);
		event.any.target=GII_EV_TARGET_QUEUE;
		event.key.label=x;
		event.key.button=x;
		event.key.sym=x;
		if ( x & 1 ) giiEventSend(inp,&event);
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp,&tval);
	}

	/* Open some input.
	 */
	inp2 = giiOpen("input-stdin", NULL);
	if (inp2 == NULL) {
		fprintf(stderr,
			"%s: unable to open stdin-source, exiting.\n",
			argv[0]);
		giiExit();
		exit(1);
	}
	printf("Doing test of input-stdin.\n");
	for(x = 0; x < 10; x++) {
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp2,&tval);
	}
	for(x = 0; x < 10; x++) {
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp2,&tval);
	}
	printf("Joining inputs.\n");
	inp = giiJoinInputs(inp,inp2);
	/* Note that this mend inp2 into inp. That is you may not call
	   giiClose(inp2) - this happens together with giiClose(inp) ! */
	for(x = 0; x < 10; x++) {
		tval.tv_sec = 1*x;
		tval.tv_usec = 0;
		test_gii_poll(inp,&tval);
	}

	printf("Splitting inputs.\n");
	res = giiSplitInputs(inp, &inp2, GII_EV_ORIGIN_NONE, 0);
	if (res == 1) {
		gii_input_t tmp;
		tmp = inp2;
		inp2 = inp;
		inp = tmp;
        } else if (res < 0) {
		fprintf(stderr, "Failed to split inputs\n");
		giiClose(inp);
	}

	printf("Doing test of each input from the split.\n");
	for(x = 0; x < 3; x++) {
		event.any.type=evKeyPress;
		event.any.size=sizeof(gii_key_event);
		event.any.target=GII_EV_TARGET_QUEUE;
		event.key.label=x;
		event.key.button=x;
		event.key.sym=x;
		giiEventSend(inp,&event);
		printf ("No event should be returned here:\n");
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp2,&tval);
	        printf ("It should be returned here:\n");
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp,&tval);
	}

	for(x = 0; x < 3; x++) {
		event.any.type=evKeyPress;
		event.any.size=sizeof(gii_key_event);
		event.any.target=GII_EV_TARGET_QUEUE;
		event.key.label=x;
		event.key.button=x;
		event.key.sym=x;
		giiEventSend(inp2,&event);
		printf ("No event should be returned here:\n");
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp,&tval);
		printf ("It should be returned here:\n");
		tval.tv_sec = 0;
		tval.tv_usec = 100000*x;
		test_gii_poll(inp2,&tval);
	}

	printf("Joining inputs back together.\n");
	inp=giiJoinInputs(inp,inp2);

	printf("testing rejoined inputs\n");
	for(x = 0; x < 5; x++) {
		event.any.type=evKeyPress;
		event.any.size=sizeof(gii_key_event);
		event.any.target=GII_EV_TARGET_QUEUE;
		event.key.label=x;
		event.key.button=x;
		event.key.sym=x;
		if ( x & 1 ) giiEventSend(inp2,&event);
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
