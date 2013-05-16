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

#ifndef __VPOZOOM_M_TILES_H__
#define __VPOZOOM_M_TILES_H__


typedef unsigned int datum_t;

#define DATUM_UNSET      0
#define DATUM_VOID       0xFDFDFDFD
#define DATUM_CLOSED     0xFEFEFEFE
#define DATUM_OVERFLOW   0xFFFFFFFF

#define DATUM_GET_VISPLANES(d)   ((((d)      ) & 0xff) - 1)
#define DATUM_GET_DRAWSEGS(d)   (((((d) >>  8) & 0xff) - 1) * 2)
#define DATUM_GET_OPENINGS(d)   (((((d) >> 16) & 0xff) - 1) * 160)
#define DATUM_GET_SOLIDSEGS(d)  (((((d) >> 24) & 0xff) - 1) / 4)


class Tile
{
	/* a Tile is a 64x64 block of data (e.g. visplane counts).
	 * ordering is [X][Y], X increases east and Y increases north.
	 * tiles are anchored at a map coordinate (for bottom left datum)
	 * and a certain stepping in map units.
	 * data values of 0 mean that the value has not been determined yet.
	 */

private:
	// whether tile is being used or not
	bool free;

	// your precious data
	datum_t data[64][64];

	// coordinates (meaningless when 'free' is true)
	int base_x, base_y;
	int step;

	// place where next datum is computed (if free)
	// will start at 0 and increase to 4095, then we are complete.
	int rover;

	// all datums have been computed
	bool complete;

public:
	Tile *next;  // link in list

public:
	Tile();
	virtual ~Tile();

public:
	// set all data values to zero
	void Clear();

	// allocate this tile (if free) and set the base coordinate and stepping.
	// this clears the whole data array.
	void Allocate(int bx, int by, int bstep);

	// mark this tile as free
	void Free();

	inline bool isFree() const
	{
		return free;
	}

	inline bool isComplete() const
	{
		return complete;
	}

	inline int baseX() const
	{
		return base_x;
	}

	inline int baseY() const
	{
		return base_y;
	}

	inline int baseStep() const
	{
		return step;
	}

	void calcProgress(int *count, int *total)
	{
		*count += rover;
		*total += 4096;
	}

	// set one of the unused (zero) datums by performing a visplane
	// test (which is expensive / slow).  Which datum is set is chosen
	// by a butterfly type algorithm (like PNG interlacing), where the
	// datum at [0][0] is the "head" or most important.
	void Process();
	void ProcessStep();

	// find the "nearest" datum which is not zero.  The search method
	// is not too expensive (no more than 7 datums are tried).
	datum_t ReadNearest(int x, int y) const;

	// see if the other tile overlaps this one, and if so then copy
	// some of that precious data into this tile.
	void CopyData(const Tile *other);
};


#endif  /* __VPOZOOM_M_TILES_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
