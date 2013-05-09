/* $Id: cocoa.m,v 1.2 2003/01/30 23:04:36 cegger Exp $
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
#include <stdio.h>
#include <string.h>

#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include "cocoa.h"


@implementation GIIObject
- (void)runeventloop:(id)anObject
{
	NSAutoreleasePool *pool;

	pool = [ [ NSAutoreleasePool alloc ] init ];

	fprintf(stderr, "GIIObject: Start event loop\n");

	/* Start the main event loop */
	[ anObject run ];

	fprintf(stderr, "GIIObject: event loop stopped\n");

	[ pool release ];
	[ NSThread exit ];
}
@end



int CocoaInit(cocoa_priv *priv)
{
	if (NULL == priv) return GGI_EARGINVAL;

	priv->giiobject = [ [ GIIObject alloc ] init ];

	fprintf(stderr, "CocoaInit: leave!\n");

	return GGI_OK;
}	/* CocoaInit */


int CocoaFinishLaunch(cocoa_priv *priv)
{
	NSApplication *GIIApp;

	GIIApp = priv->GIIApp;

	[ GIIApp setDelegate:priv->giiobject ];

	[ NSThread detachNewThreadSelector:@selector(runeventloop:) toTarget:priv->giiobject withObject:GIIApp ];

	fprintf(stderr, "CocoaFinishLaunch: leave!\n");

	return GGI_OK;
}	/* CocoaFinishLaunch */


int CocoaExit(cocoa_priv *priv)
{
	NSApplication *GIIApp;
	NSAutoreleasePool *pool;

	pool = [ [ NSAutoreleasePool alloc ] init ];

	GIIApp = priv->GIIApp;

	[ GIIApp stop:priv->giiobject ];
	[ priv->giiobject release ];

	[ pool release ];

	return 0;
}	/* CocoaExit */


NSEvent *CocoaNextEvent(cocoa_priv *priv)
{
	NSDate *distantPast;
	NSEvent *event;
	NSAutoreleasePool *pool;

	pool = [ [ NSAutoreleasePool alloc ] init ];

	distantPast = [ NSDate distantPast ];

	/* Poll for an event. This will not block */
	event = [ priv->GIIApp nextEventMatchingMask:NSAnyEventMask
			untilDate:distantPast
			inMode:NSDefaultRunLoopMode dequeue:YES ];

	[ pool release ];

	return event;
}	/* CocoaNextEvent */


void CocoaPrintEvent(cocoa_priv *priv, FILE *f, NSEvent *ev)
{
	if (ev == nil) {
		fprintf(f, "no event\n");
		return;
	}	/* if */

	fprintf(f, "event: type %i, scancode %i, unicode %i, modifiers %i\n",
		[ ev type ], [ ev keyCode ], [ [ ev characters ] intValue ],
		[ ev modifierFlags ]);

	return;
}	/* CocoaPrintEvent */
