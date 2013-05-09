/* $Id$
******************************************************************************

   Quartz: Input driver

   Copyright (C) 2004 Christoph Egger

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

#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>
#include <ggi/gg.h>

#include "quartz.h"
#include "keysyms.h"


int GII_quartz_seteventmask(gii_input *inp, gii_event_mask evm)
{
	return 0;
}

int GII_quartz_geteventmask(gii_input *inp)
{
	return 0;
}

gii_event_mask GII_quartz_eventpoll(gii_input *inp, void *arg)
{
	gii_event_mask mask = 0;

	EventRef theEvent;
	EventTargetRef theTarget;
	OSStatus theErr;


	/* Get event */
	theTarget = GetEventDispatcherTarget();
	theErr = ReceiveNextEvent(0,0, kEventDurationNoWait, TRUE, &theEvent);
	if (theErr == noErr && theEvent != NULL) {
		mask = transEvent2Mask(theEvent);
		SendEventToEventTarget(theEvent, theTarget);
		ReleaseEvent(theEvent);
	}	/* if */

	return mask;
}	/* GII_quartz_eventpoll */




static int transKeyQuartz2GII(gii_event *ev, uint32_t macKeyCode, char macCharCodes, uint32_t modifiers)
{
	uint32_t symbol;
	uint32_t label;
	uint32_t button = macCharCodes;

	switch (macKeyCode) {
	case QUARTZ_F1:
		symbol = label = GIIK_F1;
		break;
	case QUARTZ_F2:
		symbol = label = GIIK_F2;
		break;
	case QUARTZ_F3:
		symbol = label = GIIK_F3;
		break;
	case QUARTZ_F4:
		symbol = label = GIIK_F4;
		break;
	case QUARTZ_F5:
		symbol = label = GIIK_F5;
		break;
	case QUARTZ_F6:
		symbol = label = GIIK_F6;
		break;
	case QUARTZ_F7:
		symbol = label = GIIK_F7;
		break;
	case QUARTZ_F8:
		symbol = label = GIIK_F8;
		break;
	case QUARTZ_F9:
		symbol = label = GIIK_F9;
		break;
	case QUARTZ_F10:
		symbol = label = GIIK_F10;
		break;
	case QUARTZ_F11:
		symbol = label = GIIK_F11;
		break;
	case QUARTZ_F12:
		symbol = label = GIIK_F12;
		break;

	case QUARTZ_KP_DIVIDE:
		symbol = label = GIIK_PSlash;
		break;
	case QUARTZ_KP_ENTER:
		symbol = label = GIIK_PEnter;
		break;
	case QUARTZ_KP_EQUALS:
		symbol = label = GIIK_PEqual;
		break;
	case QUARTZ_KP_MINUS:
		symbol = label = GIIK_PMinus;
		break;
	case QUARTZ_KP_MULTIPLY:
		symbol = label = GIIK_PAsterisk;
		break;
	case QUARTZ_KP_PERIOD:
		fprintf(stderr, "Pressed QUATRZ_KP_PERIOD key!\n");
		symbol = GIIUC_PeriodCentered;
		label = GIIK_P5;
		break;
	case QUARTZ_KP_PLUS:
		symbol = label = GIIK_PMinus;
		break;
	case QUARTZ_KP0:
		symbol = label = GIIK_P0;
		break;
	case QUARTZ_KP1:
		symbol = label = GIIK_P1;
		break;
	case QUARTZ_KP2:
		symbol = label = GIIK_P2;
		break;
	case QUARTZ_KP3:
		symbol = label = GIIK_P3;
		break;
	case QUARTZ_KP4:
		symbol = label = GIIK_P4;
		break;
	case QUARTZ_KP5:
		symbol = label = GIIK_P5;
		break;
	case QUARTZ_KP6:
		symbol = label = GIIK_P6;
		break;
	case QUARTZ_KP7:
		symbol = label = GIIK_P7;
		break;
	case QUARTZ_KP8:
		symbol = label = GIIK_P8;
		break;
	case QUARTZ_KP9:
		symbol = label = GIIK_P9;
		break;

	case QUARTZ_CAPSLOCK:
		symbol = label = GIIK_CapsLock;
		break;
	case QUARTZ_NUMLOCK:
		symbol = label = GIIK_NumLock;
		break;
	case QUARTZ_PAUSE:
		symbol = label = GIIK_Pause;
		break;
	case QUARTZ_POWER:
		symbol = label = GIIK_Power;
		break;

	case QUARTZ_PRINT:
		symbol = label = GIIK_PrintScreen;
		break;
	case QUARTZ_SCROLLOCK:
		symbol = label = GIIK_ScrollLock;
		break;

	case QUARTZ_BACKQUOTE:
		symbol = GIIUC_Circumflex;
		label = GIIUC_Apostrophe;
		break;
	case QUARTZ_DELETE:
		symbol = label = GIIUC_Delete;
		break;
	case QUARTZ_INSERT:
		symbol = label = GIIK_Insert;
		break;

	case QUARTZ_LSHIFT:
#if 0
	case QUARTZ_RSHIFT:
#endif
		symbol = label = GIIK_Shift;
		break;

	case QUARTZ_LCTRL:
#if 0
	case QUARTZ_RCTRL:
#endif
		symbol = label = GIIK_Ctrl;
		break;

	case QUARTZ_LALT:
#if 0
	case QUARTZ_RALT:
#endif
		symbol = label = GIIK_Alt;
		break;

	case QUARTZ_LMETA:
#if 0
	case QUARTZ_RMETA:
#endif
		symbol = label = GIIK_Meta;
		break;

	case QUARTZ_HOME:
		symbol = label = GIIK_Home;
		break;
	case QUARTZ_PAGEUP:
		symbol = label = GIIK_PageUp;
		break;
	case QUARTZ_END:
		symbol = label = GIIK_End;
		break;
	case QUARTZ_PAGEDOWN:
		symbol = label = GIIK_PageDown;
		break;
	case QUARTZ_LEFT:
		symbol = label = GIIK_Left;
		break;
	case QUARTZ_DOWN:
		symbol = label = GIIK_Down;
		break;
	case QUARTZ_RIGHT:
		symbol = label = GIIK_Right;
		break;
	case QUARTZ_UP:
		symbol = label = GIIK_Up;
		break;

	case QUARTZ_NOTEBOOK_ENTER:
		symbol = label = GIIK_Enter;
		break;
	case QUARTZ_NOTEBOOK_LEFT:
		symbol = label = GIIK_Left;
		break;
	case QUARTZ_NOTEBOOK_RIGHT:
		symbol = label = GIIK_Right;
		break;
	case QUARTZ_NOTEBOOK_DOWN:
		symbol = label = GIIK_Down;
		break;
	case QUARTZ_NOTEBOOK_UP:
		symbol = label = GIIK_Up;
		break;

	default:

		/* handle all ASCII codes as symbols */
		DPRINT_EVENTS("macKeyCode = %X, macCharCodes = %X\n",
			macKeyCode, macCharCodes);
		symbol = macCharCodes;
#if 0
		label = macKeyCode;
#else
		label = toupper((unsigned char)macCharCodes);
#endif
		break;
	}	/* switch */


	ev->key.sym = symbol;
	ev->key.label = label;
	ev->key.button = button;

	return 0;
}	/* transKeyQuartz2GII */




OSStatus DefaultWindowEventHandler(EventHandlerCallRef nextHandler,
			EventRef event, void *userData)
{
	OSStatus result = noErr;
	uint32_t eventClass = GetEventClass(event);
	uint32_t eventKind = GetEventKind(event);
	gii_input *inp = (gii_input *)userData;
	quartz_priv *priv = QUARTZ_PRIV(inp);

	result = CallNextEventHandler(nextHandler, event);

	switch (eventClass) {
	case kEventClassWindow:
		switch (eventKind) {
		case kEventWindowClosed:
			do {
				WindowRef window;

				DPRINT("Received kEventClassWindow::kEventWindowClosed\n");

				GetEventParameter(event, kEventParamDirectObject, typeWindowRef,
					NULL, sizeof(WindowRef), NULL, &window);

			} while (0);
			break;

		case kEventWindowBoundsChanging:
		case kEventWindowBoundsChanged:
			do {
				WindowRef window;
				Rect origRect, prevRect, curRect;

				DPRINT("Received kEventClassWindow::kEventWindowBoundsChanging\n");

				if (priv->resizefunc == NULL) break;

				GetEventParameter(event, kEventParamDirectObject, typeWindowRef,
					NULL, sizeof(WindowRef), NULL, &window);

				GetEventParameter(event, kEventParamOriginalBounds, typeQDRectangle,
					NULL, sizeof(Rect), NULL, &origRect);
				GetEventParameter(event, kEventParamPreviousBounds, typeQDRectangle,
					NULL, sizeof(Rect), NULL, &prevRect);
				GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle,
					NULL, sizeof(Rect), NULL, &curRect);

				priv->resizefunc(window, origRect, prevRect, curRect);
			} while (0);
			break;

		default:
			DPRINT("Received unknown event kind of kEventClassWindow: %i\n",
				eventKind);
			result = eventNotHandledErr;
			break;
		}	/* switch */
		break;

	default:
		DPRINT("Received unknown event class: %c%c%c%c\n",
			eventClass>>24,eventClass>>16,eventClass>>8,eventClass&0xff);
		result = eventNotHandledErr;
		break;
	}	/* switch */

	return result;
}	/* DefaultWindowEventHandler */


OSStatus DefaultApplicationEventHandler(EventHandlerCallRef nextHandler,
			EventRef event, void *userData)
{
	OSStatus result = noErr;
	uint32_t eventClass = GetEventClass(event);
	uint32_t eventKind = GetEventKind(event);
	gii_input *inp = (gii_input *)userData;
	quartz_priv *priv = QUARTZ_PRIV(inp);
	gii_event ev;
	static int ignore_mouse = 0;

	result = CallNextEventHandler(nextHandler, event);

	switch (eventClass) {
	case kEventClassMouse:
		switch (eventKind) {
		case kEventMouseDown:
			do {
				EventMouseButton button;

				if (ignore_mouse) break;
				DPRINT("Received kEventClassMouse::kEventMouseDown\n");

				GetEventParameter(event, kEventParamMouseButton, typeMouseButton,
						0, sizeof(EventMouseButton), 0, &button);

				DPRINT("Pressed mouse button: %i\n", button);

				_giiEventBlank(&ev, sizeof(gii_pbutton_event));
				ev.any.size = sizeof(gii_pbutton_event);
				ev.any.type = evPtrButtonPress;
				ev.any.origin = priv->origin[QZ_DEV_MOUSE];
				ev.pbutton.button = button;

				_giiEvQueueAdd(inp, &ev);
			} while (0);
			break;

		case kEventMouseUp:
			do {
				EventMouseButton button;

				if (ignore_mouse) break;
				DPRINT("Received kEventClassMouse::kEventMouseUp\n");

				GetEventParameter(event, kEventParamMouseButton, typeMouseButton,
						0, sizeof(EventMouseButton), 0, &button);

				DPRINT("Released mouse button: %i\n", button);

				_giiEventBlank(&ev, sizeof(gii_pbutton_event));
				ev.any.size = sizeof(gii_pbutton_event);
				ev.any.type = evPtrButtonRelease;
				ev.any.origin = priv->origin[QZ_DEV_MOUSE];
				ev.pbutton.button = button;

				_giiEvQueueAdd(inp, &ev);
			} while (0);
			break;

		case kEventMouseDragged:
			do {
				HIPoint mouse_delta;
				Point mouse_pos;
				HIPoint mouse_winpos;
				EventMouseButton button;

				DPRINT("Received kEventClassMouse::kEventMouseDragged\n");

				GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
						0, sizeof(Point), 0, &mouse_pos);
				GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint,
						0, sizeof(HIPoint), 0, &mouse_winpos);
				GetEventParameter(event, kEventParamMouseDelta, typeHIPoint,
						0, sizeof(HIPoint), 0, &mouse_delta);
				GetEventParameter(event, kEventParamMouseButton, typeMouseButton,
						0, sizeof(EventMouseButton), 0, &button);


				DPRINT("Mouse position: %i,%i\n", mouse_pos.h, mouse_pos.v);
				DPRINT("Mouse win position: %f,%f\n", mouse_winpos.x, mouse_winpos.y);
				DPRINT("Mouse delta: %f,%f\n", mouse_delta.x, mouse_delta.y);
				DPRINT("Released mouse button: %i\n", button);


				_giiEventBlank(&ev, sizeof(gii_pmove_event));
				ev.any.size = sizeof(gii_pmove_event);
				ev.any.origin = priv->origin[QZ_DEV_MOUSE];
#if 0
				ev.any.type = evPtrRelative;
				ev.pmove.x = (int)mouse_delta.x;
				ev.pmove.y = (int)mouse_delta.y;
#else
				ev.any.type = evPtrAbsolute;
				ev.pmove.x = (int)mouse_winpos.x;
				ev.pmove.y = (int)mouse_winpos.y;
#endif
				_giiEvQueueAdd(inp, &ev);


				_giiEventBlank(&ev, sizeof(gii_pbutton_event));
				ev.any.size = sizeof(gii_pbutton_event);
				ev.any.type = evPtrButtonRelease;
				ev.any.origin = priv->origin[QZ_DEV_MOUSE];
				ev.pbutton.button = button;

				_giiEvQueueAdd(inp, &ev);
			} while (0);
			break;

		case kEventMouseMoved:
			do {
				HIPoint mouse_delta;
				Point mouse_pos;
				HIPoint mouse_winpos;

				if (ignore_mouse) break;
				DPRINT("Received kEventClassMouse::kEventMouseMoved\n");

				GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
						0, sizeof(Point), 0, &mouse_pos);
				GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint,
						0, sizeof(HIPoint), 0, &mouse_winpos);
				GetEventParameter(event, kEventParamMouseDelta, typeHIPoint,
						0, sizeof(HIPoint), 0, &mouse_delta);

				DPRINT("Mouse position: %i,%i\n", mouse_pos.h, mouse_pos.v);
				DPRINT("Mouse win position: %f,%f\n", mouse_winpos.x, mouse_winpos.y);
				DPRINT("Mouse delta: %f,%f\n", mouse_delta.x, mouse_delta.y);

				_giiEventBlank(&ev, sizeof(gii_pmove_event));
				ev.any.size = sizeof(gii_pmove_event);
				ev.any.origin = priv->origin[QZ_DEV_MOUSE];
#if 0
				ev.any.type = evPtrRelative;
				ev.pmove.x = (int)mouse_delta.x;
				ev.pmove.y = (int)mouse_delta.y;
#else
				ev.any.type = evPtrAbsolute;
				ev.pmove.x = (int)mouse_winpos.x;
				ev.pmove.y = (int)mouse_winpos.y;
#endif

				_giiEvQueueAdd(inp, &ev);
			} while (0);
			break;

		case kEventMouseEntered:
			do {
				Point mouse_pos;
				HIPoint mouse_winpos;

				ignore_mouse = 0;
				DPRINT("Received kEventClassMouse::kEventMouseEntered\n");

				GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
						0, sizeof(Point), 0, &mouse_pos);
				GetEventParameter(event, kEventParamWindowMouseLocation, typeHIPoint,
						0, sizeof(HIPoint), 0, &mouse_winpos);

				DPRINT("Mouse position: %i,%i\n", mouse_pos.h, mouse_pos.v);
				DPRINT("Mouse win position: %f,%f\n", mouse_winpos.x, mouse_winpos.y);

				_giiEventBlank(&ev, sizeof(gii_pmove_event));
				ev.any.size = sizeof(gii_pmove_event);
				ev.any.origin = priv->origin[QZ_DEV_MOUSE];

				ev.any.type = evPtrAbsolute;
				ev.pmove.x = (int)mouse_winpos.x;
				ev.pmove.y = (int)mouse_winpos.y;

				_giiEvQueueAdd(inp, &ev);
			} while (0);
			break;

		case kEventMouseExited:
			DPRINT("Received kEventClassMouse::kEventMouseExited\n");
			ignore_mouse = 1;
			break;

		case kEventMouseWheelMoved:
			do {
				int32_t wheel;
				EventMouseWheelAxis wheel_axis;

				if (ignore_mouse) break;
				DPRINT("Received kEventClassMouse::kEventMouseWheelMoved\n");

				GetEventParameter(event, kEventParamMouseWheelDelta,
						typeSInt32, 0, sizeof(int), 0, &wheel);
				GetEventParameter(event, kEventParamMouseWheelAxis,
						kEventParamTabletPointRec, 0,
						sizeof(EventMouseWheelAxis), 0, &wheel_axis);

				DPRINT("Wheel moved: %i\n", wheel);

				_giiEventBlank(&ev, sizeof(gii_pmove_event));
				ev.any.size = sizeof(gii_pmove_event);
				ev.any.type = evPtrRelative;
				ev.any.origin = priv->origin[QZ_DEV_MOUSE];
				ev.pmove.wheel = wheel;

				_giiEvQueueAdd(inp, &ev);
			} while (0);
			break;

		default:
			DPRINT("Received unknown event kind of kEventClassMouse: %i\n",
				eventKind);
			if (ignore_mouse) break;
			result = eventNotHandledErr;
			return result;
		}
		break;

	case kEventClassKeyboard:
		do {
			char macCharCodes;
			uint32_t macKeyCode;
			uint32_t macKeyModifiers;

			GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar,
					NULL, sizeof(macCharCodes), NULL, &macCharCodes);
			GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL,
					sizeof(macKeyCode), NULL, &macKeyCode);
			GetEventParameter(event, kEventParamKeyModifiers, typeUInt32,
					NULL, sizeof(macKeyModifiers), NULL, &macKeyModifiers);

			_giiEventBlank(&ev, sizeof(gii_key_event));
			ev.any.size = sizeof(gii_key_event);
			ev.any.origin = priv->origin[QZ_DEV_KEY];
			ev.key.modifiers = priv->modifiers;

			switch (eventKind) {
			case kEventRawKeyDown:
				DPRINT("Received kEventClassKeyboard::kEventRawKeyDown\n");

				ev.any.type = evKeyPress;
				transKeyQuartz2GII(&ev, macKeyCode, macCharCodes, priv->modifiers);

				_giiEvQueueAdd(inp, &ev);
				break;

			case kEventRawKeyUp:
				DPRINT("Received kEventClassKeyboard::kEventRawKeyUp\n");

				ev.any.type = evKeyRelease;
				transKeyQuartz2GII(&ev, macKeyCode, macCharCodes, priv->modifiers);

				_giiEvQueueAdd(inp, &ev);
				break;

			case kEventRawKeyRepeat:
				DPRINT("Received kEventClassKeyboard::kEventRawKeyRepeat\n");

				ev.any.type = evKeyRepeat;
				transKeyQuartz2GII(&ev, macKeyCode, macCharCodes, priv->modifiers);

				_giiEvQueueAdd(inp, &ev);
				break;

			case kEventRawKeyModifiersChanged:
				DPRINT("Received kEventClassKeyboard::kEventRawKeyModifiersChanged\n");

				if (macKeyModifiers & kEventKeyModifierNumLockBit) {
					priv->modifiers |= GII_MOD_NUM;
				} else {
					priv->modifiers |= ~GII_MOD_NUM;
				}
#if 0
				if (macKeyModifiers & kEventKeyModifierFnBit) {
					priv->modifiers |= GII_MOD_FN;
				} else {
					priv->modifiers |= ~GII_MOD_FN;
				}
#endif
				if (macKeyModifiers & cmdKeyBit) {
					priv->modifiers |= GII_MOD_META;
				} else {
					priv->modifiers |= ~GII_MOD_META;
				}
				if ((macKeyModifiers & shiftKeyBit)
				  || (macKeyModifiers & rightShiftKeyBit))
				{
					priv->modifiers |= GII_MOD_SHIFT;
				} else {
					priv->modifiers |= ~GII_MOD_SHIFT;
				}
				if (macKeyModifiers & alphaLockBit) {
					priv->modifiers |= GII_MOD_CAPS;
				} else {
					priv->modifiers |= ~GII_MOD_CAPS;
				}
				if ((macKeyModifiers & optionKeyBit)
				  || (macKeyModifiers & rightOptionKeyBit))
				{
					priv->modifiers |= GII_MOD_ALT;
				} else {
					priv->modifiers |= ~GII_MOD_ALT;
				}
				if ((macKeyModifiers & controlKeyBit)
				  || (macKeyModifiers & rightControlKeyBit))
				{
					priv->modifiers |= GII_MOD_CTRL;
				} else {
					priv->modifiers |= ~GII_MOD_CTRL;
				}
				break;

			default:
				DPRINT("Received unknown event kind of kEventClassKeyboard: %i\n",
					eventKind);
				result = eventNotHandledErr;
				return result;
			}
		} while(0);
		break;

	default:
		DPRINT("Received unknown event class: %c%c%c%c\n",
			eventClass>>24,eventClass>>16,eventClass>>8,eventClass&0xff);
		result = eventNotHandledErr;
		break;
	}

	return result;
}	/* DefaultApplicationEventHandler */

