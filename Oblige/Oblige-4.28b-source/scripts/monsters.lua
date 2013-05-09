----------------------------------------------------------------
--  MONSTERS/HEALTH/AMMO
----------------------------------------------------------------
--
--  Oblige Level Maker
--
--  Copyright (C) 2008-2012 Andrew Apted
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


--[[

MONSTER SELECTION
=================

Main usages:
(a) free-range
(b) guarding something [keys]
(c) cages
(d) traps (triggered closets, teleport in)
(e) surprises (behind entry door, closets on back path)


MONSTERS:

(1) have the room palette

(2) simplify selection of room palette:
    -  use info.prob
    -  basic criterion is: harder monsters occur later
    -  want harder monsters in KEY/EXIT rooms

(3) give each mon a 'density' value:
    -  use info.density (NOT info.prob)
    -  adjust with damage/time/along/level/purpose
    -  when all monsters have a density, normalise by dividing by total

(4) give each room a TOTAL count (adjust for along, purpose)
    -  want[mon] = total * density[mon]


IDEAS:

Free range monsters make up the bulk of the level, and are
subject to the palette.  The palette applies to a fair size
of a map, on "small" setting --> 1 palette only, upto 2 on
"regular", between 2-3 on "large" maps.

Trap and Surprise monsters can use any monster (actually
better when different from palette and different from
previous traps/surprises).

Cages and Guarding monsters should have a smaller and
longer-term palette, changing about 4 times less often
than the free-range palette.  MORE PRECISELY: palette
evolves about same rate IN TERMS OF # MONSTERS ADDED.

--------------------------------------------------------------]]


MONSTER_QUANTITIES =
{
  scarce=6, less=12, normal=18, more=30, heaps=50, nuts=200
}

MONSTER_KIND_TAB =
{
  scarce=2, less=3, normal=4, more=4.5, heaps=6, nuts=6
}

HEALTH_AMMO_ADJUSTS =
{
  none=0, scarce=0.4, less=0.7, normal=1.0, more=1.5, heaps=2.5,
}


COOP_MON_FACTOR = 1.35
COOP_HEALTH_FACTOR = 1.3
COOP_AMMO_FACTOR   = 1.6

MONSTER_MAX_TIME   = { weak=6,  medium=9,   tough=12  }
MONSTER_MAX_DAMAGE = { weak=70, medium=110, tough=180 }


-- Doom flags
DOOM_FLAGS =
{
  EASY    = 1
  MEDIUM  = 2
  HARD    = 4
  AMBUSH  = 8
}

-- Hexen thing flags
HEXEN_FLAGS =
{
  FIGHTER = 32
  CLERIC  = 64
  MAGE    = 128

  DM      = 1024
}

-- Quake flags
QUAKE_FLAGS =
{
  AMBUSH     = 1

  NOT_EASY   = 256
  NOT_MEDIUM = 512
  NOT_HARD   = 1024
  NOT_DM     = 2048
}

-- Hexen2 flags   [FIXME: NOT USED YET]
HEXEN2_FLAGS =
{
  NOT_PALADIN  = 256
  NOT_CLERIC   = 512
  NOT_NECRO    = 1024
  NOT_ASSASSIN = 2048

  NOT_EASY     = 4096
  NOT_MEDIUM   = 8192
  NOT_HARD     = 16384
  NOT_DM       = 32768
}


function Player_init()
  LEVEL.hmodels = table.deep_copy(GAME.PLAYER_MODEL)

  each CL,hmodel in LEVEL.hmodels do
    hmodel.class = CL
  end
end


function Player_give_weapon(weapon, only_CL)
  gui.printf("Giving weapon: %s\n", weapon)

  each CL,hmodel in LEVEL.hmodels do
    if not only_CL or (only_CL == CL) then
      hmodel.weapons[weapon] = 1
    end
  end
end


function Player_give_class_weapon(slot)
  each name,W in GAME.WEAPONS do
    each CL,hmodel in LEVEL.hmodels do
      if W.slot == slot and W.class == CL then
        hmodel.weapons[name] = 1
      end
    end
  end
end


function Player_give_map_stuff()
  if LEVEL.assume_weapons then
    each name,_ in LEVEL.assume_weapons do
          if name == "weapon2" then Player_give_class_weapon(2)
      elseif name == "weapon3" then Player_give_class_weapon(3)
      elseif name == "weapon4" then Player_give_class_weapon(4)
      else
        Player_give_weapon(name)
      end
    end
  end
end


function Player_give_room_stuff(L)
  if L.weapons and not PARAM.hexen_weapons then
    each name in L.weapons do
      Player_give_weapon(name)
    end
  end
end


function Player_give_stuff(hmodel, give_list)
  each give in give_list do
    if give.health then
      gui.debugf("Giving [%s] health: %d\n",
                 hmodel.class, give.health)
      hmodel.stats.health = hmodel.stats.health + give.health

    elseif give.ammo then
      gui.debugf("Giving [%s] ammo: %dx %s\n",
                 hmodel.class, give.count, give.ammo)

      hmodel.stats[give.ammo] = (hmodel.stats[give.ammo] or 0) + give.count

    elseif give.weapon then
      gui.debugf("Giving [%s] weapon: %s\n",
                 hmodel.class, give.weapon)

      hmodel.weapons[give.weapon] = 1
    else
      error("Bad give item : not health, ammo or weapon")
    end
  end
end


function Player_firepower()
  -- The 'firepower' is (roughly) how much damage per second
  -- the player would normally do using their current set of
  -- weapons.
  --
  -- If there are different classes (Hexen) then the result
  -- will be an average of each class, as all classes face
  -- the same monsters.

  local function get_firepower(hmodel)
    local firepower = 0 
    local divisor   = 0

    for weapon,_ in pairs(hmodel.weapons) do
      local info = GAME.WEAPONS[weapon]

      if not info then
        error("Missing weapon info for: " .. weapon)
      end

      local dm = info.damage * info.rate
      if info.splash then dm = dm + info.splash[1] end

      -- melee attacks are hard to use, and
      -- projectiles miss more often than hitscan
      if info.attack == "melee" then
        dm = dm / 3.0
      elseif info.attack == "missile" then
        dm = dm / 1.3
      end

      local pref = info.pref or 1

---   gui.debugf("  weapon:%s dm:%1.1f pref:%1.1f\n", weapon, dm, pref)

      firepower = firepower + dm * pref
      divisor   = divisor + pref
    end

    if divisor == 0 then
      error("Player_firepower: no weapons???")
    end

    return firepower / divisor
  end

  ---| Player_firepower |---

  local fp_total  = 0
  local class_num = 0

  for CL,hmodel in pairs(LEVEL.hmodels) do
    fp_total = fp_total + get_firepower(hmodel)
    class_num = class_num + 1
  end

  assert(class_num > 0)

  return fp_total / class_num
end


function Player_has_weapon(weap_needed)
  
  local function class_has_one(hmodel)
    for name,_ in pairs(hmodel.weapons) do
      if weap_needed[name] then
        return true
      end
    end
    return false
  end

  --| Player_has_weapon |--

  -- we require a match for every class

  for CL,hmodel in pairs(LEVEL.hmodels) do
    if not class_has_one(hmodel) then
      return false
    end
  end

  return true -- OK
end


----------------------------------------------------------------


function Monsters_init()
  table.name_up(GAME.MONSTERS)
  table.name_up(GAME.WEAPONS)
  table.name_up(GAME.PICKUPS)

  for name,info in pairs(GAME.MONSTERS) do
    if not info.id then
      error(string.format("Monster '%s' lacks an id field", name))
    end
  end

  LEVEL.mon_stats = {}

  local low_q  = MONSTER_QUANTITIES.scarce
  local high_q = MONSTER_QUANTITIES.more

  local low_k  = MONSTER_KIND_TAB.scarce
  local high_k = MONSTER_KIND_TAB.heaps

  LEVEL.prog_mons_qty  = low_q + LEVEL.ep_along * (high_q - low_q)
  LEVEL.prog_mons_kind = low_k + LEVEL.ep_along * (high_k - low_k)

  -- build replacement table --

  LEVEL.mon_replacement = {}

  local dead_ones = {}

  for name,info in pairs(GAME.MONSTERS) do
    local orig = info.replaces
    if orig then
      assert(info.replace_prob)
      if not GAME.MONSTERS[orig] then
        dead_ones[name] = true
      else
        if not LEVEL.mon_replacement[orig] then
          -- the basic replacement table allows the monster to
          -- pick itself at the time of replacement.
          LEVEL.mon_replacement[orig] = { [orig]=70 }
        end
        LEVEL.mon_replacement[orig][name] = info.replace_prob
      end
    end

    -- calculate a level if not present
    if not info.level then
      local hp = info.health * (PARAM.level_factor or 1)
          if hp < 45  then info.level = 1
      elseif hp < 130 then info.level = 3
      elseif hp < 450 then info.level = 5
      else  info.level = 7
      end
    end
  end

  -- remove a replacement monster if the monster it replaces
  -- does not exist (e.g. stealth_gunner in DOOM 1 mode).
  for name,_ in pairs(dead_ones) do
    GAME.MONSTERS[name] = nil
  end
end


function Monsters_max_level()
  local max_level = 10 * (LEVEL.mon_along or 0.5)

  if max_level < 1 then max_level = 1 end

  LEVEL.max_level = max_level

  gui.printf("Monster max_level: %1.2f\n", max_level)
end


function Monsters_pick_single_for_level()
  local tab = {}

  if not EPISODE.single_mons then
    EPISODE.single_mons = {}
  end

  each name,prob in LEVEL.global_pal do
    local info = GAME.MONSTERS[name]
    tab[name] = (info.level or 5) * 10

    -- prefer monsters which have not been used before
    if EPISODE.single_mons[name] then
      tab[name] = tab[name] / 10
    end
  end

  local name = rand.key_by_probs(tab)

  -- mark it as used
  EPISODE.single_mons[name] = 1

  return name
end


function Monsters_global_palette()
  -- Decides which monsters we will use on this level.
  -- Easiest way is to pick some monsters NOT to use.

  LEVEL.global_pal = {}

  each name,info in GAME.MONSTERS do
    if info.prob  and info.prob > 0 and
       info.level and info.level <= LEVEL.max_level
    then
      LEVEL.global_pal[name] = 1
    end
  end

  if not LEVEL.monster_prefs then
    LEVEL.monster_prefs = {}
  end


  -- only one kind of monster in this level?
  if STYLE.mon_variety == "none" then
    local the_mon = Monsters_pick_single_for_level()

    LEVEL.global_pal = {}
    LEVEL.global_pal[the_mon] = 1
  end


  -- sometimes skip monsters (for more variety).
  -- we don't skip when their level is close to max_level, to allow
  -- the gradual introduction of monsters to occur normally.

  if PARAM.skip_monsters and STYLE.mon_variety != "none" and STYLE.mon_variety != "heaps" then
    local perc = rand.pick(PARAM.skip_monsters)

    local skip_list = {}
    local skip_total = 0
    local mon_total  = 0

    each name,_ in LEVEL.global_pal do
      local M = GAME.MONSTERS[name]
      local prob = M.skip_prob or 50

      if M.level > LEVEL.max_level - 4 then prob = 0 end

      if prob > 0 then
        -- NOTE: we _could_ adjust probability based on Strength setting.
        -- _BUT_ it is probably better not to, otherwise we would just be
        -- skipping monsters which would not have been added anyway.

        -- adjust skip chance based on monster_prefs
        if LEVEL.monster_prefs then
          prob = prob / (0.1 + (LEVEL.monster_prefs[name] or 1))
        end
        if THEME.monster_prefs then
          prob = prob / (0.1 + (THEME.monster_prefs[name] or 1))
        end

        skip_list[name] = prob
        skip_total = skip_total + 1
      end

      mon_total = mon_total + 1
    end

    local count = int(skip_total * perc / 100 + gui.random())

    for i = 1,count do
      if table.empty(skip_list) then break; end

      local name = rand.key_by_probs(skip_list)
      skip_list[name] = nil

      gui.printf("Skipping monster: %s\n", name)
      LEVEL.global_pal[name] = nil
    end
  end

  gui.debugf("Monster global palette:\n%s\n", table.tostr(LEVEL.global_pal))

  gui.printf("\n")
end



function Monsters_dist_between_spots(A, B)
  local dist_x = 0
  local dist_y = 0

      if A.x1 > B.x2 then dist_x = A.x1 - B.x2
  elseif A.x2 < B.x1 then dist_x = B.x1 - A.x2
  end

      if A.y1 > B.y2 then dist_y = A.y1 - B.y2
  elseif A.y2 < B.y1 then dist_y = B.y1 - A.y2
  end

  local dist = math.max(dist_x, dist_y)

  -- large penalty for height difference
  if A.z1 != B.z1 then
    dist = dist + 1000
  end

  return dist
end



function Monsters_distribute_stats()

  local function distribute(R, health_qty, ammo_qty, N)
    each CL,R_stats in R.fight_stats do
      local N_stats = N.fight_stats[CL]

      each stat,count in R_stats do
        if count <= 0 then continue end

        local value = count * (stat == "health" ? health_qty ; ammo_qty)

        N_stats[stat] = (N_stats[stat] or 0) + value
        R_stats[stat] =  R_stats[stat]       - value

        gui.debugf("Distributing %s:%1.1f [%s]  %s --> %s\n",
                   stat, value,  CL, R:tostr(), N:tostr())
      end
    end
  end


  local function get_previous_prefs(room)
    local list = {}

    -- find previous rooms
    local R = room

    while R.entry_conn do
      R = R.entry_conn:neighbor(R)

      if R.kind != "hallway" then
        local ratio = rand.irange(3,7) * (0.5 ^ #list)
        table.insert(list, { room=R, ratio=ratio })
      end
    end

    -- add storage rooms
    if room.quest.storage_rooms then
      each R in room.quest.storage_rooms do
        local ratio = rand.irange(3,7) / 11
        table.insert(list, { room=R, ratio=ratio })
      end
    end

    return list
  end


  local function distribute_to_list(R, health_qty, ammo_qty, list)
    local total = 0

    each pos in list do
      total = total + pos.ratio
    end

    each pos in list do
      local h_qty = health_qty * pos.ratio / total
      local a_qty = ammo_qty   * pos.ratio / total

      distribute(R, h_qty, a_qty, pos.room)
    end
  end


  ---| Monsters_distribute_stats |---

  -- Note: we don't distribute to or from hallways

  gui.debugf("--- Monsters_distribute_stats ---\n")

  each R in LEVEL.rooms do
    if R.is_storage then continue end

    if R.fight_stats then
      distribute_to_list(R, 0.25, 0.75, get_previous_prefs(R))
    end
  end

  each R in LEVEL.rooms do
    if R.fight_stats then
      gui.debugf("final result @ %s = \n%s\n", R:tostr(),
                 table.tostr(R.fight_stats, 2))
    end
  end
end



function Monsters_do_pickups()

  local function sort_spots(L)
    rand.shuffle(L.item_spots)
  end


  local function decide_pickup(L, stat, qty)
    local item_tab = {}

    for name,info in pairs(GAME.PICKUPS) do
      if info.prob and
         (stat == "health" and info.give[1].health) or
         (info.give[1].ammo == stat)
      then
        item_tab[name] = info.prob

        if L.purpose == "START" and info.start_prob then
          item_tab[name] = info.start_prob
        end
      end
    end

    assert(not table.empty(item_tab))
    local name = rand.key_by_probs(item_tab)
    local info = GAME.PICKUPS[name]

    local count = 1
    
    if info.cluster then
      local each_qty = info.give[1].health or info.give[1].count
      local min_num  = info.cluster[1]
      local max_num  = info.cluster[2]

      assert(max_num <= 9)

      --- count = rand.irange(min_num, max_num)

      if min_num * each_qty >= qty then
        count = min_num
      elseif max_num * each_qty <= qty then
        count = max_num - rand.sel(20,1,0)
      else
        count = 1 + int(qty / each_qty)
      end
    end

    return GAME.PICKUPS[name], count
  end


  local function select_pickups(L, item_list, stat, qty, hmodel)
gui.debugf("Initial %s = %1.1f\n", stat, hmodel.stats[stat] or 0)

    -- subtract any previous gotten stuff
    qty = qty - (hmodel.stats[stat] or 0)
    hmodel.stats[stat] = 0

    -- more ammo in start room
    local excess = 0

    if L.purpose == "START" and L:has_weapon_using_ammo(stat) then
      if qty > 0 then
        excess = (OB_CONFIG.strength == "crazy" ? 1.2 ; 0.6) * qty
      end

      if GAME.AMMOS and GAME.AMMOS[stat] then
        excess = math.max(excess, GAME.AMMOS[stat].start_bonus)
      end
      gui.debugf("Bonus %s = %1.1f\n", stat, excess)
    end

    qty = qty + excess

    while qty > 0 do
      local item, count = decide_pickup(L, stat, qty)
      table.insert(item_list, { item=item, count=count, random=gui.random() })
      
      if stat == "health" then
        qty = qty - item.give[1].health * count
      else
        assert(item.give[1].ammo)
        qty = qty - item.give[1].count * count
      end
    end

    excess = excess + (-qty)

    -- accumulate any excess quantity into the hmodel

    if excess > 0 then
gui.debugf("Excess %s = %1.1f\n", stat, excess)
      hmodel.stats[stat] = hmodel.stats[stat] + excess
    end
  end


  local function place_item(item_name, x, y, z)
    local props = {}

    if PARAM.use_spawnflags then
      -- no change
    else
      props.flags = DOOM_FLAGS.EASY + DOOM_FLAGS.MEDIUM + DOOM_FLAGS.HARD
    end

    entity_helper(item_name, x, y, z, props)
  end


  local function place_big_item(spot, item)
    -- FIXME : this is unused atm, need a tidy up!!

    local x, y = spot.x, spot.y

    -- assume big spots will sometimes run out (and be reused),
    -- so don't put multiple items at exactly the same place.
    x = x + rand.irange(-6, 6)
    y = y + rand.irange(-6, 6)

    place_item(item, x, y, spot.z)
  end


  local function OLD__place_small_item(spot, item, count)
    local x1, y1 = spot.x, spot.y
    local x2, y2 = spot.x, spot.y

    if count == 1 then
      place_item(spot.S, item, x1,y1)
      return
    end

    local away = (count == 2 ? 20 ; 40)
    local dir  = spot.dir

    if geom.is_vert(dir) then
      y1, y2 = y1-away, y2+away
    elseif geom.is_horiz(dir) then
      x1, x2 = x1-away, x2+away
    elseif dir == 1 or dir == 9 then
      x1, y1 = x1-away, y1-away
      x2, y2 = x2+away, y2+away
    elseif dir == 3 or dir == 7 then
      x1, y1 = x1-away, y1+away
      x2, y2 = x2+away, y2-away
    else
      error("place_small_item: bad dir: " .. tostring(dir))
    end

    for i = 1,count do
      local x = x1 + (x2 - x1) * (i-1) / (count-1)
      local y = y1 + (y2 - y1) * (i-1) / (count-1)

      place_item(spot.S, item, x, y)
    end
  end


  local function find_cluster_spot(L, prev_spots, item_name)
    if #prev_spots == 0 then
      local spot = table.remove(L.item_spots, 1)
      table.insert(prev_spots, spot)
      return spot
    end

    local best_idx
    local best_dist

    -- FIXME: optimise this!
    for index = 1,#L.item_spots do
      local spot = L.item_spots[index]
      local dist = 9e9

      each prev in prev_spots do
        local d = Monsters_dist_between_spots(prev, spot)
        dist = math.min(dist, d)
      end

      -- avoid already used spots
      if spot.used then dist = dist + 9000 end

      if not best_idx or dist < best_dist then
        best_idx  = index
        best_dist = dist
      end
    end

    assert(best_idx)

    local spot = table.remove(L.item_spots, best_idx)

    if #prev_spots >= 3 then
      table.remove(prev_spots, 1)
    end

    table.insert(prev_spots, spot)

    return spot
  end


  local function place_item_list(L, item_list, CL)
    for _,pair in ipairs(item_list) do
      local item  = pair.item
      local count = pair.count
      local spot

      if item.big_item and not table.empty(L.big_item_spots) then
        assert(count == 1)

        spot = table.remove(L.big_item_spots)
        spot.x, spot.y = geom.box_mid(spot.x1, spot.y1, spot.x2, spot.y2)
        spot.z = spot.z1

        place_big_item(spot, item.name, CL)

        return
      end

      -- keep track of a limited number of previously chosen spots.
      -- when making clusters, this is used to find the next spot.
      local prev_spots = {}

      for i = 1,count do
        if table.empty(L.item_spots) then
          gui.printf("Unable to place items: %s x %d\n", item.name, count+1-i)
          break;
        end

        local spot = find_cluster_spot(L, prev_spots, item.name)

        local x, y = geom.box_mid(spot.x1, spot.y1, spot.x2, spot.y2)

        place_item(item.name, x, y, spot.z1)

        -- reuse spots if they run out (except in Wolf3D)
        spot.used = true
        table.insert(L.item_spots, spot)

      end
    end
  end


  local function pickups_for_hmodel(L, CL, hmodel)
    if table.empty(GAME.PICKUPS) then
      return
    end

    local stats = L.fight_stats[CL]
    local item_list = {}

    for stat,qty in pairs(stats) do
      if qty > 0 then
        select_pickups(L, item_list, stat, qty, hmodel)

        gui.debugf("Item list for %s:%1.1f [%s] @ %s\n", stat,qty, CL, L:tostr())

        for _,pair in ipairs(item_list) do
          local item = pair.item
          gui.debugf("   %dx %s (%d)\n", pair.count, item.name,
                     item.give[1].health or item.give[1].count)
        end
      end
    end

    sort_spots(L)

    -- place large clusters before small ones
    table.sort(item_list, function(A,B) return (A.count + A.random) > (B.count + B.random) end)

    -- kludge to add some backpacks to DOOM maps
    -- TODO: better system for "nice start items" or so
    if L.purpose == "START" and GAME.ENTITIES["backpack"] then
      table.insert(item_list, 1, { item={ name="backpack", big_item=true }, count=1 })
    end

    place_item_list(L, item_list, CL)
  end


  local function pickups_in_room(L)
    for CL,hmodel in pairs(LEVEL.hmodels) do
      pickups_for_hmodel(L, CL, hmodel)
    end
  end


  ---| Monsters_do_pickups |---

  gui.debugf("--- Monsters_do_pickups ---\n")

  each Z in LEVEL.zones do
    each L in Z.rooms do
      pickups_in_room(L)
    end
  end
end


function Monsters_in_room(L)


  local CAGE_REUSE_FACTORS = { 5, 20, 60, 200 }

  local function is_big(mon)
    return GAME.MONSTERS[mon].r > 30
  end

  local function is_huge(mon)
    return GAME.MONSTERS[mon].r > 60
  end


  local function OLD_calc_toughness()
    -- determine a "toughness" value, where 1.0 is easy and
    -- higher values produces tougher monsters.

    -- each level gets progressively tougher
    local toughness = LEVEL.episode.index + LEVEL.ep_along * 4

    -- each room is tougher too
    toughness = toughness + L.lev_along

    -- spice it up
    toughness = toughness + gui.random() ^ 2

    gui.debugf("Toughness = %1.3f\n", toughness)

    return toughness
  end


  local function calc_quantity()
    local qty

    if LEVEL.quantity then
      qty = LEVEL.quantity

    elseif OB_CONFIG.mons == "mixed" then
      qty = rand.pick { 5,10,25,50 }

    elseif OB_CONFIG.mons == "prog" then
      qty = LEVEL.prog_mons_qty

    else
      qty = MONSTER_QUANTITIES[OB_CONFIG.mons]
      assert(qty)

      -- tend to have more monsters in later rooms and levels
      qty = qty * (2 + L.lev_along + LEVEL.ep_along) / 4
    end

    -- game specific adjustment
    qty = qty * (PARAM.monster_factor or 1)

    -- more monsters for Co-operative games
    if OB_CONFIG.mode == "coop" then
      qty = qty * COOP_MON_FACTOR
    end


    -- less in hallways
    if L.kind == "hallway" then
      qty = qty * rand.pick { 0.6, 0.8, 1.1 }

    -- more in EXIT or KEY rooms (extra boost in small rooms)
    elseif L.purpose and L.purpose != "START" then
      qty = qty * rand.pick { 1.4, 1.6, 1.9 }

      local is_small = (L.svolume <= 20)

      if is_small then qty = qty * 1.25 end
    else
      -- random variation
          if rand.odds(5) then qty = qty * 0.4
      elseif rand.odds(5) then qty = qty * 1.9
      else
        qty = qty * rand.pick { 0.7, 1.0, 1.3 }
      end
    end

    gui.debugf("Quantity = %1.1f\n", qty)
    return qty
  end


  local function tally_spots(spot_list)
    -- This is meant to give a rough estimate, and assumes each monster
    -- fits in a 64x64 square and there is no height restrictions.
    -- We can adjust for the real monster size later.

    local count = 0

    each spot in spot_list do
      local w, h = geom.box_size(spot.x1, spot.y1, spot.x2, spot.y2)

      w = int(w / 64) ; if w < 1 then w = 1 end
      h = int(h / 64) ; if h < 1 then h = 1 end

      count = count + w * h
    end

    return count
  end


  local function calc_strength_factor(info)
    local factor = (info.level or 1)

    if OB_CONFIG.strength == "weak" then
      return 2 / (1 + factor)

    elseif OB_CONFIG.strength == "tough" then
      return (factor + 1) / 2

    else
      return 1
    end
  end


  local function prob_for_mon(name, info, enviro)
    local prob = info.prob

    if enviro == "cage" then
      -- no flying monsters in cage (some cages don't have bars)
      if info.float then return 0 end

      -- monster needs a long distance attack
      if info.attack == "melee" then return 0 end
    end

    if THEME.force_mon_probs then
      prob = THEME.force_mon_probs[name] or
             THEME.force_mon_probs._else
      if prob then return prob end
    end

    prob = prob or 0

    if not LEVEL.global_pal[name] then
      return 0
    end

    if info.weap_needed and not Player_has_weapon(info.weap_needed) then
      return 0
    end

    -- TODO: merge THEME.monster_prefs into LEVEL.monster_prefs
    if LEVEL.monster_prefs then
      prob = prob * (LEVEL.monster_prefs[name] or 1)
    end
    if THEME.monster_prefs then
      prob = prob * (THEME.monster_prefs[name] or 1)
    end
    if L.theme.monster_prefs then
      prob = prob * (L.theme.monster_prefs[name] or 1)
    end

    if L.kind == "outdoor" or L.semi_outdoor then
      prob = prob * (info.outdoor_factor or 1)
    end

    if prob == 0 then return 0 end


    -- apply user's Strength setting
    prob = prob * calc_strength_factor(info)


    -- level check (harder monsters occur in later rooms)
    assert(info.level)

    if not L.purpose then
      local max_level = LEVEL.max_level * L.lev_along
      if max_level < 2 then max_level = 2 end

      if info.level > max_level then
        prob = prob / 20
      end
    end

    return prob
  end


  local function density_for_mon(mon)
    local info = GAME.MONSTERS[mon]

    local d = info.density or 1
    
    -- level check
    local max_level = LEVEL.max_level * L.lev_along
    if max_level < 2 then max_level = 2 end

    if info.level > max_level then
      d = d / 4
    end

    -- random variation
    d = d * rand.range(0.5, 1.5)


    -- time and damage checks

    local toughness = 1  -- FIXME

    local time   = info.health / L.firepower
    local damage = info.damage * time

    if info.attack == "melee" then
      damage = damage / 5
    elseif info.attack == "missile" then
      damage = damage / 2
    end

    if toughness > 1 then
      time = time / math.sqrt(toughness)
    end

    damage = damage / toughness

    if PARAM.time_factor then
      time = time * PARAM.time_factor
    end
    if PARAM.damage_factor then
      damage = damage * PARAM.damage_factor
    end


    -- would the monster take too long to kill?
    local max_time = MONSTER_MAX_TIME[OB_CONFIG.strength] or 8

    if time > max_time*2 then
      d = d / 4
    elseif time > max_time then
      d = d / 2
    end


    -- would the monster inflict too much damage on the player?

    --> TOO FRICKIN' BAD

--[[
    local max_damage = MONSTER_MAX_DAMAGE[OB_CONFIG.strength] or 100

    if damage > max_damage then
      d = d / 
    end
--]]
    return d
  end


  local function number_of_kinds()
    local base_num

    if STYLE.mon_variety == "heaps" or rand.odds(3) then return 9 end
    if STYLE.mon_variety != "some"  or rand.odds(3) then return 1 end

    if OB_CONFIG.mons == "mixed" then
      base_num = rand.range(MONSTER_KIND_TAB.scarce, MONSTER_KIND_TAB.heaps)

    elseif OB_CONFIG.mons == "prog" then
      base_num = LEVEL.prog_mons_kind

    else
      base_num = MONSTER_KIND_TAB[OB_CONFIG.mons]
    end

    assert(base_num)

    if L.kind == "hallway" then
      return rand.index_by_probs { 60, 40, 20 }
    end

    -- adjust the base number to account for room size
    local size = math.sqrt(L.svolume)
    local num  = int(base_num * size / 12 + 0.6 + gui.random())

    if num < 1 then num = 1 end
    if num > 5 then num = 5 end  -- FIXME: game specific --> PARAM.xxx

    if rand.odds(30) then num = num + 1 end
    if rand.odds(3)  then num = num + 1 end

    gui.debugf("number_of_kinds: %d (base: %d)\n", num, base_num)

    return num
  end


  local function crazy_palette()
    local num_kinds

    if L.kind == "hallway" then
      num_kinds = rand.index_by_probs({ 40, 60, 20 })
    else
      local size = math.sqrt(L.svolume)
      num_kinds = int(size / 2)
    end

    local list = {}

    for name,info in pairs(GAME.MONSTERS) do
      local prob = info.crazy_prob or info.prob or 0

--??  if not LEVEL.global_palette[name] then
--??    prob = 0
--??  end

      if info.weap_needed and not Player_has_weapon(info.weap_needed) then
        prob = 0
      end

      if THEME.force_mon_probs then
        prob = THEME.force_mon_probs[name] or
               THEME.force_mon_probs._else or prob  
      end

      if prob > 0 and LEVEL.monster_prefs then
        prob = prob * (LEVEL.monster_prefs[name] or 1)
        if info.replaces then
          prob = prob * (LEVEL.monster_prefs[info.replaces] or 1)
        end
      end

      if prob > 0 then
        list[name] = prob
      end
    end

    local palette = {}

    gui.debugf("Monster palette: (%d kinds, %d actual)\n", num_kinds, table.size(list))

    for i = 1,num_kinds do
      if table.empty(list) then break; end

      local mon = rand.key_by_probs(list)
      palette[mon] = list[mon]

      gui.debugf("  #%d %s\n", i, mon)
      LEVEL.mon_stats[mon] = (LEVEL.mon_stats[mon] or 0) + 1

      list[mon] = nil
    end

    return palette
  end


  local function room_palette()
    local list = {}
    gui.debugf("Monster list:\n")

    for mon,info in pairs(GAME.MONSTERS) do
      local prob = prob_for_mon(mon, info)

      if prob > 0 then
        list[mon] = prob
        gui.debugf("  %s --> prob:%1.1f\n", mon, prob)
      end
    end

    local num_kinds = number_of_kinds()

    local palette = {}

    gui.debugf("Monster palette: (%d kinds, %d actual)\n", num_kinds, table.size(list))

    for i = 1,num_kinds do
      if table.empty(list) then break; end

      local mon  = rand.key_by_probs(list)
      local prob = list[mon]
      list[mon] = nil

      -- sometimes replace it completely (e.g. all demons become spectres)
      if rand.odds(25) and LEVEL.mon_replacement[mon] then
        mon = rand.key_by_probs(LEVEL.mon_replacement[mon])
      end

---##   -- give large monsters a boost so they are more likely to find a
---##   -- usable spot.  This does not affect the desired quantity.
---##   if is_big(mon) then prob = prob * 2 end

      palette[mon] = prob

      gui.debugf("  #%d %s\n", i, mon)
      LEVEL.mon_stats[mon] = (LEVEL.mon_stats[mon] or 0) + 1
    end

    return palette
  end


  local function FIXME__monster_fits(S, mon, info)
    if S.usage or S.no_monster or not S.floor_h then
      return false
    end

    -- keep entryway clear
    if R.entry_conn and S:has_conn(R.entry_conn) then
      return false
    end

    -- check seed kind
    -- (floating monsters can go in more places)
    if S.kind == "walk" or (S.kind == "liquid" and info.float) then
      -- OK
    else
      return false
    end

    -- check if fits vertically
    local ceil_h = S.ceil_h or R.ceil_h or SKY_H
    local ent = assert(GAME.MONSTERS[mon])

    if ent.h >= (ceil_h - S.floor_h - 1) then
      return false
    end

    if is_huge(mon) and S.solid_corner then
      return false
    end

    return true
  end


  local function monster_angle(spot, x, y, z)
    -- TODO: sometimes make all monsters (or a certain type) face
    --       the same direction, or look towards the entrance, or
    --       towards the guard_spot.

    if spot.angle then
      return spot.angle
    end

    local face = spot.face or spot.face_away

    if face then
      local away = (face == spot.face_away)

      local dir = geom.closest_dir(face.x - x, face.y - y)

      if away then
        dir = 10 - dir
      end

      local angle = geom.ANGLES[dir]

      local delta = rand.irange(-1,1) * 45

      return geom.angle_add(angle, delta)
    end

    -- fallback : purely random angle
    return rand.irange(0,7) * 45
  end


  local function calc_min_skill(all_skills)
    if all_skills then return 1 end

    local dither = Plan_alloc_id("mon_dither")

    -- skill 3 (hard) is always added
    -- skill 2 (medium) alternates between 100% and 60% chance
    -- skill 1 (easy) is always 60% chance of adding

    if rand.odds(60) then
      return 1
    elseif (dither % 2) == 0 then
      return 2
    else
      return 3
    end
  end


  local function place_monster(mon, spot, x, y, z, all_skills)
    local angle = monster_angle(spot, x, y, z)

    local ambush = rand.sel(70, 1, 0)

    local info = GAME.MONSTERS[mon]

    -- handle replacements
    if LEVEL.mon_replacement[mon] and not L.no_replacement then
      mon  = rand.key_by_probs(LEVEL.mon_replacement[mon])
      info = assert(GAME.MONSTERS[mon])
    end

    table.insert(L.monster_list, info)

    -- minimum skill needed for the monster to appear
    local skill = calc_min_skill(all_skills)

    local props = { }

    props.angle = angle

    if PARAM.use_spawnflags then
      props.spawnflags = 0

      -- UGH, special check needed for Quake zombie
      if ambush and mon != "zombie" then
        props.spawnflags = props.spawnflags + QUAKE_FLAGS.AMBUSH
      end

      if (skill > 1) then props.spawnflags = props.spawnflags + QUAKE_FLAGS.NOT_EASY end
      if (skill > 2) then props.spawnflags = props.spawnflags + QUAKE_FLAGS.NOT_MEDIUM end
    else
      props.flags = DOOM_FLAGS.HARD

      if ambush then
        props.flags = props.flags + DOOM_FLAGS.AMBUSH
      end

      if (skill <= 1) then props.flags = props.flags + DOOM_FLAGS.EASY end
      if (skill <= 2) then props.flags = props.flags + DOOM_FLAGS.MEDIUM end
    end

    entity_helper(mon, x, y, z, props)
  end


  local function mon_fits(mon, spot)
    local info  = GAME.MONSTERS[mon]

    -- FIXME !!!
    -- if info.h >= (spot.z2 - spot.z1) then return 0 end

    local w, h = geom.box_size(spot.x1, spot.y1, spot.x2, spot.y2)

    w = int(w / info.r / 2.2)
    h = int(h / info.r / 2.2)

    return w * h
  end


  local function place_in_spot(mon, spot, all_skills)
    local info = GAME.MONSTERS[mon]

    local x, y = geom.box_mid (spot.x1, spot.y1, spot.x2, spot.y2)
    local w, h = geom.box_size(spot.x1, spot.y1, spot.x2, spot.y2)

    local z = spot.z1

    -- move monster to random place within the box
    local dx = w / 2 - info.r
    local dy = h / 2 - info.r

    if dx > 0 then
      x = x + rand.range(-dx, dx)
    end

    if dy > 0 then
      y = y + rand.range(-dy, dy)
    end

    place_monster(mon, spot, x, y, z, all_skills)

--[[
    local w, h = geom.box_size(spot.x1, spot.y1, spot.x2, spot.y2)

    w = int(w / info.r / 2.2)
    h = int(h / info.r / 2.2)

    for mx = 1,w do for my = 1,h do
      local x = spot.x1 + info.r * 2.2 * (mx-0.5)
      local y = spot.y1 + info.r * 2.2 * (my-0.5)
      local z = spot.z1

      place_monster(mon, x, y, z)

      count = count - 1
      if count < 1 then return end
    end end
--]]
  end


  local function split_huge_spots(max_size)
    local list = L.mon_spots

    -- recreate the spot list
    L.mon_spots = {}

    for _,spot in ipairs(list) do
      local w, h = geom.box_size(spot.x1, spot.y1, spot.x2, spot.y2)

      local XN = math.ceil(w / max_size)
      local YN = math.ceil(h / max_size)

      assert(XN > 0 and YN > 0)

      if XN < 2 and YN < 2 then
        table.insert(L.mon_spots, spot)
      else
        for x = 1,XN do for y = 1,YN do
          local x1 = spot.x1 + (x - 1) * w / XN
          local x2 = spot.x1 + (x    ) * w / XN

          local y1 = spot.y1 + (y - 1) * h / YN
          local y2 = spot.y1 + (y    ) * h / YN

          local new_spot = table.copy(spot)

          new_spot.x1 = int(x1) ; new_spot.y1 = int(y1)
          new_spot.x2 = int(x2) ; new_spot.y2 = int(y2)

          table.insert(L.mon_spots, new_spot)
        end end
      end
    end
  end


  local function split_spot(index, r, near_to)
    local spot = table.remove(L.mon_spots, index)

    local w, h = geom.box_size(spot.x1, spot.y1, spot.x2, spot.y2)

    assert(w >= r*2 and h >= r*2)

    -- for small monsters, up their size to 64 units so that when
    -- the split-off pieces are created it allows other kinds of
    -- small monsters to fit in those pieces.
    local r2 = math.max(r*2, 64)

    if w >= r2 + 64 then
      local remain = table.copy(spot)

      local side = rand.sel(50, 4, 6)

      if near_to then
        local d1 = math.abs(near_to.x1 - spot.x1)
        local d2 = math.abs(near_to.x2 - spot.x2)
        side = (d1 < d2 ? 4 ; 6)
      end

      if side == 4 then
        spot.x2   = spot.x1 + r2
        remain.x1 = spot.x1 + r2
      else
        spot.x1   = spot.x2 - r2
        remain.x2 = spot.x2 - r2
      end

      table.insert(L.mon_spots, remain)
    end

    if h >= r2 + 64 then
      local remain = table.copy(spot)

      local side = rand.sel(50, 2, 8)

      if near_to then
        local d1 = math.abs(near_to.y1 - spot.y1)
        local d2 = math.abs(near_to.y2 - spot.y2)
        side = (d1 < d2 ? 2 ; 8)
      end

      if side == 2 then
        spot.y2   = spot.y1 + r2
        remain.y1 = spot.y1 + r2
      else
        spot.y1   = spot.y2 - r2
        remain.y2 = spot.y2 - r2
      end

      table.insert(L.mon_spots, remain)
    end

    w, h = geom.box_size(spot.x1, spot.y1, spot.x2, spot.y2)
    assert(w >= r*2 and h >= r*2)

    return spot
  end


  local function find_spot(mon, near_to)
    local info = GAME.MONSTERS[mon]

    local poss_spots = {}

    for index,spot in ipairs(L.mon_spots) do
      local fit_num = mon_fits(mon, spot)
      if fit_num > 0 then

        if near_to then
          spot.find_cost = Monsters_dist_between_spots(spot, near_to)
        else
          spot.find_cost = 0
        end 

        -- tie breeker
        spot.find_cost  = spot.find_cost + gui.random() * 16
        spot.find_index = index

        table.insert(poss_spots, spot)
      end
    end

    if table.empty(poss_spots) then
      return nil  -- no available spots!
    end

    local result = table.pick_best(poss_spots,
        function(A, B) return A.find_cost < B.find_cost end)
  
    return split_spot(result.find_index, info.r, near_to)
  end


  local function try_add_mon_group(mon, count, all_skills)
    local spot
    local actual = 0
    
    for i = 1,count do
      spot = find_spot(mon, spot)

      if not spot then break; end

      place_in_spot(mon, spot, all_skills)

      actual = actual + 1
    end

    return actual
  end


  local function how_many_dudes(palette, want_total)
    -- the 'NONE' entry is a stabilizing element, in case we have a
    -- palette containing mostly undesirable monsters (Archviles etc).
    local densities = { NONE=1.0 }

    local total_density = densities.NONE

    each mon,_ in palette do
      densities[mon] = density_for_mon(mon)

      total_density = total_density + densities[mon]
    end

gui.debugf("densities =  total:%1.3f\n%s\n\n", total_density, table.tostr(densities,1))

    -- convert density map to monster counts
    local wants = {}

    each mon,d in densities do
      if mon != "NONE" then
        local num = want_total * d / total_density

        wants[mon] = int(num + gui.random())
      end
    end

gui.debugf("wants =\n%s\n\n", table.tostr(wants))

    return wants
  end


  local function calc_horde_size(mon, info)
    local horde = 1

    if info.health <= 500 and rand.odds(30) then horde = horde + 1 end
    if info.health <= 100 then horde = horde + rand.index_by_probs { 90, 40, 10, 3, 0.5 } end

    if L.kind == "hallway" then horde = horde + 1 end
    
    return horde
  end


  local function fill_monster_map(palette, barrel_chance)
    -- check if any huge monsters
    local has_huge = false
    for mon,prob in pairs(palette) do
      if is_huge(mon) then has_huge = true end
    end

    -- break up really large monster spots, so that we get a better
    -- distribution of monsters.
    split_huge_spots((has_huge ? 288 ; 144))


    -- total number of monsters wanted
    local qty = calc_quantity()

    local want_total = tally_spots(L.mon_spots)

    want_total = int(want_total * qty / 100 + gui.random())

    -- determine how many of each kind of monster we want
    local wants = how_many_dudes(palette, want_total)


    -- add at least one monster of each kind, trying larger ones first
    for pass = 1,3 do
      for mon,qty in pairs(wants) do if qty >= 1 then
        if (pass == 1 and is_huge(mon)) or
           (pass == 2 and is_big(mon) and not is_huge(mon)) or
           (pass == 3 and not is_big(mon))
        then
          try_add_mon_group(mon, 1, true)

          wants[mon] = wants[mon] - 1

          -- extra one for very large rooms
          if (L.svolume or 0) > 70 then
            try_add_mon_group(mon, 1)
          end
        end
      end end -- mon, qty
    end


    -- try to add these monsters until we have the desired number or
    -- we have run out of monster spots.

    local pal2 = {}

    each mon,_ in palette do
      pal2[mon] = 50
    end


    while not table.empty(pal2) and not table.empty(L.mon_spots) do
      local mon = rand.key_by_probs(pal2)
      local info = GAME.MONSTERS[mon]

      if wants[mon] < 1 then
        pal2[mon] = nil
        continue
      end

      -- figure out how many to place together
      local horde = calc_horde_size(mon, info)

      horde = math.min(horde, wants[mon])

      local actual = try_add_mon_group(mon, horde)

      if actual > 0 and actual < wants[mon] then
        wants[mon] = wants[mon] - actual
      else
        pal2[mon] = nil
      end
    end
  end


  local function decide_cage_monster(enviro, spot, room_pal, used_mons)
    -- Note: this function is used for traps too

    -- FIXME: decide cage_palette EARLIER (before laying out the room)

    local list = {}

    local used_num = table.size(used_mons)
    if used_num > 4 then used_num = 4 end

    each mon,info in GAME.MONSTERS do
      local prob = prob_for_mon(mon, info, enviro)

      if STYLE.mon_variety == "none" and not LEVEL.global_pal[mon] then continue end

      if mon_fits(mon, spot) <= 0 then continue end

      -- prefer monsters not in the room palette
      if room_pal[mon] then prob = prob / 100 end

      -- prefer previously used monsters
      if used_mons[mon] then
        prob = prob * CAGE_REUSE_FACTORS[used_num]
      end

      if prob > 0 then
        list[mon] = prob
      end
    end

    -- Ouch : cage will be empty
    if table.empty(list) then return nil end

    return rand.key_by_probs(list)
  end


  local function fill_cage_area(mon, spot)
    local info = assert(GAME.MONSTERS[mon])

    -- determine maximum number that will fit
    local w, h = geom.box_size(spot.x1,spot.y1, spot.x2,spot.y2)

    w = int(w / info.r / 2.2)
    h = int(h / info.r / 2.2)

    assert(w >= 1 and h >= 1)

    local total = w * h

    -- generate list of coordinates to use
    local list = {}

    for mx = 1,w do for my = 1,h do
      local loc =
      {
        x = spot.x1 + info.r * 2.2 * (mx-0.5)
        y = spot.y1 + info.r * 2.2 * (my-0.5)
        z = spot.z1
      }
      table.insert(list, loc)
    end end

    rand.shuffle(list)

    -- determine quantity, applying user settings
    local qty = calc_quantity() * 1.4

    local d = info.cage_density or 1
    local f = gui.random()

    local want = int(total * d * qty / 100 + f * f * 2)
    want = math.clamp(1, want, total)

    gui.debugf("monsters_in_cage: %d (of %d) qty=%1.1f\n", want, total, qty)

    for i = 1,want do
      -- ensure first monster in present in all skills
      local all_skills = (i == 1)
      local loc = list[i]

      place_monster(mon, spot, loc.x, loc.y, loc.z, all_skills)
    end
  end


  local function fill_cages(enviro, spot_list, room_pal)
    if table.empty(L.cage_spots) then return end

    local qty = calc_quantity()

    local used_mons = {}

    each spot in spot_list do
      local mon = decide_cage_monster(enviro, spot, room_pal, used_mons)

      if mon then
        fill_cage_area(mon, spot, qty)

        used_mons[mon] = 1
      end
    end
  end


  local function add_monsters()
    local palette

    if OB_CONFIG.strength == "crazy" then
      palette = crazy_palette()
    else
      palette = room_palette()
    end

    local barrel_chance = (L.kind == "building" ? 15 ; 2)
--!!    if R.natural then barrel_chance = 3 end
--!!    if R.hallway then barrel_chance = 5 end

    if STYLE.barrels == "heaps" or rand.odds( 5) then barrel_chance = barrel_chance * 4 + 10 end
    if STYLE.barrels == "few"   or rand.odds(25) then barrel_chance = barrel_chance / 4 end

    if STYLE.barrels == "none" then barrel_chance = 0 end

    -- sometimes prevent monster replacements
    if rand.odds(40) or OB_CONFIG.strength == "crazy" then
      L.no_replacement = true
    end

    -- FIXME: add barrels even when no monsters in room

    if not table.empty(palette) then
      fill_monster_map(palette, barrel_chance)
    end

    -- this value keeps track of the number of "normal" monsters
    -- (i.e. monsters not in cages or traps).  Later we use this
    -- value to only give monster drops for accessible monsters.
    L.normal_count = #L.monster_list

    fill_cages("cage", L.cage_spots, palette)
    fill_cages("trap", L.trap_spots, palette)
  end


  local function make_empty_stats()
    local stats = {}

    for CL,_ in pairs(GAME.PLAYER_MODEL) do
      stats[CL] = {}
    end

    return stats
  end


  local function collect_weapons(hmodel)
    local list = {}

    for name,_ in pairs(hmodel.weapons) do
      local info = assert(GAME.WEAPONS[name])
      if info.pref then
        table.insert(list, info)
      end
    end

    if #list == 0 then
      error("No usable weapons???")
    end

    return list
  end


  local function give_monster_drops(hmodel, mon_list, count)
    for i = 1,count do
      local info = mon_list[i]

      if info.give then
        Player_give_stuff(hmodel, info.give)
      end
    end
  end


  local function user_adjust_result(stats)
    -- apply the user's health/ammo adjustments here

    local heal_mul = HEALTH_AMMO_ADJUSTS[OB_CONFIG.health]
    local ammo_mul = HEALTH_AMMO_ADJUSTS[OB_CONFIG.ammo]

    heal_mul = heal_mul * (PARAM.health_factor or 1)
    ammo_mul = ammo_mul * (PARAM.ammo_factor or 1)

    if OB_CONFIG.mode == "coop" then
      heal_mul = heal_mul * COOP_HEALTH_FACTOR
      ammo_mul = ammo_mul * COOP_AMMO_FACTOR
    end

    for name,qty in pairs(stats) do
      stats[name] = qty * (name == "health" ? heal_mul ; ammo_mul)
    end
  end


  local function subtract_stuff_we_have(stats, hmodel)
    for name,have_qty in pairs(hmodel.stats) do
      local need_qty = stats[name] or 0
      if have_qty > 0 and need_qty > 0 then
        local min_q = math.min(have_qty, need_qty)

               stats[name] =        stats[name] - min_q
        hmodel.stats[name] = hmodel.stats[name] - min_q
      end
    end
  end


  local function battle_for_class(CL, hmodel)
    local mon_list = L.monster_list

    local weap_list = collect_weapons(hmodel)

    local stats = L.fight_stats[CL]

    gui.debugf("Fight Simulator @ %s  class: %s\n", L:tostr(), CL)

    gui.debugf("weapons = \n")
    for _,info in ipairs(weap_list) do
      gui.debugf("  %s\n", info.name)
    end

    local weap_prefs = LEVEL.weap_prefs or THEME.weap_prefs or {}

    Fight_Simulator(mon_list, weap_list, weap_prefs, stats)

--  gui.debugf("raw result = \n%s\n", table.tostr(stats,1))

    user_adjust_result(stats)

--  gui.debugf("adjusted result = \n%s\n", table.tostr(stats,1))

    give_monster_drops(hmodel, mon_list, L.normal_count)

    subtract_stuff_we_have(stats, hmodel)
  end


  local function sim_battle()
    assert(L.monster_list)

    if #L.monster_list >= 1 then
      for CL,hmodel in pairs(LEVEL.hmodels) do
        battle_for_class(CL, hmodel)
      end
    end
  end


  local function should_add_monsters()
    if OB_CONFIG.mons == "none" then
      return false
    end

    if L.no_monsters then return false end

    assert(not L.scenic)

    if L.kind == "hallway" and #L.sections == 1 then
      return rand.odds(90)
    end

    return true
  end


  ---| Monsters_in_room |---

  gui.debugf("Monsters_in_room @ %s\n", L:tostr())

  L.monster_list = {}
  L.fight_stats  = make_empty_stats()

  L.big_item_spots = {} -- FIXME table.deep_copy(R.mon_spots)

---???  L.toughness = calc_toughness()

  L.firepower = Player_firepower()

  gui.debugf("Firepower = %1.3f\n", L.firepower)

  if should_add_monsters() then
    add_monsters()
    sim_battle()
  end
end


function Monsters_show_stats()
  local total = 0
  for _,count in pairs(LEVEL.mon_stats) do
    total = total + count
  end

  local function get_stat(mon)
    local num = LEVEL.mon_stats[mon] or 0
    local div = int(num * 99.8 / total)
    if div == 0 and num > 0 then div = 1 end
    return string.format("%02d", div)
  end

  if total == 0 then
    gui.debugf("STATS  no monsters at all\n")
  else
    gui.debugf("STATS  zsi:%s,%s,%s  crk:%s,%s,%s  mvb:%s,%s,%s  gap:%s,%s,%s\n",
               get_stat("zombie"), get_stat("shooter"), get_stat("imp"),
               get_stat("caco"), get_stat("revenant"), get_stat("knight"),
               get_stat("mancubus"), get_stat("vile"), get_stat("baron"),
               get_stat("gunner"), get_stat("arach"), get_stat("pain"))
  end
end


function Monsters_make_battles()
  
  gui.printf("\n--==| Make Battles |==--\n\n")

  gui.prog_step("Mons")

  Player_init()

  Monsters_init()
  Monsters_global_palette()

  Levels_invoke_hook("make_battles", LEVEL.seed)

  -- Rooms have been sorted into a visitation order, so we
  -- simply visit them one-by-one and insert some monsters
  -- and simulate each battle.

  Player_give_map_stuff()

  each R in LEVEL.rooms do
    Player_give_room_stuff(R)
    Monsters_in_room(R)
  end

  each H in LEVEL.halls do
    Monsters_in_room(H)
  end

  Monsters_show_stats()

  -- Once all monsters have been chosen and all battles
  -- (including cages and traps) have been simulated, then
  -- we can decide what pickups to add (the easy part) and
  -- _where_ to place them (the hard part).

  Monsters_distribute_stats()
  Monsters_do_pickups()
end

