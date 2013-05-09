/* $Id: common.c,v 1.7 2004/11/27 14:37:54 soyt Exp $
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

#include "common.h"

static gii_cmddata_getvalinfo ts_valinfo[2] =
{
    {   0,                              /* valuator number */
        "Raw X position",               /* long valuator name */
        "rx",                           /* shorthand */
        { 0, +32768, +65535 },               /* range */
        GII_PT_LENGTH,
        0, 1, 1, 0                 /* SI constants (bogus!) */
    },
    {   1,                              /* valuator number */
        "Raw Y position",               /* long valuator name */
        "ry",                           /* shorthand */
        { 0, +32768, +65535 },               /* range */
        GII_PT_LENGTH,
        0, 1, 1, 0                 /* SI constants (bogus!) */
    }
};

static Transformation_Coefficients tc={1,0,0,0,1,0,1};



int touchscreen_init(void)
{
  FILE *fp = NULL;
  int
    calwidth = 640,
    calheight = 480,
    values;
  int configured = 0;

  fp=fopen("/etc/ggi/touchscreen.calibration", "r");
  if (fp != NULL)
    {
      values=fscanf(fp, "%d %d %d %d %d %d %d %d %d", &tc.a, &tc.b,
		    &tc.c, &tc.d, &tc.e, &tc.f, &tc.s, &calwidth, &calheight);
      /* tc.s will normally be 65536, enough to cover the
       * inaccuracies of this scaling, I hope - otherwise
       * we need to scale it here */
      if(values == 9)
	{
	  tc.a/=calwidth;
	  tc.b/=calwidth;
	  tc.c/=calwidth;
	  tc.d/=calheight;
	  tc.e/=calheight;
	  tc.f/=calheight;
	  configured = 1;
	}
      else if(values==7)
	  configured = 1;
      fclose(fp);
      DPRINT_MISC("touchscreen: %d %d %d | %d %d %d (%dx%d)\n",
		     tc.a, tc.b, tc.c, tc.d, tc.e, tc.e, calwidth, calheight);
    }
  if (!configured)
    DPRINT_MISC("touchscreen: NOT configured! Calibrate before use.\n");

  return configured;
}



int touchscreen_sendvalinfo(gii_input *inp, int val)
{
  gii_cmddata_getvalinfo *VI;
  gii_event ev;

  DPRINT_MISC("touchscreen sending valinfo\n");
    
  if (val >= 2) return GGI_ENOSPACE;

  int size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getvalinfo);
  
  _giiEventBlank(&ev, size);
  
  ev.any.size   = size;
  ev.any.type   = evCommand;
  ev.any.origin = inp->origin;
  ev.cmd.code   = GII_CMDCODE_GETVALINFO;

  VI = (gii_cmddata_getvalinfo *) ev.cmd.data;
  
  *VI = ts_valinfo[val];
  
  return _giiEvQueueAdd(inp, &ev);
}



int touchscreen_senddevinfo(gii_input * inp)
{
  gii_event ev;
  gii_cmddata_getdevinfo * dinfo;
  
  int size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);
  
  DPRINT_MISC("touchscreen sending devinfo\n");
  
  _giiEventBlank(&ev, size);
  
  ev.any.size   = size;
  ev.any.type   = evCommand;
  ev.any.origin = inp->origin;
  ev.cmd.code   = GII_CMDCODE_GETDEVINFO;
  
  dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
  *dinfo = *(TOUCHSCREEN_PRIV(inp)->dev_info);
  
  return _giiEvQueueAdd(inp, &ev);
}



void touchscreen_pentoscreen(long *x, long *y)
{
	if(tc.s) {
		long m, n;

		/* apply the calibrated transformation
		 *
		 * yes, x and y should be the other way around:
		 * but then the image is rotated;-)
		 */
		m=(tc.a*(*y)+tc.b*(*x)+tc.c)/tc.s;
		n=(tc.d*(*y)+tc.e*(*x)+tc.f)/tc.s;
		*x=m;
		*y=n;
	}
}



/*
 * The following filter routine was adapted from the Agenda VR3 touchscreen
 * driver by Bradley D. Laronde. (I think.. please correct this if i'm
 * wrong --Micah)
 *
 * If the filter returns nonzero, the sample should be discarded.
 */
int touchscreen_filter(long *x, long *y, long pendown)
{
  /*
   * I do some error masking by tossing out really wild data points.
   * Lower data_change_limit value means pointer get's "left behind" more easily.
   * Higher value means less errors caught.
   * The right setting of this value is just slightly higher than the number of
   * units traversed per sample during a "quick" stroke.
   * I figure a fast pen scribble can cover about 3000 pixels in one second on
   * a typical screen.
   * There are typically about 3 to 6 points of touch-panel resolution per pixel.
   * So 3000 pixels-per-second * 3 to 6 tp-points-per-pixel / 50 samples-per-second =
   * 180 to 360 tp points per scan.
   */

  /* TODO: make this configurable */
  const  int  data_change_limit = 360;
  static int  have_last_data = 0;
  static long last_data_x = 0;
  static long last_data_y = 0;
  static long last_pendown = 0;
  static long prev_out_x = 0;
  static long prev_out_y = 0;
  
  /* Skip (discard) next n-samples. */
  static int skip_samples = 0;
  
  /*
   * Thanks to John Siau <jsiau@benchmarkmedia.com> for help with the
   * noise filter. I use an infinite impulse response low-pass filter on the
   * data to filter out high-frequency noise.  Results look better than a
   * finite impulse response filter. If I understand it right, the nice
   * thing is that the noise now acts as a dither signal that effectively
   * increases the resolution of the a/d converter by a few bits and drops
   * the noise level by about 10db. Please don't quote me on those db numbers
   * however. :-) The end result is that the pointer goes from very fuzzy
   * to much more steady. Hysteresis really calms it down in the end
   * (elsewhere).
   *
   * iir_shift_bits effectively sets the number of samples used by the filter
   * (number of samples is 2^iir_shift_bits).
   *
   * Setting iir_shift_bits lower allows shorter "taps" and less pointer lag,
   * but a fuzzier pointer, which can be especially bad for display update
   * when dragging.
   *
   * Setting iir_shift_bits higher requires longer "presses" and causes more
   * pointer lag, but it provides steadier pointer.
   *
   * If you adjust iir_shift_bits, you may also want to adjust the sample
   * interval in VrTpanelInit.
   *
   * The filter gain is fixed at 8 times the input (gives room for increased
   * resolution potentially added by the noise-dithering).
   *
   * The filter won't start outputing data until iir_count is above
   * iir_output_threshold.
   */
  const int iir_shift_bits = 2;
  const int iir_sample_depth = (1 << iir_shift_bits);
  const int iir_output_threshold = 1;
  const int iir_gain = 1;       /* NOTE: I know this gain should be 8, but for
				 * some reason that
				 * breaks calibration??
				 */
  static int iir_accum_x = 0;
  static int iir_accum_y = 0;
  static int iir_count = 0;
  
  /* Pen down */
  if (pendown && !last_pendown) {
    /* reset the limiter */
    have_last_data = 0;
    
    /* reset the filter */
    iir_count = 0;
    iir_accum_x = 0;
    iir_accum_y = 0;
    
    /* skip the next sample (first sample after pen down) */
    skip_samples = 1;
    
    last_pendown = pendown;
    
    /* ignore pen-down since we don't know where it is */
    return 1;
  }

  /* pen up */
  if (!pendown && last_pendown) {
    /* Always use the penup */
    *x = prev_out_x;
    *y = prev_out_y;
    last_pendown = pendown;
    return 0;
  }
  
  /* we have position data */
  /* Should we skip this sample? */
  if(skip_samples > 0) {
    skip_samples--;
    return 1;
  }
  
  /* has the position changed more than we will allow? */
  if(have_last_data )
    if((abs(*x - last_data_x) > data_change_limit)
       || ( abs(*y - last_data_y) > data_change_limit )) {
      return 1;
    }
  
  /* save last position */
  last_data_x = *x;
  last_data_y = *y;
  have_last_data = 1;
  
  /* is filter full? */
  if(iir_count == iir_sample_depth) {
    /* make room for new sample */
    iir_accum_x -= iir_accum_x >> iir_shift_bits;
    iir_accum_y -= iir_accum_y >> iir_shift_bits;
    iir_count--;
  }
  
  /* feed new sample to filter */
  iir_accum_x += *x;
  iir_accum_y += *y;
  iir_count++;
  
  /* aren't we over the threshold yet? */
  if(iir_count <= iir_output_threshold)
    return 1;
  
  /* figure filter output */
  /* TODO: optimize for shifts instead of divides (when possible)? */
  *x = (iir_accum_x * iir_gain) / iir_count;
  *y = (iir_accum_y * iir_gain) / iir_count;
  
  prev_out_x = *x;
  prev_out_y = *y;
  
  return 0;
}



gii_event_mask touchscreen_handledata(gii_input *inp,
				      long x, long y, long pressure,
				      int is_calibrated)
{
  gii_event ev;
  touchscreen_priv * tshook = TOUCHSCREEN_PRIV(inp);
  gii_event_mask sent = 0;

  if (touchscreen_filter(&x, &y, pressure))
    {
      DPRINT_EVENTS("touchscreen: filtered out!\n");
      return sent;
    }

  if(inp->curreventmask & emValuator)
    {
      DPRINT_EVENTS("touchscreen: valuator (%d,%d)\n", x, y);
      
      _giiEventBlank(&ev, sizeof(gii_val_event));
      ev.any.type   = evValAbsolute;
      ev.any.size   = sizeof(gii_val_event);
      ev.any.origin = inp->origin;
      ev.val.first  = 0;
      ev.val.count  = 2;
      
      ev.val.value[0] = x;
      ev.val.value[1] = y;
      
      _giiEvQueueAdd(inp, &ev);
      sent |= emValAbsolute;
    }

  if ((inp->curreventmask & emPtrAbsolute) &&
      pressure &&
      is_calibrated)
    {
      touchscreen_pentoscreen(&x, &y);
      DPRINT_EVENTS("touchscreen: pmove event (%d,%d)\n", x, y);

      _giiEventBlank(&ev, sizeof(gii_pmove_event));
      
      ev.pmove.size   = sizeof(gii_pmove_event);
      ev.pmove.type   = evPtrAbsolute;
      ev.pmove.origin = inp->origin;
      
      ev.pmove.x      = x;
      ev.pmove.y      = y;
      ev.pmove.z      = 0;
      ev.pmove.wheel  = 0;
      
      _giiEvQueueAdd(inp, &ev);
      sent |= emPtrAbsolute;
    }
  
  if (inp->curreventmask & (emPtrButtonPress | emPtrButtonRelease))
    {
      _giiEventBlank(&ev, sizeof(gii_pbutton_event));
      if ((inp->curreventmask & emPtrButtonPress) &&
	  pressure && !tshook->is_pressed)
	{
	  DPRINT_EVENTS("touchscreen: pbutton pressed event\n");

	  /* touches surface */
	  ev.pbutton.type = evPtrButtonPress;
	  tshook->is_pressed = 1;
	  sent |= emPtrButtonPress;
	}
      else if ((inp->curreventmask & emPtrButtonRelease) && 
	       !pressure && tshook->is_pressed)
	{
	  DPRINT_EVENTS("touchscreen: pbutton release event\n");

	  /* pen leaves surface */
	  ev.pbutton.type = evPtrButtonRelease;
	  tshook->is_pressed = 0;
	  sent |= emPtrButtonRelease;
	}
      ev.pbutton.size = sizeof(gii_pbutton_event);
      ev.pmove.origin = inp->origin;
      ev.pbutton.button = 1;
      
      _giiEvQueueAdd(inp, &ev);
    }

  return sent;
}



int touchscreen_sendevent(gii_input *inp,
			  gii_event *ev)
{
  if ((ev->any.target != inp->origin) &&
      (ev->any.target != GII_EV_TARGET_ALL))
    /* not for us */
    return GGI_EEVNOTARGET;
  
  if (ev->any.type != evCommand)
    return GGI_EEVUNKNOWN;
  
  if (ev->cmd.code == GII_CMDCODE_GETDEVINFO)
    {
      touchscreen_senddevinfo(inp);
      return 0;
    }
  
  if (ev->cmd.code == GII_CMDCODE_GETVALINFO)
    {
      int i;
      gii_cmddata_getvalinfo *vi;
      
      vi = (gii_cmddata_getvalinfo *) ev->cmd.data;
      
      if (vi->number == GII_VAL_QUERY_ALL)
	{
	  for (i=0; i < 2; i++)
	    touchscreen_sendvalinfo(inp, i);
	  return 0;
	}
      
      return touchscreen_sendvalinfo(inp, vi->number);
    }
  
  return GGI_EEVUNKNOWN; /* unknown command */
}
