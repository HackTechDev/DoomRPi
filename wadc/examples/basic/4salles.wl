#"standard.h"

main {
  thing		-- Position 0,0
  movestep(-32,-32)

  room0()
  room1()
  room2()
  room3()
}


room0() {

  wall({ "ASHWALL6"  })
  floor({ "FLOOR4_5"  })
  ceil({ "SLIME14"  })

  box({ 16 }, 		-- floor
      { 128  },      	-- ceiling
      { 192 },  	-- light
      {64},    		-- Taille y
      { 256 })      	-- Taille x

  move({64}) -- Deplacement vers +y

  step(0,{ 64 }) -- translation -/+ x pour le prochain point

}


room1() {

  wall({ "ASHWALL6"  })
  floor({ "FLOOR4_5"  })
  ceil({ "SLIME14"  })

  box({ 16 },   	-- floor
      { 128  },      	-- ceiling
      { 192 }, 		-- light
      { 64 },         	-- Taille y
      { 160 })      	-- Taille x

  move({64})  -- Deplacement vers +y

  step(0,{ -128 }) -- translation -/+ x pour le prochain point

}


room2() {

  wall({ "ASHWALL6"  })
  floor({ "FLOOR4_5"  })
  ceil({ "SLIME14"  })

  box({ 16 },   	-- floor
      { 128  }, 	-- ceiling
      { 192 },  	-- light
      { 64 },    		-- Taille y
      {  256 })      	-- Taille x

  move({64})

  step(0,{ 64 })  	-- translation -/+ x pour le prochain point

}

room3() {

  wall({ "ASHWALL6"  })
  floor({ "FLOOR4_5"  })
  ceil({ "SLIME14"  })

  box({ 16 },   	-- floor
      { 128  }, 	-- ceiling
      { 192 },  	-- light
      { 64 },    		-- Taille y
      {  128 })      	-- Taille x

  move({64})

  step(0,{ 32 })	-- translation -/+ x pour le prochain point

}
