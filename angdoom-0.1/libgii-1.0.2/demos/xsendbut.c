/* $Id: xsendbut.c,v 1.1.1.1 2001/05/12 22:59:55 cegger Exp $
******************************************************************************

   xsendbut - A mhub to X button press converter

   Copyright (C) 1998	Marcus Sundberg		[marcus@ggi-project.org]

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

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>

static Display *disp;
static Window  win;

static char *progname;

static int
fillevent(XButtonEvent *event, int button)
{
	Window root, child;
	int x, y, x_root, y_root;
	int state;
	unsigned int mask;

	XGetInputFocus(disp, &win, &state);
	XQueryPointer(disp, win, &root, &child, &x_root, &y_root,
		      &x, &y, &mask);

	event->display = disp;
	event->window = win;
	event->root = root;
	event->subwindow = child;
	event->time = CurrentTime;
	event->x = x;
	event->y = y;
	event->x_root = x_root;
	event->y_root = y_root;
	event->state = mask;
	event->button = button;
	event->same_screen = 1;

	return 0;
}

static int
send_but(int press, int button)
{
	XEvent event;

	fillevent((XButtonPressedEvent*)&event, button);
	if (press) {
		event.type = ButtonPress;
		XSendEvent(disp, win, False, ButtonPressMask, &event);
	} else {
		event.type = ButtonRelease;
		XSendEvent(disp, win, False, ButtonReleaseMask, &event);
	}

	return 0;
}
	

int
main(int argc, char *argv[])
{
	unsigned char buf[4];
	int button;
	
	progname = argv[0];

	if ((disp = XOpenDisplay(NULL)) == NULL) { 
		fprintf(stderr, "%s: unable to open display", progname);
		exit (1);
	}
	
	while (fread(buf, 1, 2, stdin) == 2) {
		int press = 0;
		if ((buf[0] & (1<<7))) {
			press = 1;
			buf[0] &= ~(1<<7);
		}
		switch (buf[0]) {
		case 1:
			button = Button1;
			break;
		case 2:
			button = Button2;
			break;
		case 3:
			button = Button3;
			break;
		case 4:
			button = Button4;
			break;
		case 5:
			button = Button5;
			break;
		default:
			button = buf[0];
			break;
		}
		if (buf[1] == 0) {
			send_but(press, button);
		} else {
			while (buf[1]) {
				send_but(1, button);
				send_but(0, button);
				buf[1]--;
			}
		}
		XFlush(disp);
	}
	
	return 0;
}
