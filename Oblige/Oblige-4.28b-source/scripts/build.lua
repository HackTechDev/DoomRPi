----------------------------------------------------------------
--  BUILDING TOOLS
----------------------------------------------------------------
--
--  Oblige Level Maker
--
--  Copyright (C) 2006-2012 Andrew Apted
--
--  This program is free software; you can redistribute it and/or
--  modify it under the terms of the GNU General Public License
--  as published by the Free Software Foundation; either version 2
--  of the License, or (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
----------------------------------------------------------------


GLOBAL_SKIN_DEFAULTS =
{
  outer = "?wall"
  fence = "?wall"
  floor = "?wall"
  ceil  = "?floor"

  tag = ""
  special = ""
  light = ""
  style = ""
  message = ""
  wait = ""
  targetname = ""
}


CSG_BRUSHES =
{
  solid  = 1
  detail = 1
  clip   = 1
  sky    = 1
  liquid = 1
  rail   = 1
  light  = 1
}


DOOM_LINE_FLAGS =
{
  blocked     = 0x01
  block_mon   = 0x02
  sound_block = 0x40

  draw_secret = 0x20
  draw_never  = 0x80
  draw_always = 0x100

-- BOOM:  pass_thru = 0x200
}


HEXEN_ACTIONS =
{
  W1 = 0x0000, WR = 0x0200,  -- walk
  S1 = 0x0400, SR = 0x0600,  -- switch
  M1 = 0x0800, MR = 0x0A00,  -- monster
  G1 = 0x0c00, GR = 0x0E00,  -- gun / projectile
  B1 = 0x1000, BR = 0x1200,  -- bump
}


function raw_add_brush(brush)
  gui.add_brush(brush)

  if GAME.add_brush_func then
     GAME.add_brush_func(brush)
  end
end


function raw_add_entity(ent)
  if GAME.format == "quake" then
    ent.mangle = ent.angles ; ent.angles = nil
  end

  gui.add_entity(ent)

  if GAME.add_entity_func then
     GAME.add_entity_func(ent)
  end
end


function raw_add_model(model)
  assert(model.entity)

  model.entity.model = gui.q1_add_mapmodel(model)

  gui.add_entity(model.entity)

  if GAME.add_model_func then
     GAME.add_model_func(model)
  end
end


function Brush_collect_flags(C)
  if GAME.format == "doom" then
    local flags = C.flags or 0

    if C.act and PARAM.sub_format == "hexen" then
      local spac = HEXEN_ACTIONS[C.act]
      if not spac then
        error("Unknown act value: " .. tostring(C.act))
      end
      flags = bit.bor(flags, spac)
    end

    for name,value in pairs(DOOM_LINE_FLAGS) do
      if C[name] and C[name] != 0 then
        flags = bit.bor(flags, value)
      end
    end

    if flags != 0 then
      C.flags = flags

      -- this makes sure the flags get applied
      if not C.special then C.special = 0 end
    end
  end
end


function brush_helper(brush)
  local mode = brush[1].m

  -- light and rail brushes only make sense for 2.5D games
  if mode == "light" and not PARAM.light_brushes then return end
  if mode == "rail"  and not PARAM.rails then return end

  each C in brush do
    Brush_collect_flags(C)
  end

  raw_add_brush(brush)
end


function entity_helper(name, x, y, z, props)
  assert(name)

  if PARAM.light_brushes and name == "light" then
    return
  end

  local ent

  if props then
    ent = table.copy(props)
  else
    ent = {}
  end

  local info = GAME.ENTITIES[name] or
               GAME.MONSTERS[name] or
               GAME.WEAPONS[name] or
               GAME.PICKUPS[name]

  if not info then
    gui.printf("\nLACKING ENTITY : %s\n\n", name)
    return
  end

  local delta_z = info.delta_z or PARAM.entity_delta_z

  ent.id = assert(info.id)

  ent.x = x
  ent.y = y
  ent.z = z + (delta_z or 0)

  if info.spawnflags then
    ent.spawnflags = ((props and props.spawnflags) or 0) + info.spawnflags
  end

  if info.fields then
    each name,value in info.fields do ent[name] = value end
  end

  raw_add_entity(ent)
end


function Brush_new_quad(x1,y1, x2,y2, b,t)
  local coords =
  {
    { x=x1, y=y1 }
    { x=x2, y=y1 }
    { x=x2, y=y2 }
    { x=x1, y=y2 }
  }

  if b then table.insert(coords, { b=b }) end
  if t then table.insert(coords, { t=t }) end

  return coords
end


function Brush_new_triangle(x1,y1, x2,y2, x3,y3, b,t)
  local coords =
  {
    { x=x1, y=y1 }
    { x=x2, y=y2 }
    { x=x3, y=y3 }
  }

  if b then table.insert(coords, { b=b }) end
  if t then table.insert(coords, { t=t }) end

  return coords
end



function Brush_dump(brush, title)
  gui.debugf("%s:\n{\n", title or "Brush")

  for _,C in ipairs(brush) do
    local field_list = {}

    for name,val in pairs(C) do
      local pos
      if name == "m" or name == "x" or name == "b" or name == "t" then
        pos = 1
      elseif name == "y" then
        pos = 2
      end

      if pos then
        table.insert(field_list, pos, name) 
      else
        table.insert(field_list, name)
      end
    end

    local line = ""

    for idx,name in ipairs(field_list) do
      local val = C[name]
      
      if idx > 1 then line = line .. ", " end

      line = line .. string.format("%s=%s", name, tostring(val))
    end

    gui.debugf("  { %s }\n", line)
  end

  gui.debugf("|\n")
end


function Brush_copy(brush)
  local newb = {}

  for _,C in ipairs(brush) do
    table.insert(newb, table.copy(C))
  end

  return newb
end


function Brush_bbox(brush)
  local x1, x2 = 9e9, -9e9
  local y1, y2 = 9e9, -9e9

  for _,C in ipairs(brush) do
    if C.x then
      x1 = math.min(x1, C.x) ; x2 = math.max(x2, C.x)
      y1 = math.min(y1, C.y) ; y2 = math.max(y2, C.y)
    end
  end

  assert(x1 < x2)
  assert(y1 < y2)

  return x1,y1, x2,y2
end


function Brush_add_top(brush, z, mat)
  table.insert(brush, { t=z, mat=mat })
end


function Brush_add_bottom(brush, z, mat)
  table.insert(brush, { b=z, mat=mat })
end


function Brush_get_b(brush)
  for _,C in ipairs(brush) do
    if C.b then return C.b end
  end
end


function Brush_get_t(brush)
  for _,C in ipairs(brush) do
    if C.t then return C.t end
  end
end


function Brush_is_quad(brush)
  local x1,y1, x2,y2 = Brush_bbox(brush)

  for _,C in ipairs(brush) do
    if C.x then
      if C.x > x1+0.1 and C.x < x2-0.1 then return false end
      if C.y > y1+0.1 and C.y < y2-0.1 then return false end
    end
  end

  return true
end


function Brush_line_cuts_brush(brush, px1, py1, px2, py2)
  local front, back

  for _,C in ipairs(brush) do
    if C.x then
      local d = geom.perp_dist(C.x, C.y, px1,py1, px2,py2)
      if d >  0.5 then front = true end
      if d < -0.5 then  back = true end

      if front and back then return true end
    end
  end

  return false
end


function Brush_cut(brush, px1, py1, px2, py2)
  -- returns the cut-off piece (on back of given line)
  -- NOTE: assumes the line actually cuts the brush!

  local newb = {}

  -- transfer XY coords to a separate list for processing
  local coords = {}

  for index = #brush,1,-1 do
    local C = brush[index]
    
    if C.x then
      table.insert(coords, 1, table.remove(brush, index))
    else
      -- copy non-XY-coordinates into new brush
      table.insert(newb, table.copy(C))
    end
  end

  for idx,C in ipairs(coords) do
    local k = 1 + (idx % #coords)

    local cx2 = coords[k].x
    local cy2 = coords[k].y

    local a = geom.perp_dist(C.x, C.y, px1,py1, px2,py2)
    local b = geom.perp_dist(cx2, cy2, px1,py1, px2,py2)

    local a_side = 0
    local b_side = 0
    
    if a < -0.5 then a_side = -1 end
    if a >  0.5 then a_side =  1 end

    if b < -0.5 then b_side = -1 end
    if b >  0.5 then b_side =  1 end

    if a_side >= 0 then table.insert(brush, C) end
    if a_side <= 0 then table.insert(newb,  table.copy(C)) end

    if a_side != 0 and b_side != 0 and a_side != b_side then
      -- this edge crosses the cutting line --

      -- calc the intersection point
      local along = a / (a - b)

      local ix = C.x + along * (cx2 - C.x)
      local iy = C.y + along * (cy2 - C.y)

      local C1 = table.copy(C) ; C1.x = ix ; C1.y = iy
      local C2 = table.copy(C) ; C2.x = ix ; C2.y = iy

      table.insert(brush, C1)
      table.insert(newb,  C2)
    end
  end

  return newb
end


function Brush_clip_list_to_rects(brushes, rects)
  local process_list = {}

  -- transfer brushes to a separate list for processing, new brushes
  -- will be added back into the 'brushes' lists (if any).
  for index = 1,#brushes do
    table.insert(process_list, table.remove(brushes))
  end
  
  local function clip_to_line(B, x1, y1, x2, y2)
    if Brush_line_cuts_brush(B, x1, y1, x2, y2) then
       Brush_cut(B, x1, y1, x2, y2)
    end
  end

  local function clip_brush(B, R)
    B = Brush_copy(B)

    clip_to_line(B, R.x1, R.y1, R.x1, R.y2)  -- left
    clip_to_line(B, R.x2, R.y2, R.x2, R.y1)  -- right
    clip_to_line(B, R.x1, R.y2, R.x2, R.y2)  -- top
    clip_to_line(B, R.x2, R.y1, R.x1, R.y1)  -- bottom

    local bx1,by1, bx2,by2 = Brush_bbox(B)

    -- it lies completely outside the rectangle?
    if bx2 <= R.x1 + 1 or bx1 >= R.x2 - 1 then return end
    if by2 <= R.y1 + 1 or by1 >= R.y2 - 1 then return end

    table.insert(brushes, B)
  end

  for _,B in ipairs(process_list) do
    for _,R in ipairs(rects) do
      clip_brush(B, R)
    end
  end
end


------------------------------------------------------------------------

function Mat_prepare_trip()

  -- build the psychedelic mapping
  local m_before = {}
  local m_after  = {}

  for m,def in pairs(GAME.MATERIALS) do
    if not def.sane and
       not def.rail_h and
       not (string.sub(m,1,1) == "_") and
       not (string.sub(m,1,2) == "SW") and
       not (string.sub(m,1,3) == "BUT")
    then
      table.insert(m_before, m)
      table.insert(m_after,  m)
    end
  end

  rand.shuffle(m_after)

  LEVEL.psycho_map = {}

  for i = 1,#m_before do
    LEVEL.psycho_map[m_before[i]] = m_after[i]
  end
end


function Mat_lookup(name)
  if not name then name = "_ERROR" end

  if OB_CONFIG.theme == "psycho" and LEVEL.psycho_map[name] then
    name = LEVEL.psycho_map[name]
  end

  local mat = GAME.MATERIALS[name]

  -- special handling for DOOM / HERETIC switches
  if not mat and string.sub(name,1,3) == "SW2" then
    local sw1_name = "SW1" .. string.sub(name,4)
    mat = GAME.MATERIALS[sw1_name]
    if mat and mat.t == sw1_name then
      mat = { t=name, f=mat.f }  -- create new SW2XXX material
      GAME.MATERIALS[name] = mat
    end
  end

  if not mat then
    gui.printf("\nLACKING MATERIAL : %s\n\n", name)
    mat = assert(GAME.MATERIALS["_ERROR"])

    -- prevent further messages
    GAME.MATERIALS[name] = mat
  end

  return mat
end


function Mat_similar(A, B)
  A = GAME.MATERIALS[A]
  B = GAME.MATERIALS[B]

  if A and B then
    if A.t == B.t then return true end
    if A.f and A.f == B.f then return true end
  end

  return false
end


function Brush_set_tex(brush, wall, flat)
  each C in brush do
    if wall and C.x and not C.tex then
      C.tex = wall
    end
    if flat and (C.b or C.t) and not C.tex then
      C.tex = flat
    end
  end
end


function Brush_set_mat(brush, wall, flat)
  if wall then
    wall = Mat_lookup(wall)
    wall = assert(wall.t)
  end

  if flat then
    flat = Mat_lookup(flat)
    flat = flat.f or assert(flat.t)
  end

  Brush_set_tex(brush, wall, flat)
end


function Brush_has_sky(brush)
  each C in brush do
    if C.mat == "_SKY" then return true end
  end

  return false
end


------------------------------------------------------------------------


Trans = {}


Trans.TRANSFORM =
{
  -- mirror_x  : flip horizontally (about given X)
  -- mirror_y  : flip vertically (about given Y)
  -- mirror_z

  -- groups_x  : coordinate remapping groups
  -- groups_y
  -- groups_z

  -- scale_x   : scaling factor
  -- scale_y
  -- scale_z

  -- rotate    : angle in degrees, counter-clockwise,
  --             rotates about the origin

  -- add_x     : translation, i.e. new origin coords
  -- add_y
  -- add_z

  -- fitted_x  : sizes which a "fitted" prefab needs to become
  -- fitted_y
  -- fitted_z
}


--[[
struct GROUP
{
  low,  high,  size   : the original coordinate range
  low2, high2, size2  : the new coordinate range

  weight  : relative weight of this group (in relation to others).
            needed for setting up the groups, but not for using them.
}
--]]


function Trans.clear()
  Trans.TRANSFORM = { }
end

function Trans.set(T)
  Trans.TRANSFORM = table.copy(T)
end

function Trans.set_pos(x, y, z)
  Trans.TRANSFORM = { add_x=x, add_y=y, add_z=z }
end

function Trans.modify(what, value)
  Trans.TRANSFORM[what] = value
end


function Trans.set_cap(z1, z2)
  -- either z1 or z2 can be nil
  Trans.z1_cap = z1
  Trans.z2_cap = z2
end

function Trans.clear_cap()
  Trans.z1_cap = nil
  Trans.z2_cap = nil
end


function Trans.dump(T, title)
  -- debugging aid : show current transform

  gui.debugf("%s:\n", title or "Transform")

  if not T then
    T = Trans.TRANSFORM
    assert(T)
  end

  if T.mirror_x then gui.debugf("  mirror_x = %1.0f\n", T.mirror_x) end
  if T.mirror_y then gui.debugf("  mirror_y = %1.0f\n", T.mirror_y) end
  if T.mirror_z then gui.debugf("  mirror_z = %1.0f\n", T.mirror_z) end

  if T.scale_x then gui.debugf("  scale_x = %1.0f\n", T.scale_x) end
  if T.scale_y then gui.debugf("  scale_y = %1.0f\n", T.scale_y) end
  if T.scale_z then gui.debugf("  scale_z = %1.0f\n", T.scale_z) end

  if T.rotate then gui.debugf("  ROTATE = %1.1f\n", T.rotate) end

  if T.add_x then gui.debugf("  add_x = %1.0f\n", T.add_x) end
  if T.add_y then gui.debugf("  add_y = %1.0f\n", T.add_y) end
  if T.add_z then gui.debugf("  add_z = %1.0f\n", T.add_z) end

  if T.fitted_x then gui.debugf("  fitted_x = %1.0f\n", T.fitted_x) end
  if T.fitted_y then gui.debugf("  fitted_y = %1.0f\n", T.fitted_y) end
  if T.fitted_z then gui.debugf("  fitted_z = %1.0f\n", T.fitted_z) end

  local function dump_groups(name, groups)
    gui.debugf("  %s =\n", name)
    gui.debugf("  {\n")

    each G in groups do
      gui.debugf("    (%1.0f %1.0f %1.0f) --> (%1.0f %1.0f %1.0f)  wt:%1.2f\n",
                 G.low  or -1, G.high  or -1, G.size  or -1,
                 G.low2 or -1, G.high2 or -1, G.size2 or -1, G.weight)
    end

    gui.debugf("  }\n")
  end

  if T.groups_x then dump_groups("groups_x", T.groups_x) end
  if T.groups_y then dump_groups("groups_y", T.groups_y) end
  if T.groups_z then dump_groups("groups_z", T.groups_z) end
end


function Trans.remap_coord(groups, n)
  if not groups then return n end

  local T = #groups
  assert(T >= 1)

  if n <= groups[1].low  then return n + (groups[1].low2 -  groups[1].low)  end
  if n >= groups[T].high then return n + (groups[T].high2 - groups[T].high) end

  local idx = 1

  while (idx < T) and (n > groups[idx].high) do
    idx = idx + 1
  end

  local G = groups[idx]

  return G.low2 + (n - G.low) * G.size2 / G.size;
end


function Trans.apply_xy(x, y)
  local T = Trans.TRANSFORM

  -- apply mirroring first
  if T.mirror_x then x = T.mirror_x * 2 - x end
  if T.mirror_y then y = T.mirror_y * 2 - y end

  -- apply groups
  if T.groups_x then x = Trans.remap_coord(T.groups_x, x) end
  if T.groups_y then y = Trans.remap_coord(T.groups_y, y) end

  -- apply scaling
  x = x * (T.scale_x or 1)
  y = y * (T.scale_y or 1)

  -- apply rotation
  if T.rotate then
    x, y = geom.rotate_vec(x, y, T.rotate)
  end

  -- apply translation last
  x = x + (T.add_x or 0)
  y = y + (T.add_y or 0)

  return x, y
end


function Trans.apply_z(z)
  local T = Trans.TRANSFORM

  -- apply mirroring first
  if T.mirror_z then z = T.mirror_z * 2 - z end

  -- apply groups
  if T.groups_z then z = Trans.remap_coord(T.groups_z, z) end

  -- apply scaling
  z = z * (T.scale_z or 1)

  -- apply translation last
  z = z + (T.add_z or 0)

  return z
end


function Trans.apply_slope(slope)
  if not slope then return nil end

  local T = Trans.TRANSFORM

  slope = table.copy(slope)

  slope.x1, slope.y1 = Trans.apply_xy(slope.x1, slope.y1)
  slope.x2, slope.y2 = Trans.apply_xy(slope.x2, slope.y2)

  if T.mirror_z then slope.dz = - slope.dz end

  slope.dz = slope.dz * (T.scale_z or 1)

  return slope
end


function Trans.apply_angle(ang)
  local T = Trans.TRANSFORM

  if not (T.rotate or T.mirror_x or T.mirror_y) then
    return ang
  end

  if T.mirror_x or T.mirror_y then
    local dx = math.cos(ang * math.pi / 180)
    local dy = math.sin(ang * math.pi / 180)

    if T.mirror_x then dx = -dx end
    if T.mirror_y then dy = -dy end

    ang = math.round(geom.calc_angle(dx, dy))
  end

  if T.rotate then ang = ang + T.rotate end

  if ang >= 360 then ang = ang - 360 end
  if ang <    0 then ang = ang + 360 end

  return ang
end


function Trans.apply_mlook(ang)
  local T = Trans.TRANSFORM

  if T.mirror_z then
    if ang == 0 then return 0 end
    return 360 - ang
  else
    return ang
  end
end


-- handle three-part angle strings (Quake)
function Trans.apply_angles_xy(ang_str)
  local mlook, angle, roll = string.match(ang_str, "(%d+) +(%d+) +(%d+)")
  angle = Trans.apply_angle(0 + angle)
  return string.format("%d %d %d", mlook, angle, roll)
end


function Trans.apply_angles_z(ang_str)
  local mlook, angle, roll = string.match(ang_str, "(%d+) +(%d+) +(%d+)")
  mlook = Trans.apply_mlook(0 + mlook)
  return string.format("%d %d %d", mlook, angle, roll)
end



function Trans.adjust_spot(x1,y1, x2,y2, z1,z2)  -- not used atm
  local T = Trans.TRANSFORM

  local x_size = (x2 - x1) * (T.scale_x or 1)
  local y_size = (y2 - y1) * (T.scale_y or 1)
  local z_size = (z2 - z1) * (T.scale_z or 1)

  local spot = {}

  spot.x, spot.y = Trans.apply_xy((x1+x2) / 2, (y1+y2) / 2)

  spot.z = Trans.apply_z(z1)

  spot.r = math.min(x_size, y_size)
  spot.h = z_size

  -- when rotated, find largest square that will fit inside it
  if T.rotate then
    local k = T.rotate % 90
    if k > 45 then k = 90 - k end

    local t = math.tan((45 - k) * math.pi / 180.0)
    local s = math.sqrt(0.5 * (1 + t*t))

    spot.r = spot.r * s
  end

  return spot
end


function Trans.spot_transform(x, y, z, dir)
  local ANGS = { [2]=0, [8]=180, [4]=270, [6]=90 }

  return
  {
    add_x = x
    add_y = y
    add_z = z
    rotate = ANGS[dir]
  }
end


function Trans.box_transform(x1, y1, x2, y2, z, dir)
  local XS   = { [2]=x1, [8]= x2, [4]= x1, [6]=x2 }
  local YS   = { [2]=y1, [8]= y2, [4]= y2, [6]=y1 }
  local ANGS = { [2]=0,  [8]=180, [4]=270, [6]=90 }

  local T = {}

  T.add_x = XS[dir]
  T.add_y = YS[dir]
  T.add_z = z

  T.rotate = ANGS[dir]

  if geom.is_vert(dir) then
    T.fitted_x = x2 - x1
    T.fitted_y = y2 - y1
  else
    T.fitted_x = y2 - y1
    T.fitted_y = x2 - x1
  end

  return T
end


function Trans.corner_transform(x1,y1, x2,y2, z, side, horiz, vert)
  local XS   = { [1]=x1, [9]= x2, [7]= x1, [3]=x2 }
  local YS   = { [1]=y1, [9]= y2, [7]= y2, [3]=y1 }
  local ANGS = { [1]=0,  [9]=180, [7]=270, [3]=90 }

  local T = {}

  T.add_x = XS[side]
  T.add_y = YS[side]
  T.add_z = z

  T.rotate = ANGS[side]

  if side == 1 or side == 9 then
    T.fitted_x = horiz
    T.fitted_y = vert
  else
    T.fitted_x = vert
    T.fitted_y = horiz
  end

  return T
end


function Trans.edge_transform(x1,y1, x2,y2, z, side, long1, long2, out, back)
  if side == 4 then x2 = x1 + out ; x1 = x1 - back end
  if side == 6 then x1 = x2 - out ; x2 = x2 + back end
  if side == 2 then y2 = y1 + out ; y1 = y1 - back end
  if side == 8 then y1 = y2 - out ; y2 = y2 + back end

  if side == 2 then x1 = x2 - long2 ; x2 = x2 - long1 end
  if side == 8 then x2 = x1 + long2 ; x1 = x1 + long1 end
  if side == 4 then y2 = y1 + long2 ; y1 = y1 + long1 end
  if side == 6 then y1 = y2 - long2 ; y2 = y2 - long1 end

  return Trans.box_transform(x1,y1, x2,y2, z, side)
end


function Trans.set_fitted_z(T, z1, z2)
  T.add_z    = z1
  T.fitted_z = z2 - z1
end


function Trans.categorize_linkage(dir2, dir4, dir6, dir8)
  local link_str = ""

  if dir2 then link_str = link_str .. '2' end
  if dir4 then link_str = link_str .. '4' end
  if dir6 then link_str = link_str .. '6' end
  if dir8 then link_str = link_str .. '8' end

  -- nothing?
  if link_str == "" then
    return 'N', 2

  -- facing one direction
  elseif link_str == "2" then
    return 'F', 2

  elseif link_str == "4" then
    return 'F', 4

  elseif link_str == "6" then
    return 'F', 6

  elseif link_str == "8" then
    return 'F', 8

  -- straight through
  elseif link_str == "28" then
    return 'I', 2

  elseif link_str == "46" then
    return 'I', 4
  
  -- corner
  elseif link_str == "26" then
    return 'C', 6

  elseif link_str == "24" then
    return 'C', 2

  elseif link_str == "48" then
    return 'C', 4

  elseif link_str == "68" then
    return 'C', 8
  
  -- T junction
  elseif link_str == "246" then
    return 'T', 2

  elseif link_str == "248" then
    return 'T', 4

  elseif link_str == "268" then
    return 'T', 6

  elseif link_str == "468" then
    return 'T', 8

  -- plus shape, all four directions
  elseif link_str == "2468" then
    return 'P', 2

  else
    error("categorize_linkage failed on: " .. link_str)
  end
end



function Trans.create_groups(ranges, pf_min, pf_max)

  -- pf_min and pf_max are in the 'prefab' space (i.e. before any
  -- stretching or shrinkin is done).

  assert(pf_min and pf_max)

  local groups = { }

  if not ranges then
    local G =
    {
      low  = pf_min
      high = pf_max
      size = pf_max - pf_min
    }

    G.weight = 1 * G.size

    table.insert(groups, G)

    return groups
  end


  -- create groups

  assert(#ranges >= 1)

  local pf_pos = pf_min

  each S in ranges do
    local G = { }

    G.size = S[1]

    G.low  = pf_pos ; pf_pos = pf_pos + G.size
    G.high = pf_pos

    G.weight = S[2]

    if S[3] then
      G.size2 = S[3]
    elseif G.weight == 0 then
      G.size2 = G.size
    end

    G.weight = G.weight * G.size

    table.insert(groups, G)
  end

  -- verify that group sizes match the coordinate bbox
  if math.abs(pf_pos - pf_max) > 0.1 then
    error(string.format("Prefab: groups mismatch with coords (%d != %d)", pf_pos, pf_max))
  end

  return groups
end


function Trans.fitted_group_targets(groups, low2, high2)
  local extra = high2 - low2
  local extra_weight = 0

  each G in groups do
    extra = extra - (G.size2 or G.size)

    if not G.size2 then
      extra_weight = extra_weight + G.weight
    end
  end

  local pos2 = low2

  each G in groups do
    if not G.size2 then
      G.size2 = G.size + extra * G.weight / extra_weight
    end

    if G.size2 <= 1 then
      error("Prefab does not fit!")
    end

    G.low2  = pos2 ; pos2 = pos2 + G.size2
    G.high2 = pos2
  end

  -- verify the results
  assert(math.abs(pos2 - high2) < 0.1)
end


function Trans.loose_group_targets(groups, scale)

  -- TODO: TEST THIS CODE !!!

  local total_size  = 0
  local total_size2 = 0

  local extra_weight = 0

  each G in groups do
    if not G.size2 then
      G.weight = G.weight * (scale - 1)
      extra_weight = extra_weight + G.weight
    end
  end

  each G in groups do
    if not G.size2 then
      if extra_weight > 1 then
        assert(G.weight > 0)
        G.size2 = G.size * (1 + G.weight / extra_weight)
      else
        G.size2 = G.size
      end
    end

    if G.size2 <= 1 then
      error("Prefab does not fit!")
    end

    total_size  = total_size  + G.size 
    total_size2 = total_size2 + G.size2
  end

  -- this assumes the left/bottom coord will be zero (fitted) or negative (focal)
  local pos2 = groups[1].low * total_size2 / total_size

  each G in groups do
    G.low2  = pos2 ; pos2 = pos2 + G.size2
    G.high2 = pos2
  end
end


------------------------------------------------------------------------


function Trans.is_subst(value)
  return type(value) == "string" and string.match(value, "^[!?]")
end


function Trans.substitute(SKIN, value)
  if not Trans.is_subst(value) then
    return value
  end

  -- a simple substitution is just: "?varname"
  -- a more complex one has an operator: "?varname+3",  "?foo==1"

  local neg, var_name, op, number = string.match(value, "(.)([%w_]*)(%p*)(%-?[%d.]*)");

  if var_name == "" then var_name = nil end
  if op       == "" then op       = nil end
  if number   == "" then number   = nil end

  if not var_name or (op and not number) or (op and neg == '!') then
    error("bad substitution: " .. tostring(value));
  end

  -- first lookup variable name, abort if not present
  value = SKIN[var_name]

  if value == nil or Trans.is_subst(value) then
    return value
  end

  -- apply the boolean negation
  if neg == '!' then
    return 1 - convert_bool(value)

  -- apply the operator
  elseif op then
    value  = 0 + value
    number = 0 + number

    if op == "+" then return value + number end
    if op == "-" then return value - number end

    if op == "==" then return (value == number ? 1 ; 0) end
    if op == "!=" then return (value != number ? 1 ; 0) end

    error("bad subst operator: " .. tostring(op))
  end

  return value
end


function Trans.process_skins(list)

  local SKIN = {}

  local function misc_stuff()
    -- these are useful for conditional brushes/ents
    if GAME.format == "doom" or GAME.format == "nukem" then
      SKIN["doomy"] = 1
    else
      SKIN["quakey"] = 1
    end
  end


  local function random_pass(keys)
    -- most fields with a table value are considered to be random
    -- replacement, e.g. pic = { COMPSTA1=50, COMPWERD=50 }.
    --
    -- fields starting with an underscore are ignored, to allow for
    -- special fields in the skin.

    each name in keys do
      local value = SKIN[name]

      if type(value) == "table" and not string.match(name, "^_") then
        if table.size(value) == 0 then
          error("process_skins: random table is empty: " .. tostring(name))
        end

        SKIN[name] = rand.key_by_probs(value)
      end
    end
  end


  local function function_pass(keys)
    each name in keys do
      local func = SKIN[name]

      if type(func) == "function" then
        SKIN[name] = func(SKIN)
      end
    end
  end


  local function subst_pass(keys)
    local changes = 0

    -- look for unresolved substitutions first
    for _,name in ipairs(keys) do
      local value = SKIN[name]

      if Trans.is_subst(value) then
        local ref = Trans.substitute(SKIN, value)

        if ref and type(ref) == "function" then
          error("Substitution references a function: " .. value)
        end

        if ref and Trans.is_subst(ref) then
          -- need to resolve the other one first
        else
          SKIN[name] = ref
          changes = changes + 1
        end
      end
    end

    return changes
  end


  ---| Trans.process_skins |---

  misc_stuff()

  each skin_tab in list do
    table.merge(SKIN, skin_tab)
  end 

  -- Note: iterate over a copy of the key names, since we cannot
  --       safely modify a table while iterating through it.
  local keys = table.keys(SKIN)

  random_pass(keys)

  for loop = 1,20 do
    if subst_pass(keys) == 0 then
      function_pass(keys)
      return SKIN
    end
  end

  -- failed !
  gui.debugf("\nSKIN =\n%s\n\n", table.tostr(SKIN, 1))

  error("process_skins: cannot resolve refs")
end



function Fab_create(name)

  local function mark_outliers(fab)
    for _,B in ipairs(fab.brushes) do
      if B[1].m and not B[1].insider and
         (B[1].m == "light" or B[1].m == "spot")
      then
        B[1].outlier = true
      end

      if B[1].m == "spot" then
        fab.has_spots = true
      end
    end
  end


  ---| Fab_create |---

  local info = PREFAB[name]

  if not info then
    error("Unknown prefab: " .. name)
  end

  local fab = table.deep_copy(info)

  fab.name = name

  if not fab.brushes  then fab.brushes  = {} end
  if not fab.models   then fab.models   = {} end
  if not fab.entities then fab.entities = {} end

  mark_outliers(fab)

  fab.state = "raw"

  return fab
end



function Fab_apply_skins(fab, list)
  -- perform skin substitutions on everything in the prefab.
  -- Note that the 'list' parameter is modified.

  local SKIN

  local function do_substitutions(t)
    each k in table.keys(t) do
      local v = t[k]

      if type(v) == "string" then
        v = Trans.substitute(SKIN, v)

        if v == nil then
          error(string.format("Prefab: missing value for \"%s\"", t[k]))
        end

        -- empty strings are the way to specify NIL
        if v == "" then v = nil end

        t[k] = v
      end

      -- recursively handle sub-tables
      if type(v) == "table" then
        do_substitutions(v)
      end
    end
  end


  local function has_failing_condition(tab)
    -- tab can be: a brush (the 'm' table), a model or an entity.
    -- the 'only_if' field will also be removed now.

    if tab.only_if == nil then
      return false
    end

    local result = (convert_bool(tab.only_if) == 0)

    tab.only_if = nil

    return result
  end


  local function do_conditionals(fab)
    -- brushes....
    for index = #fab.brushes, 1, -1 do
      local B = fab.brushes[index]
      if B[1].m and has_failing_condition(B[1]) then
        table.remove(fab.brushes, index)
      end
    end

    -- models....
    for index = #fab.models, 1, -1 do
      local M = fab.models[index]
      if has_failing_condition(M) then
        table.remove(fab.models, index)
      end
    end

    -- NOTE: entities are done below (in do_entities)
  end


  local function process_materials(brush)
    -- check if it should be a sky brush
    if not brush[1].m and Brush_has_sky(brush) then
      table.insert(brush, 1, { m="sky" })
    end

    each C in brush do
      if C.mat then
        local mat = Mat_lookup(C.mat)
        assert(mat and mat.t)

        if C.b then
          C.tex = mat.c or mat.f or mat.t
        elseif C.t then
          C.tex = mat.f or mat.t
        else
          C.tex = mat.t
        end

        C.mat = nil
      end
    end
  end


  local function process_model_face(face, is_flat)
    if face.mat then
      local mat = Mat_lookup(face.mat)
      assert(mat and mat.t)

      if is_flat and mat.f then
        face.tex = mat.f
      else
        face.tex = mat.t
      end

      face.mat = nil
    end
  end


  local function do_materials(fab)
    each B in fab.brushes do
      process_materials(B)
    end

    each M in fab.models do
      process_model_face(M.x_face, false)
      process_model_face(M.y_face, false)
      process_model_face(M.z_face, true)
    end
  end
  

  local function process_entity(E)
    local name = E.ent

    if not name then
      error("prefab entity with missing 'ent' field")
    end

    if name == "none" then
      return false
    end

    if PARAM.light_brushes and (name == "light" or name == "sun") then
      return false
    end

    local info = GAME.ENTITIES[name] or
                 GAME.MONSTERS[name] or
                 GAME.WEAPONS[name] or
                 GAME.PICKUPS[name]

    if not info then
      error("No such entity: " .. tostring(name))
    end

    E.id = assert(info.id)
    E.ent = nil

    if E.z then
      E.delta_z = info.delta_z or PARAM.entity_delta_z
    end

    if info.spawnflags then
      E.spawnflags = bit.bor((E.spawnflags or 0), info.spawnflags)
    end

    if info.fields then
      each name,value in info.fields do E[name] = value end
    end

    return true -- OK --
  end


  local function do_entities(fab)
    for index = #fab.entities,1,-1 do
      local E = fab.entities[index]

      -- we allow entities to be unknown, removing them from the list
      if has_failing_condition(E) or not process_entity(E) then
        table.remove(fab.entities, index)
      end
    end

    for _,M in ipairs(fab.models) do
      if not process_entity(M.entity) then
        error("Prefab model has 'none' entity")
      end
    end
  end


  local function determine_bbox(fab)
    local x1, y1, z1
    local x2, y2, z2

    -- Note: no need to handle slopes, they are defined to be "shrinky"
    --       (i.e. never higher that t, never lower than b).

    each B in fab.brushes do
      if not B[1].outlier then
        each C in B do

          if C.x then 
            if not x1 then
              x1, y1 = C.x, C.y
              x2, y2 = C.x, C.y
            else
              x1 = math.min(x1, C.x)
              y1 = math.min(y1, C.y)
              x2 = math.max(x2, C.x)
              y2 = math.max(y2, C.y)
            end

          elseif C.b or C.t then
            local z = C.b or C.t
            if not z1 then
              z1, z2 = z, z
            else
              z1 = math.min(z1, z)
              z2 = math.max(z2, z)
            end
          end

        end -- C
      end
    end -- B

    assert(x1 and y1 and x2 and y2)

    -- Note: it is OK when z1 and z2 are not set (this happens with
    --       prefabs consisting entirely of infinitely tall solids).

    -- Note: It is possible to get dz == 0
 
    local dz
    if z1 then dz = z2 - z1 end

    fab.bbox = { x1=x1, x2=x2, dx=(x2 - x1),
                 y1=y1, y2=y2, dy=(y2 - y1),
                 z1=z1, z2=z2, dz=dz,
               }
  end


  local function brush_stuff()
    each B in fab.brushes do
      if not B[1].m then
        table.insert(B, 1, { m="solid" })
      end

      B[1].fab = fab

      each C in B do
        Brush_collect_flags(C)

        -- convert X/Y offsets
        if C.x_offset then C.u1 = C.x_offset ; C.x_offset = nil end
        if C.y_offset then C.v1 = C.y_offset ; C.y_offset = nil end
      end
    end
  end

 
  ---| Fab_apply_skins |---

  assert(fab.state == "raw")

  fab.state = "skinned"

  -- add the standard skin tables into the list
  if fab.defaults        then table.insert(list, 1, fab.defaults) end
  if THEME.skin_defaults then table.insert(list, 1, THEME.skin_defaults) end
  if GAME.SKIN_DEFAULTS  then table.insert(list, 1, GAME.SKIN_DEFAULTS) end

  table.insert(list, 1, GLOBAL_SKIN_DEFAULTS)

  SKIN = Trans.process_skins(list)

  -- defaults are applied, don't need it anymore
  fab.defaults = nil

  if SKIN._tags then
    for i = 1, SKIN._tags do
      local name = "tag"
      if i >= 2 then name = name .. i end
      SKIN[name] = Plan_alloc_id("tag")
    end
  end

  if fab.team_models then
    SKIN.team = Plan_alloc_id("team")
  end

  -- perform substitutions (values beginning with '?' are skin refs)
  do_substitutions(fab)

  -- remove brushes (etc) which fail their conditional test 
  do_conditionals(fab)

  -- convert 'mat' fields to 'tex' fields
  do_materials(fab)

  -- lookup entity names
  do_entities(fab)

  -- handle "prefab" brushes.  NOTE: recursive
  Fab_composition(fab, SKIN)

  -- find bounding box (in prefab space)
  determine_bbox(fab)

  brush_stuff()

  -- grab repeat vars from skin
  fab.x_repeat = SKIN._repeat
  fab.y_repeat = SKIN._repeat_y

  SKIN = nil
end



function Fab_transform_XY(fab, T)

  local function brush_xy(brush)
    each C in brush do
      if C.x then C.x, C.y = Trans.apply_xy(C.x, C.y) end

      -- Note: this does Z too (fixme?)
      if C.s then C.s = Trans.apply_slope(C.s) end

      if C.angle then C.angle = Trans.apply_angle(C.angle) end
    end
  end

  
  local function entity_xy(E)
    if E.x then
      E.x, E.y = Trans.apply_xy(E.x, E.y)
    end

    if E.angle then
      E.angle = Trans.apply_angle(E.angle)
    end

    if E.angles then
      E.angles = Trans.apply_angles_xy(E.angles)
    end
  end


  local function model_xy(M)
    M.x1, M.y1 = Trans.apply_xy(M.x1, M.y1)
    M.x2, M.y2 = Trans.apply_xy(M.x2, M.y2)

    -- handle rotation / mirroring
    -- NOTE: we only support 0/90/180/270 rotations

    if M.x1 > M.x2 then M.x1, M.x2 = M.x2, M.x1 ; M.y_face.u1, M.y_face.u2 = M.y_face.u2, M.y_face.u1 end
    if M.y1 > M.y2 then M.y1, M.y2 = M.y2, M.y1 ; M.x_face.u1, M.x_face.u2 = M.x_face.u2, M.x_face.u1 end

    -- handle 90 and 270 degree rotations : swap X and Y faces
    local rotate = T.rotate or 0

    if math.abs(T.rotate - 90) < 15 or math.abs(T.rotate - 270) < 15 then
      M.x_face, M.y_face = M.y_face, M.x_face
    end
  end

  
  ---| Fab_transform_XY |---

  assert(fab.state == "skinned")

  fab.state = "transform_xy"

  Trans.set(T)

  local bbox = fab.bbox

  local groups_x = Trans.create_groups(fab.x_ranges, bbox.x1, bbox.x2)
  local groups_y = Trans.create_groups(fab.y_ranges, bbox.y1, bbox.y2)


  Trans.TRANSFORM.groups_x = groups_x
  Trans.TRANSFORM.groups_y = groups_y

  --- X ---

  if fab.fitted and string.find(fab.fitted, "x") then
    if not T.fitted_x then
      error("Fitted prefab used without fitted X transform")

    elseif T.scale_x then
      error("Fitted transform used with scale_x")

    elseif math.abs(bbox.x1) > 0.1 then
      error("Fitted prefab must have lowest X coord at 0")
    end

    Trans.fitted_group_targets(groups_x, 0, T.fitted_x)

  else  -- "loose" placement
    if T.fitted_x then
      error("Loose prefab used with fitted X transform")
    end

    Trans.loose_group_targets(groups_x, T.scale_x or 1)

    Trans.TRANSFORM.scale_x = nil
  end


  --- Y ---

  if fab.fitted and string.find(fab.fitted, "y") then
    if not T.fitted_y then
      error("Fitted prefab used without fitted Y transform")

    elseif T.scale_y then
      error("Fitted transform used with scale_y")

    elseif math.abs(bbox.y1) > 0.1 then
      error("Fitted prefab must have lowest Y coord at 0")
    end

    Trans.fitted_group_targets(groups_y, 0, T.fitted_y)

  else
    if T.fitted_y then
      error("Loose prefab used with fitted Y transform")
    end

    Trans.loose_group_targets(groups_y, T.scale_y or 1)

    Trans.TRANSFORM.scale_y = nil
  end

  -- apply the coordinate transform to all parts of the prefab

  each B in fab.brushes do
    brush_xy(B)
  end

  each E in fab.entities do
    entity_xy(E)
  end

  each M in fab.models do
    model_xy(M)
    entity_xy(M.entity)
  end

  Trans.clear()
end



function Fab_transform_Z(fab, T)

  local function brush_z(brush)
    local b, t

    each C in brush do
      if C.b  then C.b  = Trans.apply_z(C.b)  ; b = C.b end
      if C.t  then C.t  = Trans.apply_z(C.t)  ; t = C.t end
      if C.zv then C.zv = Trans.apply_z(C.zv) end

      if Trans.mirror_z then
        C.b, C.t = C.t, C.b
      end
    end

    -- apply capping
    if Trans.z1_cap and not b and (not t or t.t > Trans.z1_cap) then
      table.insert(brush, { b = Trans.z1_cap })
    end

    if Trans.z2_cap and not t and (not b or b.b < Trans.z2_cap) then
      table.insert(brush, { t = Trans.z2_cap })
    end
  end

  
  local function entity_z(E)
    if E.z then
      E.z = Trans.apply_z(E.z)

      if E.delta_z then
        E.z = E.z + E.delta_z
        E.delta_z = nil
      end

      if E.angles then
        E.angles = Trans.apply_angles_z(E.angles)
      end
    end
  end


  local function model_z(M)
    M.z1 = Trans.apply_z(M.z1)
    M.z2 = Trans.apply_z(M.z2)

    if M.delta_z then
      M.z1 = M.z1 + M.delta_z
      M.z2 = M.z2 + M.delta_z
    end

    if Trans.mirror_z then
      M.z1, M.z2 = M.z2, M.z1
    end

    -- handle QUAKE I / II platforms
    if M.entity.height and T.scale_z then
      M.entity.height = M.entity.height * T.scale_z
    end
  end

  
  ---| Fab_transform_Z |---

  assert(fab.state == "transform_xy")

  fab.state = "transform_z"

  Trans.set(T)

  local bbox = fab.bbox
  local groups_z

  if bbox.z1 and bbox.dz > 1 then
    groups_z = Trans.create_groups(fab.z_ranges, bbox.z1, bbox.z2)

    Trans.TRANSFORM.groups_z = groups_z
  end

  --- Z ---

  if fab.fitted and string.find(fab.fitted, "z") then
    if not T.fitted_z then
      error("Fitted prefab used without fitted Z transform")

    elseif T.scale_z then
      error("Fitted transform used with scale_z")

    elseif not groups_z then
      error("Fitted prefab has no vertical range!")

    elseif math.abs(bbox.z1) > 0.1 then
      error("Fitted prefab must have lowest Z coord at 0")
    end

    if groups_z then
      Trans.fitted_group_targets(groups_z, 0, T.fitted_z)
    end

  else  -- "loose" mode

    if T.fitted_z then
      error("Loose prefab used with fitted Z transform")
    end

    if groups_z then
      Trans.loose_group_targets(groups_z, T.scale_z or 1)

      Trans.TRANSFORM.scale_z = nil
    end
  end

  -- apply the coordinate transform to all parts of the prefab

  each B in fab.brushes do
    brush_z(B)
  end

  each E in fab.entities do
    entity_z(E)
  end

  each M in fab.models do
    model_z(M)
  end

  Trans.clear()
end



function Fab_composition(parent, parent_skin)
  -- handles "prefab" brushes, replacing them with the brushes of
  -- the child prefab (transformed to fit into the "prefab" brush),
  -- and adding all the child's entities and models too.
  --
  -- This function is called by Fab_apply_skins() and never needs
  -- to be called by other code.

  local function transform_child(brush, skin, dir)
    local child = Fab_create(skin._prefab)

    Fab_apply_skins(child, { parent_skin, skin })

    -- TODO: support arbitrary rectangles (rotation etc)

    local bx1, by1, bx2, by2 = Brush_bbox(brush)

    local low_z  = Brush_get_b(brush)
    local high_z = Brush_get_t(brush)

    local T = Trans.box_transform(bx1, by1, bx2, by2, low_z, dir)
     
    if child.fitted and string.find(child.fitted, "z") then
      Trans.set_fitted_z(T, low_z, high_z)
    end

    Fab_transform_XY(child, T)
    Fab_transform_Z (child, T)

    each B in child.brushes do
      table.insert(parent.brushes, B)
    end

    each E in child.entities do
      table.insert(parent.entities, E)
    end

    each M in child.models do
      table.insert(parent.models, M)
    end

    child = nil
  end


  ---| Fab_composition |---

  for index = #parent.brushes,1,-1 do
    local B = parent.brushes[index]

    if B[1].m == "prefab" then
      table.remove(parent.brushes, index)

      local child_name = assert(B[1].skin)
      local child_skin = GAME.SKINS[child_name]
      local child_dir  = B[1].dir or 2

      if not child_skin then
        error("prefab compostion: no such skin: " .. tostring(child_name))
      end

      transform_child(B, child_skin, child_dir)
    end
  end
end



function Fab_repetition_X(fab, T)

  local orig_brushes  = #fab.brushes
  local orig_models   = #fab.models
  local orig_entities = #fab.entities

  local function copy_brush(B, x_offset, y_offset)
    local B2 = {}

    each C in B do
      C2 = table.copy(C)

      if C.x then C2.x = C.x + x_offset end
      if C.y then C2.y = C.y + y_offset end

      -- FIXME: slopes

      table.insert(B2, C2)
    end

    table.insert(fab.brushes, B2)
  end


  local function copy_model(M, x_offset, y_offset)
    local M2 = table.copy(M)

    M2.entity = table.copy(M.entity)

    M2.x1 = M.x1 + x_offset
    M2.x2 = M.x2 + x_offset

    M2.y1 = M.y1 + y_offset
    M2.y2 = M.y2 + y_offset

    table.insert(fab.models, M2)
  end


  local function copy_entity(E, x_offset, y_offset)
    local E2 = table.copy(E)

    if E.x then E2.x = E.x + x_offset end
    if E.y then E2.y = E.y + x_offset end

    table.insert(fab.entities, E2)
  end


  local function replicate_w_offsets(x_offset, y_offset)
    -- cannot use 'each B in' since we are changing the list (adding new ones)
    for index = 1, orig_brushes do
      local B = fab.brushes[index]
      copy_brush(B, x_offset, y_offset)
    end

    for index = 1, orig_models do
      local M = fab.models[index]
      copy_model(M, x_offset, y_offset)
    end

    for index = 1, orig_entities do
      local E = fab.entities[index]
      copy_entity(E, x_offset, y_offset)
    end
  end


  ---| Fab_repetition_X |---

  if not fab.x_repeat then return end

  if not T.fitted_x then
    error("x_repeat used in loose prefab")
  end

  local count = math.floor(T.fitted_x / fab.x_repeat)

  if count <= 1 then return end

  for pass = 1,count-1 do
    local x_offset = pass * fab.bbox.x2
    local y_offset = 0

    replicate_w_offsets(x_offset, y_offset)
  end

  -- update bbox
  fab.bbox.x2 = fab.bbox.x2 * count

  -- update ranges
  if fab.x_ranges then
    local new_x_ranges = {}

    for pass = 1,count do
      table.append(new_x_ranges, fab.x_ranges)
    end

    fab.x_ranges = new_x_ranges
  end
end



function Fab_bound_Z(fab, z1, z2)
  if not (z1 or z2) then return end

  for _,B in ipairs(fab.brushes) do
    if CSG_BRUSHES[B[1].m] then
      local b = Brush_get_b(B)
      local t = Brush_get_t(B)

      if z1 and not b then table.insert(B, { b = z1 }) end
      if z2 and not t then table.insert(B, { t = z2 }) end
    end
  end
end



function Fab_render(fab)

  assert(fab.state == "transform_z")

  fab.state = "rendered"

  each B in fab.brushes do
    if CSG_BRUSHES[B[1].m] then
      --- DEBUG AID:
      --- stderrf("brush %d/%d\n", _index, #fab.brushes)

      raw_add_brush(B)
    end
  end

  each M in fab.models do
    raw_add_model(M)
  end

  each E in fab.entities do
    raw_add_entity(E)
  end
end



function Fab_read_spots(fab)
  -- prefab must be rendered (or ready to render)

  local function add_spot(list, B)
    local x1,y1, x2,y2
    local z1,z2

    if Brush_is_quad(B) then
      x1,y1, x2,y2 = Brush_bbox(B)

      each C in B do
        if C.b then z1 = C.b end
        if C.t then z2 = C.t end
      end
    else
      -- FIXME: use original brushes (assume quads), break into squares,
      --        apply the rotated square formula from Trans.apply_spot. 
      error("Unimplemented: cage spots on rotated prefabs")
    end

    if not z1 or not z2 then
      error("monster spot brush is missing t/b coord")
    end

    local SPOT =
    {
      kind  = B[1].spot_kind
      angle = B[1].angle

      x1 = x1, y1 = y1, z1 = z1
      x2 = x2, y2 = y2, z2 = z2
    }

    table.insert(list, SPOT)
  end

  ---| Fab_read_spots |---

  local list = {}

  each B in fab.brushes do
    if B[1].m == "spot" then
      add_spot(list, B)
    end
  end

  return list
end



function Fabricate(name, T, skins)
  
-- stderrf("=========  FABRICATE %s\n", name)

  local fab = Fab_create(name)

  -- FIXME: not here
  skins = table.copy(skins)
  if  ROOM and  ROOM.skin then table.insert(skins, 1, ROOM.skin) end
  if THEME and THEME.skin then table.insert(skins, 1, THEME.skin) end

  Fab_apply_skins(fab, skins)

  Fab_repetition_X(fab, T)

  Fab_transform_XY(fab, T)
  Fab_transform_Z (fab, T)

  Fab_render(fab)

  return fab
end


function Fab_size_check(skin, long, deep)
  -- the 'long' and 'deep' parameters can be nil : means anything is OK

  if long and skin._long then
    if type(skin._long) == "number" then
      if long < skin._long then return false end
    else
      if long < skin._long[1] then return false end
      if long > skin._long[2] then return false end
    end
  end

  if deep and skin._deep then
    if type(skin._deep) == "number" then
      if deep < skin._deep then return false end
    else
      if deep < skin._deep[1] then return false end
      if deep > skin._deep[2] then return false end
    end
  end

  if skin._aspect then
    -- we don't know the target size, so cannot guarantee any aspect ratio
    if not (long and deep) then return false end

    local aspect = long / deep

    if type(skin._aspect) == "number" then
      aspect = aspect / skin._aspect
      -- fair bit of lee-way here
      if aspect < 0.9 or aspect > 1.1 then return false end
    else
      if aspect < skin._aspect[1] * 0.95 then return false end
      if aspect > skin._aspect[2] * 1.05 then return false end
    end
  end

  return true  -- OK --
end


------------------------------------------------------------------------


function Brush_shadowify(coords, dist)
  --
  -- ALGORITHM
  --
  -- Each side of the brush can be in one of three states:
  --    "light"  (not facing the shadow)
  --    "dark"
  --    "edge"   (parallel to shadow extrusion)
  --
  -- For each vertex we can do one of three operations:
  --    KEEP : when both lines are light or edge
  --    MOVE : when both lines are dark or edge
  --    DUPLICATE : one line is dark, one is light

  for i = 1,#coords do
    local v1 = coords[i]
    local v2 = coords[(i < #coords ? i+1 ; 1)]

    local dx = v2.x - v1.x
    local dy = v2.y - v1.y

    -- simplify detection : map extrusion to X axis
    dy = dy + dx

    if dy < -0.01 then
      coords[i].light = true
    elseif dy > 0.01 then
      coords[i].dark = true
    else
      -- it is an edge : implied
    end
  end

  local new_coords = {}

  for i = 1,#coords do
    local P = coords[(i > 1 ? i-1 ; #coords)]
    local N = coords[i]

    if not (P.dark or N.dark) then
      -- KEEP
      table.insert(new_coords, N)
    else
      local NEW = { x=N.x+dist, y=N.y-dist}

      if not (P.light or N.light) then
        -- MOVE
        table.insert(new_coords, NEW)
      else
        -- DUPLICATE
        assert(P.light or P.dark)
        assert(N.light or N.dark)

        if P.light and N.dark then
          table.insert(new_coords, N)
          table.insert(new_coords, NEW)
        else
          table.insert(new_coords, NEW)
          table.insert(new_coords, N)
        end
      end
    end
  end

  return new_coords
end


function Build_shadow(S, side, dist, z2)
  assert(dist)

  if not PARAM.outdoor_shadows then return end

  if not S then return end

  if side < 0 then
    S = S:neighbor(-side)
    if not (S and S.room and S.room.kind == "outdoor") then return end
    side = 10 + side
  end

  local x1, y1 = S.x1, S.y1
  local x2, y2 = S.x2, S.y2

  if side == 8 then
    local N = S:neighbor(6)
    local clip = not (N and N.room and N.room.kind == "outdoor")

    -- FIXME: update for new brush system
    Trans.old_brush(get_light(-1),
    {
      { x=x2, y=y2 }
      { x=x1, y=y2 }
      { x=x1+dist, y=y2-dist }
      { x=x2+sel(clip,0,dist), y=y2-dist }
    },
    -EXTREME_H, z2 or EXTREME_H)
  end

  if side == 4 then
    local N = S:neighbor(2)
    local clip = not (N and N.room and N.room.kind == "outdoor")

    Trans.old_brush(get_light(-1),
    {
      { x=x1, y=y2 }
      { x=x1, y=y1 }
      { x=x1+dist, y=y1-sel(clip,0,dist) }
      { x=x1+dist, y=y2-dist }
    },
    -EXTREME_H, z2 or EXTREME_H)
  end
end


---==========================================================---


function OLD__Quake_test()

  -- FIXME: convert this into a prefab

  Trans.old_quad(get_mat("METAL1_2"), 0, 128, 256, 384,  -24, 0)
  Trans.old_quad(get_mat("CEIL1_1"),  0, 128, 256, 384,  192, 208)

  -- 3D floor test
  if false then
    Trans.old_quad(get_mat("METAL2_4"), 112, 192, 144, 208, 20, 30);
  end

  -- liquid test
  if false then
    raw_add_brush(
    {
      { m="liquid", medium="water" },
      { t=119, tex="e1u1/water4" },
      { x=0,   y=0,   tex="e1u1/water4" },
      { x=100, y=0,   tex="e1u1/water4" },
      { x=100, y=600, tex="e1u1/water4" },
      { x=0,   y=600, tex="e1u1/water4" },
    })
  end

  local wall_i = get_mat("COMP1_1")

  Trans.old_quad(wall_i, 0,   128,  32, 384,  0, 192)
  Trans.old_quad(wall_i, 224, 128, 256, 384,  0, 192)
  Trans.old_quad(wall_i, 0,   128, 256, 144,  0, 192)
  Trans.old_quad(wall_i, 0,   370, 256, 384,  0, 192)

  entity_helper("player1", 80, 256, 64)
  entity_helper("light",   80, 256, 160, { light=200 })
end

