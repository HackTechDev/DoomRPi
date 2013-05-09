//----------------------------------------------------------------
//  Play Settings
//----------------------------------------------------------------
//
//  Oblige Level Maker
//
//  Copyright (C) 2006-2009 Andrew Apted
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
//----------------------------------------------------------------

#include "headers.h"
#include "hdr_fltk.h"
#include "hdr_lua.h"
#include "hdr_ui.h"

#include "lib_signal.h"
#include "lib_util.h"
#include "m_lua.h"
#include "main.h"


#define MY_RED  fl_rgb_color(224,0,0)


//
// Constructor
//
UI_Play::UI_Play(int x, int y, int w, int h, const char *label) :
    Fl_Group(x, y, w, h, label)
{
  end(); // cancel begin() in Fl_Group constructor
 
  box(FL_THIN_UP_BOX);

  if (! alternate_look)
    color(BUILD_BG, BUILD_BG);


  int y_step = 6 + KF;

  int cx = x + 84 + KF * 11;
  int cy = y + y_step + KF * 3;

  Fl_Box *heading = new Fl_Box(FL_NO_BOX, x+6, cy, w-12, 24, "Playing Style");
  heading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  heading->labeltype(FL_NORMAL_LABEL);
  heading->labelfont(FL_HELVETICA_BOLD);
  heading->labelsize(FL_NORMAL_SIZE + 2);

  add(heading);

  cy += heading->h() + y_step;


  int cw = 114 + KF * 16;
  int ch = 24 + KF * 2;

  mons = new UI_RChoice(cx, cy, cw, ch, "Monsters: ");
  mons->align(FL_ALIGN_LEFT);
  mons->selection_color(MY_RED);
  mons->callback(callback_Monsters, this);

  setup_Monsters();

  add(mons);

  cy += mons->h() + y_step;


  strength = new UI_RChoice(cx, cy, cw, ch, "Strength: ");
  strength->align(FL_ALIGN_LEFT);
  strength->selection_color(MY_RED);
  strength->callback(callback_Strength, this);

  setup_Strength();

  add(strength);

  cy += strength->h() + y_step;

  cy += y_step + y_step; //  /2 - 1;


  health = new UI_RChoice(cx, cy, cw, ch, "Health: ");
  health->align(FL_ALIGN_LEFT);
  health->selection_color(MY_RED);
  health->callback(callback_Health, this);

  setup_Health();

  add(health);

  cy += health->h() + y_step;


  ammo = new UI_RChoice(cx, cy, cw, ch, "Ammo: ");
  ammo->align(FL_ALIGN_LEFT);
  ammo->selection_color(MY_RED);
  ammo->callback(callback_Ammo, this);
 
  setup_Ammo();

  add(ammo);
  
  cy += ammo->h() + y_step;

//  cy += y_step + y_step/2 - 1;


  powers = new UI_RChoice(cx, cy, cw, ch, "Powerups: ");
  powers->align(FL_ALIGN_LEFT);
  powers->selection_color(MY_RED);
  powers->callback(callback_Powers, this);

  setup_Powers();

  add(powers);

  cy += powers->h() + y_step;


//  DebugPrintf("UI_Play: final h = %d\n", cy - y);

  Signal_Watch("mode", notify_Mode, this);
}


//
// Destructor
//
UI_Play::~UI_Play()
{
}


void UI_Play::Locked(bool value)
{
  if (value)
  {
    mons  ->deactivate();
    strength->deactivate();
    powers->deactivate();
    health->deactivate();
    ammo  ->deactivate();
  }
  else
  {
    mons  ->activate();
    strength->activate();
    powers->activate();
    health->activate();
    ammo  ->activate();
  }
}


void UI_Play::notify_Mode(const char *name, void *priv_dat)
{
  if (! main_win)
    return;

  UI_Play *play = (UI_Play *)priv_dat;
  SYS_ASSERT(play);

  const char *mode = main_win->game_box->mode->GetID();

  if (strcmp(mode, "dm") == 0)
  {
    play->mons->label ("Players: ");
    play->powers->label("Weapons: ");
  }
  else
  {
    play->mons->label ("Monsters: ");
    play->powers->label("Powerups: ");
  }

  play->redraw();
}


//----------------------------------------------------------------

void UI_Play::callback_Monsters(Fl_Widget *w, void *data)
{
  UI_Play *that = (UI_Play *) data;

  ob_set_config("mons", that->mons->GetID());
}

void UI_Play::callback_Strength(Fl_Widget *w, void *data)
{
  UI_Play *that = (UI_Play *) data;

  ob_set_config("strength", that->strength->GetID());
}

void UI_Play::callback_Powers(Fl_Widget *w, void *data)
{
  UI_Play *that = (UI_Play *) data;

  ob_set_config("powers", that->powers->GetID());
}

void UI_Play::callback_Health(Fl_Widget *w, void *data)
{
  UI_Play *that = (UI_Play *) data;

  ob_set_config("health", that->health->GetID());
}

void UI_Play::callback_Ammo(Fl_Widget *w, void *data)
{
  UI_Play *that = (UI_Play *) data;

  ob_set_config("ammo", that->ammo->GetID());
}

void UI_Play::Defaults()
{
  ParseValue("mons",    "normal");
  ParseValue("strength","medium");
  ParseValue("powers",  "normal");
  ParseValue("health",  "normal");
  ParseValue("ammo",    "normal");
}
 
bool UI_Play::ParseValue(const char *key, const char *value)
{
  if (StringCaseCmp(key, "mons") == 0)
  {
    mons->SetID(value);
    callback_Monsters(NULL, this);
    return true;
  }

  if (StringCaseCmp(key, "strength") == 0)
  {
    strength->SetID(value);
    callback_Strength(NULL, this);
    return true;
  }

  if (StringCaseCmp(key, "powers") == 0)
  {
    powers->SetID(value);
    callback_Powers(NULL, this);
    return true;
  }

  if (StringCaseCmp(key, "health") == 0)
  {
    health->SetID(value);
    callback_Health(NULL, this);
    return true;
  }

  if (StringCaseCmp(key, "ammo") == 0)
  {
    ammo->SetID(value);
    callback_Ammo(NULL, this);
    return true;
  }

  return false;
}


//----------------------------------------------------------------

const char * UI_Play::monster_syms[] =
{
  // also used for: Players (DM)

  "none",   "NONE",
  "scarce", "Scarce",
  "less",   "Less",
  "normal", "Normal",
  "more",   "More",
  "heaps",  "Hordes",
  "nuts",   "Nuts!",

  "prog",   "Progressive",
  "mixed",  "Mix It Up",

  NULL, NULL
};

const char * UI_Play::strength_syms[] =
{
  "weak",   "Weak",
  "medium", "Medium",
  "tough",  "Tough",
  "crazy",  "Crazy",

  NULL, NULL
};

const char * UI_Play::power_syms[] =
{
  // also used for: Weapons (DM)

  "none",   "NONE",  
  "less",   "Less",  
  "normal", "Normal",
  "more",   "More", 

  "mixed",  "Mix It Up",

  NULL, NULL
};

const char * UI_Play::health_syms[] =
{
  // also used for: Ammo

  "none",   "NONE",
  "scarce", "Scarce",
  "less",   "Less",
  "normal", "Normal",
  "more",   "More",
  "heaps",  "Heaps",

  NULL, NULL
};

void UI_Play::setup_Monsters()
{
  for (int i = 0; monster_syms[i]; i += 2)
  {
    mons->AddPair(monster_syms[i], monster_syms[i+1]);
    mons->ShowOrHide(monster_syms[i], 1);
  }
}

void UI_Play::setup_Strength()
{
  for (int i = 0; strength_syms[i]; i += 2)
  {
    strength->AddPair(strength_syms[i], strength_syms[i+1]);
    strength->ShowOrHide(strength_syms[i], 1);
  }
}

void UI_Play::setup_Powers()
{
  for (int i = 0; power_syms[i]; i += 2)
  {
    powers->AddPair(power_syms[i], power_syms[i+1]);
    powers->ShowOrHide(power_syms[i], 1);
  }
}

void UI_Play::setup_Health()
{
  for (int i = 0; health_syms[i]; i += 2)
  {
    health->AddPair(health_syms[i], health_syms[i+1]);
    health->ShowOrHide(health_syms[i], 1);
  }

}

void UI_Play::setup_Ammo()
{
  for (int i = 0; health_syms[i]; i += 2)
  {
    ammo->AddPair(health_syms[i], health_syms[i+1]);
    ammo->ShowOrHide(health_syms[i], 1);
  }
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
