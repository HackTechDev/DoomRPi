#"standard.h"
#"quaketextures.h"

main() {
  thing
  movestep(-128,256)
  box1
  box2 
  movestep(-320,128)
  box3
  movestep(320,256)
  box4  
}

box4{
left(256)
left(192)
straight(128)
straight(192)
left(256)
left(512)
leftsector(0,128,128)
}

box3(){
 left(128)
 left(192)
 left(128)
 left(192)
 leftsector(0,128,128)
}

box2 {
  straight(128)
  right(192)
  right(128)
  right(192)
  rightsector(0, 128,128)
}

box1 {
  straight(256)
  left(512)
  left(256)
  left(192)
  straight(128)
  straight(192)
  leftsector(0,128,128)
  rotright

}

