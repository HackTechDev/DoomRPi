----------------------------------------------------------------
--  SIMPLE ROOMS : CAVES and MAZES
----------------------------------------------------------------
--
--  Oblige Level Maker
--
--  Copyright (C) 2009-2012 Andrew Apted
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


function Simple_cave_or_maze(R)

  local map
  local cave

  local point_list

--[[
  R.cave_floor_h = 0 --!!!!!!  R.entry_floor_h
  R.cave_h = rand.pick { 128, 128, 192, 256 }
--]]

--[[
  if R.outdoor and THEME.landscape_walls then
    R.cave_tex = rand.key_by_probs(THEME.landscape_walls)

    if LEVEL.liquid and
       R.svolume >= style_sel("lakes", 99, 49, 49, 30) and
       rand.odds(style_sel("lakes", 0, 10, 30, 90))
    then
      R.is_lake = true
    end
  else
    R.cave_tex = rand.key_by_probs(THEME.cave_walls)
  end
--]]


  local function set_whole(C, value)
    for cx = C.cave_x1, C.cave_x2 do
      for cy = C.cave_y1, C.cave_y2 do
        map:set(cx, cy, value)
      end
    end
  end


  local function set_side(cx1, cy1, cx2, cy2, side, value)
    local x1,y1, x2,y2 = geom.side_coords(side, cx1,cy1, cx2,cy2)

    for cx = x1,x2 do for cy = y1,y2 do
      map:set(cx, cy, value)
    end end
  end


--[[
  local function set_corner(S, side, value)
    local mx = (S.sx - R.sx1) * 3 + 1
    local my = (S.sy - R.sy1) * 3 + 1

    local dx, dy = geom.delta(side)

    mx = mx + 1 + dx
    my = my + 1 + dy

    map:set(mx, my, value)
  end


  local function handle_wall(S, side)
    local N = S:neighbor(side)

    if not N or not N.room then
      return set_side(S, side, (R.is_lake ? -1 ; 1))
    end

    if N.room == S.room then return end

    if N.room.natural then
      return set_side(S, side, (R.is_lake ? -1 ; 1))
    end

    set_side(S, side, (R.is_lake ? -1 ; 1))
  end


  local function handle_corner(S, side)
    local N = S:neighbor(side)

    local A = S:neighbor(geom.ROTATE[1][side])
    local B = S:neighbor(geom.ROTATE[7][side])

    if not (A and A.room == R) or not (B and B.room == R) then
      return
    end

    if not N or not N.room then
      if R.is_lake then set_corner(S, side, -1) end
      return
    end

    if N.room == S.room then return end

    if N.room.nature then return end

    set_corner(S, side, -1)
  end
--]]


  local function clear_importants()
    each C in R.chunks do
      if C.foobage == "important" or C.foobage == "conn" or
         C.crossover_hall
      then
        set_whole(C, -1)
      end
    end
  end


  local function create_map()
    R.cave_base_x = SEEDS[R.sx1][R.sy1].x1
    R.cave_base_y = SEEDS[R.sx1][R.sy1].y1

    map = CAVE_CLASS.new(R.cave_base_x, R.cave_base_y, R.sw * 3, R.sh * 3)

    -- determine location of chunks inside this map
    each C in R.chunks do
      C.cave_x1 = (C.sx1 - R.sx1) * 3 + 1
      C.cave_y1 = (C.sy1 - R.sy1) * 3 + 1

      C.cave_x2 = (C.sx2 - R.sx1) * 3 + 3
      C.cave_y2 = (C.sy2 - R.sy1) * 3 + 3
    end
  end


  local function mark_boundaries()
    for sx = R.sx1, R.sx2 do for sy = R.sy1, R.sy2 do
      local S = SEEDS[sx][sy]

      if S.room != R then continue end

      local C = S.chunk

      if C and (C.void or C.scenic or C.cross_junc) then continue end

      local cx = (sx - R.sx1) * 3 + 1
      local cy = (sy - R.sy1) * 3 + 1

      map:fill(cx, cy, cx+2, cy+2, 0)

      for dir = 2,8,2 do
        if not S:same_room(dir) and not (C and C.link[dir]) and
           not (C and C.foobage == "important")
        then
          set_side(cx, cy, cx+2, cy+2, dir, (R.is_lake ? -1 ; 1))
        end
      end
    end end -- sx, sy
  end


  local function clear_walks()
    for sx = R.sx1, R.sx2 do for sy = R.sy1, R.sy2 do
      local S = SEEDS[sx][sy]

      if S.room != R then continue end

      if S.is_walk and rand.odds(25) then

        local cx = (sx - R.sx1) * 3 + 1
        local cy = (sy - R.sy1) * 3 + 1

        for cx = cx, cx+2 do
          for cy = cy, cy+2 do
            if map:get(cx, cy) == 0 and rand.odds(25) then
              map:set(cx, cy, -1)
            end
          end
        end

      end
    end end -- sx, sy
  end


  local function collect_important_points()
    point_list = {}

    each C in R.chunks do
      if C.foobage == "important" or C.foobage == "conn" then

        local POINT =
        {
          x = math.i_mid(C.cave_x1, C.cave_x2)
          y = math.i_mid(C.cave_y1, C.cave_y2)
        }

        table.insert(point_list, POINT)
      end
    end

    assert(#point_list > 0)

    R.point_list = point_list
  end


  local function is_cave_good(cave)
    -- check that all important parts are connected

    if not cave:validate_conns(point_list) then
      gui.debugf("cave failed connection check\n")
      return false
    end

    cave.empty_id = cave.flood[point_list[1].x][point_list[1].y]

    assert(cave.empty_id)
    assert(cave.empty_id < 0)

    if not cave:validate_size(cave.empty_id) then
      gui.debugf("cave failed size check\n")
      return false
    end

    return true
  end


  local function generate_cave()
    map:dump("Empty Cave:")

    local MAX_LOOP = 10

    for loop = 1,MAX_LOOP do
      gui.debugf("Trying to make a cave: loop %d\n", loop)

      cave = map:copy()

      if loop >= MAX_LOOP then
        gui.printf("Failed to generate a usable cave! (%s)\n", R:tostr())

        -- emergency fallback
        cave:gen_empty()

        cave:flood_fill()

        is_cave_good(cave)  -- set empty_id
        break
      end

      cave:generate((R.is_lake ? 58 ; 38))

      cave:remove_dots()

      cave:flood_fill()

      if is_cave_good(cave) then
        break
      end

      -- randomly clear some cells along the walk path.
      -- After each iteration the number of cleared cells will keep
      -- increasing, making it more likely to generate a valid cave.
      clear_walks()
    end

    R.cave_map = cave

    cave:solidify_pockets()

    cave:dump("Filled Cave:")

    cave:find_islands()
  end


  ----------->


  local w_tex  = cave_tex



  ---| Simple_cave_or_maze |---

  -- create the cave object and make the boundaries solid
  create_map()

  collect_important_points()

  mark_boundaries()

  clear_importants()

  generate_cave()

end



function Simple_create_areas(R)

  local cover_chunks
  local chunk_map


  local function one_big_area()
    local AREA = AREA_CLASS.new("floor", R)

    table.insert(R.areas, AREA)

    each C in R.chunks do
      if not C.void and not C.scenic and not C.cross_junc then
        C.area = AREA
        C.cave = true
        table.insert(AREA.chunks, C)
      end
    end

    AREA.floor_map = R.cave_map:copy()

    AREA.floor_map:negate()
  end


  local function collect_cover_chunks()
    -- find the chunks which must be completed covered by any area.
    cover_chunks = {}

    each C in R.chunks do
      if C.foobage == "important" or C.foobage == "conn" or
         C.foobage == "crossover"
      then
        table.insert(cover_chunks, C)

        C.no_floor = true
        C.no_ceil  = true
      end
    end
  end


  local function add_chunk_to_map(C)
    for x = C.cave_x1, C.cave_x2 do
      for y = C.cave_y1, C.cave_y2 do
        chunk_map.cells[x][y] = C
      end
    end
  end


  local function remove_chunk_from_map(C)
    for x = C.cave_x1, C.cave_x2 do
      for y = C.cave_y1, C.cave_y2 do
        chunk_map.cells[x][y] = nil
      end
    end
  end


  local function create_chunk_map()
    collect_cover_chunks()

    chunk_map = CAVE_CLASS.blank_copy(R.cave_map)

    each C in cover_chunks do
      add_chunk_to_map(C)
    end
  end


  local cave_floor_h = 0 ---  assert(A.chunks[1].floor_h)


  local function grow_step_areas()

    local pos_list = { }

    pos_list[1] =
    {
      x = R.point_list[1].x
      y = R.point_list[1].y
    }


    local free  = CAVE_CLASS.copy(R.cave_map)
    local f_cel = free.cells

    local step
    local s_cel
    local size

    local cw = free.w
    local ch = free.h

    -- current bbox : big speed up by limiting the scan area
    local cx1, cy1
    local cx2, cy2

    local touched_chunks
    

    -- mark free areas with zero instead of negative [TODO: make into a cave method]
    for fx = 1,cw do for fy = 1,ch do
      if (f_cel[fx][fy] or 0) < 0 then
        f_cel[fx][fy] = 0
      end
    end end


    local function touch_a_chunk(C)
      table.insert(touched_chunks, C)

      table.kill_elem(cover_chunks, C)

      remove_chunk_from_map(C)

      -- add the whole chunk to the current step
      for x = C.cave_x1, C.cave_x2 do
        for y = C.cave_y1, C.cave_y2 do
          s_cel[x][y] = 1
          f_cel[x][y] = 1

          size = size + 1
        end
      end

      -- update bbox
      cx1 = math.min(cx1, C.cave_x1)
      cy1 = math.min(cy1, C.cave_y1)

      cx2 = math.max(cx2, C.cave_x2)
      cy2 = math.max(cy2, C.cave_y2)
    end


    local function grow_add(x, y)
      s_cel[x][y] = 1
      f_cel[x][y] = 1

      size = size + 1

      -- update bbox
      if x < cx1 then cx1 = x end
      if x > cx2 then cx2 = x end

      if y < cy1 then cy1 = y end
      if y > cy2 then cy2 = y end

      if chunk_map.cells[x][y] then
        touch_a_chunk(chunk_map.cells[x][y])
      end
    end


    local function grow_horiz(y, prob)
      for x = cx1, cx2 do
        if x > 1 and s_cel[x][y] == 1 and s_cel[x-1][y] == 0 and f_cel[x-1][y] == 0 and rand.odds(prob) then
          grow_add(x-1, y)
        end
      end

      for x = cx2, cx1, -1 do
        if x < cw and s_cel[x][y] == 1 and s_cel[x+1][y] == 0 and f_cel[x+1][y] == 0 and rand.odds(prob) then
          grow_add(x+1, y)
        end
      end
    end


    local function grow_vert(x, prob)
      for y = cy1, cy2 do
        if y > 1 and s_cel[x][y] == 1 and s_cel[x][y-1] == 0 and f_cel[x][y-1] == 0 and rand.odds(prob) then
          grow_add(x, y-1)
        end
      end

      for y = cy2, cy1, -1 do
        if y < ch and s_cel[x][y] == 1 and s_cel[x][y+1] == 0 and f_cel[x][y+1] == 0 and rand.odds(prob) then
          grow_add(x, y+1)
        end
      end
    end


    local function grow_it(prob)
      for y = cy1, cy2 do
        grow_horiz(y, prob)
      end

      for x = cx1, cx2 do
        grow_vert(x, prob)
      end
    end


    local function merge_step(prev)
      -- meh, need this functions because union() method does not
      --      consider '0' as a valid value.
      -- At least this one is a bit more efficient :)

      for x = cx1, cx2 do for y = cy1, cy2 do
        if s_cel[x][y] == 1 then
          prev.cells[x][y] = 1
        end
      end end
    end


    local function grow_an_area(cx, cy, prev_A)

-- free:dump("Free:")

      step  = CAVE_CLASS.blank_copy(free)
      s_cel = step.cells

      step:set_all(0)

      size = 0

      touched_chunks = {}

      cx1 = cx ; cx2 = cx
      cy1 = cy ; cy2 = cy

      -- set initial point
      grow_add(cx, cy)

      local count = rand.pick { 3, 4, 5 }

      grow_it(100)

      for loop = 1, count do
        grow_it(50)
      end

      if size < 4 or #touched_chunks > 0 then
        grow_it(100)
        grow_it(40)
      end

step:dump("Step:")

      -- when the step is too small, merge it into previous area
      if size < 4 and prev_A then

        merge_step(prev_A.floor_map)

      else
        local AREA = AREA_CLASS.new("floor", R) 

        table.insert(R.areas, AREA)

        AREA.floor_map = step

        prev_A = AREA
      end

      each C in touched_chunks do
        table.insert(prev_A.chunks, C)

        C.area = prev_A
      end


      -- find new positions for growth

      for x = cx1, cx2 do for y = cy1, cy2 do
        if s_cel[x][y] == 1 then
          for dir = 2,8,2 do
            local nx, ny = geom.nudge(x, y, dir)
            if free:valid_cell(nx, ny) and f_cel[nx][ny] == 0 then
              table.insert(pos_list, { x=nx, y=ny, prev=prev_A })
            end
          end
        end
      end end

-- LIGHTING TEST CRAP
  if GAME.format != "doom" then
    prev_A.light_x = step.base_x + 32 + (cx-1) * 64
    prev_A.light_y = step.base_y + 32 + (cy-1) * 64
  end

    end

    ------>

    while #pos_list > 0 do
      local pos = table.remove(pos_list, rand.irange(1, #pos_list))

      -- ignore out-of-date positions
      if f_cel[pos.x][pos.y] != 0 then continue end

      grow_an_area(pos.x, pos.y, pos.prev)
    end
  end


  local function create_area_map()
    -- create a map where each cell refers to an AREA, or is NIL.

    local area_map = CAVE_CLASS.blank_copy(R.cave_map)

    local W = R.cave_map.w
    local H = R.cave_map.h

    each A in R.areas do
      for x = 1,W do for y = 1,H do
        if (A.floor_map.cells[x][y] or 0) > 0 then
          area_map.cells[x][y] = A
        end
      end end
    end

    R.area_map = area_map
  end


  local function determine_touching_areas()
    local W = R.cave_map.w
    local H = R.cave_map.h

    local area_map = R.area_map

    for x = 1,W do for y = 1,H do
      local A1 = area_map.cells[x][y]

      if not A1 then continue end

      for dir = 2,4,2 do
        local nx, ny = geom.nudge(x, y, dir)

        if not area_map:valid_cell(nx, ny) then continue end

        local A2 = area_map.cells[nx][ny]

        if A2 and A2 != A1 then
          A1:add_touching(A2)
          A2:add_touching(A1)
        end
      end
    end end

    -- verify all areas touch at least one other
    if #R.areas > 1 then
      each A in R.areas do
        assert(not table.empty(A.touching))
      end
    end
  end


  ---| Simple_create_areas |---

  if false then
    -- FIXME: this probably broken!  (since no more filler chunks)
    one_big_area()
  else
    create_chunk_map()

    grow_step_areas()

    if #cover_chunks > 0 then
      error("Cave steps failed to cover all important chunks\n")
    end
  end

  create_area_map()

  determine_touching_areas()

--[[ debugging
  each A in R.areas do
    assert(A.floor_map)

    A.floor_map:dump("Step for " .. A:tostr())
  end
--]]
end



function Simple_connect_all_areas(R)

  local z_change_prob = 10
  if rand.odds(10) then z_change_prob = 40 end
  if rand.odds(15) then z_change_prob =  0 end

  local function recurse(A, z_dir)
    assert(A.floor_h)

    if not A.touching then return end

    if rand.odds(z_change_prob) then
      z_dir = - z_dir
    end

    each N in A.touching do
      if not N.floor_h then
        local new_h = A.floor_h + z_dir * rand.sel(35, 8, 16)

        N:set_floor(new_h)

-- LIGHTING TEST CRAP
  if GAME.format != "doom" and R.ceil_mat != "_SKY" then
    local x = assert(N.light_x)
    local y = assert(N.light_y)
    local z = N.floor_h + rand.pick { 40,60,80 }
    local light = rand.pick { 70, 110, 150 }
    entity_helper("light", x, y, z, { light=light })
  end

        recurse(N, z_dir)
      end
    end
  end


  ---| Simple_connect_all_areas |---

  local z_dir = rand.sel(35, 1, -1)

  recurse(R.entry_area, z_dir)
end



function Simple_render_cave(R)

  local cave = R.cave_map

  local cave_tex = R.wall_mat or "_ERROR"

  -- the delta map specifies how to move each corner of the 64x64 cells
  -- (where the cells form a continuous mesh).
  local delta_x_map
  local delta_y_map

  local dw = cave.w + 1
  local dh = cave.h + 1

  local B_CORNERS = { 1,3,9,7 }


  local function grab_cell(x, y)
    if not cave:valid_cell(x, y) then
      return nil
    end

    local C = cave:get(x, y)

    -- in some places we build nothing (e.g. other rooms)
    if C == nil then return nil end

    -- check for a solid cell
    if C > 0 then return "#" end

    -- otherwise there should be a floor area here
    local A = R.area_map:get(x, y)
    assert(A)

    return assert(A.floor_h)
  end


  local function analyse_corner(x, y)
    --  A | B
    --  --+--
    --  C | D

    local A = grab_cell(x-1, y)
    local B = grab_cell(x,   y)
    local C = grab_cell(x-1, y-1)
    local D = grab_cell(x,   y-1)

    -- never move a corner at edge of room
    if not A or not B or not C or not D then
      return
    end

    -- solid cells always override floor cells
    if A == "#" or B == "#" or C == "#" or D == "#" then
      A = (A == "#")
      B = (B == "#")
      C = (C == "#")
      D = (D == "#")
    else
      -- otherwise pick highest floor (since that can block a lower floor)
      local max_h = math.max(A, B, C, D) - 2

      A = (A > max_h)
      B = (B > max_h)
      C = (C > max_h)
      D = (D > max_h)
    end

    -- no need to move when all cells are the same
    if A == B and A == C and A == D then
      return
    end

    local x_mul =  1
    local y_mul = -1

    -- flip horizontally and/or vertically to ease analysis
    if not A and B then
      A, B = B, A
      C, D = D, C
      x_mul = -1

    elseif not A and C then
      A, C = C, A
      B, D = D, B
      y_mul = 1

    elseif not A and D then
      A, D = D, A
      B, C = C, B
      x_mul = -1
      y_mul =  1
    end

    assert(A)

    --- ANALYSE! ---

    if not B and not C and not D then
      -- sticking out corner
      if rand.odds(90) then delta_x_map[x][y] = -16 * x_mul end
      if rand.odds(90) then delta_y_map[x][y] = -16 * y_mul end

    elseif B and not C and not D then
      -- horizontal wall
      if rand.odds(55) then delta_y_map[x][y] = -24 * y_mul end

    elseif C and not B and not D then
      -- vertical wall
      if rand.odds(55) then delta_x_map[x][y] = -24 * x_mul end

    elseif D and not B and not C then
      -- checkerboard
      -- (not moving it : this situation should not occur)

    else
      -- an empty corner
      -- expand a bit, but not enough to block player movement
          if not B then y_mul = -y_mul
      elseif not C then x_mul = -x_mul
      end

      if rand.odds(80) then delta_x_map[x][y] = 12 * x_mul end
      if rand.odds(80) then delta_y_map[x][y] = 12 * y_mul end
    end
  end


  local function create_delta_map()
    delta_x_map = table.array_2D(dw, dh)
    delta_y_map = table.array_2D(dw, dh)

    if square_cave then return end

    for x = 1,dw do for y = 1,dh do
      analyse_corner(x, y)
    end end
  end


  local function brush_for_cell(x, y)
    local bx = cave.base_x + (x - 1) * 64
    local by = cave.base_y + (y - 1) * 64

    local coords = { }

    each side in B_CORNERS do
      local dx, dy = geom.delta(side)

      local fx = bx + (dx < 0 ? 0 ; 64)
      local fy = by + (dy < 0 ? 0 ; 64)

      local cx = x + (dx < 0 ? 0 ; 1)
      local cy = y + (dy < 0 ? 0 ; 1)

      fx = fx + (delta_x_map[cx][cy] or 0)
      fy = fy + (delta_y_map[cx][cy] or 0)

      table.insert(coords, { x=fx, y=fy })
    end

    return coords
  end


  local function choose_tex(last, tab)
    local tex = rand.key_by_probs(tab)

    if last then
      for loop = 1,5 do
        if not Mat_similar(last, tex) then break; end
        tex = rand.key_by_probs(tab)
      end
    end

    return tex
  end


  local function WALL_brush(data, coords)
    if data.shadow_info then
      local sh_coords = Brush_shadowify(coords, 40)
--!!!!      Trans.old_brush(data.shadow_info, sh_coords, -EXTREME_H, (data.z2 or EXTREME_H) - 4)
    end

    if data.f_h then table.insert(coords, { t=data.f_h, delta_z=data.f_delta }) end
    if data.c_h then table.insert(coords, { b=data.c_h, delta_z=data.c_delta }) end

    Brush_set_mat(coords, data.w_mat, data.w_mat)

    brush_helper(coords)
  end


  local function FC_brush(data, coords)
    if data.f_h then
      local coord2 = table.deep_copy(coords)
      table.insert(coord2, { t=data.f_h, delta_z=data.f_delta })

      Brush_set_mat(coord2, data.f_mat, data.f_mat)

      brush_helper(coord2)
    end

    if data.c_h then
      local coord2 = table.deep_copy(coords)
      table.insert(coord2, { b=data.c_h, delta_z=data.c_delta })

      Brush_set_mat(coord2, data.c_mat, data.c_mat)

      brush_helper(coord2)
    end
  end


  local function render_walls_OLD()

    --- DO WALLS ---

    local data = { w_mat = cave_tex }

    cave:render(WALL_brush, data)


    -- handle islands first

--[[
    each island in cave.islands do

      -- FIXME
      if LEVEL.liquid and not R.is_lake and --(( reg.cells > 4 and --))
         rand.odds(1)
      then

        -- create a lava/nukage pit
        local pit = Mat_lookup(LEVEL.liquid.mat)

        island:render(WALL_brush,
                      { f_h=R.cave_floor_h+8, pit.f or pit.t,
                        delta_f=rand.sel(70, -52, -76) })

        cave:subtract(island)
      end
    end
--]]


do return end ----!!!!!!!


    if R.is_lake then return end
    if THEME.square_caves then return end
    if PARAM.simple_caves then return end


    local ceil_h = R.cave_floor_h + R.cave_h

    -- TODO: @ pass 3, 4 : come back up (ESP with liquid)

    local last_ftex = R.cave_tex

    for i = 1,rand.index_by_probs({ 10,10,70 })-1 do
      walkway:shrink(false)

  ---???    if rand.odds(sel(i==1, 20, 50)) then
  ---???      walkway:shrink(false)
  ---???    end

      walkway:remove_dots()


      -- DO FLOOR and CEILING --

      data = {}


      if R.outdoor then
        data.ftex = choose_tex(last_ftex, THEME.landscape_trims or THEME.landscape_walls)
      else
        data.ftex = choose_tex(last_ftex, THEME.cave_trims or THEME.cave_walls)
      end

      last_ftex = data.ftex

      data.do_floor = true

      if LEVEL.liquid and i==2 and rand.odds(60) then  -- TODO: theme specific prob
        local liq_mat = Mat_lookup(LEVEL.liquid.mat)
        data.ftex = liq_mat.f or liq_mat.t

        -- FIXME: this bugs up monster/pickup/key spots
        if rand.odds(0) then
          data.delta_f = -(i * 10 + 40)
        end
      end

      if true then
        data.delta_f = -(i * 10)
      end

      data.f_h = R.cave_floor_h + i

      data.do_ceil = false

      if R.kind != "outdoor" then
        data.do_ceil = true

        if i==2 and rand.odds(60) then
          local mat = Mat_lookup("_SKY")
          data.ctex = mat.f or mat.t
        elseif rand.odds(50) then
          data.ctex = data.ftex
        elseif rand.odds(80) then
          data.ctex = choose_tex(data.ctex, THEME.cave_trims or THEME.cave_walls)
        end

        data.delta_c = int((0.6 + (i-1)*0.3) * R.cave_h)

        data.c_z = ceil_h - i
      end


      walkway:render(FC_brush, data)
    end
  end


  local function render_walls()
    local w_mat = cave_tex

    for x = 1,cave.w do for y = 1,cave.h do
      if (cave:get(x, y) or 0) > 0 then
        local brush = brush_for_cell(x, y)

        Brush_set_mat(brush, w_mat, w_mat)

        brush_helper(brush)
      end
    end end
  end


  local function area_cell_bbox(A)
    -- TODO: this is whole cave, ideally have just the area

    local x1 = cave.base_x + 40
    local y1 = cave.base_y + 40

    local x2 = x1 + cave.w * 64 - 40
    local y2 = y1 + cave.w * 64 - 40

    return x1,y1, x2,y2

--  return A:shrink_bbox_for_room_edges(x1,y1, x2,y2)
  end


  local function render_floor_ceil(A)
    assert(A.floor_map)

    local f_mat = R.floor_mat or cave_tex
    local c_mat = R.ceil_mat  or cave_tex

    local f_h = A.floor_h
    local c_h = R.max_floor_h + rand.pick { 128, 192,192,192, 288 }

    if c_mat == "_SKY" then
      c_h = R.max_floor_h + 192
    elseif rand.odds(25) then
      c_h = R.max_floor_h + 400
    end

    A.ceil_h = c_h

    -- Spot stuff
    local x1, y1, x2, y2 = area_cell_bbox(A)

    gui.spots_begin(x1+4, y1+4, x2-4, y2-4, 2)

    for x = 1,cave.w do for y = 1,cave.h do
      if (A.floor_map:get(x, y) or 0) > 0 then

        local f_brush = brush_for_cell(x, y)
        local c_brush = brush_for_cell(x, y)

        Brush_add_top   (f_brush, f_h)
        Brush_add_bottom(c_brush, c_h)

        Brush_set_mat(f_brush, f_mat, f_mat)
        Brush_set_mat(c_brush, c_mat, c_mat)

        if c_mat == "_SKY" then
          table.insert(c_brush, 1, { m="sky" })
        end

        gui.spots_fill_poly(f_brush, 0)

        brush_helper(f_brush)
        brush_helper(c_brush)

        -- handle walls (Spot stuff)
        for dir = 2,8,2 do
          local nx, ny = geom.nudge(x, y, dir)
          if cave:valid_cell(nx, ny) and R.area_map:get(nx, ny) != A then
            local is_wall = (R.area_map:get(nx, ny) == nil)
            local poly = brush_for_cell(nx, ny)
            gui.spots_fill_poly(poly, (is_wall ? 1 ; 2))
          end
        end

      end
    end end

    A:grab_spots()

    gui.spots_end()
  end


  local function heights_near_island(island)
    local min_floor =  9e9
    local max_ceil  = -9e9
  
    for x = 1,cave.w do for y = 1,cave.h do
      if ((island:get(x, y) or 0) > 0) then
        for dir = 2,8,2 do
          local nx, ny = geom.nudge(x, y, dir)

          if not island:valid_cell(nx, ny) then continue end

          local A = R.area_map:get(nx, ny)
          if not A then continue end

          min_floor = math.min(min_floor, A.floor_h)
          max_ceil  = math.max(max_ceil , A.ceil_h)
        end
      end
    end end

--!!!! FIXME  assert(min_floor < max_ceil)

    return min_floor, max_ceil
  end


  local function render_liquid_area(island)
    -- create a lava/nukage pit

    local f_mat = R.floor_mat or cave_tex
    local c_mat = R.ceil_mat  or cave_tex
    local l_mat = LEVEL.liquid.mat

    local f_h, c_h = heights_near_island(island)

    -- FIXME!! should not happen
    if f_h >= c_h then return end

    f_h = f_h - 24
    c_h = c_h + 64

    -- TODO: fireballs for Quake

    for x = 1,cave.w do for y = 1,cave.h do

      if ((island:get(x, y) or 0) > 0) then

        -- do not render a wall here
        cave:set(x, y, 0)

        local f_brush = brush_for_cell(x, y)
        local c_brush = brush_for_cell(x, y)

        if PARAM.deep_liquids then
          Brush_add_top(f_brush, f_h-128)
          Brush_set_mat(f_brush, f_mat, f_mat)

          brush_helper(f_brush)

          local l_brush = brush_for_cell(x, y)

          table.insert(l_brush, 1, { m="liquid", medium=LEVEL.liquid.medium })

          Brush_add_top(l_brush, f_h)
          Brush_set_mat(l_brush, "_LIQUID", "_LIQUID")

          brush_helper(l_brush)

          -- TODO: lighting

        else
          Brush_add_top(f_brush, f_h)

          -- damaging (FIXME)
          f_brush[#f_brush].special = 16

          Brush_set_mat(f_brush, l_mat, l_mat)

          brush_helper(f_brush)
        end

        -- common ceiling code

        Brush_add_bottom(c_brush, c_h)
        Brush_set_mat(c_brush, c_mat, c_mat)

        brush_helper(c_brush)
      end

    end end -- x, y
  end


  local function add_liquid_pools()
    if not LEVEL.liquid then return end

    local prob = 70  -- FIXME

    each island in cave.islands do
      if rand.odds(prob) then
        render_liquid_area(island)
      end
    end
  end


  ---| Simple_render_cave |---
  
  create_delta_map()

  each A in R.areas do
    render_floor_ceil(A)
  end

  add_liquid_pools()

  render_walls()
end

