/* $Id: filter.c,v 1.22 2005/08/04 16:36:36 soyt Exp $
******************************************************************************

   Filter-mouse - generic mouse event translaator.

   Copyright (C) 1999 Andreas Beck      [becka@ggi-project.org]

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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <ggi/internal/gg.h>
#include <ggi/internal/gg_replace.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

static void send_devinfo(gii_input *inp);

enum maptype { 
	MAP_KEY,
	MAP_REL,
	MAP_ABS,
	MAP_BUTTON,
	MAP_TO,		/* Dummy entry for parsing */
	MAP_END
};

static const char *maptypelist[MAP_END]={
	"KEY",
	"REL",
	"ABS",
	"BUT",
	"TO"
};

enum axis {
	AX_X,
	AX_Y,
	AX_Z,
	AX_WHEEL,
	AX_END
};

static const char *axislist[AX_END]={
	"X",
	"Y",
	"Z",
	"W"
};

struct transform {
	enum axis axis;
	double factor,treshold,higher;
};

typedef struct mapping_entry {

	struct mapping_entry *next;

	enum maptype from;
	uint32_t     modifier_mask;	/* all modifiers in mask */
	uint32_t     modifier_value;	/* must match value */
	union	{
		struct key {
			uint32_t button, label, symbol;
		} key;
		enum axis axis;
		unsigned int button;
	} fromdata;
	enum maptype to;
	union {
		struct transform trans;
		unsigned int button;
	} todata;
} mapping_entry;

typedef struct {
	mapping_entry *entry;
	uint32_t modifiers;
} fmouse_priv;

static void fmouse_send_pbutton(gii_input *inp, uint8_t type, uint32_t nr)
{
	gii_event ev;

	_giiEventBlank(&ev, sizeof(gii_pbutton_event));
	ev.pbutton.type = type;
	ev.pbutton.size = sizeof(gii_pbutton_event);
	ev.pbutton.origin = inp->origin;
	ev.pbutton.target = GII_EV_TARGET_ALL;
	ev.pbutton.button = nr;
	_giiEvQueueAdd(inp, &ev);
}


static int32_t getaxis(gii_pmove_event *move,enum axis axis) {
	switch(axis) {
		case AX_X:	return move->x;
		case AX_Y:	return move->y;
		case AX_Z: 	return move->z;
		case AX_WHEEL:	return move->wheel;
		default:	return 0;
	}
	return 0;	/* shouldn't happen. */
}

static void setaxis(gii_pmove_event *move,enum axis axis,int32_t value) {
	switch(axis) {
	case AX_X:	move->x=value;break;
	case AX_Y:	move->y=value;break;
	case AX_Z: 	move->z=value;break;
	case AX_WHEEL:	move->wheel=value;break;
	default:	return;
	}
}

static int32_t gettrans(struct transform *tf,double invalue) {
	return invalue * tf->factor +
		((fabs(invalue)>tf->treshold) ? (invalue > 0.0 ? invalue-tf->treshold : invalue+tf->treshold)*tf->higher : 0) ;
}

#define HASREL 1
#define HASABS 2

static int
GII_fmouse_handler(gii_input *inp, gii_event *event)
{
	fmouse_priv   *priv = inp->priv;
	mapping_entry *entry;
	int ret = 0;
	int has = 0;
	int invalue=0;
	static int di_sent=0;
	gii_pmove_event pmrel,pmabs;
	
	/* Did we already send the device info record ? Do so, if we didn't. */
	if (di_sent==0) { 
		di_sent=1;
		send_devinfo(inp); 
	}
	
	DPRINT_MISC("filter-mouse: Filt check.\n");
	if (event->any.origin==inp->origin) 
		return 0;	/* avoid recursion ! */
	DPRINT_MISC("filter-mouse: Real check.\n");

	/* Track modifiers. This allows to use stuff like shift-clicking */
	if (event->any.type==evKeyPress  ||
	    event->any.type==evKeyRepeat ||
	    event->any.type==evKeyRelease) {
		priv->modifiers=event->key.modifiers;
	}

	/* Clear the eventual relative and absolute events that will 
	 * get sent after evaluating all rules. We should probably keep the
	 * absolute events between calls.
	 */
	_giiEventBlank((gii_event *)&pmrel, sizeof(gii_pmove_event));
	pmrel.type = evPtrRelative;
	pmrel.size = sizeof(gii_pmove_event);
	pmrel.origin = inp->origin;
	pmrel.target = GII_EV_TARGET_ALL;
	pmrel.x = pmrel.y = pmrel.z = pmrel.wheel = 0;

	_giiEventBlank((gii_event *)&pmabs, sizeof(gii_pmove_event));
	pmabs.type = evPtrAbsolute;
	pmabs.size = sizeof(gii_pmove_event);
	pmabs.origin = inp->origin;
	pmabs.target = GII_EV_TARGET_ALL;
	pmabs.x = pmabs.y = pmabs.z = pmabs.wheel = 0;

	/* Now go through the entries and convert as appropriate.
	 */
	for(entry = priv->entry;entry;entry = entry->next) {


		DPRINT_MISC("filter-mouse: Checking entry %p.\n",entry);
		if ((priv->modifiers&entry->modifier_mask)!=
		    entry->modifier_value) continue;	/* Modifiers are wrong. Forget it. */

		switch(entry->from) {
			case MAP_KEY: 
				if (event->any.type==evKeyPress||
				    event->any.type==evKeyRepeat) invalue=1;	/* Key press */
				else if (event->any.type==evKeyRelease) invalue=0;	/* Key release*/
				else continue;					/* Something else - forget it. */

				/* Continue, if the button/label/symbol doesn't match */
				if (entry->fromdata.key.button!=GIIK_NIL&&
				    entry->fromdata.key.button!=event->key.button) continue;
				if (entry->fromdata.key.label!=GIIK_NIL&&
				    entry->fromdata.key.label!=event->key.label) continue;
				if (entry->fromdata.key.symbol!=GIIK_NIL&&
				    entry->fromdata.key.symbol!=event->key.sym) continue;
				break;
			case MAP_REL:
				if (event->any.type==evPtrRelative) 
					invalue=getaxis(&event->pmove,entry->fromdata.axis);
				else continue;
				break;
			case MAP_ABS:
				if (event->any.type==evPtrAbsolute) 
					invalue=getaxis(&event->pmove,entry->fromdata.axis);
				else continue;
				break;
			case MAP_BUTTON:
				if (event->any.type==evPtrButtonPress &&
				    event->pbutton.button==entry->fromdata.button) invalue=1;
				else if (event->any.type==evPtrButtonRelease &&
				    event->pbutton.button==entry->fromdata.button) invalue=0;
				else continue;
				break;
			default:continue;	/* Something is wrong */
		}
		switch(entry->to) {
			case MAP_REL:
				setaxis(&pmrel, 
					entry->todata.trans.axis, 
					gettrans(&entry->todata.trans,
						(double)invalue));
				ret=1;has|=HASREL;
				break;
			case MAP_ABS:
				setaxis(&pmabs, 
					entry->todata.trans.axis, 
					gettrans(&entry->todata.trans, 
						(double)invalue));
				ret = 1;
				has |= HASABS;
				break;
			case MAP_BUTTON:
				fmouse_send_pbutton(inp, 
					invalue ? evPtrButtonPress :
						evPtrButtonRelease,
					entry->todata.button);
				ret = 1;
				break;
			default:
				continue; /* Something is WRONG here. */
		}
	}
	DPRINT_MISC("filter-mouse: Checking entry %p.\n",entry);

	if (has&HASABS) {
		_giiEvQueueAdd(inp, (gii_event *) &pmabs);
	}
	if (has&HASREL) {
		_giiEvQueueAdd(inp, (gii_event *) &pmrel);
	}
	
	if (ret) DPRINT_MISC("filter-mouse: Eating event.\n");

	return ret;
}

static int GII_fmouse_close(gii_input *inp)
{
	fmouse_priv   *priv = inp->priv;
	mapping_entry *entry,*next;

	DPRINT_MISC("GII_fmouse_close(%p) called\n", inp);

	entry=priv->entry;
	while(entry) {
		next=entry->next;
		free(entry);
		entry=next;
	}

	free(priv);

	return 0;
}

static gii_cmddata_getdevinfo devinfo =
{
	"Mouse filter",	/* long device name */
	"mouf",		/* shorthand */
	emPointer,	/* can_generate */
	4,		/* num_buttons	(no supported device have more) */
	0		/* num_axes 	(only for valuators) */
};

static void send_devinfo(gii_input *inp)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	size_t size;

	size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);
	
	ev.any.size   = size;
	ev.any.type   = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code   = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	*dinfo = devinfo;

	_giiEvQueueAdd(inp, &ev);
}

static int checkkeyword(char *str,char **endptr,const char *list[],int numlist) {
	int x;

	if (endptr) *endptr=str;
	while(isspace((uint8_t)*str)) str++;

	for(x=0;x<numlist;x++) {
		if (0==strncasecmp(str, list[x], strlen(list[x]))) {
			if (endptr) *endptr=str+strlen(list[x]);
			return x;
		}
	}
	return GGI_ENOMATCH;
}

static int fmouse_doload(const char *filename,fmouse_priv *priv) {

	FILE *infile;
	char buffer[2048],*parsepoint,*pp2;
	const char *expect = "nothing";
	int line=0;
	mapping_entry entry;
	mapping_entry **ptr;
	
	ptr=&priv->entry;
	
	while(*ptr) ptr=&((*ptr)->next);

	DPRINT_MISC("filter-keymap opening config \"%s\" called\n", 
		    filename ? filename : "(nil)");
	if ( NULL==(infile=fopen(filename,"r")) ) {
		return GGI_ENOFILE;
	}
	while(fgets(buffer,2048,infile)) {
		line++;
		/* DPRINT_MISC("filter-mouse should parse %s\n",buffer); - Shouldn't be needed anymore. */
		entry.next=NULL;
		parsepoint=pp2=buffer;
		switch(entry.from=checkkeyword(parsepoint=pp2,&pp2,maptypelist,MAP_TO)) {
			case MAP_KEY:
				entry.modifier_mask=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="modmask";goto error;}
				entry.modifier_value=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="modval";goto error;}
				entry.fromdata.key.button=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="key-button";goto error;}
				entry.fromdata.key.label=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="key-label";goto error;}
				entry.fromdata.key.symbol=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="key-symbol";goto error;}
				break;

			case MAP_REL:
			case MAP_ABS:
				entry.modifier_mask=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="modmask";goto error;}
				entry.modifier_value=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="modval";goto error;}
				entry.fromdata.axis=checkkeyword(parsepoint=pp2,&pp2,axislist,AX_END);
				if (pp2==parsepoint) {expect="axis";goto error;}
				break;
			case MAP_BUTTON:
				entry.modifier_mask=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="modmask";goto error;}
				entry.modifier_value=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="modval";goto error;}
				entry.fromdata.button=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="butnum";goto error;}
				break;
			default: continue;	/* silently ignore all unparseables */
		}
		if (MAP_TO!=checkkeyword(parsepoint=pp2,&pp2,maptypelist,MAP_END)) {
			expect="TO";
			error:
			DPRINT_MISC("filter-mouse Parse error at %d:%s (expecting %s)\n",line,parsepoint,expect);
			continue;
		}
		switch(entry.to=checkkeyword(parsepoint=pp2,&pp2,maptypelist,MAP_TO)) {
			case MAP_KEY:
				expect="no KEY output allowed";
				goto error;	/* Maybe we should add that for mouse->key ? */
			case MAP_REL:
			case MAP_ABS:
				entry.todata.trans.axis=checkkeyword(parsepoint=pp2,&pp2,axislist,AX_END);
				if (pp2==parsepoint) {expect="axis";goto error;}
				entry.todata.trans.factor=strtod(parsepoint=pp2, &pp2);
				if (pp2==parsepoint) entry.todata.trans.factor=1.0;	/* O.k. - the rest will fail as well. So all defaults. */
				entry.todata.trans.treshold=strtod(parsepoint=pp2, &pp2);
				if (pp2==parsepoint) entry.todata.trans.treshold=9999.0;
				entry.todata.trans.higher=strtod(parsepoint=pp2, &pp2);
				if (pp2==parsepoint) entry.todata.trans.higher=entry.todata.trans.factor;
				break;
			case MAP_BUTTON:
				entry.todata.button=strtol(parsepoint=pp2, &pp2, 0);
				if (pp2==parsepoint) {expect="button";goto error;}
				break;
			default: goto error;
		}
		*ptr=malloc(sizeof(mapping_entry));
		if (*ptr) {
			memcpy(*ptr,&entry,sizeof(mapping_entry));
			ptr=&((*ptr)->next);
		} else {
			fclose(infile);
			return GGI_ENOMEM;
		}
	}
	fclose(infile);
	return 0;
}

static int fmouse_loadmap(const char *args,fmouse_priv *priv) {

	const char *dirname;
	char fname[2048];
	char appendstr[] = "/filter/mouse";

	if (args&&*args) return fmouse_doload(args,priv);

	dirname = ggGetUserDir();
	if (strlen(dirname) + sizeof(appendstr) < 2048) {

		snprintf(fname, 2048, "%s%s", dirname, appendstr);
		if (fmouse_doload(fname,priv)
		    == 0) {
			return 0;
		}
	}

	dirname = giiGetConfDir();
	if (strlen(dirname) + sizeof(appendstr) < 2048) {

		snprintf(fname, 2048, "%s%s", dirname, appendstr);
		if (fmouse_doload(fname,priv)
		    == 0) {
			return 0;
		}
	}
	return 1;	/* Failure */
}


EXPORTFUNC int GIIdl_filter_mouse(gii_input *inp, const char *args, void *argptr);

int GIIdl_filter_mouse(gii_input *inp, const char *args, void *argptr)
{
	fmouse_priv   *priv;

	DPRINT_MISC("filter-mouse init(%p, \"%s\") called\n", inp,
		    args ? args : "");

	priv = malloc(sizeof(fmouse_priv));
        if (priv == NULL) return GGI_ENOMEM;

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
	  free(priv);
	  return GGI_ENOMEM;
	}

	priv->entry = NULL;
	priv->modifiers = 0;
        fmouse_loadmap(args,priv);
	
	inp->priv       = priv;
	inp->GIIhandler = GII_fmouse_handler;
	inp->GIIclose   = GII_fmouse_close;

	DPRINT_MISC("filter-mouse fully up\n");

	return 0;
}
