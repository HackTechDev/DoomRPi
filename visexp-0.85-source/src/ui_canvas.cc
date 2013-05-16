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

#include "headers.h"


#define MIN_ZOOM   1
#define MAX_ZOOM  64

#define DEFAULT_ZOOM  32


// extra tiles (on each side of the window)
#define EXTRA_TILE_W  16
#define EXTRA_TILE_H  12

#define OFFSCREEN_DIVISOR  3


//
// UI_Canvas Constructor
//
UI_Canvas::UI_Canvas(int X, int Y, int W, int H, const char *label) : 
    Fl_Widget(X, Y, W, H, label),
    orig_x(0), orig_y(0),
	zoom(DEFAULT_ZOOM),
	render_tiles(false),
	render_mode('v'),
	tiles(NULL), free_tiles(NULL),
	tile_rover(NULL), offscreen_rover(NULL),
	offscreen_counter(0)
{ }

//
// UI_Canvas Destructor
//
UI_Canvas::~UI_Canvas()
{
	Tile * t;

	for (t = tiles ; t ; t = t->next)
		delete t;

	for (t = free_tiles ; t ; t = t->next)
		delete t;
}


void UI_Canvas::resize(int X, int Y, int W, int H)
{
	Fl_Widget::resize(X, Y, W, H);
}


void UI_Canvas::draw()
{
	fl_push_clip(x(), y(), w(), h());

	fl_color(FL_WHITE);
	fl_font(FL_COURIER, 14);

	map_lx = MAPX(x());
	map_ly = MAPY(y()+h());
	map_hx = MAPX(x()+w());
	map_hy = MAPY(y());

	if (render_tiles && map_is_loaded)
		RenderTiles();
	else
		DrawMap();

	fl_pop_clip();
}


int UI_Canvas::handle(int event)
{
	//// fprintf(stderr, "HANDLE EVENT %d\n", event);

	switch (event)
	{
		case FL_FOCUS:
			return 1;

		case FL_KEYDOWN:
		case FL_SHORTCUT:
		{
			int key   = Fl::event_key();
			int state = Fl::event_state();

			switch (key)
			{
				case FL_Num_Lock:
				case FL_Caps_Lock:

				case FL_Shift_L: case FL_Control_L:
				case FL_Shift_R: case FL_Control_R:
				case FL_Meta_L:  case FL_Alt_L:
				case FL_Meta_R:  case FL_Alt_R:

					/* IGNORE */
					return 1;

				default:
					/* OK */
					break;
			}

			if (key != 0)
			{
				if (key < 127 && isalpha(key) && (state & FL_SHIFT))
					key = toupper(key);

				if (key < 127 && isalpha(key) && (state & FL_CTRL))
					key &= 31;

				int result = HandleKey(key, state);

				MouseMotion(Fl::event_x(), Fl::event_y());

				return result;
			}

			return 0;
		}

		case FL_ENTER:
			// we greedily grab the focus
			if (Fl::focus() != this)
				take_focus(); 

			return 1;

		case FL_LEAVE:
///???			EditorLeaveWindow();
			redraw();
			return 1;

		case FL_DRAG:
			if (Fl::event_button() != 1)
				RightButtonScroll(2);
			return 1;

		case FL_MOVE:
			MouseMotion(Fl::event_x(), Fl::event_y());
			return 1;

		case FL_PUSH:
			if (Fl::event_button() != 1)
				RightButtonScroll(1);
			return 1;

		case FL_RELEASE:
			if (Fl::event_button() != 1)
				RightButtonScroll(0);
			return 1;

		case FL_MOUSEWHEEL:
			HandleWheel(0 - Fl::event_dy(), Fl::event_x(), Fl::event_y());
			return 1;

		default:
			break;
	}

	return 0;  // unused
}


//------------------------------------------------------------------------


void UI_Canvas::DrawMap()
{
	// draw the grid first since it's in the background
	DrawGrid();

	if (! map_is_loaded)
		return;

	DrawLinedefs();
}


/*
 *  draw_grid - draw the grid in the background of the edit window
 */
#define GRID_POINT    fl_rgb_color(0, 0, 0xFF)
#define GRID_DARK     fl_rgb_color(0, 0, 0x77)
#define GRID_MEDIUM   fl_rgb_color(0, 0, 0xAA)
#define GRID_BRIGHT   fl_rgb_color(0, 0, 0xEE)

void UI_Canvas::DrawGrid()
{
	int grid_step_1 = 128;    // Map units between dots
	int grid_step_2 = grid_step_1 * 8;   // Map units between dim lines
	int grid_step_3 = grid_step_2 * 8;  // Map units between bright lines

	float pixels_1 = grid_step_1 * Scale();


	fl_color(FL_BLACK);
	fl_rectf(x(), y(), w(), h());


	fl_color(GRID_MEDIUM);
	{
		int gx = (map_lx / grid_step_2) * grid_step_2;

		for (; gx <= map_hx; gx += grid_step_2)
			if (gx % grid_step_3 != 0)
				fl_line(SCREENX(gx), y(), SCREENX(gx), y()+h());

		int gy = (map_ly / grid_step_2) * grid_step_2;

		for (; gy <= map_hy; gy += grid_step_2)
			if (gy % grid_step_3 != 0)
				fl_line(x(), SCREENY(gy), x()+w(), SCREENY(gy));
	}


	fl_color(GRID_BRIGHT);
	{
		int gx = (map_lx / grid_step_3) * grid_step_3;

		for (; gx <= map_hx; gx += grid_step_3)
		{
			int sx = SCREENX(gx);

			fl_line(sx, y(), sx, y()+h());

			if (gx == 0)
				fl_line(sx+1, y(), sx+1, y()+h());
		}

		int gy = (map_ly / grid_step_3) * grid_step_3;

		for (; gy <=  map_hy; gy += grid_step_3)
		{
			int sy = SCREENY(gy);

			fl_line(x(), sy, x()+w(), sy);

			if (gy == 0)
				fl_line(x(), sy+1, x()+w(), sy+1);
		}
	}


	// POINTS

	if (pixels_1 < 7.9)
		return;

	if (false)
		fl_color(GRID_MEDIUM);
	else
		fl_color(GRID_POINT);

	{
		int gx = (map_lx / grid_step_1) * grid_step_1;
		int gy = (map_ly / grid_step_1) * grid_step_1;

		for (int ny = gy; ny <= map_hy; ny += grid_step_1)
		for (int nx = gx; nx <= map_hx; nx += grid_step_1)
		{
			int sx = SCREENX(nx);
			int sy = SCREENY(ny);

			if (pixels_1 < 30.99)
				fl_point(sx, sy);
			else
			{
				fl_line(sx-0, sy, sx+1, sy);
				fl_line(sx, sy-0, sx, sy+1);
			}
		}
	}
}


/*
 *  draw_linedefs - draw the linedefs
 */
#define LIGHTGREY  fl_rgb_color(160,160,160)

void UI_Canvas::DrawLinedefs()
{
	for (unsigned int i = 0 ; ; i++)
	{
		int x1, y1, x2, y2;

		int sides = VPO_GetLinedef(i, &x1, &y1, &x2, &y2);

		if (sides < 0)	// no more lines?
			break;

		if (! Vis(MIN(x1,x2), MIN(y1,y2), MAX(x1,x2), MAX(y1,y2)))
			continue;

		if (sides == 0)
			fl_color(FL_RED);
		else if (sides == 1)
			fl_color(FL_WHITE);
		else
			fl_color(LIGHTGREY);

		if (false)
			DrawKnobbyLine(x1, y1, x2, y2);
		else
			DrawMapLine(x1, y1, x2, y2);
	}
}


/*
 *  draw_map_point - draw a point at map coordinates
 *
 *  The point is drawn at map coordinates (<map_x>, <map_y>)
 */
void UI_Canvas::DrawMapPoint(int map_x, int map_y)
{
    fl_point(SCREENX(map_x), SCREENY(map_y));
}


/*
 *  DrawMapLine - draw a line on the screen from map coords
 */
void UI_Canvas::DrawMapLine(int map_x1, int map_y1, int map_x2, int map_y2)
{
    fl_line(SCREENX(map_x1), SCREENY(map_y1),
            SCREENX(map_x2), SCREENY(map_y2));
}


void UI_Canvas::DrawKnobbyLine(int map_x1, int map_y1, int map_x2, int map_y2)
{
	int x1 = SCREENX(map_x1);
	int y1 = SCREENY(map_y1);
	int x2 = SCREENX(map_x2);
	int y2 = SCREENY(map_y2);

    fl_line(x1, y1, x2, y2);

	// indicate direction of line
   	int mx = (x1 + x2) / 2;
   	int my = (y1 + y2) / 2;

	int dx = (y1 - y2);
	int dy = (x2 - x1);

	int len = MAX(4, MAX(abs(dx), abs(dy)));

	int want_len = MIN(12, len / 5);

	dx = dx * want_len / len;
	dy = dy * want_len / len;

	if (! (dx == 0 && dy == 0))
	{
		fl_line(mx, my, mx + dx, my + dy);
	}
}


/*
 *  DrawMapVector - draw an arrow on the screen from map coords
 */
void UI_Canvas::DrawMapVector (int map_x1, int map_y1, int map_x2, int map_y2)
{
	int scrx1 = SCREENX (map_x1);
	int scry1 = SCREENY (map_y1);
	int scrx2 = SCREENX (map_x2);
	int scry2 = SCREENY (map_y2);

	double r  = hypot((double) (scrx1 - scrx2), (double) (scry1 - scry2));
#if 0
	/* AYM 19980216 to avoid getting huge arrowheads when zooming in */
	int    scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
	int    scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
#else
	int scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale() / 2)) : 0;
	int scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale() / 2)) : 0;
#endif

	fl_line(scrx1, scry1, scrx2, scry2);

	scrx1 = scrx2 + 2 * scrXoff;
	scry1 = scry2 + 2 * scrYoff;

	fl_line(scrx1 - scrYoff, scry1 + scrXoff, scrx2, scry2);
	fl_line(scrx1 + scrYoff, scry1 - scrXoff, scrx2, scry2);
}


/*
 *  DrawMapArrow - draw an arrow on the screen from map coords and angle (0 - 65535)
 */
void UI_Canvas::DrawMapArrow(int map_x1, int map_y1, unsigned angle)
{
	int map_x2 = map_x1 + (int) (50 * cos(angle / 10430.37835));
	int map_y2 = map_y1 + (int) (50 * sin(angle / 10430.37835));

	int scrx1 = SCREENX(map_x1);
	int scry1 = SCREENY(map_y1);
	int scrx2 = SCREENX(map_x2);
	int scry2 = SCREENY(map_y2);

	double r = hypot(scrx1 - scrx2, scry1 - scry2);
#if 0
	int    scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
	int    scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale < 1 ? Scale : 1)) : 0;
#else
	int scrXoff = (r >= 1.0) ? (int) ((scrx1 - scrx2) * 8.0 / r * (Scale() / 2)) : 0;
	int scrYoff = (r >= 1.0) ? (int) ((scry1 - scry2) * 8.0 / r * (Scale() / 2)) : 0;
#endif

	fl_line(scrx1, scry1, scrx2, scry2);

	scrx1 = scrx2 + 2 * scrXoff;
	scry1 = scry2 + 2 * scrYoff;

	fl_line(scrx1 - scrYoff, scry1 + scrXoff, scrx2, scry2);
	fl_line(scrx1 + scrYoff, scry1 - scrXoff, scrx2, scry2);
}


void UI_Canvas::ChangeRenderMode(int mode)
{
	render_tiles = mode ? true : false;

	redraw();
}


int UI_Canvas::HandleKey(int key, int state)
{
	switch (key)
	{
		case FL_Tab: case '\t':
			render_tiles = !render_tiles;
			redraw();
			return 1;
	
		case FL_Home:
			GoHome();
			return 1;

		// zooming...

		case '+': case '=':
			Zoom(+1);
			return 1;

		case '-': case '_':
			Zoom(-1);
			return 1;

		// scrolling...
		
		case FL_Up:
			MoveOrigin(orig_x, orig_y + zoom * h() / 4);
			return 1;

		case FL_Down:
			MoveOrigin(orig_x, orig_y - zoom * h() / 4);
			return 1;

		case FL_Left:
			MoveOrigin(orig_x - zoom * w() / 5, orig_y);
			return 1;

		case FL_Right:
			MoveOrigin(orig_x + zoom * w() / 5, orig_y);
			return 1;

		// setting the view mode...

		case 'm': case 'M':
		case 'v': case 'V':
		case 'd': case 'D':
		case 's': case 'S':
//--	case 'o': case 'O':
			SetMode(tolower(key));
			return 1;

		default:
			// unknown key
			return 0;
	}
}


void UI_Canvas::SetMode(char new_mode)
{
	if (new_mode == 'm')
	{
		if (! render_tiles)
			return;

		render_tiles = false;
	}
	else
	{
		if (render_tiles && render_mode == new_mode)
			return;
	
		render_tiles = true;
		render_mode  = new_mode;
	}

	redraw();
}


void UI_Canvas::MouseMotion(int new_sx, int new_sy)
{
	float map_x = MAPX(new_sx);
	float map_y = MAPY(new_sy);

	main_win->infobar->SetMouse(map_x, map_y);

	// retrieve visplane (etc) value
	datum_t d = AccessDatum(map_x, map_y);

	main_win->infobar->ShowDatum(render_mode, d);
}


void UI_Canvas::Zoom(int delta)
{
	HandleWheel(delta, x() + w()/2, y() + h()/2);
}


void UI_Canvas::HandleWheel(int dy, int focus_sx, int focus_sy)
{
	float map_x = MAPX(focus_sx);
	float map_y = MAPY(focus_sy);

	int old_zoom = zoom;

	if (dy > 0)
	{
		if (zoom > MIN_ZOOM)
			zoom = zoom / 2;
	}
	else if (dy < 0)
	{
		if (zoom < MAX_ZOOM)
			zoom = zoom * 2;
	}

	if (zoom == old_zoom)
		return;

	if (focus_sx >= 0)
	{
		double dist_factor = (1.0 - zoom / (double)old_zoom);

		orig_x += (map_x - orig_x) * dist_factor;
		orig_y += (map_y - orig_y) * dist_factor;
	}
	
	main_win->infobar->SetScale(zoom);

	MakeTiles(false /* scrolling */, true /* zooming */);

	redraw();
}


void UI_Canvas::RightButtonScroll(int mode)
{
	if (mode == 0)
		main_win->SetCursor(FL_CURSOR_DEFAULT);
	else if (mode == 1)
		main_win->SetCursor(FL_CURSOR_HAND);

	if (mode == 2)
	{
		int dx = Fl::event_x() - rbscroll_x;
		int dy = Fl::event_y() - rbscroll_y;

		double speed = 1.0;  // TODO: preference

		double o_dx = -dx * speed / Scale();
		double o_dy =  dy * speed / Scale();

		MoveOrigin(orig_x + o_dx, orig_y + o_dy);
	}

	rbscroll_x = Fl::event_x();
	rbscroll_y = Fl::event_y();
}


void UI_Canvas::GoHome()
{
	if (! map_is_loaded)
	{
		MoveOrigin(0, 0);
		return;
	}

	// compute bounds of level (to find center)

	int min_x, max_x;
	int min_y, max_y;

	VPO_GetBBox(&min_x, &min_y, &max_x, &max_y);

	MoveOrigin((min_x + max_x) / 2, (min_y + max_y) / 2);

	// determine zoom factor

	double zoom_x = 1;
	double zoom_y = 1;

	if (max_x > min_x) zoom_x = (max_x - min_x) / (double)MAX(1, w());
	if (max_y > min_y) zoom_y = (max_y - min_y) / (double)MAX(1, h());

	zoom_x = MAX(zoom_x, zoom_y) * 1.1;

	zoom = 1;
	while (zoom + 1 < MAX_ZOOM && zoom < zoom_x)
		zoom = zoom * 2;

	main_win->infobar->SetScale(zoom);

	MakeTiles(false /* scrolling */, false /* zooming */);
}


void UI_Canvas::MoveOrigin(double new_x, double new_y)
{
	orig_x = new_x;
	orig_y = new_y;

	MakeTiles(true /* scrolling */, false /* zooming */);

	redraw();
}


void UI_Canvas::Refresh()
{
	if (render_tiles)
		redraw();
}


int UI_Canvas::calcTileMemory() const
{
	int bytes = 0;

	Tile *t;

	for (t = tiles ; t ; t = t->next)
		bytes += sizeof(Tile);

	for (t = free_tiles ; t ; t = t->next)
		bytes += sizeof(Tile);

	return bytes;
}


float UI_Canvas::calcProgress() const
{
	int count = 0;
	int total = 0;

	for (Tile *t = tiles ; t ; t = t->next)
	{
		if (! isTileVisible(t))
			continue;

		t->calcProgress(&count, &total);
	}

	if (total == 0)
		return 0;

	return 100.0 * (double)count / (double)total;
}


Tile * UI_Canvas::NewTile(int base_x, int base_y, int step)
{
	Tile *t;

	// remove from free list (if possible)
	if (free_tiles)
	{
		t = free_tiles;

		free_tiles = free_tiles->next;
	}
	else
		t = new Tile;

	t->Allocate(base_x, base_y, zoom);

	return t;
}


void UI_Canvas::FreeTile(Tile *t)
{
	// Note: assumes tile is not in any list

	t->Free();

	// add to head of free list
	t->next = free_tiles;

	free_tiles = t;
}


void UI_Canvas::FreeAllTiles(Tile *list)
{
	while (list)
	{
		Tile *t = list;

		list = t->next;

		FreeTile(t);
	}
}


void UI_Canvas::KillTiles()
{
	if (tiles)
	{
		FreeAllTiles(tiles);
		tiles = NULL;
	}

	redraw();
}


void UI_Canvas::AddTile(Tile *t)
{
	t->next = tiles;

	tiles = t;
}


void UI_Canvas::MakeTiles(bool scrolling, bool zooming)
{
	if (! map_is_loaded)
		return;


	Tile *old_tiles = tiles;

	if (! (scrolling || zooming))
		FreeAllTiles(tiles);

	tiles = NULL;
	tile_rover = NULL;
	offscreen_rover = NULL;


	// the number we need on each axis
	int count_x = w() / 64 + 2;
	int count_y = h() / 64 + 2;

	count_x += EXTRA_TILE_W * 2;
	count_y += EXTRA_TILE_H * 2;


	tile_lx = MAPX(x());
	tile_ly = MAPY(y() + h());

	// align bottom left coordinate with a natural tile boundary
	// (absolutely essential for scrolling and zooming logic)
	tile_lx -= (tile_lx & (zoom * 64 - 1));
	tile_ly -= (tile_ly & (zoom * 64 - 1));

	tile_lx -= EXTRA_TILE_W * 64 * zoom;
	tile_ly -= EXTRA_TILE_H * 64 * zoom;

	tile_hx = tile_lx + count_x * 64 * zoom;
	tile_hy = tile_ly + count_y * 64 * zoom;


	for (int x = count_x-1 ; x >= 0 ; x--)
	for (int y = count_y-1 ; y >= 0 ; y--)
	{
		int base_x = tile_lx + x * 64 * zoom;
		int base_y = tile_ly + y * 64 * zoom;

		Tile *t = NULL;

		// for scrolling, try to reuse an existing tile
		if (scrolling)
			t = FindOldTile(old_tiles, base_x, base_y);

		if (! t)
			t = NewTile(base_x, base_y, zoom);

		AddTile(t);

		// for zooming, try to copy data from the previous tiles
		if (zooming)
			for (Tile *old_t = old_tiles ; old_t ; old_t = old_t->next)
				t->CopyData(old_t);
	}

	if (scrolling || zooming)
		FreeAllTiles(old_tiles);
}


Tile * UI_Canvas::FindOldTile(Tile * &list, int base_x, int base_y)
{
	// recreate the list
	Tile *orig = list;
	Tile *next = NULL;

	Tile *result = NULL;

	list = NULL;

	for (Tile *t = orig ; t ; t = next)
	{
		next = t->next;

		if (t->baseX() == base_x && t->baseY() == base_y)
		{
			// found it
			result = t;
		}
		else
		{
			// add it back to list
			t->next = list;
			list    = t;
		}
	}

	return result;
}


void UI_Canvas::ProcessOneTile()
{
	if (! tiles || ! map_is_loaded)
		return;

	// possibly process an off-screen tile as well

	offscreen_counter += 1;

	if (offscreen_counter >= OFFSCREEN_DIVISOR || ! tile_rover)
	{
		offscreen_counter = 0;

		ProcessOffscreenTile();
	}

	// on-screen tile logic...

	if (! tile_rover)
		tile_rover = tiles;

	while (tile_rover->isFree() ||
	       tile_rover->isComplete() ||
		   ! isTileVisible(tile_rover))
	{
		tile_rover = tile_rover->next;

		if (! tile_rover)
			return;
	}

	tile_rover->Process();

	tile_rover = tile_rover->next;
}


void UI_Canvas::ProcessOffscreenTile()
{
	if (! offscreen_rover)
		offscreen_rover = tiles;

	while (offscreen_rover->isFree() ||
	       offscreen_rover->isComplete() ||
		   isTileVisible(offscreen_rover))
	{
		offscreen_rover = offscreen_rover->next;

		if (! offscreen_rover)
			return;
	}

	offscreen_rover->Process();

	offscreen_rover = offscreen_rover->next;
}


void UI_Canvas::RenderTiles()
{
	for (Tile *t = tiles ; t ; t = t->next)
	{
		if (t->isFree())
			continue;

		RenderOneTile(t);
	}
}


bool UI_Canvas::isTileVisible(const Tile *t) const
{
	int map_dx = t->baseX() - tile_lx;
	int map_dy = t->baseY() - tile_ly;

	int screen_x = SCREENX(tile_lx) + map_dx / zoom;
	int screen_y = SCREENY(tile_ly) - map_dy / zoom - 63;

	if (screen_x + 64 <= x() || screen_x >= x() + w()) return false;
	if (screen_y + 64 <= y() || screen_y >= y() + h()) return false;

	return true;
}


void UI_Canvas::RenderOneTile(Tile *t)
{
	int map_dx = t->baseX() - tile_lx;
	int map_dy = t->baseY() - tile_ly;

	int screen_x = SCREENX(tile_lx) + map_dx / zoom;
	int screen_y = SCREENY(tile_ly) - map_dy / zoom - 63;

	// off screen?
	if (screen_x + 64 <= x() || screen_x >= x() + w() ||
		screen_y + 64 <= y() || screen_y >= y() + h())
		return;

	// buffer for a single row of pixels
	u8_t rgb_buf[64 * 3];

	const byte *palette;
	int shift;

	switch (render_mode)
	{
		case 'd':
			palette = drawseg_palette;
			shift   = 8;
			break;

		case 'o':
			palette = opening_palette;
			shift   = 16;
			break;

		case 's':
			palette = solidseg_palette;
			shift   = 24;
			break;

		case 'v':
		default:
			palette = visplane_palette;
			shift   = 0;
			break;
	}

	for (int dy = 63 ; dy >= 0 ; dy--, screen_y++)
	{
		u8_t *dest = rgb_buf;

		for (int dx = 0 ; dx < 64 ; dx++)
		{
			int val = (t->ReadNearest(dx, dy) >> shift) & 0xFF;

			const byte *src = &palette[val * 3];

			*dest++ = *src++;
			*dest++ = *src++;
			*dest++ = *src++;
		}

		fl_draw_image(rgb_buf, screen_x, screen_y, 64, 1);
	}
}


datum_t UI_Canvas::AccessDatum(int map_x, int map_y)
{
	// find tile that corresponds with this coordinate

	for (Tile *t = tiles ; t ; t = t->next)
	{
		int bx1 = t->baseX();
		int by1 = t->baseY();

		int bx2 = bx1 + t->baseStep() * 63;
		int by2 = by1 + t->baseStep() * 63;

		if (map_x < bx1 || map_x > bx2 ||
			map_y < by1 || map_y > by2)
			continue;

		int dx = (map_x - bx1) / t->baseStep();
		int dy = (map_y - by1) / t->baseStep();

		dx = CLAMP(0, dx, 63);
		dy = CLAMP(0, dy, 63);

		return t->ReadNearest(dx, dy);
	}

	return 0;  // unknown
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
