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

#include "headers.h"


rgb_color_t heat_colors[6+2] =
{
	0x000000,   // BLACK
	0x000077,   // BLUE
	0x880088,   // PURPLE
	0xCC3333,   // RED
	0xFFCC33,   // ORANGE
	0xFFFFFF,   // WHITE

	0x665E55,   // dark gray        (VOID)
	0xFFAADD    // bright pink  (OVERFLOW)
};


byte visplane_palette[256*3];
byte  drawseg_palette[256*3];
byte solidseg_palette[256*3];
byte  opening_palette[256*3];



static void CreateHeatPalette(byte * palette)
{
	int limit = 128;

	int i = 0;

	// DATUM_UNSET
	palette[i*3 + 0] = RGB_RED(  heat_colors[6]);
	palette[i*3 + 1] = RGB_GREEN(heat_colors[6]);
	palette[i*3 + 2] = RGB_BLUE( heat_colors[6]);

	i++;

	// normal heat range : 1 .. limit 

	for (; i <= limit ; i++)
	{
		int first = (i-1) * 4 / limit;

		int r1 = RGB_RED(  heat_colors[first  ]);
		int g1 = RGB_GREEN(heat_colors[first  ]);
		int b1 = RGB_BLUE( heat_colors[first  ]);

		int r2 = RGB_RED(  heat_colors[first+1]);
		int g2 = RGB_GREEN(heat_colors[first+1]);
		int b2 = RGB_BLUE( heat_colors[first+1]);
		
		int along = (i-1) * 4 - (first * limit);

		int r = (r1 * (limit - along) + r2 * along) / limit;
		int g = (g1 * (limit - along) + g2 * along) / limit;
		int b = (b1 * (limit - along) + b2 * along) / limit;

		palette[i*3 + 0] = r;
		palette[i*3 + 1] = g;
		palette[i*3 + 2] = b;
	}

	// single color over the limit

	for (; i < 0xFD ; i++)
	{
		palette[i*3 + 0] = RGB_RED(  heat_colors[5]);
		palette[i*3 + 1] = RGB_GREEN(heat_colors[5]);
		palette[i*3 + 2] = RGB_BLUE( heat_colors[5]);
	}

	// DATUM_VOID : 0xFD
	palette[i*3 + 0] = RGB_RED(  heat_colors[6]);
	palette[i*3 + 1] = RGB_GREEN(heat_colors[6]);
	palette[i*3 + 2] = RGB_BLUE( heat_colors[6]);

	i++;

	// DATUM_CLOSED : 0xFE
	palette[i*3 + 0] = RGB_RED(  heat_colors[0]);
	palette[i*3 + 1] = RGB_GREEN(heat_colors[0]);
	palette[i*3 + 2] = RGB_BLUE( heat_colors[0]);

	i++;

	// DATUM_OVERFLOW : 0xFF
	palette[i*3 + 0] = RGB_RED(  heat_colors[7]);
	palette[i*3 + 1] = RGB_GREEN(heat_colors[7]);
	palette[i*3 + 2] = RGB_BLUE( heat_colors[7]);
}


static void CreateSimplePalette(byte *palette, rgb_color_t hue)
{
	int i = 0;

	// DATUM_UNSET
	palette[i*3 + 0] = RGB_RED(  heat_colors[6]);
	palette[i*3 + 1] = RGB_GREEN(heat_colors[6]);
	palette[i*3 + 2] = RGB_BLUE( heat_colors[6]);

	i++;

	// normal heat range : 1 .. limit 

	int limit = 128;

	int r2 = RGB_RED(  hue);
	int g2 = RGB_GREEN(hue);
	int b2 = RGB_BLUE( hue);

	for (; i <= limit ; i++)
	{
		int r = r2 * (i-1) / limit;
		int g = g2 * (i-1) / limit;
		int b = b2 * (i-1) / limit;

		palette[i*3 + 0] = r;
		palette[i*3 + 1] = g;
		palette[i*3 + 2] = b;
	}

	// red fading to pink over the limit

	int r7 = RGB_RED(  heat_colors[7]);
	int g7 = RGB_GREEN(heat_colors[7]);
	int b7 = RGB_BLUE( heat_colors[7]);

	for (; i < 0xFD ; i++)
	{
		palette[i*3 + 0] = 0xFF;
		palette[i*3 + 1] = (i - 129) * g7 / 160;
		palette[i*3 + 2] = (i - 129) * b7 / 160;
	}

	// DATUM_VOID : 0xFD
	palette[i*3 + 0] = RGB_RED(  heat_colors[6]);
	palette[i*3 + 1] = RGB_GREEN(heat_colors[6]);
	palette[i*3 + 2] = RGB_BLUE( heat_colors[6]);

	i++;

	// DATUM_CLOSED : 0xFE
	palette[i*3 + 0] = 0;
	palette[i*3 + 1] = 0;
	palette[i*3 + 2] = 0;

	i++;

	// DATUM_OVERFLOW : 0xFF
	palette[i*3 + 0] = r7;
	palette[i*3 + 1] = g7;
	palette[i*3 + 2] = b7;
}


void M_CreatePalettes(bool heat)
{
	if (heat)
	{
		CreateHeatPalette(visplane_palette);
		CreateHeatPalette( drawseg_palette);
		CreateHeatPalette(solidseg_palette);
		CreateHeatPalette( opening_palette);
	}
	else
	{
		CreateSimplePalette(visplane_palette, 0xFFCC77);
		CreateSimplePalette( drawseg_palette, 0x33DDFF);
		CreateSimplePalette(solidseg_palette, 0x77FF77);
		CreateSimplePalette( opening_palette, 0xFFFFFF);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
