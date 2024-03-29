/* $Id$
******************************************************************************

   24-bpp linear direct-access framebuffer renderer for LibGGI:

    -- functions implementing cross-blitting from other visuals.

   Copyright (C) 1997 Jason McMullan	[jmcc@ggi-project.org]
   Copyright (C) 1998 Andrew Apted	[andrew.apted@ggi-project.org]
   Copyright (C) 1998 Andreas Beck	[becka@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 2002 Brian S. Julin	[bri@tull.umassp.edu]

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

#include <string.h>
#include "lin24lib.h"
#include <ggi/internal/ggi_debug.h>

#define put24(addr,pixel)	\
do { \
	(addr)[0] = (pixel) & 0xff; \
	(addr)[1] = ((pixel)>>8)  & 0xff; \
	(addr)[2] = ((pixel)>>16) & 0xff; \
}while(0)

#define get24(addr)        ((addr)[0]+((addr)[1]<<8)+((addr)[2]<<16))

/* Default fallback */
static inline void
fallback(ggi_visual *src, int sx, int sy, int w, int h, 
	 ggi_visual *dst, int dx, int dy)
{
	ggi_pixel cur_src;
	uint32_t cur_dst = 0;
	uint8_t *dstptr;
	int stride;

	DPRINT_DRAW("linear-24: fallback to slow crossblit\n");

	LIBGGIGetPixel(src, sx, sy, &cur_src);
	cur_src++; /* assure safe init */
	
	stride = LIBGGI_FB_W_STRIDE(dst);
	dstptr = (uint8_t*)LIBGGI_CURWRITE(dst) + dy*stride + dx*3;

	for (; h > 0; h--, sy++, dy++) {
		int x;
		for (x=0; x < w; x++) {
			ggi_pixel pixel;

			LIBGGIGetPixel(src, sx+x, sy, &pixel);
			if (pixel != cur_src) {
				ggi_color col;
				LIBGGIUnmapPixel(src, pixel, &col);

				cur_dst = LIBGGIMapColor(dst, &col);
				cur_src = pixel;
			}
			put24(dstptr+x*3, cur_dst);
		}
		dstptr += stride;
	}
}


/* Blitting bewteen identical visuals
 */
static inline void
crossblit_same(ggi_visual *src, int sx, int sy, int w, int h,
	       ggi_visual *dst, int dx, int dy)
{
	uint8_t *srcp, *dstp;
	int srcstride = LIBGGI_FB_R_STRIDE(src);
	int dststride = LIBGGI_FB_W_STRIDE(dst);

	DPRINT_DRAW("linear-24: simple memcpy crossblit.\n");
	
	srcp = (uint8_t*)LIBGGI_CURREAD(src)  + srcstride*sy + sx*3;
	dstp = (uint8_t*)LIBGGI_CURWRITE(dst) + dststride*dy + dx*3;

	/* Width should be in bytes */
	w *= 3;

	for (; h != 0; h--) {
		memcpy(dstp, srcp, (size_t)(w));
		srcp += srcstride;
		dstp += dststride;
	}
}


/* 4 bit to 24 bit crossblitting.
 * TODO: a 256-entry lookup table could map two pixels at once and
 * avoid a lot of bit-fiddling.  Would this be faster considering cache?
 */
static inline void
cb4to24(ggi_visual *src, int sx, int sy, int w, int h,
	ggi_visual *dst, int dx, int dy)
{
	uint8_t *srcp, *dstp;
	int srcstride = LIBGGI_FB_R_STRIDE(src);
	int dststride = LIBGGI_FB_W_STRIDE(dst);
	uint32_t conv_tab[16];

	DPRINT_DRAW("linear-24: cb4to24.\n");

	do {
		unsigned int i;
		for (i = 0; i < 16; i++) {
			ggi_color col;

			LIBGGIUnmapPixel(src, i, &col);
			conv_tab[i] = LIBGGIMapColor(dst, &col);
		}
	} while (0);

	srcp = (uint8_t*)LIBGGI_CURREAD(src)  + srcstride*sy + sx/2;
	dstp = (uint8_t*)LIBGGI_CURWRITE(dst) + dststride*dy + dx*3;

	dststride -= w*3;

	if (((sx ^ w) & 1) && (GT_SUBSCHEME(LIBGGI_GT(src)) & GT_SUB_HIGHBIT_RIGHT)) {
		for (; h > 0; h--) {
			uint8_t  *srcpb = srcp;
			
			int i = (w + 7) / 8;

			/* Unroll manually. */
			switch (w & 0x7) {
			default:
				for (; i > 0; i--) {
				case 0:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				case 7:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				case 6:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				case 5:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				case 4:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				case 3:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				case 2:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				case 1:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				}
			}
			
			srcp += srcstride;
			dstp += dststride;
		}
		return;
	}
	if (GT_SUBSCHEME(LIBGGI_GT(src)) & GT_SUB_HIGHBIT_RIGHT) {
		for (; h > 0; h--) {
			uint8_t *srcpb = srcp;
			
			int i = (w + 7) / 8;

			/* Unroll manually. */			
			switch (w & 0x7) {
			default:
				for (; i > 0; i--) {
				case 0:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				case 7:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				case 6:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				case 5:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				case 4:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				case 3:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				case 2:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  dstp += 3;
				case 1:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  srcpb++;
				  dstp += 3;
				}
			}
			
			srcp += srcstride;
			dstp += dststride;
		}
		return;
	}
	if ((sx ^ w) & 1) {
		for (; h > 0; h--) {
			uint8_t  *srcpb = srcp;
			
			int i = (w + 7) / 8;

			/* Unroll manually. */
			switch (w & 0x7) {
			default:
				for (; i > 0; i--) {
				case 0:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				case 7:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				case 6:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				case 5:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				case 4:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				case 3:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				case 2:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				case 1:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				}
			}
			
			srcp += srcstride;
			dstp += dststride;
		}
	}
	else {
		for (; h > 0; h--) {
			uint8_t *srcpb = srcp;
			
			int i = (w + 7) / 8;

			/* Unroll manually. */			
			switch (w & 0x7) {
			default:
				for (; i > 0; i--) {
				case 0:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				case 7:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				case 6:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				case 5:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				case 4:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				case 3:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				case 2:
				  put24(dstp, conv_tab[(*srcpb & 0xf0) >> 4]);
				  dstp += 3;
				case 1:
				  put24(dstp, conv_tab[(*srcpb & 0x0f)]);
				  srcpb++;
				  dstp += 3;
				}
			}
			
			srcp += srcstride;
			dstp += dststride;
		}
	}
}

/* 8 bit to 24 bit crossblitting.
 */
static inline void
cb8to24(ggi_visual *src, int sx, int sy, int w, int h,
	ggi_visual *dst, int dx, int dy)
{
	uint8_t *srcp, *dstp;
	int srcstride = LIBGGI_FB_R_STRIDE(src);
	int dststride = LIBGGI_FB_W_STRIDE(dst);
	uint32_t conv_tab[256];

	DPRINT_DRAW("linear-24: cb8to24.\n");

	/* Creates the conversion table. A bit simplistic approach, perhaps?
	 */
	do {
		unsigned int i;
		for (i = 0; i < 256; i++) {
			ggi_color col;

			LIBGGIUnmapPixel(src, i, &col);
			conv_tab[i] = LIBGGIMapColor(dst, &col);
		}
	} while (0);

	srcp = (uint8_t*)LIBGGI_CURREAD(src)  + srcstride*sy + sx;
	dstp = (uint8_t*)LIBGGI_CURWRITE(dst) + dststride*dy + dx*3;

	srcstride -= w;
	dststride -= w*3;

	for (; h > 0; h--) {
		int i = (w + 7) / 8;

		/* We don't believe in the optimizing capabilities of the
		 * compiler hence unroll manually.
		 */
		switch (w & 0x7) {
		default:
			for (; i > 0; i--) {
			case 0x0: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			case 0x7: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			case 0x6: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			case 0x5: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			case 0x4: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			case 0x3: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			case 0x2: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			case 0x1: put24(dstp, conv_tab[*srcp]);
				dstp += 3;
				srcp++;
			}
		}

		srcp += srcstride;
		dstp += dststride;
	}
}


/* Create a table of shift and mask operations needed to translate the source 
 * visual pixelformat to that of the destination visual.  This is a complex 
 * do-all function that can create tables suitable for various different SWAR 
 * implementations.  Most of the complexities are mainly for MMX-style SWARs 
 * which have the most deficiencies, so the inlining should produce a much 
 * more simple function for other SWARs.
 *
 * src and dst are the visuals.
 * rshift, gshift, and bshift are temporary arrays used to unpack 
 *   the "bitmeaning" array in the visual's pixelformat.
 * shift is the location to store the first element of the column in the
 *   table which contains the shifts.  It may overlap with 
 *   rshift/gshift/bshift.
 * sskip is the number of bytes to skip between shift values, in case the
 *   SWAR works best when the shift and mask values are interleaved, and/or
 *   in case the SWAR works best with different size values than int32_t.
 * soff defines a bit offset added to bitshifts. The actual direction of the
 *   shift may be altered by this offset.
 * mask is the location to store the first element of the column in the 
 *   table which contains the bitmasks.  It must NOT overlap with 
 *   rshift/gshift/bshift.
 * mskip is the number of bytes to skip between mask values, in case the
 *   SWAR works best when the shift and mask values are interleaved, and/or
 *   in case the SWAR works best with different size values than ggi_pixel.
 * maskpost is a bitflag register:  
 *   If bit 0 is set than left masks are set to values that are appropriate 
 *     to apply after the shift operation, else before the shift operation.
 *   If bit 1 is set than right masks are set to values that are appropriate 
 *     to apply after the shift operation, else before the shift operation.
 * nl returns the number of actual left shifts after the effect of soff is 
 *     factored in.  This can also be found by counting the number 
 *     of nonzero shift values at the beggining of the shift column.
 * nr returns the number of actual right shifts after the effect of soff
 *     is factored in.  This can also be found by counting the 
 *     number of nonzero mask values in the mask column after the value 
 *     corresponding to the zero-shift value.
 * 
 * Thus, neglecting actual memory layout, the produced table looks like this:
 *
 * [mask != 0] [left shift count > 0]
 * [mask != 0] [left shift count > 0]
 * [mask != 0] [left shift count > 0]
 * [...]
 * [mask]      [shift == 0]
 * [mask != 0] [right shift count > 0]
 * [mask != 0] [right shift count > 0]
 * [mask != 0] [right shift count > 0]
 * [...]
 * [mask == 0]
 *
 * Note if nl is 0 there won't be any left shift rows and the table will
 * start with the zero shift, and if nr is 0 the zero mask will immediately
 * succeed the zero shift row.  The SWAR can either use nr and nl or branch
 * on the telltale zeroes in the table.
 *   
 */

static inline void build_masktab(ggi_visual *src, ggi_visual *dst, 
				 int32_t *rshift,int32_t *gshift,int32_t *bshift,
				 int32_t *shift, int sskip, int soff,
				 ggi_pixel *mask, int masklen, int mskip,
				 int maskpost, int *nl, int *nr) {
	int i, j;

	for (i = 0; i < masklen * mskip; i += mskip) mask[i] = 0;
	for (i = 0; i < 24 * sskip; i += sskip)
		rshift[i] = bshift[i] = gshift[i] = -1;

	for (i = 0; i < masklen - 24; i++) {
		ggi_pixel bm;
		int val;
		
		bm = src->r_frame->buffer.plb.pixelformat->bitmeaning[i];
		val = (bm & 0xff) - 232;
		if (val < 0) continue;
		
		switch(bm & 0xffffff00) {
		case GGI_BM_TYPE_COLOR | GGI_BM_SUB_RED:
			rshift[val * sskip] = i;
			break;
		case GGI_BM_TYPE_COLOR | GGI_BM_SUB_GREEN:
			gshift[val * sskip] = i;
			break;
		case GGI_BM_TYPE_COLOR | GGI_BM_SUB_BLUE:
			bshift[val * sskip] = i;
			break;
		default:
			break;
		}
	}

	/* Ensure pixel-correct fillout when destination channel is deeper. 
	 */
	for (i=23,j=23; i >= 0; i--) if (rshift[i * sskip] < 0)
		rshift[i * sskip] = rshift[j-- * sskip];
	for (i=23,j=23; i >= 0; i--) if (gshift[i * sskip] < 0)
		gshift[i * sskip] = gshift[j-- * sskip];
	for (i=23,j=23; i >= 0; i--) if (bshift[i * sskip] < 0)
		bshift[i * sskip] = bshift[j-- * sskip];

	for (i = 0; i < 24; i++) {
		ggi_pixel bm;
		int val, stmp;
        
		bm = dst->w_frame->buffer.plb.pixelformat->bitmeaning[i];
		val = (bm & 0xff) - 232;
		if (val < 0) continue;

#define SETMASK(arr) \
stmp = arr[val * sskip] + 23 - i;				\
if (stmp <= 23) {						\
	if (maskpost & 1) mask[stmp * mskip] |= 1 << i;		\
	else mask[stmp * mskip] |= 1 << arr[val * sskip];	\
} else {							\
	if (maskpost & 2) mask[stmp * mskip] |= 1 << i;	        \
	else mask[stmp * mskip] |= 1 << arr[val * sskip];	\
}

		switch(bm & 0xffffff00) {
		case GGI_BM_TYPE_COLOR | GGI_BM_SUB_RED:
		  SETMASK(rshift);
		  break;
		case GGI_BM_TYPE_COLOR | GGI_BM_SUB_GREEN:
		  SETMASK(gshift);
		  break;
		case GGI_BM_TYPE_COLOR | GGI_BM_SUB_BLUE:
		  SETMASK(bshift);
		  break;
		default:
		  break;
		}

#undef SETMASK

	}

	/* Precipitate the array of masks and generate accompanying shifts */
	for (i = 0, j = 0; i < 23 - soff; i++) 
		if (mask[i * mskip]) {
			mask[j * mskip] = mask[i * mskip];
			shift[j * sskip] = 23 - i - soff;
			j++;
		}
	*nl = j;
	mask[j * mskip] = mask[(23 - soff) * mskip];
	shift[j * sskip] = 0;
	j++; i++;
	for (; i < masklen; i++) 
		if (mask[i * mskip]) {
			mask[j * mskip] = mask[i * mskip];
			shift[j * sskip] = i - 23 + soff;
			j++;
		}
	*nr = j - *nl - 1;
	mask[j * mskip] = 0;
}

/* 24 bit to 24 bit crossblitting.
 */
static inline void cb24to24(ggi_visual *src, int sx, int sy, int w, int h, 
			    ggi_visual *dst, int dx, int dy) {
	int32_t shifts[72], rshifts[24];
	ggi_pixel masks[48], rmasks[24];
	int nl, nr;
	uint8_t *stoprow, *dstp;
	uint8_t *srcp;
	int dstride, sstride;
	
	DPRINT_DRAW("linear-24: cb24to24.\n");

	build_masktab(src, dst, shifts, shifts + 24, shifts + 48, 
		      shifts, 1, 0, masks, 48, 1, 0, &nl, &nr);

	dstp = (uint8_t*)LIBGGI_CURWRITE(dst) + 
	  dy*(LIBGGI_FB_W_STRIDE(dst)) + dx*3;
	srcp = (uint8_t*)LIBGGI_CURREAD(src) + 
	  sy*(LIBGGI_FB_R_STRIDE(src)) + sx*3;
	dstride = LIBGGI_FB_W_STRIDE(dst);
	sstride = LIBGGI_FB_R_STRIDE(src);
	
	stoprow = dstp + h * dstride;
	dstride -= w * 3;
	sstride -= w * 3;

	memcpy(rmasks, masks + nl + 1, nr * sizeof(ggi_pixel));
	memcpy(rshifts, shifts + nl + 1, nr * sizeof(int32_t));
	
	while (stoprow > dstp) {
		uint8_t *stopcol;

		stopcol = dstp + w * 3;
		while (stopcol > dstp) {
			ggi_pixel tmp, cache;
			
			tmp = 0;
			cache = get24(srcp);
			switch (nl) {
			case 23:
				tmp |= (cache & masks[22]) << shifts[22];
			case 22:
				tmp |= (cache & masks[21]) << shifts[21];
			case 21:
				tmp |= (cache & masks[20]) << shifts[20];
			case 20:
				tmp |= (cache & masks[19]) << shifts[19];
			case 19:
				tmp |= (cache & masks[18]) << shifts[18];
			case 18:
				tmp |= (cache & masks[17]) << shifts[17];
			case 17:
				tmp |= (cache & masks[16]) << shifts[16];
			case 16:
				tmp |= (cache & masks[15]) << shifts[15];
			case 15:
				tmp |= (cache & masks[14]) << shifts[14];
			case 14:
				tmp |= (cache & masks[13]) << shifts[13];
			case 13:
				tmp |= (cache & masks[12]) << shifts[12];
			case 12:
				tmp |= (cache & masks[11]) << shifts[11];
			case 11:
				tmp |= (cache & masks[10]) << shifts[10];
			case 10:
				tmp |= (cache & masks[9]) << shifts[9];
			case 9:
				tmp |= (cache & masks[8]) << shifts[8];
			case 8:
				tmp |= (cache & masks[7]) << shifts[7];
			case 7:
				tmp |= (cache & masks[6]) << shifts[6];
			case 6:
				tmp |= (cache & masks[5]) << shifts[5];
			case 5:
				tmp |= (cache & masks[4]) << shifts[4];
			case 4:
				tmp |= (cache & masks[3]) << shifts[3];
			case 3:
				tmp |= (cache & masks[2]) << shifts[2];
			case 2:
				tmp |= (cache & masks[1]) << shifts[1];
			case 1:
				tmp |= (cache & masks[0]) << shifts[0];
			case 0:
				break;
			}
			if (masks[nl]) { tmp |= cache & masks[nl]; }
			switch (nr) {
			case 23:
				tmp |= (cache & rmasks[22]) >> rshifts[22];
			case 22:
				tmp |= (cache & rmasks[21]) >> rshifts[21];
			case 21:
				tmp |= (cache & rmasks[20]) >> rshifts[20];
			case 20:
				tmp |= (cache & rmasks[19]) >> rshifts[19];
			case 19:
				tmp |= (cache & rmasks[18]) >> rshifts[18];
			case 18:
				tmp |= (cache & rmasks[17]) >> rshifts[17];
			case 17:
				tmp |= (cache & rmasks[16]) >> rshifts[16];
			case 16:
				tmp |= (cache & rmasks[15]) >> rshifts[15];
			case 15:
				tmp |= (cache & rmasks[14]) >> rshifts[14];
			case 14:
				tmp |= (cache & rmasks[13]) >> rshifts[13];
			case 13:
				tmp |= (cache & rmasks[12]) >> rshifts[12];
			case 12:
				tmp |= (cache & rmasks[11]) >> rshifts[11];
			case 11:
				tmp |= (cache & rmasks[10]) >> rshifts[10];
			case 10:
				tmp |= (cache & rmasks[9]) >> rshifts[9];
			case 9:
				tmp |= (cache & rmasks[8]) >> rshifts[8];
			case 8:
				tmp |= (cache & rmasks[7]) >> rshifts[7];
			case 7:
				tmp |= (cache & rmasks[6]) >> rshifts[6];
			case 6:
				tmp |= (cache & rmasks[5]) >> rshifts[5];
			case 5:
				tmp |= (cache & rmasks[4]) >> rshifts[4];
			case 4:
				tmp |= (cache & rmasks[3]) >> rshifts[3];
			case 3:
				tmp |= (cache & rmasks[2]) >> rshifts[2];
			case 2:
				tmp |= (cache & rmasks[1]) >> rshifts[1];
			case 1:
				tmp |= (cache & rmasks[0]) >> rshifts[0];
			case 0:
				break;
			}
			
			put24(dstp, tmp);
			dstp += 3;
			srcp += 3;
		}
		srcp += sstride;
		dstp += dstride;
	}
	return;
}

/* 16 bit to 24 bit crossblitting.
 */
static inline void cb16to24(ggi_visual *src, int sx, int sy, int w, int h, 
			    ggi_visual *dst, int dx, int dy) {
	int32_t shifts[72], rshifts[16];
	ggi_pixel masks[40], rmasks[16];
	int nl, nr;
	uint8_t *stoprow, *dstp;
	uint16_t *srcp;
	int dstride, sstride;
	
	DPRINT_DRAW("linear-24: cb16to24.\n");

	build_masktab(src, dst, shifts, shifts + 24, shifts + 48,
		      shifts, 1, 0, masks, 48, 1, 0, &nl, &nr);
		
	dstp = (uint8_t*)LIBGGI_CURWRITE(dst) + 
	  dy*(LIBGGI_FB_W_STRIDE(dst)) + dx*3;
	srcp = (uint16_t*)((uint8_t*)LIBGGI_CURREAD(src) + 
			 sy*(LIBGGI_FB_R_STRIDE(src)) + sx*2);
	dstride = LIBGGI_FB_W_STRIDE(dst);
	sstride = LIBGGI_FB_R_STRIDE(src)/2;
	
	stoprow = dstp + h * dstride;
	dstride -= w * 3;
	sstride -= w;

	memcpy(rmasks, masks + nl + 1, nr * sizeof(ggi_pixel));
	memcpy(rshifts, shifts + nl + 1, nr * sizeof(int32_t));

	while (stoprow > dstp) {
		uint8_t *stopcol;
		
		stopcol = dstp + w * 3;
		while (stopcol > dstp) {
			ggi_pixel tmp, cache;
			
			tmp = 0;
			cache = *srcp;
			switch (nl) {
			case 23:
				tmp |= (cache & masks[22]) << shifts[22];
			case 22:
				tmp |= (cache & masks[21]) << shifts[21];
			case 21:
				tmp |= (cache & masks[20]) << shifts[20];
			case 20:
				tmp |= (cache & masks[19]) << shifts[19];
			case 19:
				tmp |= (cache & masks[18]) << shifts[18];
			case 18:
				tmp |= (cache & masks[17]) << shifts[17];
			case 17:
				tmp |= (cache & masks[16]) << shifts[16];
			case 16:
				tmp |= (cache & masks[15]) << shifts[15];
			case 15:
				tmp |= (cache & masks[14]) << shifts[14];
			case 14:
				tmp |= (cache & masks[13]) << shifts[13];
			case 13:
				tmp |= (cache & masks[12]) << shifts[12];
			case 12:
				tmp |= (cache & masks[11]) << shifts[11];
			case 11:
				tmp |= (cache & masks[10]) << shifts[10];
			case 10:
				tmp |= (cache & masks[9]) << shifts[9];
			case 9:
				tmp |= (cache & masks[8]) << shifts[8];
			case 8:
				tmp |= (cache & masks[7]) << shifts[7];
			case 7:
				tmp |= (cache & masks[6]) << shifts[6];
			case 6:
				tmp |= (cache & masks[5]) << shifts[5];
			case 5:
				tmp |= (cache & masks[4]) << shifts[4];
			case 4:
				tmp |= (cache & masks[3]) << shifts[3];
			case 3:
				tmp |= (cache & masks[2]) << shifts[2];
			case 2:
				tmp |= (cache & masks[1]) << shifts[1];
			case 1:
				tmp |= (cache & masks[0]) << shifts[0];
			case 0:
				break;
			}
			if (masks[nl]) tmp |= cache & masks[nl];
			switch (nr) {
			case 15:
				tmp |= (cache & rmasks[14]) >> rshifts[14];
			case 14:
				tmp |= (cache & rmasks[13]) >> rshifts[13];
			case 13:
				tmp |= (cache & rmasks[12]) >> rshifts[12];
			case 12:
				tmp |= (cache & rmasks[11]) >> rshifts[11];
			case 11:
				tmp |= (cache & rmasks[10]) >> rshifts[10];
			case 10:
				tmp |= (cache & rmasks[9]) >> rshifts[9];
			case 9:
				tmp |= (cache & rmasks[8]) >> rshifts[8];
			case 8:
				tmp |= (cache & rmasks[7]) >> rshifts[7];
			case 7:
				tmp |= (cache & rmasks[6]) >> rshifts[6];
			case 6:
				tmp |= (cache & rmasks[5]) >> rshifts[5];
			case 5:
				tmp |= (cache & rmasks[4]) >> rshifts[4];
			case 4:
				tmp |= (cache & rmasks[3]) >> rshifts[3];
			case 3:
				tmp |= (cache & rmasks[2]) >> rshifts[2];
			case 2:
				tmp |= (cache & rmasks[1]) >> rshifts[1];
			case 1:
				tmp |= (cache & rmasks[0]) >> rshifts[0];
			case 0:
				break;
			}
			
			put24(dstp, tmp);
			dstp += 3;
			srcp++;
		}
		srcp += sstride;
		dstp += dstride;
	}
	return;
}

/* 32 bit to 24 bit crossblitting.
 */
static inline void cb32to24(ggi_visual *src, int sx, int sy, int w, int h, 
			    ggi_visual *dst, int dx, int dy) {
	int32_t shifts[72], rshifts[32];
	ggi_pixel masks[56], rmasks[32];
	int nl, nr;
	uint32_t  *srcp;
	uint8_t *stoprow, *dstp;
	int dstride, sstride;
	
	DPRINT_DRAW("linear-24: cb32to24.\n");

	build_masktab(src, dst, shifts, shifts + 24, shifts + 48, 
		      shifts, 1, 0, masks, 56, 1, 0, &nl, &nr);

	dstp = (uint8_t*)LIBGGI_CURWRITE(dst) + 
	  dy*(LIBGGI_FB_W_STRIDE(dst)) + dx*3;
	srcp = (uint32_t*)((uint8_t*)LIBGGI_CURREAD(src) + 
			 sy*(LIBGGI_FB_R_STRIDE(src)) + sx*4);
	dstride = LIBGGI_FB_W_STRIDE(dst);
	sstride = LIBGGI_FB_R_STRIDE(src)/4;
		
	stoprow = dstp + h * dstride;
	dstride -= w * 3;
	sstride -= w;

	memcpy(rmasks, masks + nl + 1, nr * sizeof(ggi_pixel));
	memcpy(rshifts, shifts + nl + 1, nr * sizeof(int32_t));

	while (stoprow > dstp) {
		uint8_t *stopcol;

		stopcol = dstp + w * 3;
		while (stopcol > dstp) {
			ggi_pixel tmp, cache;

			tmp = 0;
			cache = *srcp;
			switch (nl) {
			case 23:
				tmp |= (cache & masks[22]) << shifts[22];
			case 22:
				tmp |= (cache & masks[21]) << shifts[21];
			case 21:
				tmp |= (cache & masks[20]) << shifts[20];
			case 20:
				tmp |= (cache & masks[19]) << shifts[19];
			case 19:
				tmp |= (cache & masks[18]) << shifts[18];
			case 18:
				tmp |= (cache & masks[17]) << shifts[17];
			case 17:
				tmp |= (cache & masks[16]) << shifts[16];
			case 16:
				tmp |= (cache & masks[15]) << shifts[15];
			case 15:
				tmp |= (cache & masks[14]) << shifts[14];
			case 14:
				tmp |= (cache & masks[13]) << shifts[13];
			case 13:
				tmp |= (cache & masks[12]) << shifts[12];
			case 12:
				tmp |= (cache & masks[11]) << shifts[11];
			case 11:
				tmp |= (cache & masks[10]) << shifts[10];
			case 10:
				tmp |= (cache & masks[9]) << shifts[9];
			case 9:
				tmp |= (cache & masks[8]) << shifts[8];
			case 8:
				tmp |= (cache & masks[7]) << shifts[7];
			case 7:
				tmp |= (cache & masks[6]) << shifts[6];
			case 6:
				tmp |= (cache & masks[5]) << shifts[5];
			case 5:
				tmp |= (cache & masks[4]) << shifts[4];
			case 4:
				tmp |= (cache & masks[3]) << shifts[3];
			case 3:
				tmp |= (cache & masks[2]) << shifts[2];
			case 2:
				tmp |= (cache & masks[1]) << shifts[1];
			case 1:
				tmp |= (cache & masks[0]) << shifts[0];
			case 0:
				break;
			}
			if (masks[nl]) { tmp |= cache & masks[nl]; }
			switch (nr) {
			case 31:
				tmp |= (cache & rmasks[30]) >> rshifts[30];
			case 30:
				tmp |= (cache & rmasks[29]) >> rshifts[29];
			case 29:
				tmp |= (cache & rmasks[28]) >> rshifts[28];
			case 28:
				tmp |= (cache & rmasks[27]) >> rshifts[27];
			case 27:
				tmp |= (cache & rmasks[26]) >> rshifts[26];
			case 26:
				tmp |= (cache & rmasks[25]) >> rshifts[25];
			case 25:
				tmp |= (cache & rmasks[24]) >> rshifts[24];
			case 24:
				tmp |= (cache & rmasks[23]) >> rshifts[23];
			case 23:
				tmp |= (cache & rmasks[22]) >> rshifts[22];
			case 22:
				tmp |= (cache & rmasks[21]) >> rshifts[21];
			case 21:
				tmp |= (cache & rmasks[20]) >> rshifts[20];
			case 20:
				tmp |= (cache & rmasks[19]) >> rshifts[19];
			case 19:
				tmp |= (cache & rmasks[18]) >> rshifts[18];
			case 18:
				tmp |= (cache & rmasks[17]) >> rshifts[17];
			case 17:
				tmp |= (cache & rmasks[16]) >> rshifts[16];
			case 16:
				tmp |= (cache & rmasks[15]) >> rshifts[15];
			case 15:
				tmp |= (cache & rmasks[14]) >> rshifts[14];
			case 14:
				tmp |= (cache & rmasks[13]) >> rshifts[13];
			case 13:
				tmp |= (cache & rmasks[12]) >> rshifts[12];
			case 12:
				tmp |= (cache & rmasks[11]) >> rshifts[11];
			case 11:
				tmp |= (cache & rmasks[10]) >> rshifts[10];
			case 10:
				tmp |= (cache & rmasks[9]) >> rshifts[9];
			case 9:
				tmp |= (cache & rmasks[8]) >> rshifts[8];
			case 8:
				tmp |= (cache & rmasks[7]) >> rshifts[7];
			case 7:
				tmp |= (cache & rmasks[6]) >> rshifts[6];
			case 6:
				tmp |= (cache & rmasks[5]) >> rshifts[5];
			case 5:
				tmp |= (cache & rmasks[4]) >> rshifts[4];
			case 4:
				tmp |= (cache & rmasks[3]) >> rshifts[3];
			case 3:
				tmp |= (cache & rmasks[2]) >> rshifts[2];
			case 2:
				tmp |= (cache & rmasks[1]) >> rshifts[1];
			case 1:
				tmp |= (cache & rmasks[0]) >> rshifts[0];
			case 0:
				break;
			}
			
			put24(dstp, tmp);
			dstp += 3;
			srcp++;
		}
		srcp += sstride;
		dstp += dstride;
	}
	return;
}

/* Main function hook -- does some common-case preprocessing and
 * dispatches to one of the above functions.
 */
int GGI_lin24_crossblit(ggi_visual *src, int sx, int sy, int w, int h, 
			ggi_visual *dst, int dx, int dy)
{
	LIBGGICLIP_COPYBOX(dst,sx,sy,w,h,dx,dy);

	PREPARE_FB(dst);

	/* Check if src read buffer is also a blPixelLinearBuffer. */
	if (src->r_frame == NULL) goto fallback;
	if (src->r_frame->layout != blPixelLinearBuffer) goto fallback;

	/* No optimizations yet for reverse endian and other such weirdness */
	if (LIBGGI_PIXFMT(src)->flags) goto fallback;

	PREPARE_FB(src);

	switch (GT_SIZE(LIBGGI_GT(src))) {
	case 1:
		/* TODO */
	case 2:
		/* TODO */
		goto fallback;
	case 4:
		if (w * h > 15) cb4to24(src, sx, sy, w, h, dst, dx, dy);
		else goto fallback;
		return 0;
	case 8:
		if (w * h > 255) cb8to24(src, sx, sy, w, h, dst, dx, dy);
		else goto fallback;
		return 0;
	case 16:
		if (GT_SCHEME(LIBGGI_GT(src)) == GT_TRUECOLOR)
		  cb16to24(src, sx, sy, w, h, dst, dx, dy);
		else goto fallback;
		return 0;
	case 24:
		if (!dst->w_frame->buffer.plb.pixelformat->stdformat) 
		  goto notsame;
		if (dst->w_frame->buffer.plb.pixelformat->stdformat !=
		    src->r_frame->buffer.plb.pixelformat->stdformat) 
		  goto notsame;
		crossblit_same(src, sx, sy, w, h, dst, dx, dy);
		return 0;
	notsame:
		if (GT_SCHEME(LIBGGI_GT(src)) == GT_TRUECOLOR)
		  cb24to24(src, sx, sy, w, h, dst, dx, dy);
		else goto fallback;
		return 0;
	case 32:
		if (GT_SCHEME(LIBGGI_GT(src)) == GT_TRUECOLOR)
		  cb32to24(src, sx, sy, w, h, dst, dx, dy);
		else goto fallback;
		return 0;
	default:
		break;
	}
	
 fallback:
	fallback(src, sx, sy, w, h, dst, dx, dy);
	return 0;
}
