#"standard.h"
#"quaketextures.h"

main() {
  thing
  movestep(-128,256)
  -- Ligne 1 
  box1   
  -- Ligne 2
  movestep(-1536,128)
  box2
  movestep(512,0)
  box3
  movestep(512,0)
  box4
  movestep(512,0)
  box5
  -- Ligne 3
  movestep(0,256)
  box6
  -- Ligne 4
  movestep(-1280,128)
  box7
  movestep(512,0)
  box8
  movestep(769,0)
  box9
  -- Ligne 5
  movestep(0,256)
  box10
  -- Ligne 6
  movestep(-1024,128)
  box11
  movestep(1024,0)
  box12
  -- Ligne 7
  movestep(0,256)
  box13  
}

box13 {
  left(256)
  left(896)
  straight(128)
  straight(896)
  left(256)
  left(1920)
  leftsector(0, 128, 128)
}

box12 {
  left(128)
  left(896)
  left(128)
  left(896)
  leftsector(0, 128, 128)
}

box11 {
  left(128)
  left(896)
  left(128)
  left(896)
  leftsector(0, 128, 128)
}

box10 {
  left(256)
  left(640)
  straight(128)
  straight(384)
  straight(128)
  straight(640)
  left(256)
  left(896)
  straight(128)
  straight(896)
  leftsector(0, 128, 128)
}

box9{
  left(128)
  left(640)
  left(128)
  left(640)
  leftsector(0, 128, 128)
}

box8 {
  left(128)
  left(384)
  left(128)
  left(384)
  leftsector(0, 128, 128)
}
box7{
  left(128)
  left(640)
  left(128)
  left(640)
  leftsector(0, 128, 128)
}

box6{
  left(256)
  left(384)
  straight(128)
  straight(384)
  straight(128)
  straight(384)
  straight(128)
  straight(384)
  left(256)
  left(640)
  straight(128)
  straight(384)
  straight(128)
  straight(640)
  leftsector(0, 128, 128)
}

box5{
  left(128)
  left(384)
  left(128)
  left(384)
  leftsector(0, 128, 128)
}


box4{
  left(128)
  left(384)
  left(128)
  left(384)
  leftsector(0, 128, 128)
}


box3{
  left(128)
  left(384)
  left(128)
  left(384)
  leftsector(0, 128, 128)
}

box2 {
  left(128)
  left(384)
  left(128)
  left(384)
  leftsector(0, 128, 128)
}

box1 {
  straight(256)
  left(1920)
  left(256)
  left(384)
  straight(128)
  straight(384)
  straight(128)
  straight(384)
  straight(128)
  straight(384)
  leftsector(0, 128, 128)
}

