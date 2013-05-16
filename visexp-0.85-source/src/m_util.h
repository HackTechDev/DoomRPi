//------------------------------------------------------------------------
//  Miscellaneous Utilities
//------------------------------------------------------------------------
//
//  Visplane Explorer
//
//  Copyright (C) 2012 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __VPOZOOM_M_UTIL_H__
#define __VPOZOOM_M_UTIL_H__

/* ------ color stuff ------ */

typedef unsigned long rgb_color_t;

#define RGB_RED(col)    (((col) >> 16) & 255)
#define RGB_GREEN(col)  (((col) >>  8) & 255)
#define RGB_BLUE(col)   (((col)      ) & 255)

extern rgb_color_t heat_colors[6+2];

extern byte visplane_palette[256*3];
extern byte  drawseg_palette[256*3];
extern byte solidseg_palette[256*3];
extern byte  opening_palette[256*3];

void M_CreatePalettes(bool heat = false);

#endif  /* __VPOZOOM_M_UTIL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
