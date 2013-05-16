//------------------------------------------------------------------------
//  Information Bar (bottom of window)
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

#ifndef __VPOZOOM_UI_INFOBAR_H__
#define __VPOZOOM_UI_INFOBAR_H__


class UI_InfoBar : public Fl_Group
{
private:
	char local_mode;

public:
	UI_InfoBar(int X, int Y, int W, int H, const char *label = NULL);
	virtual ~UI_InfoBar();

public:
	Fl_Output *mouse_x;
	Fl_Output *mouse_y;

	Fl_Output *counter;
	Fl_Output *scale;

public:
	void SetMouse(double mx, double my);

	void SetCount(int value, bool was_more, int limit);
	void SetCount(const char *val_str);

	void SetScale(int zoom);

	void ShowDatum(char mode, datum_t d);

private:

};

#endif  /* __VPOZOOM_UI_INFOBAR_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
