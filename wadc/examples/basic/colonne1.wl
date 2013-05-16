#"standard.h"
#"quaketextures.h"

main() {
  thing
  movestep(-32,-32)
  box4
}

box4 {
  box1
  box1
  box1
  box1
  rotleft
  
}

box1 {
  straight(32)
  straight(64)
  straight(32)
  left(128)
  left(64)
  straight(64)
  left(32)
  straight(96)
  leftsector(0,128,128)

  right(16)
  rotleft
}
