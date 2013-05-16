#!/bin/sh

cd ../jeu
./bsp ../wadc/examples/$1.wad
./doomlegacy -file tmp.wad -warp 1 -opengl -width 640 -width 480
