/* $Id: input.m,v 1.10 2005/08/04 12:43:26 cegger Exp $
******************************************************************************

   Cocoa: Input driver
      
   Copyright (C) 2002 Christoph Egger

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
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <ggi/input/cocoa.h>
#include "cocoa.h"
#include "keysyms.h"


static gii_cmddata_getdevinfo devinfo =
{
	"Cocoa Input Driver",			/* long name */
	"CID",					/* short name */
	emKey | emPtrButton | emPtrRelative,	/* targetcan */
#if 0
	emAll,					/* targetcan */
#endif
	256,					/* 256 pseudo-buttons */
	0					/* num axes */
};


static gii_event_mask GII_cocoa_handle_key(gii_input *inp, gii_event *ge,
			NSEvent *ev)
{
	NSEventType type;
	NSString *chars;
	unsigned int modifiers;
	unsigned short scancode;
	unsigned short sym;

	type = [ ev type ];
	modifiers = [ ev modifierFlags ];
	chars = [ ev characters ];	/* unicode chars! */
	scancode = [ ev keyCode ];	/* device dependent key number */
//	sym = keymap[ scancode ];
	sym = scancode;

	_giiEventBlank(ge, sizeof(gii_key_event));
	ge->any.size = sizeof(gii_key_event);
	ge->any.origin = inp->origin;
	
	DPRINT_LIBS("keyboard event, sym: 0x%.8x, label: 0x%.8x, scancode: 0x%.8x, modifier: 0x%.8x\n",
		    sym, [chars intValue], scancode, modifiers);

	/* Button number is the scancode */
	ge->key.button = scancode;

	if (modifiers & NSAlphaShiftKeyMask) {
		ge->key.modifiers |= GII_MOD_CAPS;
		fprintf(stderr, "handle_key: modifier: NSAlphaShiftKeyMask - GII_MOD_CAPS\n");
	}	/* if */
	if (modifiers & NSShiftKeyMask) {
		ge->key.modifiers |= GII_MOD_SHIFT;
		fprintf(stderr, "handle_key: modifier NSShiftKeyMask - GII_MOD_SHIFT\n");
	}	/* if */
	if (modifiers & NSControlKeyMask) {
		ge->key.modifiers |= GII_MOD_CTRL;
		fprintf(stderr, "handle_key: modifier NSControlKeyMask - GII_MOD_CTRL\n");
	}	/* if */
	if (modifiers & NSAlternateKeyMask) {
		ge->key.modifiers |= GII_MOD_ALT;
		fprintf(stderr, "handle_key: modifier NSAlternateKeyMask - GII_MOD_ALT\n");
	}	/* if */
	if (modifiers & NSCommandKeyMask) {
		ge->key.modifiers |= GII_MOD_META;
		fprintf(stderr, "handle_key: modifier NSCommandKeyMask - GII_MOD_META\n");
	}	/* if */
	if (modifiers & NSNumericPadKeyMask) {
		ge->key.modifiers |= GII_MOD_NUM;
		fprintf(stderr, "handle_key: modifier NSNumericPadKeyMask - GII_MOD_NUM\n");
	}	/* if */


	/* FIXME: Are those really symbols or are
	 * they labels?
	 */
	switch ([ chars intValue ]) {
	case NSUpArrowFunctionKey:
		ge->key.sym = GIIK_Up;
		fprintf(stderr, "symbol or label: NSUpArrowFunctionKey - GIIK_Up\n");
		break;
	case NSDownArrowFunctionKey:
		ge->key.sym = GIIK_Down;
		fprintf(stderr, "symbol or label: NSDownArrowFunctionKey - GIIK_Down\n");
		break;
	case NSLeftArrowFunctionKey:
		ge->key.sym = GIIK_Left;
		fprintf(stderr, "symbol or label: NSLeftArrowFunctionKey - GIIK_Left\n");
		break;
	case NSRightArrowFunctionKey:
		ge->key.sym = GIIK_Right;
		fprintf(stderr, "symbol or label: NSRightArrowFunctionKey - GIIK_Right\n");
		break;

	case NSF1FunctionKey:
		ge->key.sym = GIIK_F1;
		fprintf(stderr, "symbol or label: NSF1FunctionKey - GIIK_F1\n");
		break;
	case NSF2FunctionKey:
		ge->key.sym = GIIK_F2;
		fprintf(stderr, "symbol or label: NSF2FunctionKey - GIIK_F2\n");
		break;
	case NSF3FunctionKey:
		ge->key.sym = GIIK_F3;
		fprintf(stderr, "symbol or label: NSF3FunctionKey - GIIK_F3\n");
		break;
	case NSF4FunctionKey:
		ge->key.sym = GIIK_F4;
		fprintf(stderr, "symbol or label: NSF4FunctionKey - GIIK_F4\n");
		break;
	case NSF5FunctionKey:
		ge->key.sym = GIIK_F5;
		fprintf(stderr, "symbol or label: NSF5FunctionKey - GIIK_F5\n");
		break;
	case NSF6FunctionKey:
		ge->key.sym = GIIK_F6;
		fprintf(stderr, "symbol or label: NSF6FunctionKey - GIIK_F6\n");
		break;
	case NSF7FunctionKey:
		ge->key.sym = GIIK_F7;
		fprintf(stderr, "symbol or label: NSF7FunctionKey - GIIK_F7\n");
		break;
	case NSF8FunctionKey:
		ge->key.sym = GIIK_F8;
		fprintf(stderr, "symbol or label: NSF8FunctionKey - GIIK_F8\n");
		break;
	case NSF9FunctionKey:
		ge->key.sym = GIIK_F9;
		fprintf(stderr, "symbol or label: NSF9FunctionKey - GIIK_F9\n");
		break;
	case NSF10FunctionKey:
		ge->key.sym = GIIK_F10;
		fprintf(stderr, "symbol or label: NSF10FunctionKey - GIIK_F10\n");
		break;
	case NSF11FunctionKey:
		ge->key.sym = GIIK_F11;
		fprintf(stderr, "symbol or label: NSF11FunctionKey - GIIK_F11\n");
		break;
	case NSF12FunctionKey:
		ge->key.sym = GIIK_F12;
		fprintf(stderr, "symbol or label: NSF12FunctionKey - GIIK_F12\n");
		break;
	case NSF13FunctionKey:
		ge->key.sym = GIIK_F13;
		fprintf(stderr, "symbol or label: NSF13FunctionKey - GIIK_F13\n");
		break;
	case NSF14FunctionKey:
		ge->key.sym = GIIK_F14;
		fprintf(stderr, "symbol or label: NSF14FunctionKey - GIIK_F14\n");
		break;
	case NSF15FunctionKey:
		ge->key.sym = GIIK_F15;
		fprintf(stderr, "symbol or label: NSF15FunctionKey - GIIK_F15\n");
		break;
	case NSF16FunctionKey:
		ge->key.sym = GIIK_F16;
		fprintf(stderr, "symbol or label: NSF16FunctionKey - GIIK_F16\n");
		break;
	case NSF17FunctionKey:
		ge->key.sym = GIIK_F17;
		fprintf(stderr, "symbol or label: NSF17FunctionKey - GIIK_F17\n");
		break;
	case NSF18FunctionKey:
		ge->key.sym = GIIK_F18;
		fprintf(stderr, "symbol or label: NSF18FunctionKey - GIIK_F18\n");
		break;
	case NSF19FunctionKey:
		ge->key.sym = GIIK_F19;
		fprintf(stderr, "symbol or label: NSF19FunctionKey - GIIK_F19\n");
		break;
	case NSF20FunctionKey:
		ge->key.sym = GIIK_F20;
		fprintf(stderr, "symbol or label: NSF20FunctionKey - GIIK_F20\n");
		break;
	case NSF21FunctionKey:
		ge->key.sym = GIIK_F21;
		fprintf(stderr, "symbol or label: NSF21FunctionKey - GIIK_F21\n");
		break;
	case NSF22FunctionKey:
		ge->key.sym = GIIK_F22;
		fprintf(stderr, "symbol or label: NSF22FunctionKey - GIIK_F22\n");
		break;
	case NSF23FunctionKey:
		ge->key.sym = GIIK_F23;
		fprintf(stderr, "symbol or label: NSF23FunctionKey - GIIK_F23\n");
		break;
	case NSF24FunctionKey:
		ge->key.sym = GIIK_F24;
		fprintf(stderr, "symbol or label: NSF24FunctionKey - GIIK_F24\n");
		break;
	case NSF25FunctionKey:
		ge->key.sym = GIIK_F25;
		fprintf(stderr, "symbol or label: NSF25FunctionKey - GIIK_F25\n");
		break;
	case NSF26FunctionKey:
		ge->key.sym = GIIK_F26;
		fprintf(stderr, "symbol or label: NSF26FunctionKey - GIIK_F26\n");
		break;
	case NSF27FunctionKey:
		ge->key.sym = GIIK_F27;
		fprintf(stderr, "symbol or label: NSF27FunctionKey - GIIK_F27\n");
		break;
	case NSF28FunctionKey:
		ge->key.sym = GIIK_F28;
		fprintf(stderr, "symbol or label: NSF28FunctionKey - GIIK_F28\n");
		break;
	case NSF29FunctionKey:
		ge->key.sym = GIIK_F29;
		fprintf(stderr, "symbol or label: NSF29FunctionKey - GIIK_F29\n");
		break;
	case NSF30FunctionKey:
		ge->key.sym = GIIK_F30;
		fprintf(stderr, "symbol or label: NSF30FunctionKey - GIIK_F30\n");
		break;
	case NSF31FunctionKey:
		ge->key.sym = GIIK_F31;
		fprintf(stderr, "symbol or label: NSF31FunctionKey - GIIK_F31\n");
		break;
	case NSF32FunctionKey:
		ge->key.sym = GIIK_F32;
		fprintf(stderr, "symbol or label: NSF32FunctionKey - GIIK_F32\n");
		break;
	case NSF33FunctionKey:
		ge->key.sym = GIIK_F33;
		fprintf(stderr, "symbol or label: NSF33FunctionKey - GIIK_F33\n");
		break;
	case NSF34FunctionKey:
		ge->key.sym = GIIK_F34;
		fprintf(stderr, "symbol or label: NSF34FunctionKey - GIIK_F34\n");
		break;
	case NSF35FunctionKey:
		ge->key.sym = GIIK_F35;
		fprintf(stderr, "symbol or label: NSF35FunctionKey - GIIK_F35\n");
		break;

	case NSInsertFunctionKey:
		ge->key.sym = GIIK_Insert;
		fprintf(stderr, "symbol or label: NSInsertFunctionKey - GIIK_Insert\n");
		break;
	case NSDeleteFunctionKey:
		ge->key.sym = GIIK_Delete;
		fprintf(stderr, "symbol or label: NSDeleteFunctionKey - GIIK_Delete\n");
		break;
	case NSHomeFunctionKey:
		ge->key.sym = GIIK_Home;
		fprintf(stderr, "symbol or label: NSHomeFunctionKey - GIIK_Home\n");
		break;
	case NSBeginFunctionKey:
		ge->key.sym = GIIK_Begin;
		fprintf(stderr, "symbol or label: NSBeginFunctionKey - GIIK_Begin\n");
		break;
	case NSEndFunctionKey:
		ge->key.sym = GIIK_End;
		fprintf(stderr, "symbol or label: NSEndFunctionKey - GIIK_End\n");
		break;
	case NSPageUpFunctionKey:
		ge->key.sym = GIIK_PageUp;
		fprintf(stderr, "symbol or label: NSPageUpFunctionKey - GIIK_PageUp\n");
		break;
	case NSPageDownFunctionKey:
		ge->key.sym = GIIK_PageDown;
		fprintf(stderr, "symbol or label: NSPageDownFunctionKey - GIIK_PageDown\n");
		break;
	case NSPrintScreenFunctionKey:
		ge->key.sym = GIIK_PrintScreen;
		fprintf(stderr, "symbol or label: NSPrintScreenFunctionKey - GIIK_PrintScreen\n");
		break;
	case NSPauseFunctionKey:
		ge->key.sym = GIIK_Pause;
		fprintf(stderr, "symbol or label: NSPauseFunctionKey - GIIK_Pause\n");
		break;
	case NSSysReqFunctionKey:
		ge->key.sym = GIIK_SysRq;
		fprintf(stderr, "symbol or label: NSSysReqFunctionKey - GIIK_SysRq\n");
		break;
	case NSBreakFunctionKey:
		ge->key.sym = GIIK_Break;
		fprintf(stderr, "symbol or label: NSBreakFunctionKey - GIIK_Break\n");
		break;
	case NSResetFunctionKey:
		fprintf(stderr, "NSResetFunctionKey not supported!\n");
		fprintf(stderr, "symbol or label: NSResetFunctionKey -\n");
		break;
	case NSStopFunctionKey:
		ge->key.sym = GIIK_Stop;
		fprintf(stderr, "symbol or label: NSStopFunctionKey - GIIK_Stop\n");
		break;
	case NSMenuFunctionKey:
		ge->key.sym = GIIK_Menu;
		fprintf(stderr, "symbol or label: NSMenuFunctionKey - GIIK_Menu\n");
		break;
	case NSUserFunctionKey:
		fprintf(stderr, "NSUserFunctionKey not supported!\n");
		fprintf(stderr, "symbol or label: NSUserFunctionKey -\n");
		break;
	case NSSystemFunctionKey:
		fprintf(stderr, "NSSystemFunctionKey not supported!\n");
		fprintf(stderr, "symbol or label: NSSystemFunctionKey -\n");
		break;
	case NSPrintFunctionKey:
		fprintf(stderr, "NSPrintFunctionKey not supported!\n");
		fprintf(stderr, "symbol or label: NSPrintFunctionKey -\n");
		break;
	case NSClearLineFunctionKey:
		ge->key.sym = GIIK_Clear;
		fprintf(stderr, "symbol or label: NSClearLineFunctionKey - GIIK_Clear\n");
		break;
	case NSClearDisplayFunctionKey:
		ge->key.sym = GIIK_Clear;
		fprintf(stderr, "symbol or label: NSClearDisplayFunctionKey - GIIK_Clear\n");
		break;
	case NSInsertLineFunctionKey:
		ge->key.sym = GIIK_Insert;
		fprintf(stderr, "symbol or label: NSInsertLineFunctionKey - GIIK_Insert\n");
		break;
	case NSDeleteLineFunctionKey:
		ge->key.sym = GIIK_Delete;
		fprintf(stderr, "symbol or label: NSDeleteLineFunctionKey - GIIK_Delete\n");
		break;
	case NSInsertCharFunctionKey:
		ge->key.sym = GIIK_Insert;
		fprintf(stderr, "symbol or label: NSInsertCharFunctionKey - GIIK_Insert\n");
		break;
	case NSDeleteCharFunctionKey:
		ge->key.sym = GIIK_Delete;
		fprintf(stderr, "symbol or label: NSDeleteCharFunctionKey - GIIK_Delete\n");
		break;
	case NSPrevFunctionKey:
		ge->key.sym = GIIK_Prior;
		fprintf(stderr, "symbol or label: NSPrevFunctionKey - GIIK_Prior\n");
		break;
	case NSNextFunctionKey:
		ge->key.sym = GIIK_Next;
		fprintf(stderr, "symbol or label: NSNextFunctionKey - GIIK_Next\n");
		break;
	case NSSelectFunctionKey:
		ge->key.sym = GIIK_Select;
		fprintf(stderr, "symbol or label: NSSelectFunctionKey - GIIK_Select\n");
		break;
	case NSExecuteFunctionKey:
		ge->key.sym = GIIK_Execute;
		fprintf(stderr, "symbol or label: NSExecuteFunctionKey - GIIK_Execute\n");
		break;
	case NSUndoFunctionKey:
		ge->key.sym = GIIK_Undo;
		fprintf(stderr, "symbol or label: NSUndoFunctionKey - GIIK_Undo\n");
		break;
	case NSRedoFunctionKey:
		ge->key.sym = GIIK_Redo;
		fprintf(stderr, "symbol or label: NSRedoFunctionKey - GIIK_Redo\n");
		break;
	case NSFindFunctionKey:
		ge->key.sym = GIIK_Find;
		fprintf(stderr, "symbol or label: NSFindFunctionKey - GIIK_Find\n");
		break;
	case NSHelpFunctionKey:
		ge->key.sym = GIIK_Help;
		fprintf(stderr, "symbol or label: NSHelpFunctionKey - GIIK_Help\n");
		break;
	case NSModeSwitchFunctionKey:
		ge->key.sym = GIIK_ModeSwitch;
		fprintf(stderr, "symbol or label: NSModeSwitchFunctionKey - GIIK_ModeSwitch\n");
		break;


	/* labels */
	case NSScrollLockFunctionKey:
		ge->key.label = GIIK_ScrollLock;
		fprintf(stderr, "label: NSScrollLockFunctionKey - GIIK_ScrollLock\n");
		break;


	default:
		fprintf(stderr, "symbol or label: unhandled symbol or label: %i\n",
			[ chars intValue ]);
		break;
	}	/* switch */



	switch (sym) {

	/* keys on numeric keypad */
	case NSSymKeypad_1:
		ge->key.sym = GIIK_P1;
		fprintf(stderr, "symbol: NSSymKeypad_1 - GIIK_P1\n");
		break;
	case NSSymKeypad_2:
		ge->key.sym = GIIK_P2;
		fprintf(stderr, "symbol: NSSymKeypad_2 - GIIK_P2\n");
		break;
	case NSSymKeypad_3:
		ge->key.sym = GIIK_P3;
		fprintf(stderr, "symbol: NSSymKeypad_3 - GIIK_P3\n");
		break;
	case NSSymKeypad_4:
		ge->key.sym = GIIK_P4;
		fprintf(stderr, "symbol: NSSymKeypad_4 - GIIK_P4\n");
		break;
	case NSSymKeypad_5:
		ge->key.sym = GIIK_P5;
		fprintf(stderr, "symbol: NSSymKeypad_5 - GIIK_P5\n");
		break;
	case NSSymKeypad_6:
		ge->key.sym = GIIK_P6;
		fprintf(stderr, "symbol: NSSymKeypad_6 - GIIK_P6\n");
		break;
	case NSSymKeypad_7:
		ge->key.sym = GIIK_P7;
		fprintf(stderr, "symbol: NSSymKeypad_7 - GIIK_P7\n");
		break;
	case NSSymKeypad_8:
		ge->key.sym = GIIK_P8;
		fprintf(stderr, "symbol: NSSymKeypad_8 - GIIK_P8\n");
		break;
	case NSSymKeypad_9:
		ge->key.sym = GIIK_P9;
		fprintf(stderr, "symbol: NSSymKeypad_9 - GIIK_P9\n");
		break;
	case NSSymKeypad_0:
		ge->key.sym = GIIK_P0;
		fprintf(stderr, "symbol: NSSymKeypad_0 - GIIK_P0\n");
		break;

	case NSSymKeypadEquals:
		ge->key.sym = GIIK_PEqual;
		fprintf(stderr, "symbol: NSSymKeypadEquals - GIIK_PEqual\n");
		break;
	case NSSymKeypadDivide:
		ge->key.sym = GIIK_PSlash;
		fprintf(stderr, "symbol: NSSymKeypadDivide - GIIK_PSlash\n");
		break;
	case NSSymKeypadMultiply:
		ge->key.sym = GIIK_PAsterisk;
		fprintf(stderr, "symbol: NSSymKeypadMultiply - GIIK_PAsterik\n");
		break;
	case NSSymKeypadMinus:
		ge->key.sym = GIIK_PMinus;
		fprintf(stderr, "symbol: NSSymKeypadMinus - GIIK_PMinus\n");
		break;
	case NSSymKeypadPlus:
		ge->key.sym = GIIK_PPlus;
		fprintf(stderr, "symbol: NSSymKeypadPlus - GIIK_PPlus\n");
		break;
	case NSSymKeypadEnter:
		ge->key.sym = GIIK_PEnter;
		fprintf(stderr, "symbol: NSSymKeypadEnter - GIIK_PEnter\n");
		break;
	case NSSymKeypadPeriod:
		ge->key.sym = GIIK_PTab;	/* Is this corect? */
		fprintf(stderr, "symbol: NSSymKeypadPeriod - GIIK_PTab\n");
		break;


	/* modifier keys */
	case NSSymShiftL:
		ge->key.sym = GIIK_Shift;
		fprintf(stderr, "symbol: NSSymShiftL - GIIK_Shift\n");
		break;
	case NSSymControlL:
		ge->key.sym = GIIK_Ctrl;
		fprintf(stderr, "symbol: NSSymControlL - GIIK_Ctrl\n");
		break;
	case NSSymAltL:
		ge->key.sym = GIIK_Alt;
		fprintf(stderr, "symbol: NSSymAltL - GIIK_Alt\n");
		break;
	case NSSymMetaL:
		ge->key.sym = GIIK_Meta;
		fprintf(stderr, "symbol: NSSymMetaL - GIIK_Meta\n");
		break;
	case NSSymCapslock:
		ge->key.sym = GIIK_Caps;
		fprintf(stderr, "symbol: NSSymCapsLock - GIIK_Caps\n");
		break;
	case NSSymNumLock:
		ge->key.sym = GIIK_Num;
		fprintf(stderr, "symbol: NSSymNumLock - GIIK_Num\n");
		break;
	case NSSymScrollLock:
		ge->key.sym = GIIK_Scroll;
		fprintf(stderr, "symbol: NSSymScrollLock - GIIK_Scroll\n");
		break;

	default:
		if (sym < 0x80) {
			ge->key.sym = sym;
			fprintf(stderr, "symbol (< 0x80) = 0x%X\n", sym);
		} else {
			return emZero;
		}	/* if */
		break;
	}	/* switch */

	/* Event types */
	switch (type) {
	case NSKeyDown:
		if ([ ev isARepeat ]) {
			ge->any.type = evKeyRepeat;
			fprintf(stderr, "key event type: evKeyRepeat\n");
		} else {
			ge->any.type = evKeyPress;
			fprintf(stderr, "key event type: evKeyPress\n");
		}	/* if */
		break;

	case NSKeyUp:
		ge->any.type = evKeyRelease;
		fprintf(stderr, "key event type: evKeyUp\n");
		break;

	default:
		fprintf(stderr, "non-handled event type: 0x%X\n",
			type);
		ge->any.type = emZero;
		break;
	}	/* switch */

	/* FIXME: Do labels */
	ge->key.label = ge->key.sym;


	if (GII_KTYP(ge->key.sym) == GII_KT_MOD) {
		ge->key.sym &= ~GII_KM_RIGHT;
	} else if (GII_KTYP(ge->key.sym) == GII_KT_PAD) {
		if (GII_KVAL(ge->key.sym) < 0x80) {
			ge->key.sym = GII_KVAL(ge->key.sym);
		}	/* if */
	} else if (GII_KTYP(ge->key.sym) == GII_KT_DEAD) {
		ge->key.sym = GIIK_VOID;
	}	/* if */

	_giiEvQueueAdd(inp, ge);

	return (1 << ge->any.type);
}	/* GII_cocoa_handle_kbd */


static gii_event_mask GII_cocoa_handle_ptr(gii_input *inp, gii_event *ge,
					NSEvent *ev)
{
#if 0
	NSPoint *point;
#endif

	switch ([ ev type ]) {
	case NSMouseMoved:

		fprintf(stderr, "mouse move event\n");

		_giiEventBlank(ge, sizeof(gii_pmove_event));

#if 0
		/* This is the code to report absolute
		 * mouse movement events.
		 * Unfortunately, I see currently no
		 * way how to distinguish between
		 * relative and absolute movements.
		 * I leave this code, in case that
		 * this is possible someday...
		 */

		ge->any.type = evPtrAbsolute;

		point = [ ev mouseLocation ];

		ge->any.origin = inp->origin;
		ge->any.size = sizeof(gii_pmove_event);
		ge->pmove.x = (int32_t)point->x;
		ge->pmove.y = (int32_t)point->y;
#endif

		ge->any.type = evPtrRelative;

		ge->any.origin = inp->origin;
		ge->any.size = sizeof(gii_pmove_event);
		ge->pmove.x = (int32_t)[ ev deltaX ];
		ge->pmove.y = (int32_t)[ ev deltaY ];
		ge->pmove.z = (int32_t)[ ev deltaZ ];

		_giiEvQueueAdd(inp, ge);
		break;

	case NSScrollWheel:
		/* pointer move event */
		_giiEventBlank(ge, sizeof(gii_pmove_event));

		fprintf(stderr, "mouse scrollwheel event\n");

		ge->any.type = evPtrRelative;

		ge->any.origin = inp->origin;
		ge->any.size = sizeof(gii_pmove_event);
		ge->pmove.x = [ ev deltaX ];
		ge->pmove.y = [ ev deltaY ];
		ge->pmove.z = [ ev deltaZ ];
		ge->pmove.wheel = [ ev deltaZ ];

		_giiEvQueueAdd(inp, ge);
		break;

	case NSLeftMouseDown:
	case NSRightMouseDown:
	case NSOtherMouseDown:

		fprintf(stderr, "mouse button (%i) down event\n", [ ev buttonNumber]);

		_giiEventBlank(ge, sizeof(gii_pbutton_event));
		ge->any.type = evPtrButtonPress;

		ge->any.origin = inp->origin;
		ge->any.size = sizeof(gii_pbutton_event);

		ge->pbutton.button = [ ev buttonNumber ];
		_giiEvQueueAdd(inp, ge);
		break;

	case NSLeftMouseUp:
	case NSRightMouseUp:
	case NSOtherMouseUp:

		fprintf(stderr, "mouse button (%i) up event\n", [ ev buttonNumber]);

		_giiEventBlank(ge, sizeof(gii_pbutton_event));
		ge->any.type = evPtrButtonRelease;

		ge->any.origin = inp->origin;
		ge->any.size = sizeof(gii_pbutton_event);

		ge->pbutton.button = [ ev buttonNumber ];
		_giiEvQueueAdd(inp, ge);
		break;

	default:
		fprintf(stderr, "unknown mouse event = %i\n", [ev type]);

		return emZero;
	}	/* switch */

	return (1 << ge->any.type);
}	/* GGI_cocoa_handle_ptr */



static gii_event_mask GII_cocoa_poll(gii_input *inp, void *arg)
{
	cocoa_priv *priv;
	NSEvent *ev;
	NSEventType type;
	gii_event_mask em;
	gii_event ge;

	priv = COCOA_PRIV(inp);

	em = emZero;
	for (;;) {
		ev = CocoaNextEvent(priv);
		if (ev == nil) break;

		type = [ ev type ];

		switch (type) {
		case NSKeyDown:
		case NSKeyUp:
		case NSFlagsChanged:
			em |= GII_cocoa_handle_key(inp, &ge, ev);
			break;

		case NSLeftMouseDown:
		case NSLeftMouseUp:
		case NSLeftMouseDragged:
		case NSRightMouseDown:
		case NSRightMouseUp:
		case NSRightMouseDragged:
		case NSOtherMouseDown:
		case NSOtherMouseUp:
		case NSOtherMouseDragged:
		case NSMouseMoved:
		case NSScrollWheel:
			em |= GII_cocoa_handle_ptr(inp, &ge, ev);
			break;

		case NSMouseEntered:
		case NSMouseExited:
		case NSCursorUpdate:
			em |= emZero;
			break;

		case NSAppKitDefined:
			switch ([ ev subtype ]) {
			case NSWindowExposedEventType:
				fprintf(stderr, "NSAppKitDefined event: NSWindowExposedEventType\n");
				break;

			case NSApplicationActivatedEventType:
				fprintf(stderr, "NSAppKitDefined event: NSApplicationActivatedEventType\n");
				break;

			case NSApplicationDeactivatedEventType:
				fprintf(stderr, "NSAppKitDefined event: NSApplicationDeactivatedEventType\n");
				break;

			case NSWindowMovedEventType:
				fprintf(stderr, "NSAppKitDefined event: NSWindowMovedEventType\n");
				break;

			case NSScreenChangedEventType:
				fprintf(stderr, "NSAppKitDefined event: NSScreenChangedEventType\n");
				break;

			case NSAWTEventType:
				fprintf(stderr, "NSAppKitDefined event: NSAWTEventType\n");
				break;

			default:
				fprintf(stderr, "unknown NSAppKitDefined event: %i\n",
					[ ev subtype ]);
				break;

			}	/* switch */
			break;

		case NSSystemDefined:
			switch ([ ev subtype ]) {
			case NSPowerOffEventType:
				fprintf(stderr, "NSSystemDefined event: NSPowerOffEventType\n");
				break;

			default:
				fprintf(stderr, "unkown NSSystemDefined event: %i\n",
					[ ev subtype ]);
				break;					
			}	/* switch */
			break;

		default:
			fprintf(stderr, "non-handled event: %i\n", type);
			break;

		}	/* switch */
	}	/* for */

	return em;
}	/* GII_cocoa_poll */


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
}	/* send_devinfo */


static int GII_cocoa_send_event(gii_input *inp, gii_event *ev)
{
	if (ev->any.target != inp->origin &&
	    ev->any.target != GII_EV_TARGET_ALL)
	{
		return GGI_EEVNOTARGET;
	}	/* if */
	
	if (ev->any.type != evCommand) {	
		return GGI_EEVUNKNOWN;
	}	/* if */
	
	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {	
		send_devinfo(inp);
		return 0;
	}	/* if */
	
	return GGI_EEVUNKNOWN;
}	/* GII_cocoa_send_event */


static int GII_cocoa_close(gii_input *inp)
{
	cocoa_priv *priv = COCOA_PRIV(inp);

	CocoaExit(priv);
	free(priv);

	DPRINT_MISC("cocoa: exit OK.\n");

	return 0;
}	/* GII_cocoa_close */


EXPORTFUNC int GIIdl_cocoa(gii_input *inp, const char *args, void *argptr);

int GIIdl_cocoa(gii_input *inp, const char *args, void *argptr)
{
	int rc = 0;
	gii_inputcocoa_arg *cocoaarg = argptr;
	cocoa_priv *priv;

	DPRINT_MISC("cocoa input starting. (args=%s,argptr=%p)\n",
		    args, argptr);

	priv = malloc(sizeof(cocoa_priv));
	if (priv == NULL) {
		rc = GGI_ENOMEM;
		goto err0;
	}	/* if */

	priv->GIIApp = cocoaarg->GGIApp;
	priv->window = cocoaarg->window;

	if (_giiRegisterDevice(inp, &devinfo, NULL) == 0) {
		rc = GGI_ENOMEM;
		goto err1;
	}	/* if */

	if (GGI_OK != CocoaInit(priv)) {
		rc = GGI_ENODEVICE;
		goto err2;
	}	/* if */

	inp->GIIsendevent = GII_cocoa_send_event;
	inp->GIIeventpoll = GII_cocoa_poll;
	inp->GIIclose     = GII_cocoa_close;

	inp->targetcan = emKey | emPtrButton | emPtrRelative;
	inp->curreventmask = emKey | emPtrButton | emPtrRelative;

	inp->maxfd = 0;	/* We poll from an event queue - ouch! */
	inp->flags |= GII_FLAGS_HASPOLLED;
	inp->priv = priv;

	inp->GIIseteventmask(inp, inp->targetcan);
	send_devinfo(inp);


	if (GGI_OK != CocoaFinishLaunch(priv)) {
		rc = GGI_ENODEVICE;
		goto err2;
	}	/* if */

	DPRINT_MISC("cocoa input fully up\n");

	return GGI_OK;

err2:
err1:
	free(priv);
err0:
	return rc;
}	/* GIIdlinit */
