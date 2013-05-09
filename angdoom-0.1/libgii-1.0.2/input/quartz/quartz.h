/* $Id: quartz.h,v 1.3 2005/07/29 16:40:58 soyt Exp $
******************************************************************************

   Quartz: Input header

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


#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>


enum quartz_devtype {
	QZ_DEV_KEY = 0,
	QZ_DEV_MOUSE,

	QZ_DEV_MAX
};

typedef struct {
	WindowRef theWindow;
	uint32_t modifiers;
	uint32_t origin[QZ_DEV_MAX];

	EventHandlerRef windowHandler;
	EventHandlerRef applicationHandler;
	int (*resizefunc)(WindowRef theWindow, Rect orig, Rect prev, Rect cur);
} quartz_priv;

#define QUARTZ_PRIV(inp)  ((quartz_priv *)inp->priv)


int QuartzInit(quartz_priv *priv);
int QuartzFinishLaunch(gii_input *inp);
int QuartzExit(quartz_priv *priv);

int GII_quartz_seteventmask(gii_input *inp, gii_event_mask evm);
int GII_quartz_geteventmask(gii_input *inp);
gii_event_mask GII_quartz_eventpoll(gii_input *inp, void *arg);

int GII_quartz_getselectfdset(gii_input *inp, fd_set *readfds);

gii_event_mask transEvent2Mask(const EventRef theEvent);
int QuartzUninitEventHandler(gii_input *inp);
int QuartzInitEventHandler(gii_input *inp);

OSStatus DefaultWindowEventHandler(EventHandlerCallRef nextHandler,
		EventRef event, void *userData);

OSStatus DefaultApplicationEventHandler(EventHandlerCallRef nextHandler,
		EventRef event, void *userData);
