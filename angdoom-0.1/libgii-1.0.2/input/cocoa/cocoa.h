/* $Id: cocoa.h,v 1.2 2005/07/31 15:31:11 soyt Exp $
******************************************************************************

   Cocoa Input

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

#ifndef _GII_INPUT_INT_COCOA_H
#define _GII_INPUT_INT_COCOA_H

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>


@interface GIIObject : NSObject
- (void)runeventloop:(id)anObject;
@end

typedef struct {
	NSApplication *GIIApp;
	GIIObject *giiobject;
	NSWindow *window;
} cocoa_priv;

#define COCOA_PRIV(inp) ((cocoa_priv *)inp->priv)


int CocoaInit(cocoa_priv *priv);
int CocoaFinishLaunch(cocoa_priv *priv);
int CocoaExit(cocoa_priv *priv);
NSEvent *CocoaNextEvent(cocoa_priv *priv);
void CocoaPrintEvent(cocoa_priv *priv, FILE *f, NSEvent *e);


#endif /* _GII_INPUT_INT_COCOA_H */
