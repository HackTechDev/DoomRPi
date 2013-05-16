//------------------------------------------------------------------------
//  Map Display
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

#ifndef __VPOZOOM_UI_CANVAS_H__
#define __VPOZOOM_UI_CANVAS_H__


class UI_Canvas : public Fl_Widget
{
private:
	// map coordinates for centre of canvas
	double orig_x;
	double orig_y;

	// scale for drawing map
	// zoom is how large each pixel is (in map units)
	// zoom must be a power of two and >= 1
	int zoom;

	inline double Scale() const { return 1.0 / zoom; }

	bool render_tiles;

	// what we are actually rendering:
	//    'v' for visplanes
	//    'd' for drawsegs
	//    'o' for openings
	//    's' for solidsegs
	char render_mode;

	int rbscroll_x, rbscroll_y;

	// list of tiles
	Tile *tiles;
	Tile *free_tiles;

	// next tile to process
	Tile *tile_rover;
	Tile *offscreen_rover;

	// off-screen tiles get processed a lot less often than on-screen
	// tiles.  this counter has to reach a certain limit.
	int offscreen_counter;

	// bounding box of render tiles, in map units
	// (not used when there are no tiles)
	int tile_lx, tile_ly;
	int tile_hx, tile_hy;

	// drawing state only
	int map_lx, map_ly;
	int map_hx, map_hy;

public:
	UI_Canvas(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_Canvas();

public:
	// FLTK virtual method for handling input events.
	int handle(int event);

	// FLTK virtual method for resizing.
	void resize(int X, int Y, int W, int H);


	// this can be 'm' for 2D map, 'v' for visplanes (etc, see above)
	void SetMode(char new_mode);

	void GoHome();

	void Zoom(int delta);

	void MakeTiles(bool scrolling, bool zooming);
	Tile * FindOldTile(Tile * &list, int base_x, int base_y);

	void ProcessOneTile();
	void Refresh();

	void KillTiles();

	void ChangeRenderMode(int mode);

	void DrawMap();

	void DrawMapPoint(int map_x, int map_y);
	void DrawMapLine(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawMapVector(int map_x1, int map_y1, int map_x2, int map_y2);
	void DrawMapArrow(int map_x1, int map_y1, unsigned angle);
	void DrawKnobbyLine(int map_x1, int map_y1, int map_x2, int map_y2);

	bool isRenderActive() { return render_tiles; }

	float calcProgress() const;
	int calcTileMemory() const;

private:
	// FLTK virtual method for drawing.
	void draw();

	void DrawGrid();
	void DrawMapBounds();
	void DrawVertices();
	void DrawLinedefs();
	void DrawThings();

	void RenderTiles();
	void RenderOneTile(Tile *t);

	 int HandleKey(int key, int state);
	void HandleWheel(int dy, int focus_sx, int focus_sy);
	void MouseMotion(int new_sx, int new_sy);

	void RightButtonScroll(int mode);

	void MoveOrigin(double new_x, double new_y);

	Tile * NewTile(int base_x, int base_y, int step);
	void FreeTile(Tile *t);
	void FreeAllTiles(Tile *list);
	void AddTile(Tile *t);

	bool isTileVisible(const Tile *t) const;

	void ProcessOffscreenTile();

	datum_t AccessDatum(int map_x, int map_y);

	// convert screen coordinates to map coordinates
	inline int MAPX(int sx) const { return I_ROUND(orig_x + (sx - w()/2 - x()) / Scale()); }
	inline int MAPY(int sy) const { return I_ROUND(orig_y + (h()/2 - sy + y()) / Scale()); }

	// convert map coordinates to screen coordinates
	inline int SCREENX(int mx) const { return (x() + w()/2 + I_ROUND((mx - orig_x) * Scale())); }
	inline int SCREENY(int my) const { return (y() + h()/2 + I_ROUND((orig_y - my) * Scale())); }

	inline bool Vis(int x, int y, int r) const
	{
		return (x+r >= map_lx) && (x-r <= map_hx) &&
		       (y+r >= map_ly) && (y-r <= map_hy);
	}
	inline bool Vis(int x1, int y1, int x2, int y2) const
	{
		return (x2 >= map_lx) && (x1 <= map_hx) &&
		       (y2 >= map_ly) && (y1 <= map_hy);
	}
};


#endif  /* __VPOZOOM_UI_CANVAS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
