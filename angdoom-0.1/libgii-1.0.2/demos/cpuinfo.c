/* $Id: cpuinfo.c,v 1.10 2005/08/01 09:26:29 pekberg Exp $
******************************************************************************

   cpuinfo - Prints out the type of SWAR's CPU's supports.

   Copyright (C) 2002 Christoph Egger	[Christoph_Egger@t-online.de]

   This software is placed in the public domain and can be used freely
   for any purpose. It comes without any kind of warranty, either
   expressed or implied, including, but not limited to the implied
   warranties of merchantability or fitness for a particular purpose.
   Use it at your own risk. the author is not responsible for any damage
   or consequences raised by use or inability to use this program.

******************************************************************************
*/


#include "config.h"
#include <ggi/gg.h>
#include <stdio.h>

int main(void)
{
	gg_swartype swar;

	if (ggInit()) exit(-1);

	swar = ggGetSwarType();

	ggExit();

	printf("This CPU has the following SWAR's:\n");

	if (swar & GG_SWAR_NONE) {
		printf("- Vanilla C implementation\n");
	}	/* if */

	if (swar & GG_SWAR_32BITC) {
		printf("- Fast 32b math vs 16b\n");
	}	/* if */

	if (swar & GG_SWAR_ALTIVEC) {
		printf("- PowerPC Altivec\n");
	}	/* if */

	if (swar & GG_SWAR_SSE) {
		printf("- x86 SSE\n");
	}	/* if */

	if (swar & GG_SWAR_SSE2) {
		printf("- x86 SSE2\n");
	}	/* if */

	if (swar & GG_SWAR_SSE3) {
		printf("- x86 SSE3\n");
	}	/* if */

	if (swar & GG_SWAR_MMX) {
		printf("- x86 MMX\n");
	}	/* if */

	if (swar & GG_SWAR_MMXPLUS) {
		printf("- Cyrix MMX plus\n");
	}	/* if */

	if (swar & GG_SWAR_3DNOW) {
		printf("- AMD 3DNow!\n");
	}	/* if */

	if (swar & GG_SWAR_ADV3DNOW) {
		printf("- AMD 3DNow! advanced\n");
	}	/* if */

	if (swar & GG_SWAR_MAX) {
		printf("- SWAR MAX\n");
	}	/* if */

	if (swar & GG_SWAR_SIGD) {
		printf("- SWAR SIGD\n");
	}	/* if */

#ifdef GG_HAVE_INT64

	if (swar & GG_SWAR_64BITC) {
		printf("- Fast 64b math vs 32b\n");
	}	/* if */

	if (swar & GG_SWAR_MVI) {
		printf("- SWAR MVI\n");
	}	/* if */

	if (swar & GG_SWAR_MAX2) {
		printf("- SWAR MAX2\n");
	}	/* if */

	if (swar & GG_SWAR_MDMX) {
		printf("- SWAR MDMX\n");
	}	/* if */

	if (swar & GG_SWAR_MAJC) {
		printf("- SWAR MAJC\n");
	}	/* if */

	if (swar & GG_SWAR_VIS) {
		printf("- Sparc VIS\n");
	}	/* if */

#endif

	return 0;
}	/* main */
