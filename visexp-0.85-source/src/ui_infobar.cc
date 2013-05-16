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

#include "headers.h"


//
// UI_InfoBar Constructor
//
UI_InfoBar::UI_InfoBar(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
	local_mode(0)
{
	end();  // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);

	color(FL_DARK2);


	// fitts' law : keep buttons flush with button of window
	Y += 4;
	H -= 4;


	mouse_x = new Fl_Output(X+90,    Y, 64, H, "Position:  x");
	mouse_y = new Fl_Output(X+90+82, Y, 64, H, "y");

	mouse_x->align(FL_ALIGN_LEFT);
	mouse_y->align(FL_ALIGN_LEFT);

//??	mouse_x->labelsize(KF_fonth); mouse_y->labelsize(KF_fonth);
//??	mouse_x->textsize(KF_fonth);  mouse_y->textsize(KF_fonth);

	add(mouse_x);
	add(mouse_y);

	X = mouse_y->x() + mouse_y->w();


	counter = new Fl_Output(X+94, Y, 130, H, "Count:");
	counter->align(FL_ALIGN_LEFT);
	counter->textfont(FL_COURIER);
	counter->value("");

	add(counter);

	X = counter->x() + counter->w();


	scale = new Fl_Output(X+70, Y, 64, H, "Scale:");
	scale->align(FL_ALIGN_LEFT);
	scale->value("1 / 32");

	add(scale);

	X = scale->x() + scale->w();


 	resizable(NULL);
}


//
// UI_InfoBar Destructor
//
UI_InfoBar::~UI_InfoBar()
{
}


//------------------------------------------------------------------------


void UI_InfoBar::SetMouse(double mx, double my)
{
	if (mx < -32767.0 || mx > 32767.0 ||
		my < -32767.0 || my > 32767.0)
	{
		mouse_x->value("off map");
		mouse_y->value("off map");

		return;
	}

	char x_buffer[60];
	char y_buffer[60];

	sprintf(x_buffer, "%d", I_ROUND(mx));
	sprintf(y_buffer, "%d", I_ROUND(my));

	mouse_x->value(x_buffer);
	mouse_y->value(y_buffer);
}


void UI_InfoBar::SetCount(int value, bool was_more, int limit)
{
	char buffer[120];

	if (was_more)
		sprintf(buffer, "%3d+/ %d", value, limit);
	else
		sprintf(buffer, "%3d / %d", value, limit);

	if (value >= limit)
		strcat(buffer, " OVF");

	counter->value(buffer);
}


void UI_InfoBar::SetCount(const char *val_str)
{
	counter->value(val_str);
}


void UI_InfoBar::SetScale(int zoom)
{
	char buffer[60];

	if (zoom == 1)
	{
		scale->value("100%");
	}
	else
	{
		sprintf(buffer, "1 / %d", zoom);

		scale->value(buffer);
	}
}


void UI_InfoBar::ShowDatum(char mode, datum_t d)
{
	if (local_mode != mode)
	{
		local_mode = mode;

		switch (mode)
		{
			case 'v': counter->label("Visplanes:"); break;
			case 'd': counter->label("Draw-segs:"); break;
			case 's': counter->label("Solid-segs:"); break;
			case 'o': counter->label("Openings:"); break;

			default: break;
		}

		redraw();
	}

	switch (d)
	{
		case DATUM_UNSET:
		case DATUM_VOID:
			SetCount("");
			return;

		case DATUM_CLOSED:
			SetCount("Closed");
			return;

		case DATUM_OVERFLOW:
			SetCount("Extreme!");
			return;

		default:  // a normal value
			break;
	}

	int visplanes = DATUM_GET_VISPLANES(d);
	int drawsegs  = DATUM_GET_DRAWSEGS(d);
	int openings  = DATUM_GET_OPENINGS(d);
	int solidsegs = DATUM_GET_SOLIDSEGS(d);

	bool was_more = false;

	switch (mode)
	{
		case 'v':
			was_more = ((d & 0xFF) == 250);
			SetCount(visplanes, was_more, limit_of_visplanes);
			break;

		case 'd':
			was_more = (((d >> 8) & 0xFF) == 250);
			SetCount(drawsegs, was_more, limit_of_drawsegs);
			break;

		case 's':
			SetCount(solidsegs, false, limit_of_solidsegs);
			break;

		case 'o':
			was_more = (((d >> 16) & 0xFF) == 250);
			SetCount(openings, was_more, limit_of_openings);
			break;

		default:
			break;
	}

}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
