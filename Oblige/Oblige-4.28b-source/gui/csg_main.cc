//------------------------------------------------------------------------
//  2.5D Constructive Solid Geometry
//------------------------------------------------------------------------
//
//  Oblige Level Maker
//
//  Copyright (C) 2006-2010 Andrew Apted
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
#include "hdr_fltk.h"
#include "hdr_lua.h"

#include <algorithm>

#include "lib_util.h"
#include "main.h"
#include "m_lua.h"

#include "csg_main.h"
#include "csg_local.h"

#include "ui_dialog.h"


#define EPSILON  0.001


std::vector<csg_brush_c *> all_brushes;

std::vector<csg_entity_c *> all_entities;

std::string dummy_wall_tex;
std::string dummy_plane_tex;


slope_info_c::slope_info_c() :
      sx(0),sy(0), ex(1),ey(0),dz(0)
{ }

slope_info_c::slope_info_c(const slope_info_c *other) :
      sx(other->sx), sy(other->sy),
      ex(other->ex), ey(other->ey), dz(other->dz)
{ }

slope_info_c::~slope_info_c()
{ }


void slope_info_c::Reverse()
{
  std::swap(sx, ex);
  std::swap(sy, ey);

  dz = -dz;
}


double slope_info_c::GetAngle() const
{
  double xy_dist = ComputeDist(sx, sy, ex, ey);

  return CalcAngle(0, 0, xy_dist, dz);
}


double slope_info_c::CalcZ(double base_z, double x, double y) const
{
  double dx = (ex - sx);
  double dy = (ey - sy);

  double along = (x - sx) * dx + (y - dy) * dy;

  return base_z + dz * along / (dx*dx + dy*dy);
}



void csg_property_set_c::Add(const char *key, const char *value)
{
  dict[key] = std::string(value);
}

void csg_property_set_c::Remove(const char *key)
{
  dict.erase(key);
}


void csg_property_set_c::DebugDump()
{
  std::map<std::string, std::string>::iterator PI;

  fprintf(stderr, "{\n");

  for (PI = dict.begin() ; PI != dict.end() ; PI++)
  {
    fprintf(stderr, "  %s = \"%s\"\n", PI->first.c_str(), PI->second.c_str());
  }

  fprintf(stderr, "}\n");
}


const char * csg_property_set_c::getStr(const char *key, const char *def_val)
{
  std::map<std::string, std::string>::iterator PI = dict.find(key);

  if (PI == dict.end())
    return def_val;

  return PI->second.c_str();
}

double csg_property_set_c::getDouble(const char *key, double def_val)
{
  const char *str = getStr(key);

  return str ? atof(str) : def_val;
}

int csg_property_set_c::getInt(const char *key, int def_val)
{
  const char *str = getStr(key);

  return str ? I_ROUND(atof(str)) : def_val;
}


void csg_property_set_c::getHexenArgs(u8_t *arg5)
{
  arg5[0] = getInt("arg1");
  arg5[1] = getInt("arg2");
  arg5[2] = getInt("arg3");
  arg5[3] = getInt("arg4");
  arg5[4] = getInt("arg5");
}


brush_vert_c::brush_vert_c(csg_brush_c *_parent, double _x, double _y) :
      parent(_parent), x(_x), y(_y),
      face()
{ }

brush_vert_c::~brush_vert_c()
{ }


brush_plane_c::brush_plane_c(const brush_plane_c& other) :
    z(other.z), slope(NULL), face(other.face)
{
  // NOTE: slope not cloned
}
 
brush_plane_c::~brush_plane_c()
{
  // free slope ??   (or keep all slopes in big list)

  // free face ??  (or keep all faces in big list)
}


csg_brush_c::csg_brush_c() :
     bkind(BKIND_Solid), bflags(0),
     props(), verts(),
     b(-EXTREME_H),
     t( EXTREME_H)
{ }

csg_brush_c::csg_brush_c(const csg_brush_c *other) :
      bkind(other->bkind), bflags(other->bflags),
      props(other->props), verts(),
      b(other->b), t(other->t)
{
  // NOTE: verts and slopes not cloned

  bflags &= ~ BRU_IF_Quad;
}

csg_brush_c::~csg_brush_c()
{
  // FIXME: free verts

  // FIXME: free slopes
}


const char * csg_brush_c::Validate()
{
  if (verts.size() < 3)
    return "Line loop contains less than 3 vertices!";

  // FIXME: make sure brush is convex (co-linear lines is OK)

  // make sure vertices are anti-clockwise
  double average_ang = 0;

  bflags |= BRU_IF_Quad;

  for (unsigned int k = 0; k < verts.size(); k++)
  {
    brush_vert_c *v1 = verts[k];
    brush_vert_c *v2 = verts[(k+1) % (int)verts.size()];
    brush_vert_c *v3 = verts[(k+2) % (int)verts.size()];

    if (fabs(v2->x - v1->x) < EPSILON && fabs(v2->y - v1->y) < EPSILON)
      return "Line loop contains a zero length line!";

    double ang1 = CalcAngle(v2->x, v2->y, v1->x, v1->y);
    double ang2 = CalcAngle(v2->x, v2->y, v3->x, v3->y);

    double diff = ang1 - ang2;

    if (diff < 0)    diff += 360.0;
    if (diff >= 360) diff -= 360.0;

    if (diff > 180.1)
      return "Line loop is not convex!";

    average_ang += diff;

    if (fabs(v1->x - v2->x) >= EPSILON &&
        fabs(v1->y - v2->y) >= EPSILON)
    {
      bflags &= ~BRU_IF_Quad;  // not a quad
    } 
  }

  average_ang /= (double)verts.size();

// fprintf(stderr, "Average angle = %1.4f\n\n", average_ang);

  if (average_ang > 180.0)
    return "Line loop is not anti-clockwise!";

  return NULL; // OK
}

void csg_brush_c::ComputeBBox()
{
  min_x = +9e7;
  min_y = +9e7;
  max_x = -9e7;
  max_y = -9e7;

  for (unsigned int i = 0; i < verts.size(); i++)
  {
    brush_vert_c *V = verts[i];

    if (V->x < min_x) min_x = V->x;
    if (V->y < min_y) min_y = V->y;

    if (V->x > max_x) max_x = V->x;
    if (V->y > max_y) max_y = V->y;
  }
}


csg_entity_c::csg_entity_c() : id(), x(0), y(0), z(0), props()
{ }

csg_entity_c::~csg_entity_c()
{ }


bool csg_entity_c::Match(const char *want_name) const
{
  return (strcmp(id.c_str(), want_name) == 0);
}


//------------------------------------------------------------------------

int Grab_Properties(lua_State *L, int stack_pos,
                    csg_property_set_c *props,
                    bool skip_singles = false)
{
  if (stack_pos < 0)
    stack_pos += lua_gettop(L) + 1;

  if (lua_isnil(L, stack_pos))
    return 0;

  if (lua_type(L, stack_pos) != LUA_TTABLE)
    return luaL_argerror(L, stack_pos, "bad property table");

  for (lua_pushnil(L) ; lua_next(L, stack_pos) != 0 ; lua_pop(L,1))
  {
    // skip keys which are not strings
    if (lua_type(L, -2) != LUA_TSTRING)
      continue;

    const char *key = lua_tostring(L, -2);

    // optionally skip single letter keys ('x', 'y', etc)
    if (skip_singles && strlen(key) == 1)
      continue;

    // validate the value
    if (lua_type(L, -1) == LUA_TBOOLEAN)
    {
      props->Add(key, lua_toboolean(L, -1) ? "1" : "0");
      continue;
    }

    if (lua_type(L, -1) == LUA_TSTRING || lua_type(L, -1) == LUA_TNUMBER)
    {
      props->Add(key, lua_tostring(L, -1));
      continue;
    }

    // ignore other values (tables etc)

//// return luaL_error(L, "bad property: weird value for '%s'", key);
  }

  return 0;
}


static slope_info_c * Grab_Slope(lua_State *L, int stack_pos, bool is_ceil)
{
  if (stack_pos < 0)
    stack_pos += lua_gettop(L) + 1;

  if (lua_isnil(L, stack_pos))
    return NULL;

  if (lua_type(L, stack_pos) != LUA_TTABLE)
  {
    luaL_argerror(L, stack_pos, "missing table: slope info");
    return NULL; /* NOT REACHED */
  }

  slope_info_c *P = new slope_info_c();

  lua_getfield(L, stack_pos, "x1");
  lua_getfield(L, stack_pos, "y1");

  P->sx = luaL_checknumber(L, -2);
  P->sy = luaL_checknumber(L, -1);

  lua_pop(L, 2);

  lua_getfield(L, stack_pos, "x2");
  lua_getfield(L, stack_pos, "y2");
  lua_getfield(L, stack_pos, "dz");

  P->ex = luaL_checknumber(L, -3);
  P->ey = luaL_checknumber(L, -2);
  P->dz = luaL_checknumber(L, -1);

  lua_pop(L, 3);

  // floor slopes should have negative dz, and ceilings positive
  if ((is_ceil ? 1 : -1) * P->dz < 0)
  {
    // P->Reverse();
    luaL_error(L, "bad slope: dz should be <0 for floor, >0 for ceiling");
    return NULL; /* NOT REACHED */
  }

  return P;
}


int Grab_BrushMode(lua_State *L, const char *kind)
{
  // parse the 'm' field of the props table

  if (! kind)
  {
    // not present, return the default
    return BKIND_Solid;
  }

  if (StringCaseCmp(kind, "solid")  == 0) return BKIND_Solid;
  if (StringCaseCmp(kind, "detail") == 0) return BKIND_Detail;
  if (StringCaseCmp(kind, "clip")   == 0) return BKIND_Clip;

  if (StringCaseCmp(kind, "sky")    == 0) return BKIND_Sky;
  if (StringCaseCmp(kind, "liquid") == 0) return BKIND_Liquid;
  if (StringCaseCmp(kind, "rail")   == 0) return BKIND_Rail;
  if (StringCaseCmp(kind, "light")  == 0) return BKIND_Light;

  return luaL_error(L, "gui.add_brush: unknown kind '%s'", kind);
}


static int Grab_Vertex(lua_State *L, int stack_pos, csg_brush_c *B)
{
  if (stack_pos < 0)
    stack_pos += lua_gettop(L) + 1;

  if (lua_type(L, stack_pos) != LUA_TTABLE)
  {
    return luaL_error(L, "gui.add_brush: missing vertex info");
  }

  lua_getfield(L, stack_pos, "m");

  if (! lua_isnil(L, -1))
  {
    const char *kind_str = luaL_checkstring(L, -1);

    B->bkind = Grab_BrushMode(L, kind_str);

    Grab_Properties(L, stack_pos, &B->props, true);

    lua_pop(L, 1);

    return 0;
  }

  lua_pop(L, 1);

  lua_getfield(L, stack_pos, "slope");
  lua_getfield(L, stack_pos, "b");
  lua_getfield(L, stack_pos, "t");

  if (! lua_isnil(L, -2) || ! lua_isnil(L, -1))
  {
    if (lua_isnil(L, -2))  // top
    {
      B->t.z = luaL_checknumber(L, -1);
      B->t.slope = Grab_Slope(L, -3, false);

      Grab_Properties(L, stack_pos, &B->t.face, true);
    }
    else  // bottom
    {
      B->b.z = luaL_checknumber(L, -2);
      B->b.slope = Grab_Slope(L, -3, true);

      Grab_Properties(L, stack_pos, &B->b.face, true);
    }
  }
  else  // side info
  {
    brush_vert_c *V = new brush_vert_c(B);

    lua_getfield(L, stack_pos, "x");
    lua_getfield(L, stack_pos, "y");

    V->x = luaL_checknumber(L, -2);
    V->y = luaL_checknumber(L, -1);

    lua_pop(L, 2);

    Grab_Properties(L, stack_pos, &V->face, true);

    B->verts.push_back(V);
  }

  lua_pop(L, 3);  // slope, b, t

  return 0;
}


static int Grab_CoordList(lua_State *L, int stack_pos, csg_brush_c *B)
{
  if (lua_type(L, stack_pos) != LUA_TTABLE)
  {
    return luaL_argerror(L, stack_pos, "missing table: coords");
  }

  int index = 1;

  for (;;)
  {
    lua_pushinteger(L, index);
    lua_gettable(L, stack_pos);

    if (lua_isnil(L, -1))
    {
      lua_pop(L, 1);
      break;
    }

    Grab_Vertex(L, -1, B);

    lua_pop(L, 1);

    index++;
  }

  const char *err_msg = B->Validate();

  if (err_msg)
    return luaL_error(L, "%s", err_msg);

  B->ComputeBBox();

  if ((B->max_x - B->min_x) < EPSILON)
    return luaL_error(L, "Line loop has zero width!");

  if ((B->max_y - B->min_y) < EPSILON)
    return luaL_error(L, "Line loop has zero height!");

  return 0;
}


// LUA: begin_level()
//
int CSG_begin_level(lua_State *L)
{
  SYS_ASSERT(game_object);

  CSG_Main_Free();

  game_object->BeginLevel();

  return 0;
}


// LUA: end_level()
//
int CSG_end_level(lua_State *L)
{
  SYS_ASSERT(game_object);

  game_object->EndLevel();

  CSG_Main_Free();

  CSG_BSP_Free();

  return 0;
}


// LUA: property(key, value)
//
int CSG_property(lua_State *L)
{
  const char *key   = luaL_checkstring(L,1);
  const char *value = luaL_checkstring(L,2);

  // eat propertities intended for CSG2

  if (strcmp(key, "error_tex") == 0)
  {
    dummy_wall_tex = std::string(value);
    return 0;
  }
  else if (strcmp(key, "error_flat") == 0)
  {
    dummy_plane_tex = std::string(value);
    return 0;
  }

  SYS_ASSERT(game_object);

  game_object->Property(key, value);

  return 0;
}


// LUA: add_brush(coords)
//
// coords is a list of coordinates of the form:
//   { m="solid", ... }                     -- properties
//   { x=123, y=456,     tex="foo", ... }   -- side of brush
//   { b=200, s={ ... }, tex="bar", ... }   -- top of brush
//   { t=240, s={ ... }, tex="gaz", ... }   -- bottom of brush
//
// 'm' is the brush mode, default "solid", can also be
//     "sky", "liquid", "detail", "clip", etc..
//
// tops and bottoms are optional, when absent then it means the
// brush extends to infinity in that direction.
//
// 's' are slope specifications, which are optional.
// They contain these fields: { x1, y1, x2, y2, dz }
// When used, the slope must be "shrinky", i.e. the z1..z2 range needs
// to cover the entirety of the full (sloped) brush.
//
// the rest of the fields are for the FACE, and can be:
//
//    tex  :  texture name
//    
//    x_offset  BLAH  FIXME
//    y_offset
//    peg
//
//    delta_z   : a post-CSG height adjustment (top & bottom only)
//    mark      : separating number (top & bottom only)
//
//    kind   : DOOM sector or linedef type
//    flags  : DOOM linedef flags
//    tag    : DOOM sector or linedef tag
//    args   : DOOM sector or linedef args (a table)
//
int CSG_add_brush(lua_State *L)
{
  csg_brush_c *B = new csg_brush_c();

  Grab_CoordList(L, 1, B);

  if (B->props.getStr("flavor"))
    Main_FatalError("Flavored brush used.\n");

  all_brushes.push_back(B);

  return 0;
}


// LUA: add_entity(props)
//
//   id      -- number or name of thing
//   x y z   -- coordinates
//   angle
//   flags
//   light   -- amount of light emitted
//
//   etc...
//
int CSG_add_entity(lua_State *L)
{
  csg_entity_c *E = new csg_entity_c();

  Grab_Properties(L, 1, &E->props);

  E->id = E->props.getStr("id", "");
  
  E->x = E->props.getDouble("x");
  E->y = E->props.getDouble("y");
  E->z = E->props.getDouble("z");

  // save a bit of space
  E->props.Remove("id"); E->props.Remove("x");
  E->props.Remove("y");  E->props.Remove("z");

  all_entities.push_back(E);

  return 0;
}


//------------------------------------------------------------------------

void CSG_Main_Free()
{
  unsigned int k;

  for (k = 0 ; k < all_brushes.size() ; k++)
    delete all_brushes[k];

  for (k = 0 ; k < all_entities.size() ; k++)
    delete all_entities[k];

  all_brushes .clear();
  all_entities.clear();

  dummy_wall_tex .clear();
  dummy_plane_tex.clear();
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
