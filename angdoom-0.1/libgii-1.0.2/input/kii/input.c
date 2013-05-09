/* $Id: input.c,v 1.16 2005/08/04 12:43:27 cegger Exp $
******************************************************************************

   Input-KII: Input for KII
      
   Copyright (C) 2002 Paul Redmond

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
#include <string.h>
#include <stdlib.h>

#include <ggi/gg.h>
#include <ggi/input/kii.h>

#define KIIGII_N 51
static struct { kii_u32_t ksym; uint32_t gsym; int len; } kiigii[KIIGII_N] =
	{
	  /*
	   *    Translation table KII symbols to GII symbols...
	   *
	   *    KGI symbol, corresp GGI label, # sequential entries
	   *    First matching KGI symbol exits the table, else try next row.
	   */
	  	{ K_FIRST_ASCII,	'\0',		0x80 },
		{ K_TYPE_LATIN,		'\0',		0x80 },
		{ K_F21,		GIIK_F21,	44 },
		{ K_F1,			GIIK_F1,        20 },
		{ K_P0,			GIIK_P0,	10 },
		{ K_DOWN,		GIIK_Down,	1 },
		{ K_LEFT,		GIIK_Left,	1 },
		{ K_RIGHT,		GIIK_Right,	1 },
		{ K_UP,			GIIK_Up,	1 },
		{ K_PPLUS,		GIIK_PPlus,	1 },
		{ K_PMINUS,		GIIK_PMinus,	1 },
		{ K_PSTAR,		GIIK_PStar,	1 },
		{ K_PSLASH,		GIIK_PSlash,	1 },
		{ K_PENTER,		GIIK_PEnter,	1 },
		{ K_PCOMMA,		GIIK_PSeparator,1 },
		{ K_PDOT,		GIIK_PDecimal,	1 },
		{ K_PPLUSMINUS,		GIIK_PPlusMinus,1 },
		{ K_PARENL,		GIIK_PParenLeft,2 },
		{ K_NORMAL_SHIFT,	GIIK_Shift,	1 },
		{ K_NORMAL_CTRL,	GIIK_Ctrl,	1 },
		{ K_NORMAL_ALT,		GIIK_Alt,	1 },
		{ K_NORMAL_ALTGR,	GIIK_AltGr,	1 },
		{ K_NORMAL_SHIFTL,	GIIK_ShiftL,	1 },
		{ K_NORMAL_SHIFTR,	GIIK_ShiftR,	1 },
		{ K_NORMAL_CTRLL,	GIIK_CtrlL,	1 },
		{ K_NORMAL_CTRLR,	GIIK_CtrlR,	1 },
		{ K_FIND,		GIIK_Find,	1 },
		{ K_INSERT,		GIIK_Insert,	1 },
		{ K_SELECT,		GIIK_Select,	1 },
		{ K_PGUP,		GIIK_PageUp,	2 },
		{ K_MACRO,		GIIK_Macro,	1 },
		{ K_HELP,		GIIK_Help,	3 },
		{ K_UNDO,		GIIK_Undo,	1 },
		{ K_ENTER,		GIIK_Enter,	1 },
		{ K_BREAK,		GIIK_Break,	1 },
		{ K_CAPS,		GIIK_CapsLock,	3 },
		{ K_BOOT,		GIIK_Boot,	1 },
		{ K_COMPOSE,		GIIK_Compose,	2 },
		{ K_SYSTEM_REQUEST,	GIIK_SysRq,	1 },
		{ K_DGRAVE,		GIIK_DeadGrave, 1 },
		{ K_DACUTE,		GIIK_DeadAcute, 1 },
		{ K_DCIRCM,		GIIK_DeadCircumflex, 1 },
		{ K_DTILDE,		GIIK_DeadTilde, 1 },
		{ K_DDIERE,		GIIK_DeadDiaeresis, 1 },
		{ K_DCEDIL,		GIIK_DeadCedilla, 1 },
		{ K_LOCKED_SHIFT,	GIIK_ShiftLock,	3 },
		{ K_LOCKED_ALTGR,	GIIK_AltGrLock, 1 },
		{ K_LOCKED_SHIFTL,	GIIK_ShiftLock, 1 },
		{ K_LOCKED_SHIFTR,	GIIK_ShiftLock, 1 },
		{ K_LOCKED_CTRLL,	GIIK_CtrlLock,	1 },
		{ K_LOCKED_CTRLR,	GIIK_CtrlLock,	1 }
	};

#define SYMLAB_N 2
static struct { uint32_t sym; uint32_t label; int len; } symlab[SYMLAB_N] = 
	{
	  /*
	   *    Translation table GII symbols to GII labels...
	   *
	   *    GII symbol, corresp GII label, # sequential entries
	   *    First matching GII symbol exits the table, else try next row.
	   */
		{ 'a',/* ucase labels */ GIIUC_A,	26 }, 
		{ 0,  /* CTRL chars */   GIIUC_At,	32 }
	};

static const gg_option optlist[] =
{
	{ "device", "/dev/event,/dev/kgi/event" },
};

typedef struct {
	kii_context_t *ctx;
} kii_priv;

#define KII_PRIV(inp) ((kii_priv *)inp->priv)
#define KII_CTX(inp)  (KII_PRIV(inp)->ctx)

static gii_cmddata_getdevinfo devinfo =
{
	"Kernel Input Interface",
	"kii",
	emAll,
	0, //GII_NUM_UNKNOWN,
	0  //GII_NUM_UNKNOWN
};

static gii_event_mask GII_kii_handle_key(gii_input *inp, gii_event *ge,
	const kii_event_t *ke)
{
	_giiEventBlank(ge, sizeof(gii_key_event));
	ge->any.size = sizeof(gii_key_event);
	ge->any.origin = inp->origin;
	
	DPRINT_LIBS("sym: 0x%.8x, code: 0x%.8x, effect: 0x%.4x, "
		"normal: 0x%.4x, locked: 0x%.4x, sticky: 0x%.4x\n",
		ke->key.sym, ke->key.code, ke->key.effect,
		ke->key.normal, ke->key.locked, ke->key.sticky);
	
	/* FIXME: What about shift L/R ctrl L/R */
	if (ke->key.effect & KII_MM_SHIFT)
		ge->key.modifiers |= GII_MOD_SHIFT;
	if (ke->key.effect & KII_MM_CTRL)
		ge->key.modifiers |= GII_MOD_CTRL;
	if (ke->key.effect & KII_MM_ALT)
		ge->key.modifiers |= GII_MOD_ALT;
	if (ke->key.effect & KII_MM_ALTGR)
		ge->key.modifiers |= GII_MOD_ALTGR;
	
	/* Event types are the same */
	ge->any.type = ke->any.type;

	/* Button number is the scancode */
	ge->key.button = ke->key.code;

        if (ke->key.code < 0x100) {
                /* Keyboard key. */
		int i;

		for (i = 0; i < KIIGII_N; i++) {
			if (ke->key.sym < kiigii[i].ksym) continue;
			if (ke->key.sym >= kiigii[i].ksym + kiigii[i].len) 
			  continue;
			ge->key.sym = 
			  kiigii[i].gsym + (ke->key.sym - kiigii[i].ksym);
			break;
		}

		if (i >= KIIGII_N) goto voidkey;

		ge->key.label = ge->key.sym;
		for (i = 0; i < SYMLAB_N; i++) {
			if (ge->key.sym < symlab[i].sym) continue;
			if (ge->key.sym >= symlab[i].sym + symlab[i].len)
			  continue;
			ge->key.label =
			  symlab[i].label + (ge->key.sym - symlab[i].sym);
			break;
		}
        } else {
                /* Other button. */
	voidkey:
                ge->key.label = ge->key.sym = GIIK_VOID;
        }
	
	_giiEvQueueAdd(inp, ge);
	
	return (1 << ge->any.type);
}

static gii_event_mask GII_kii_handle_ptr(gii_input *inp, gii_event *ge,
	const kii_event_t *ke)
{

	if (ke->any.type == KII_EV_PTR_STATE) {
	
		return emZero;
	}

	if (ke->any.type < KII_EV_PTR_BUTTON_PRESS) {

		/* pointer move event */
    		_giiEventBlank(ge, sizeof(gii_pmove_event));
		
		switch (ke->any.type) {
		
		case KII_EV_PTR_RELATIVE:
			ge->any.type = evPtrRelative;
			break;
			
		case KII_EV_PTR_ABSOLUTE:
			ge->any.type = evPtrAbsolute;
			break;
		
		default:
			return emZero;
		}
		ge->any.origin = inp->origin;
		ge->any.size = sizeof(gii_pmove_event);
		ge->pmove.x = ke->pmove.x;
		ge->pmove.y = ke->pmove.y;
		
		_giiEvQueueAdd(inp, ge);
	}
	else {
		int i;
		kii_u_t mask = 1;

    		_giiEventBlank(ge, sizeof(gii_pbutton_event));
		switch (ke->any.type) {
		
		case KII_EV_PTR_BUTTON_PRESS:
			ge->any.type = evPtrButtonPress; break;
			
		case KII_EV_PTR_BUTTON_RELEASE:
			ge->any.type = evPtrButtonRelease; break;
		
		default:
			return emZero;
		}
		ge->any.origin = inp->origin;
		ge->any.size = sizeof(gii_pbutton_event);
		for (i = 1; i <= 32; i++) {
		
			if (ke->pbutton.button & mask) {
			
				ge->pbutton.button = i;
				_giiEvQueueAdd(inp, ge);
			}
			mask <<= 1;
		}
	}
	
	return (1 << ge->any.type);
}

static gii_event_mask GII_kii_poll(gii_input *inp, void *arg)
{
	const kii_event_t *ke;
	gii_event_mask em = emZero;
	gii_event ge;

	/* FIXME: Maybe we should handle eof condition? -- ortalo */

	if (arg == NULL) {

		fd_set fds = inp->fdset;
		struct timeval tv = { 0, 0 };
		if (select(inp->maxfd, &fds, NULL, NULL, &tv) <= 0) {

			return emZero;
		}
	} else {

		if (! FD_ISSET(kiiEventDeviceFD(KII_CTX(inp)), ((fd_set*)arg))) {

			/* Nothing to read on the fd set */
			DPRINT_EVENTS("GII_kii_poll: dummy poll\n");
			return emZero;
		}
	}

	/* Now we are sure there is data. kiiEventAvailable() may block */
	if (!kiiEventAvailable(KII_CTX(inp))) {
	
		return emZero;
	}
		
	for (ke=kiiNextEvent(KII_CTX(inp));ke;ke=kiiNextEvent(KII_CTX(inp))) {

		if ((1 << ke->any.type) & KII_EM_KEYBOARD) {
		
			em |= GII_kii_handle_key(inp, &ge, ke);
		}
		else if ((1 << ke->any.type) & KII_EM_POINTER) {
		
			em |= GII_kii_handle_ptr(inp, &ge, ke);
		}
	}

	return em;
}

static void send_devinfo(gii_input *inp)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	size_t size;
	
	size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);
	
	_giiEventBlank(&ev, size);
	
	ev.any.size = size;
	ev.any.type = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code = GII_CMDCODE_GETDEVINFO;
	
	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	*dinfo = devinfo;
	
	_giiEvQueueAdd(inp, &ev);
}

static int GII_kii_send_event(gii_input *inp, gii_event *ev)
{
	if (ev->any.target != inp->origin &&
	    ev->any.target != GII_EV_TARGET_ALL) {
	
		return GGI_EEVNOTARGET;
	}
	
	if (ev->any.type != evCommand) {
	
		return GGI_EEVUNKNOWN;
	}
	
	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
	
		send_devinfo(inp);
		return 0;
	}
	
	return GGI_EEVUNKNOWN;
}

static int GII_kii_close(gii_input *inp) 
{
	kii_priv *priv = KII_PRIV(inp);
        
	free(KII_CTX(inp));
	free(priv);

	DPRINT_MISC("kii: exit OK.\n");

	return 0;
}


EXPORTFUNC int GIIdl_kii(gii_input *inp, const char *args, void *argptr);

int GIIdl_kii(gii_input *inp, const char *args, void *argptr)
{
	kii_priv *priv;
	gg_option options[KII_NUM_OPTS];

	DPRINT_MISC("kii starting. (args=%s,argptr=%p)\n",
			args, argptr);
    
	if ((priv = inp->priv = malloc(sizeof(kii_priv))) == NULL) {

		return GGI_ENOMEM;
	}
	if ((KII_CTX(inp) = calloc(1, sizeof(kii_context_t))) == NULL) {
		free(priv);
		return GGI_ENOMEM;
	}
	
	memcpy(options, optlist, sizeof(options));
	if (args) {
		args = ggParseOptions((char*)args, options, KII_NUM_OPTS);
		if (args == NULL) {
			DPRINT_LIBS("Error in arguments\n");
			free(KII_CTX(inp));
			free(priv);
			return GGI_EARGINVAL;
		}
	}

	if(_giiRegisterDevice(inp, &devinfo, NULL) == 0) {
		
		free(KII_CTX(inp));
		free(priv);
		return GGI_ENOMEM;
	}

	if (KII_EOK != kiiInit(KII_CTX(inp), options)) {
	
		free(KII_CTX(inp));
		free(priv);
		return GGI_ENODEVICE;
	}

	if (KII_EOK != kiiMapDevice(KII_CTX(inp))) {
	
		/* FIXME: kiiInit now leaks memory */
		free(KII_CTX(inp));
		free(priv);	
		return GGI_ENODEVICE;
	}

	inp->GIIsendevent = GII_kii_send_event;
	inp->GIIeventpoll = GII_kii_poll;
	inp->GIIclose     = GII_kii_close;
   
	inp->GIIseteventmask(inp, emAll);

	inp->targetcan = emAll;
	inp->curreventmask = emAll;
    
	inp->maxfd = kiiEventDeviceFD(KII_CTX(inp)) + 1;
	FD_SET(kiiEventDeviceFD(KII_CTX(inp)), &inp->fdset);

	send_devinfo(inp);

	DPRINT_MISC("kii fully up\n");

	return 0;
}
