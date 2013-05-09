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
#include "quartz.h"


static const EventTypeSpec defaultWindowEvents[] = {
	{ kEventClassWindow, kEventWindowClosed },
	{ kEventClassWindow, kEventWindowBoundsChanging },
};


static const EventTypeSpec defaultApplicationEvents[] = {
	{ kEventClassMouse, kEventMouseDown },
	{ kEventClassMouse, kEventMouseUp },
	{ kEventClassMouse, kEventMouseMoved },
	{ kEventClassMouse, kEventMouseEntered },
	{ kEventClassMouse, kEventMouseExited },
	{ kEventClassMouse, kEventMouseWheelMoved },
	{ kEventClassMouse, kEventMouseDragged },

	{ kEventClassKeyboard, kEventRawKeyDown },
	{ kEventClassKeyboard, kEventRawKeyUp },
	{ kEventClassKeyboard, kEventRawKeyRepeat },
	{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
};

#define NUM_EVENTS(ev)	(sizeof(ev) / sizeof(EventTypeSpec))


int QuartzInit(quartz_priv *priv)
{
	LIB_ASSERT(priv != NULL, "invalid argument");


	return GGI_OK;
}


int QuartzFinishLaunch(gii_input *inp)
{
	quartz_priv *priv = QUARTZ_PRIV(inp);

	LIB_ASSERT(priv != NULL, "invalid argument");

	if (priv->theWindow != NULL) {
		return QuartzInitEventHandler(inp);
	}

	return GGI_OK;
}


int QuartzExit(quartz_priv *priv)
{
	LIB_ASSERT(priv != NULL, "invalid argument");

	return GGI_OK;
}


int QuartzUninitEventHandler(gii_input *inp)
{
	quartz_priv *priv = QUARTZ_PRIV(inp);
	OSStatus err;

	LIB_ASSERT(priv != NULL, "invalid argument");
	LIB_ASSERT(priv->theWindow != NULL, "invalid window reference");

	if (priv->windowHandler != NULL) {
		err = RemoveEventHandler(priv->windowHandler);
		LIB_ASSERT(err == noErr,
			"Could not uninstall the window handler.\n");
		priv->windowHandler = NULL;
	}
	if (priv->applicationHandler != NULL) {
		err = RemoveEventHandler(priv->applicationHandler);
		LIB_ASSERT(err == noErr,
			"Could not uninstall the application handler.\n");
		priv->applicationHandler = NULL;
	}

	return GGI_OK;
}

int QuartzInitEventHandler(gii_input *inp)
{
	quartz_priv *priv = QUARTZ_PRIV(inp);
	OSStatus err;

	LIB_ASSERT(priv != NULL, "invalid argument");
	LIB_ASSERT(priv->theWindow != NULL, "invalid window reference");

	err = InstallWindowEventHandler(priv->theWindow, NewEventHandlerUPP(DefaultWindowEventHandler),
			NUM_EVENTS(defaultWindowEvents), defaultWindowEvents, (void *)inp,
			&priv->windowHandler);
	LIB_ASSERT(err == noErr, "an error occured during window handler installation.\n");

	err = InstallApplicationEventHandler(NewEventHandlerUPP(DefaultApplicationEventHandler),
			NUM_EVENTS(defaultApplicationEvents), defaultApplicationEvents, (void *)inp,
			&priv->applicationHandler);
	LIB_ASSERT(err == noErr, "an error occured during application handler installation.\n");

	return GGI_OK;
}	/* QuartzInitEventHandler */


gii_event_mask transEvent2Mask(const EventRef theEvent)
{
	gii_event_mask mask = emNothing;
	uint32_t eventClass = GetEventClass(theEvent);
	uint32_t eventKind = GetEventKind(theEvent);


	switch (eventClass) {
	case kEventClassMouse:
		switch (eventKind) {
		case kEventMouseUp:
			mask = emPtrButtonRelease;
			break;
		case kEventMouseDown:
			mask = emPtrButtonPress;
			break;
		case kEventMouseMoved:
		case kEventMouseWheelMoved:
			mask = emPtrMove;
			break;
		case kEventMouseDragged:
			mask = emPtrMove | emPtrButtonRelease;
			break;
		default:
			break;
		}
		break;
	case kEventClassKeyboard:
		mask = emKey;
		break;
	case kEventClassTextInput:
		break;
	case kEventClassApplication:
		break;
	case kEventClassAppleEvent:
		break;
	case kEventClassMenu:
		break;
	case kEventClassWindow:
		break;
	case kEventClassControl:
		break;
	case kEventClassCommand:
		break;
	case kEventClassTablet:
		break;
	case kEventClassVolume:
		break;
	case kEventClassAppearance:
		break;
	case kEventClassService:
		break;
	case kEventClassToolbar:
		break;
	case kEventClassToolbarItem:
		break;
	case kEventClassToolbarItemView:
		break;
	case kEventClassAccessibility:
		break;
	case kEventClassSystem:
		break;
	case kEventClassInk:
		break;
	case kEventClassTSMDocumentAccess:
		break;
	default:
		DPRINT("Unknown event class: %c%c%c%c\n",
			eventClass >>24, eventClass >> 16, eventClass >> 8,
			(eventClass & 0x7f));
		break;
	}

	return mask;
}	/* transEvent2Mask */


#if 0
/* convert GII event masks to Quartz/Carbon event masks
 */
EventMask convertMaskGII2Quartz(gii_event_mask giievmask)
{
	switch (giievmask) {
	case emNothing:
		return 0;

	case emCommand:
	case emInformation:
		return 0;	/* FIXME */

	case emExpose:
		return updateMask;

	case emKeyPress:
		return keyDownMask;
	case emKeyRelease:
		return keyUpMask;
	case emKeyRepeat:
		return autoKeyMask;
	case emKey:
		return keyDownMask | keyUpMask | autoKeyMask;

	case emPtrRelative:
	case emPtrAbsolute:
		return 0;	/* FIXME */
	case emPtrButtonPress:
		return mDownMask;
	case emPtrButtonRelease:
		return mUpMask;
	case emPtrMove:
		return 0;	/* FIXME */
	case emPtrButton:
		return mDownMask | mUpMask;
	case emPointer:
		return 0 | mDownMask | mUpMask;	/* FIXME */

	case emValRelative:
	case emValAbsolute:
	case emValuator:
		return 0;	/* FIXME */

	case emZero:
		return 0;

	case emAll:
		return everyEvent;
	}

	return 0;
}



/* convert Quartz/Carbon event masks to GII event masks
 */
gii_event_mask convertMaskQuartz2GII(EventMask mask)
{
	switch (mask) {
	case mDownMask:
		return emPtrButtonPress;
	case mUpMask:
		return emPtrButtonRelease;

	case keyDownMask:
		return emKeyPress;
	case keyUpMask:
		return emKeyRelease;
	case autoKeyMask:
		return emKeyRepeat;

	case updateMask:
		return emExpose;

	case diskMask:
	case activMask:
	case highLevelEventMask:
	case osMask:
		return 0; /* FIXME */
	case everyEvent:
		return emAll;
	default:
		return 0;
	}
}

#endif
