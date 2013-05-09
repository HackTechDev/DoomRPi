/* $Id: cpuid_ia32.c,v 1.12 2005/02/21 20:24:10 cegger Exp $
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

#ifndef CC_CAN_CPUID
#ifdef _MSC_VER
#pragma message("warning: You must implement CPU detection for this compiler or OS to use SWAR")
#else
#warning You must implement CPU detection for this compiler or OS to use SWAR
#endif
#else

/*********************************** x86 **********************************
   The following section of code was taken from, and may be updated from
   the cpuid system utility program.  It has been stripped of stdio
   functions, human readable strings, and some functions not directly relevant 
   to determining the type of SWAR (SIMD) extensions available.  Some of the
   structure has been left in, although unused, in case it is desirable
   in the future to differentiate older CPUs somehow.

   The original copyright of the cpuid program reads as follows:

   * Intel and AMD x86 CPUID display program v 3.3 (1 Jan 2002)
   * Copyright 2002 Phil Karn, KA9Q
   * Updated 24 Apr 2001 to latest Intel CPUID spec
   * Updated 22 Dec 2001 to decode Intel flag 28, hyper threading
   * Updated 1 Jan 2002 to cover AMD Duron, Athlon
   * May be used under the terms of the GNU Public License (GPL)

   * Reference documents:
   * ftp://download.intel.com/design/pro/applnots/24161809.pdf  (AP-485)
   * ftp://download.intel.com/technology/64bitextensions/30083402.pdf (EM64T Volume 1)
   * ftp://download.intel.com/technology/64bitextensions/30083502.pdf (EM64T Volume 2)
   * http://developer.intel.com/design/Pentium4/manuals/24547103.pdf
   * http://developer.intel.com/design/pentiumiii/applnots/24512501.pdf (AP-909)
   * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/20734.pdf
   * 
   */

static void dointel(unsigned long);
static void doamd(unsigned long);
static void docyrix(unsigned long);

/* Below I do not know why I cannot add edx and ecx into the clobber list,
 * instead of making them fake inputs.  Kinda weird.  Also note normally
 * you should NOT have two asm statements in a row like this because GCC
 * may reorder them.  In this case, it reordering doesn't matter.
 */
#ifdef _MSC_VER
#define cpuid(in,a,b,c,d)           \
__asm {                             \
	__asm mov	eax, in     \
	__asm cpuid                 \
	__asm mov	a, eax      \
	__asm mov	b, ebx      \
	__asm mov	c, ecx      \
	__asm mov	d, edx      \
}
#else
#define cpuid(in,a,b,c,d) do { \
__asm__ __volatile__ ("push %%ebx\t\ncpuid\t\npop %%ebx" :                    \
                      "=a" (a), "=c" (c), "=d" (d) :                          \
                      "a" (in), "d" (0), "c" (0): "cc");                      \
__asm__ __volatile__ ("push %%ebx\t\ncpuid\t\npush %%ebx\t\n"                 \
		      "pop %%ecx\t\npop %%ebx" :                              \
                      "=a" (a), "=c" (b) : "a" (in) :                         \
                      "edx", "cc");                                           \
} while(0);
#endif

static void x86cpuid(void)
{
	unsigned int i;
	unsigned long li, maxi, maxei, unused;
	unsigned long eax_r, ebx_r, ecx_r, edx_r;

	/* Insert code here to test if CPUID instruction is available */

	/* Dump all the CPUID results in raw hex */
	cpuid(0, maxi, unused, unused, unused);
	maxi &= 0xffff;		/* The high-order word is non-zero on some Cyrix CPUs */
	for (i = 0; i <= maxi; i++) {
		cpuid(i, eax_r, ebx_r, ecx_r, edx_r);
	}	/* for */

	cpuid(0x80000000, maxei, unused, unused, unused);
	for (li = 0x80000000; li <= maxei; li++) {
		cpuid(li, eax_r, ebx_r, ecx_r, edx_r);
	}	/* for */

	/* Vendor ID and max CPUID level supported */
	cpuid(0, unused, ebx_r, ecx_r, edx_r);

	switch (ebx_r) {
	case 0x756e6547:	/* Intel */
		dointel(maxi);
		break;
	case 0x68747541:	/* AMD */
		doamd(maxi);
		break;
	case 0x69727943:	/* Cyrix */
		docyrix(maxi);
		break;
	default:
		break;
	}	/* switch */

	return;
}	/* x86cpuid */


/* Intel-specific information */
static void dointel(unsigned long maxi)
{
	unsigned long eax_r, ebx_r, ecx_r, edx_r, unused;

	if (maxi >= 1) {
		/* Family/model/type etc */
		int extended_model = -1, extended_family = -1;
		unsigned long clf, apic_id;
		unsigned long feature_flags;	/* X86 %edx CPUID feature bits */
		unsigned long feature2_flags;	/* X86 %ecx CPUID feature bits */
		unsigned long stepping, model, family,
			type, reserved, brand, siblings;

		cpuid(1, eax_r, ebx_r, ecx_r, edx_r);
		stepping = eax_r & 0xf;
		model = (eax_r >> 4) & 0xf;
		family = (eax_r >> 8) & 0xf;
		type = (eax_r >> 12) & 0x3;
		reserved = eax_r >> 14;
		clf = (ebx_r >> 8) & 0xff;
		apic_id = (ebx_r >> 24) & 0xff;
		siblings = (ebx_r >> 16) & 0xff;
		feature_flags = edx_r;
		feature2_flags = ecx_r;	/* Used to determine SSE 3 */

		switch (family) {	/* switch kept in case we want it */
		case 3:
			/* i386 */
			break;
		case 4:
			/* i486 */
			break;
		case 5:
			/* Pentium */
			break;
		case 6:
			/* Pentium Pro */
			break;
		case 15:
			/* Pentium 4 */
			break;
		}	/* switch */

		if (family == 15) {
			extended_family = (eax_r >> 20) & 0xff;
		}	/* if */

		switch (family) {
		case 3:
			break;
		case 4:
			switch (model) {
			case 0:
			case 1:
				/* DX */
				break;
			case 2:
				/* SX */
				break;
			case 3:
				/* 487/DX2 */
				break;
			case 4:
				/* SL */
				break;
			case 5:
				/* SX2 */
				break;
			case 7:
				/* write-back enhanced DX2 */
				break;
			case 8:
				/* DX4 */
				break;
			}	/* switch */
			break;
		case 5:
			switch (model) {
			case 1:
				/* 60/66 */
				break;
			case 2:
				/* 75-200 */
				break;
			case 3:
				/* for 486 system */
				break;
			case 4:
				swars_detected |= GG_SWAR_MMX;
				break;
			}	/* switch */
			break;
		case 6:
			switch (model) {
			case 1:
				/* Pentium Pro */
				break;
			case 3:
				/* Pentium II Model 3 */
				break;
			case 5:
				/* Pentium II Model 5/Xeon/Celeron */
				break;
			case 6:
				/* Celeron */
				break;
			case 7:
				/* Pentium III/Pentium III Xeon
				 * - external L2 cache */
				break;
			case 8:
				/* Pentium III/Pentium III Xeon
				 * - internal L2 cache */
				break;
			}	/* switch */
			break;
		case 15:
			break;
		}	/* switch */

		if (model == 15) {
			extended_model = (eax_r >> 16) & 0xf;
		}	/* if */

		brand = ebx_r & 0xff;
		cpuid(0x80000000, eax_r, ebx_r, unused, edx_r);
		if (eax_r & 0x80000000) {
			/* Extended feature/signature bits supported */
			unsigned long maxe = eax_r;
			if (maxe >= 0x80000004UL) {
				unsigned long i;

				for (i = 0x80000002UL; i <= 0x80000004UL; i++) {
					cpuid(i, eax_r, ebx_r, ecx_r, edx_r);
				}	/* for */
			}	/* if */
		}



		/* if */
		/*     if(clf)  CLFLUSH instruction cache line size: %d\n */
		/*     if(apic_id)Initial APIC ID: %d\n */
#if 0
		if (feature_flags & (1 << 28)) {
			/* Hyper threading siblings, number is in siblings */
		}	/* if */
#endif
		/* Feature flags */
		if (feature_flags & (1 << 23)) {
			swars_detected |= GG_SWAR_MMX;
		}	/* if */

		/* 24  "FXSR   Fast FP/MMX Streaming SIMD
		 * Extensions save/restore" */
		if (feature_flags & (1 << 25)) {
			swars_detected |= GG_SWAR_SSE;
		}	/* if */
		if (feature_flags & (1 << 26)) {
			swars_detected |= GG_SWAR_SSE2;
		}	/* if */
		if (feature2_flags & (1 << 0)) {
			swars_detected |= GG_SWAR_SSE3;
		}	/* if */
		if (feature2_flags & (1 << 29)) {
			/* EM64T detected */
			swars_detected |= GG_SWAR_64BITC;
		}	/* if */

	}	/* if */
}	/* dointel */



/* AMD-specific information */
static void doamd(unsigned long maxi)
{
	unsigned long maxei;
	unsigned long eax_r, ebx_r, ecx_r, edx_r, unused;

	int family = 0;

	/* Do standard stuff */
	if (maxi >= 1) {
		unsigned long stepping, model, reserved;

		cpuid(1, eax_r, ebx_r, unused, edx_r);
		stepping = eax_r & 0xf;
		model = (eax_r >> 4) & 0xf;
		family = (eax_r >> 8) & 0xf;
		reserved = eax_r >> 12;

		switch (family) {
		case 4:
			/* 486 model (number in variable model) */
			break;
		case 5:
			switch (model) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 6:
			case 7:
				/* K6 Model (number in model) */
				break;
			case 8:
				/* K6-2 Model 8 */
				break;
			case 9:
				/* K6-III Model 9 */
				break;
			default:
				/* K5/K6 model (number in model) */
				break;
			}	/* switch */
			break;
		case 6:
			switch (model) {
			case 1:
			case 2:
			case 4:
				/* Athlon model (number in model) */
				break;
			case 3:
				/* Duron model 3 */
				break;
			case 6:
				/* Athlon MP/Mobile Athlon model 6 */
				break;
			case 7:
				/* Mobile Duron Model 7 */
				break;
			default:
				/* Duron/Athlon model (number in model) */
				break;
			}	/* switch */

			break;
		}		/* switch */

		{
			int i;

			for (i = 0; i < 32; i++) {
				if (family == 5 && model == 0) {
#if 0
					/* if (i == 9) */
					/* Global */
					/*  Paging Extensions \ n */
					/* else */
					/*      if (i == 13) */
					/* 13 - reserved \ n */
#endif
				} else {
					if (edx_r & (1 << i)) {
						switch (i) {
						case 22:
							/* Extra MMX instructions
							 * will have flag elsewhere
							 * later.
							 */
							break;
						case 23:
							swars_detected
							    |= GG_SWAR_MMX;
							break;
						case 24:
							/* "FXSAVE/FXRSTOR" */
							break;
						case 30:
							swars_detected
							    |=
							    GG_SWAR_3DNOW;
							swars_detected
							    |= GG_SWAR_MMX;
							break;
						case 31:
							swars_detected
							    |=
							    GG_SWAR_ADV3DNOW;
							swars_detected
							    |=
							    GG_SWAR_3DNOW;
							swars_detected
							    |= GG_SWAR_MMX;
							break;
						}	/* switch */
					}	/* if */
				}	/* if */
			}	/* for */
		}	/* if */
	}

	/* Check for presence of extended info */
	cpuid(0x80000000, maxei, unused, unused, unused);
	if (maxei == 0)
		return;

	if (maxei >= 0x80000001UL) {
		unsigned long stepping, model, generation, reserved;
		int i;

		cpuid(0x80000001, eax_r, ebx_r, ecx_r, edx_r);
		stepping = eax_r & 0xf;
		model = (eax_r >> 4) & 0xf;
		generation = (eax_r >> 8) & 0xf;
		reserved = eax_r >> 12;

		for (i = 0; i < 32; i++) {
			if (family == 5 && model == 0 && i == 9) {
				/* Global Paging Extensions\n */
			} else {
				if (edx_r & (1 << i)) {
					switch (i) {
					case 22:
						/* Extra MMX instructions
						 * will have flag elsewhere
						 * later. */
						break;
					case 23:
						swars_detected
						    |= GG_SWAR_MMX;
						break;
					case 24:
						/* "FXSAVE/FXRSTOR" */
						break;
					case 30:
						swars_detected
						    |= GG_SWAR_3DNOW;
						swars_detected
						    |= GG_SWAR_MMX;
						break;
					case 31:
						swars_detected
						    |= GG_SWAR_ADV3DNOW;
						swars_detected
						    |= GG_SWAR_3DNOW;
						swars_detected
						    |= GG_SWAR_MMX;
						break;
					}	/* switch */
				}	/* if */
			}	/* if */
		}	/* for */
	}	/* if */
}



/* docyrix */
/* Cyrix-specific information */
static void docyrix(unsigned long maxi)
{
	unsigned long maxei;
	unsigned int i;
	unsigned long eax_r, ebx_r, ecx_r, edx_r, unused;

	cpuid(0x80000000, maxei, unused, unused, unused);

	/* Dump extended info, if any, in raw hex */
	for (i = 0x80000000; i <= maxei; i++) {
		cpuid(i, eax_r, ebx_r, ecx_r, edx_r);
	}	/* for */

	/* Do standard stuff */
	if (maxi >= 1) {
		unsigned long stepping, model, family, reserved;

		cpuid(1, eax_r, unused, unused, edx_r);
		stepping = eax_r & 0xf;
		model = (eax_r >> 4) & 0xf;
		family = (eax_r >> 8) & 0xf;
		reserved = eax_r >> 12;

		switch (family) {
		case 4:
			switch (model) {
			case 4:
				/* MediaGX */
				break;
			}	/* switch */
			break;
		case 5:
			switch (model) {
			case 2:
				/* 6x86 */
				break;
			case 4:
				/* BXm */
				break;
			}	/* switch */
			break;
		case 6:
			switch (model) {
			case 0:
				/* 6x86/MX */
				break;
			}	/* switch */
			break;
		}	/* switch */

		if (family == 5 && model == 0) {
			for (i = 0; i < 32; i++) {
				if (edx_r & (1 << i)) {
					switch (i) {
					case 8:
						/* "COMPXCHG8B Instruction" */
						break;
					case 15:
						/* "CMOV  Conditional
						 * Move Instruction" */
						break;
					case 23:
						swars_detected
						    |= GG_SWAR_MMX;
						break;
					}	/* switch */
				}	/* if */
			}	/* for */
		} else {
			for (i = 0; i < 32; i++) {
				if (edx_r & (1 << i)) {
					switch (i) {
					case 8:
						/* "COMPXCHG8B Instruction" */
						break;
					case 15:
						/* "CMOV  Conditional
						 * Move Instruction" */
						break;
					case 23:
						swars_detected
						    |= GG_SWAR_MMX;
						break;
					}	/* switch */
				}	/* if */
			}	/* for */
		}	/* if */
	}

	/* if */
	/* Check for presence of extended info */
	if (maxei < 0x80000000UL)
		return;

	if (maxei >= 0x80000001UL) {
		unsigned long stepping, model, family, reserved;

		cpuid(0x80000001, eax_r, ebx_r, ecx_r, edx_r);
		stepping = eax_r & 0xf;
		model = (eax_r >> 4) & 0xf;
		family = (eax_r >> 8) & 0xf;
		reserved = eax_r >> 12;

		switch (family) {
		case 4:
			/* MediaGX */
			break;
		case 5:
			/* 6x86/GXm */
			break;
		case 6:
			/* 6x86/MX */
			break;
		}	/* switch */

		for (i = 0; i < 32; i++) {
			if (edx_r & (1 << i)) {
				switch (i) {
				case 8:
					/* "COMPXCHG8B Instruction" */
					break;
				case 15:
					/* "CMOV  Conditional
					 * Move Instruction" */
					break;
				case 16:
					/* "FPU CMOV" */
					break;
				case 23:
					swars_detected |= GG_SWAR_MMX;
					break;
				case 24:
					swars_detected |= GG_SWAR_MMXPLUS;
					break;
				case 31:
					swars_detected |= GG_SWAR_3DNOW;
					break;
				}	/* switch */
			}	/* if */
		}	/* for */
	}	/* if */
}	/* docyrix */


gg_swartype ggGetSwarType(void)
{
	if (!cpuid_tested)
		x86cpuid();
	cpuid_tested = 1;
	return (swars_enabled & (swars_detected | GG_SWAR_NONE));
}	/* ggGetSwarType */

#endif
