/* $Id: input.c,v 1.12 2005/08/04 12:43:29 cegger Exp $
******************************************************************************

   Input-pcjoy: all-in-one-file.
   
   This source is likely to be removed again later, as it is 
   _evil_ _evil_ _evil_ to access hardware directly.
   Well - it might come in handy for the DOS port ...

   Copyright (C) 1998 Andreas Beck      [becka@ggi-project.org]

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

#include <stdlib.h>
#include "config.h"
#include <ggi/internal/gg_replace.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_IO_H
#include <sys/io.h>
#else if defined HAVE_ASM_IO_H
#include <asm/io.h>
#endif

struct joystate {
	int button;
	int axis[4];
};

struct joyinfo {
	int treshold;
	struct joystate last;
	struct joystate curr;
};

static int _GII_pcjoy_getbuttons(void)
{
	DPRINT_MISC("input-pcjoy: getbuttons\n");
	return (inb(0x201)>>4)^0x0f;	/* Invert ! */
}

static int _GII_pcjoy_getaxis(int number)
{
	int x=0;
	int mask=1<<number;

	DPRINT_MISC("input-pcjoy: getaxis %d\n",number);
	while((inb(0x201)&mask)&&x<3000) x++;	/* Let old run clear */
	x=0;outb(0x00,0x201);
	while((inb(0x201)&mask)&&x<3000) x++;	/* Count new run */
	return x;
}

static void _GII_pcjoy_getnew(struct joyinfo *joy)
{
	int x,y;

	DPRINT_MISC("input-pcjoy: getnew\n");
	joy->curr.button=_GII_pcjoy_getbuttons();
	for(x=0;x<4;x++)
	{
		joy->curr.axis[x]+=_GII_pcjoy_getaxis(x);
		joy->curr.axis[x]/=2;
		y=joy->curr.axis[x]-joy->last.axis[x];
		if (y<0) y=-y;
		if (y<joy->treshold) 
			joy->curr.axis[x]=joy->last.axis[x];
	}
}

static gii_event_mask GII_pcjoy_poll(gii_input *inp, void *arg)
{
	struct joyinfo *joy=inp->priv;
	gii_event_mask rc=0;
	gii_event ev;
	int x;
	
	DPRINT_MISC("input-pcjoy: poll(%p);\n",inp);

	/* Get new data first. */
	_GII_pcjoy_getnew(joy);

	if (memcmp(&joy->curr,&joy->last,sizeof(joy->last))==0)
		return 0;	/* Nothing happened */

	for(x=0;x<4;x++)
		if ((joy->last.button&(1<<x))!=(joy->curr.button&(1<<x)))
		{
			_giiEventBlank(&ev, sizeof(gii_key_event));
			ev.key.size=sizeof(gii_key_event);
			ev.key.type=(joy->curr.button&(1<<x)) ?
				evKeyPress : evKeyRelease;
			rc |= (joy->curr.button&(1<<x)) ?
				emKeyPress : emKeyRelease;
			DPRINT_MISC("input-pcjoy: read KEY\n");
			ev.key.origin=inp->origin;
			ev.key.modifiers=0;
			ev.key.sym=ev.key.label=GIIK_VOID;
			ev.key.button=x;
			_giiEvQueueAdd(inp,&ev);
		}
	for(x=0;x<4;x++)
		if (joy->last.axis[x]!=joy->curr.axis[x])
		{
			_giiEventBlank(&ev, sizeof(gii_val_event));
			ev.size=sizeof(gii_val_event);
			DPRINT_MISC("input-pcjoy: read VAL\n");
			ev.val.type=evValAbsolute;
			rc |= emValAbsolute;
			ev.val.origin=inp->origin;
			ev.val.first=x;
			ev.val.count=1;
			ev.val.value[0]=joy->curr.axis[x];
#if 0
			for(;x<4;x++)
			if (joy->last.axis[x]!=joy->curr.axis[x])
			{
				ev.val.value[ev.val.changed]=joy->curr.axis[x];
				ev.val.changed++;
			}
#endif
			_giiEvQueueAdd(inp,&ev);
		}

	joy->last=joy->curr;

	return rc;
}

static int GII_pcjoy_close(gii_input *inp)
{
	free(inp->priv);
	
	DPRINT_MISC("input-pcjoy: close %p\n",inp);
	return 0;
}


EXPORTFUNC int GIIdl_pcjoy(gii_input *inp,const char *args, void *argptr);

int GIIdl_pcjoy(gii_input *inp,const char *args, void *argptr)
{
	struct joyinfo *joy;

	DPRINT_MISC("input-pcjoy starting.(args=\"%s\",argptr=%p)\n",args ? args : "(nil)",argptr);

	if (NULL==(inp->priv=joy=malloc(sizeof(struct joyinfo))))
	{
		DPRINT_MISC("input-pcjoy: no memory.\n");
		return GGI_ENOMEM;
	}

	if (ioperm(0x201,1,1))
	{
		DPRINT_MISC("input-pcjoy: Need to be root for pcjoy.\n");
		return -1;
	}

	joy->treshold=30;	/* FIXME ! Read all params from file or args. */
	joy->last.button=0;
	joy->last.axis[0]=joy->last.axis[1]=
	joy->last.axis[2]=joy->last.axis[3]=0;
	joy->curr=joy->last;
	
	/* We leave these on the default handlers
	 *	inp->GIIseteventmask=_GIIstdseteventmask;
	 *	inp->GIIgeteventmask=_GIIstdgeteventmask;
	 *	inp->GIIgetselectfdset=_GIIstdgetselectfd;
	 */

	/* They are already set, so we can as well use them instead of
	 * accessing the curreventmask member directly.
	 */
	inp->targetcan=emKeyPress | emKeyRelease | emValAbsolute;
	inp->GIIseteventmask(inp,emKeyPress | emKeyRelease | emValAbsolute);

	inp->maxfd=0;			/* We poll - ouch ! */
	inp->flags|=GII_FLAGS_HASPOLLED;

	inp->GIIclose	 =GII_pcjoy_close;
	inp->GIIeventpoll=GII_pcjoy_poll;

	DPRINT_MISC("input-pcjoy fully up\n");

	return 0;
}

