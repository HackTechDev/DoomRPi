#!/bin/sh

cd ../jeu
./bsp ../wadc/examples/$1.wad
./remood -file tmp.wad -warp 1 
