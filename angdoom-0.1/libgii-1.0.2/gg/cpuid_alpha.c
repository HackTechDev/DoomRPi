/* $Id: cpuid_alpha.c,v 1.4 2004/10/27 23:58:43 aldot Exp $
***************************************************************************

   LibGG - General runtime CPU-type detection code.

   Copyright (C) 2002  Brian S. Julin   [bri@tull.umassp.edu]

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

***************************************************************************
*/

#include "cpuid.h"

/* State of CPU detection/selection is kept here. */
static int cpuid_tested = 0;
gg_swartype swars_detected = 0;
gg_swartype swars_enabled = GG_SWAR_ALL;


#ifndef CC_CAN_AMASK
#warning Compiler must be able to test CPU capabilities with AMASK for SWAR. 
#else

gg_swartype ggGetSwarType(void)
{
	if (!cpuid_tested) {
		unsigned long rvb, rc;

		cpuid_tested = 1;
		rvb = 0x0000000000000100;	/* Bit 8 = MVI (a.k.a. MAX) */
#ifdef __DECC
		rc = asm("amask %a0, %r0", (rvb));
#else
		__asm__ __volatile__("amask %1, %0":"=r"(rc):"rI"(rvb));
#endif
		/* will clear bits representing features that ARE present. */
		if (!rc)
			swars_detected |= GG_SWAR_MVI;
	}
	return (swars_enabled &
		(swars_detected | GG_SWAR_64BITC | GG_SWAR_NONE));
}

#endif
