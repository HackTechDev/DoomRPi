/* $Id: filter.c,v 1.16 2005/08/04 16:36:36 soyt Exp $
******************************************************************************

   Filter-keytrans - generic key event translator.

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

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include <ggi/gg.h>
#include <ggi/internal/gg_replace.h>	/* for snprintf() */
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

static void send_devinfo(gii_input *inp);

typedef struct mapentry {
  /* GIIK_NIL means dontcare/dontchange. */
	uint32_t  modifier_mask;	/* all modifiers in mask */
	uint32_t  modifier_value;	/* must match value */
	uint32_t  symin,labelin,buttonin;	/* incoming keyvalues */
	uint32_t  modifier_changemask;	/* change these bits from the original */
	uint32_t  modifier_ormask;	/* set these bits */
	uint32_t  symout,labelout,buttonout;	/* outgoing keyvalues */
} mapping_entry;

typedef struct {
	mapping_entry *table;
	int numentries;
} fkey_priv;
 
static void
fkey_send_key(gii_input *inp, uint8_t type, uint32_t modifier, 
	uint32_t button, uint32_t label, uint32_t symbol)
{
	gii_event ev;

	_giiEventBlank(&ev, sizeof(gii_key_event));
	ev.pbutton.type = type;
	ev.key.size     = sizeof(gii_key_event);
	ev.key.origin   = inp->origin;
	ev.key.target   = GII_EV_TARGET_ALL;
	ev.key.button   = button;
	ev.key.label    = label;
	ev.key.sym      = symbol;
	_giiEvQueueAdd(inp, &ev);
}

static int
GII_fkey_handler(gii_input *inp, gii_event *event)
{
	fkey_priv   *priv = inp->priv;
	mapping_entry *entry;
	int x;
	static int di_sent=0;
	
	if (di_sent==0) { di_sent=1;send_devinfo(inp); }
	
	DPRINT_MISC("filter-keymap: Filt check.\n");
	if (event->any.origin==inp->origin) 
		return 0;	/* avoid recursion ! */
	DPRINT_MISC("filter-keymap: Real check.\n");

	if (event->any.type!=evKeyPress &&
		event->any.type!=evKeyRelease &&
		event->any.type!=evKeyRepeat)
		return 0; /* No keyboard event - ignore it */

	DPRINT_MISC("filter-keymap: Key event - looking.\n");

	for(x=priv->numentries,entry=priv->table;x--;entry++) {
		DPRINT_MISC("filter-keymap: Table.\n");
		if ( (event->key.modifiers&entry->modifier_mask) == entry->modifier_value &&
		     (entry->symin   ==GIIK_NIL || entry->symin   ==event->key.sym) && 
		     (entry->labelin ==GIIK_NIL || entry->labelin ==event->key.label) && 
		     (entry->buttonin==GIIK_NIL || entry->buttonin==event->key.button) ) {
			DPRINT_MISC("filter-keymap: Key event - got it - sending.\n");
			fkey_send_key(inp,
				event->key.type,
				(event->key.modifiers&~entry->modifier_changemask)|entry->modifier_ormask,
				(entry->buttonout==GIIK_NIL) ? event->key.button : entry->buttonout,
				(entry->labelout ==GIIK_NIL) ? event->key.label  : entry->labelout,
				(entry->symout   ==GIIK_NIL) ? event->key.sym    : entry->symout
				);
			return 1;
		}
	}
	return 0;
}

static int GII_fkey_close(gii_input *inp)
{
	fkey_priv   *priv = inp->priv;
	DPRINT_MISC("GII_fkey_close(%p) called\n", inp);

	free(priv->table);
	free(inp->priv);

	return 0;
}

static gii_cmddata_getdevinfo devinfo =
{
	"Keymap filter",	/* long device name */
	"keyf",		/* shorthand */
	emKey,	/* can_generate */
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

static int fkey_doload(const char *filename,fkey_priv *priv) {

	FILE *infile;
	mapping_entry *newmap,mapbuf;
	char buffer[2048];

	DPRINT_MISC("filter-keymap opening config \"%s\" called\n", 
		    filename ? filename : "(nil)");
	if ( NULL==(infile=fopen(filename,"r")) ) {
		return GGI_ENOFILE;
	}
	while(fgets(buffer,2048,infile)) {
		if ( 10 != sscanf(buffer,"%u %u %u %u %u %u %u %u %u %u",
			&mapbuf.modifier_mask,
			&mapbuf.modifier_value,
			&mapbuf.buttonin,
			&mapbuf.labelin,
			&mapbuf.symin,
			&mapbuf.modifier_changemask,
			&mapbuf.modifier_ormask,
			&mapbuf.buttonout,
			&mapbuf.labelout,
			&mapbuf.symout
			) ) continue;	/* Seems not a legal entry */
		DPRINT_MISC("filter-keymap have entry #%d\n",priv->numentries);
		newmap=realloc(priv->table,(priv->numentries+1)*sizeof(mapping_entry));
		if (newmap) {
			priv->table=newmap;
			priv->table[priv->numentries]=mapbuf;
			priv->numentries++;
		} else {
			free(priv->table);
			fclose(infile);
			return GGI_ENOMEM;
		}
	}
	fclose(infile);
	return 0;
}

static int fkey_loadmap(const char *args,fkey_priv *priv) {

	const char *dirname;
	char fname[2048];
	char appendstr[] = "/filter/keytrans";

	if (args&&*args) return fkey_doload(args,priv);

	dirname = ggGetUserDir();
	if (strlen(dirname) + sizeof(appendstr) < 2048) {

		snprintf(fname, 2048, "%s%s", dirname, appendstr);
		if (fkey_doload(fname,priv)
		    == 0) {
			return 0;
		}
	}

	dirname = giiGetConfDir();
	if (strlen(dirname) + sizeof(appendstr) < 2048) {

		snprintf(fname, 2048, "%s%s", dirname, appendstr);
		if (fkey_doload(fname,priv)
		    == 0) {
			return 0;
		}
	}
	return 1;	/* Failure */
}



EXPORTFUNC int GIIdl_filter_keytrans(gii_input *inp, const char *args, void *argptr);

int GIIdl_filter_keytrans(gii_input *inp, const char *args, void *argptr)
{
	fkey_priv   *priv;

	DPRINT_MISC("filter-keymap init(%p, \"%s\") called\n", inp,
		    args ? args : "");

	priv = malloc(sizeof(fkey_priv));
        if (priv == NULL) return GGI_ENOMEM;

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
	  free(priv);
	  return GGI_ENOMEM;
	}


	priv->table = NULL;
	priv->numentries = 0;
	fkey_loadmap(args,priv);
	
	inp->priv       = priv;
	inp->GIIhandler = GII_fkey_handler;
	inp->GIIclose   = GII_fkey_close;
	
	DPRINT_MISC("filter-keymap fully up\n");
	
	return 0;
}
