/* $Id: common.h,v 1.2 2005/07/31 15:31:13 soyt Exp $
******************************************************************************

   Input-touchscreen_common: common functions for touchscreen handling

   Copyright (C) 2001 Tobias Hunger [tobias@fresco.org]

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

This code is taken from PicoGUI (http://www.picogui.org/
*/

#include <stdlib.h>
#include "config.h"
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <fcntl.h>

typedef struct
{
  /*
   * Coefficients for the transformation formulas:
   *
   *     m = (ax + by + c) / s
   *     n = (dx + ey + f) / s
   *
   * These formulas will transform a device point (x, y) to a
   * screen point (m, n) in fractional pixels.  The fraction
   * is 1 / Transformation_Units_per_Pixel.
   */

  int a, b, c, d, e, f, s;
} Transformation_Coefficients;

typedef struct {
  int fd;
  int readonly;
  gii_event_mask sent;

  int is_pressed;
  int is_calibrated;
  gii_cmddata_getdevinfo * dev_info;
} touchscreen_priv;

#define TOUCHSCREEN_PRIV(inp) ((touchscreen_priv *) inp->priv)



/*
 functions
 */

int touchscreen_init(void);
int touchscreen_sendevent(gii_input *, gii_event *);
int touchscreen_sendvalinfo(gii_input *, int);
int touchscreen_senddevinfo(gii_input *);
gii_event_mask touchscreen_handledata(gii_input *, long, long, long, int);
