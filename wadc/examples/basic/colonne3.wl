#"standard.h"
#"spawns.h"
#"monsters.h"
#"pickups.h"

main {

  
  
  straight(512)
  left(512)
  left(512)
  left(512)
  leftsector(8,264,128)

  movestep(-128,-128)
  ibox(264,264,0,64,64)
  popsector
  movestep(-320,0)
  ibox(264,264,0,64,64)

 

  movestep(128,-64)
  thing

}
