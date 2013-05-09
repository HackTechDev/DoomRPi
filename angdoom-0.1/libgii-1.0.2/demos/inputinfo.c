/* $Id: inputinfo.c,v 1.3 2005/08/01 09:26:29 pekberg Exp $
******************************************************************************

   inputinfo - get information about all available input devices

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
#include <ggi/gii.h>
#include <ggi/gg.h>
#include <stdio.h>


static gii_input_t inp = NULL;


static void show_common(gii_event *ev)
{
	printf("    size=0x%02x origin=0x%08x "
		"err=%d\n", ev->any.size, ev->any.origin,
		ev->any.error);
}


static void show_devinfo(gii_cmd_event *ev)
{
	gii_event qev;
	gii_cmddata_getdevinfo *DI = (void *) ev->data;
	gii_cmddata_getvalinfo *VI = (void *) &qev.cmd.data;

	if (ev->origin & GII_EV_ORIGIN_SENDEVENT) {
		printf("(empty query)\n");
		return;
	}

	printf("short='%s' long='%s'\n"
		"    axes=%d buttons=%d generate=0x%06x\n", 
		DI->shortname, DI->longname,
		DI->num_axes, DI->num_buttons, DI->can_generate);

	/* ask device for valuator info */

	if (DI->num_axes > 0) {
		qev.any.size   = sizeof(gii_cmd_event);
		qev.any.type   = evCommand;
		qev.any.origin = GII_EV_ORIGIN_NONE;
		qev.any.target = GII_EV_TARGET_ALL;
		qev.cmd.code   = GII_CMDCODE_GETVALINFO;

		VI->number = GII_VAL_QUERY_ALL;

		giiEventSend(inp, &qev);
	}
}


static void show_valinfo(gii_cmd_event *ev)
{
	gii_cmddata_getvalinfo *VI = (void *) ev->data;

	if (ev->origin & GII_EV_ORIGIN_SENDEVENT) {
		printf("(empty query)\n");
		return;
	}

	printf("num=0x%02x short='%s' long='%s'\n"
		"    raw_range=%d..%d..%d unit=",
		VI->number, VI->shortname, VI->longname,
		VI->range.min, VI->range.center, VI->range.max);

	switch (VI->phystype) {
	case GII_PT_TIME:            printf("s");       break;
	case GII_PT_FREQUENCY:       printf("Hz");      break;
	case GII_PT_LENGTH:          printf("m");       break;
	case GII_PT_VELOCITY:        printf("m/s");     break;
	case GII_PT_ACCELERATION:    printf("m/s^2");   break;
	case GII_PT_ANGLE:           printf("rad");     break;
	case GII_PT_ANGVELOCITY:     printf("rad/s");   break;
	case GII_PT_ANGACCELERATION: printf("rad/s^2"); break;
	case GII_PT_AREA:            printf("m^2");     break;
	case GII_PT_VOLUME:          printf("m^3");     break;
	case GII_PT_MASS:            printf("kg");      break;
	case GII_PT_FORCE:           printf("N ");      break;
	case GII_PT_PRESSURE:        printf("Pa");      break;
	case GII_PT_TORQUE:          printf("Nm");      break;
	case GII_PT_ENERGY:          printf("J");       break;
	case GII_PT_POWER:           printf("W");       break;
	case GII_PT_TEMPERATURE:     printf("K");       break;
	case GII_PT_CURRENT:         printf("A");       break;
	case GII_PT_VOLTAGE:         printf("V");       break;
	case GII_PT_RESISTANCE:      printf("ohm)");    break;
	case GII_PT_CAPACITY:        printf("farad");   break;
	case GII_PT_INDUCTIVITY:     printf("henry");   break;
	default:                     printf("???");     break;
	}

	printf("\n");
}


static void show_command(gii_cmd_event *ev)
{
	switch (ev->code) {
	case GII_CMDCODE_GETDEVINFO:
		printf("GetDevInfo: ");
		show_devinfo(ev);
		return;

	case GII_CMDCODE_GETVALINFO:
		printf("GetValInfo: ");
		show_valinfo(ev);
		return;
	}

	printf("code=%0x08x\n", ev->code);
}


static void show_event(gii_event *ev)
{
	switch(ev->any.type) {
	case evCommand:
		printf("Command: ");
		show_command(&ev->cmd);
		break;
	default:
		printf("show_event: input type: %u\n",
			ev->any.type);
		break;
	}

	show_common(ev);
	fflush(stdout);
}




int main(void)
{
	int n;
	int rc;
	gii_event ev;

	rc = giiInit();
	if (rc < 0) giiPanic("Couldn't initialize libgii\n");

	/* open default input */
	inp = giiOpen(NULL);
	if (!inp) giiPanic("Couldn't open default input\n");

	/* The input target sends device infos
	 * about all available devices at opening time.
	 * So we just read them here and that's it.
	 */
	for (;;) {
		struct timeval tv = { 0, 100 };
		giiEventPoll(inp, emAll, &tv);
		n = giiEventsQueued(inp, emAll);
		if (n == 0) break;
		while (n--) {
			giiEventRead(inp, &ev, emAll);
			show_event(&ev);
		}
	}

	giiClose(inp);

	giiExit();
	return 0;
}
