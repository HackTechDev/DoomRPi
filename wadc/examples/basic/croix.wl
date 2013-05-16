#"standard.h"

main {
    thing		-- Position 0,0

    movestep(-64,-256)
    roomLeft()
    movestep(0,384)

    roomRight()
    movestep(-192,-192)

    roomBottom()
    movestep(384,0)
    roomTop()  
    movestep(-256,-64)
    roomCenter()
}

roomCenter() {
    wall({ "ASHWALL6"  })
    floor({ "FLOOR4_5"  })
    ceil({ "SLIME14"  })

    box({ 16 }, 		-- floor
    { 128  },      	    -- ceiling
    { 192 },  	        -- light
    { 256 },    		-- Taille y
    { 256 })      	    -- Taille x
}

roomLeft() {
    wall({ "ASHWALL6"  })
    floor({ "FLOOR4_5"  })
    ceil({ "SLIME14"  })

    box({ 16 },   	-- floor
    { 128  },      	-- ceiling
    { 192 }, 		-- light
    { 128 },        -- Taille y
    { 128 })      	-- Taille x
}

roomRight() {
    wall({ "ASHWALL6"  })
    floor({ "FLOOR4_5"  })
    ceil({ "SLIME14"  })

    box({ 16 },   	-- floor
    { 128  },      	-- ceiling
    { 192 }, 		-- light
    { 128 },       	-- Taille y
    { 128 })      	-- Taille x
}

roomBottom() {
    wall({ "ASHWALL6"  })
    floor({ "FLOOR4_5"  })
    ceil({ "SLIME14"  })
    box({ 16 },   	-- floor
    { 128  },      	-- ceiling
    { 192 }, 		-- light
    { 128 },       	-- Taille y
    { 128 })      	-- Taille x
}

roomTop() {
    wall({ "ASHWALL6"  })
    floor({ "FLOOR4_5"  })
    ceil({ "SLIME14"  })

    box({ 16 },   	-- floor
    { 128  },      	-- ceiling
    { 192 }, 		-- light
    { 128 },       	-- Taille y
    { 128 })      	-- Taille x
}
