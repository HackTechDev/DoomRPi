//------------------------------------------------------------------------
//  Tile Management
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


Tile::Tile() : free(true), next(NULL)
{
	// Note: not cleared by default
}


Tile::~Tile()
{
	// nothing needed
}


void Tile::Clear()
{
	memset(data, 0, sizeof(data));

	rover = 0;
	complete = false;
}


void Tile::Allocate(int bx, int by, int bstep)
{
	free = false;

	base_x = bx;
	base_y = by;
	step   = bstep;

	Clear();
}


void Tile::Free()
{
	free = true;
}


#define ROV_DX(r, shift)  (((r) & 1) << shift)
#define ROV_DY(r, shift)  (((((r) >> 1) ^ (r)) & 1) << shift)


static inline void ButterflyCoord(int rover, int& dx, int& dy)
{
	// this is a "butterfly" style sequence, which begins like:
	//    ( 0  0)  (32 32)  ( 0 32)  (32  0)
	//    (16 16)  (48 48)  (16 48)  (48 16)
	//    ( 0 16)  (32 48)  ( 0 48)  (32 16)
	//    (16  0)  (48 32)  (16 32)  (48  0)
	//    ( 8  8)  (40 40)  ( 8 40)  (40  8)
	//    etc....

	dx = ROV_DX(rover, 5);
	dy = ROV_DY(rover, 5);

	rover >>= 2;
	dx += ROV_DX(rover, 4);
	dy += ROV_DY(rover, 4);

	rover >>= 2;
	dx |= ROV_DX(rover, 3);
	dy |= ROV_DY(rover, 3);

	rover >>= 2;
	dx |= ROV_DX(rover, 2);
	dy |= ROV_DY(rover, 2);

	rover >>= 2;
	dx |= ROV_DX(rover, 1);
	dy |= ROV_DY(rover, 1);

	rover >>= 2;
	dx |= ROV_DX(rover, 0);
	dy |= ROV_DY(rover, 0);
}


void Tile::ProcessStep()
{
	while (rover < 4096)
	{
		int dx, dy;

		ButterflyCoord(rover, dx, dy);

		rover++;

		if (data[dx][dy] == DATUM_UNSET)
		{
			int map_x = base_x + dx * step;
			int map_y = base_y + dy * step;

			data[dx][dy] = Main_TestSpot(map_x, map_y);
			return;
		}
	}

	// all datums are set, yay!
	complete = true;
}


void Tile::Process()
{
	int count = 1;

	// give nearly empty tiles extra processing
	// (so that scrolling or zooming out updates the new tiles faster)
	if (rover < 64)
		count = 4;

	for (; count > 0 ; count--)
		ProcessStep();
}


datum_t Tile::ReadNearest(int x, int y) const
{
	for (;;)
	{
		datum_t val = data[x][y];

		if (val != DATUM_UNSET)
			return val;

		// move coordinate a step closer to (0,0)
		// NOTE: if the 64x64 size is changes, this will need more/less stages

		int xy = x | y;

		if (xy & 1)
		{
			x &= ~1; y &= ~1;
		}
		else if (xy & 2)
		{
			x &= ~2; y &= ~2;
		}
		else if (xy & 4)
		{
			x &= ~4; y &= ~4;
		}
		else if (xy & 8)
		{
			x &= ~8; y &= ~8;
		}
		else if (xy & 16)
		{
			x &= ~16; y &= ~16;
		}
		else // terminating condition
		{
			return data[0][0];
		}
	}
}


void Tile::CopyData(const Tile *other)
{
	int bx1 = base_x;
	int by1 = base_y;
	int bx2 = base_x + step * 63;
	int by2 = base_y + step * 63;

	int cx1 = other->base_x;
	int cy1 = other->base_y;
	int cx2 = other->base_x + other->step * 63;
	int cy2 = other->base_y + other->step * 63;

	// determine overlap (in map units)

	int map_x1 = MAX(bx1, cx1);
	int map_y1 = MAX(by1, cy1);

	int map_x2 = MIN(bx2, cx2);
	int map_y2 = MIN(by2, cy2);

	if (map_x1 > map_x2 || map_y1 > map_y2)
		return;
	
	// convert to local units (of _this_ tile)

	int dx1 = (map_x1 - bx1) / step - 1;
	int dx2 = (map_x2 - bx1) / step + 2;

	int dy1 = (map_y1 - by1) / step - 1;
	int dy2 = (map_y2 - by1) / step + 2;

	dx1 = CLAMP(0, dx1, 63);
	dy1 = CLAMP(0, dy1, 63);
	dx2 = CLAMP(0, dx2, 63);
	dy2 = CLAMP(0, dy2, 63);

	// try to copy data....

	for (int dx = dx1 ; dx <= dx2 ; dx++)
	for (int dy = dy1 ; dy <= dy2 ; dy++)
	{
		int dx2 = bx1 + dx * step - cx1;
		int dy2 = by1 + dy * step - cy1;

		// need an exact match on the rows/columns of other tile
		if (! ((dx2 % other->step == 0) &&
			   (dy2 % other->step == 0)) )
			continue;

		dx2 /= other->step;
		dy2 /= other->step;

		if (dx2 < 0 || dx2 > 63 || dy2 < 0 || dy2 > 63)
			continue;

		datum_t val = other->data[dx2][dy2];

		if (val != DATUM_UNSET)
			data[dx][dy] = val;
	}

	// return rover to start
	rover = 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
