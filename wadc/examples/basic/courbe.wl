#"standard.h"

main {
  
  movestep(64,64)
  quad(
    curve(128,256,10,1)
    --curve(128,64,10,1)
  )
  rightsector(0,128,128)
  movestep(0, 128) 
  thing
}
