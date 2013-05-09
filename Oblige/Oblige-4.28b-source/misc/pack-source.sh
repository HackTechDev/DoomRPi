#!/bin/bash
set -e

if [ ! -d glbsp_src ]; then
  echo "Run this script from the top level."
  exit 1
fi

echo "Creating the source package for Oblige..."

dest=~/PACK-SRC

mkdir $dest

#
#  Lua scripts
#
mkdir $dest/scripts
cp -av scripts/*.* $dest/scripts

mkdir $dest/games
cp -av games/*.* $dest/games

mkdir $dest/engines
cp -av engines/*.* $dest/engines

mkdir $dest/modules
cp -av modules/*.* $dest/modules

mkdir $dest/prefabs
cp -av prefabs/*.* $dest/prefabs

#
#  Source code
#
mkdir $dest/gui
cp -av gui/*.[chr]* $dest/gui
cp -av gui/*.ico $dest/gui
cp -av Makefile.* $dest/

mkdir $dest/lua_src
cp -av lua_src/*.[chr]* $dest/lua_src

mkdir $dest/glbsp_src
cp -av glbsp_src/*.[chr]* $dest/glbsp_src

mkdir $dest/tools
mkdir $dest/tools/qsavetex
cp -av tools/qsavetex/*.[ch]* $dest/tools/qsavetex
cp -av tools/qsavetex/Makefile* $dest/tools/qsavetex

mkdir $dest/misc
cp -av misc/pack*.sh $dest/misc

mkdir $dest/obj_linux
mkdir $dest/obj_linux/lua
mkdir $dest/obj_linux/glbsp

mkdir $dest/obj_win32
mkdir $dest/obj_win32/lua
mkdir $dest/obj_win32/glbsp

#
#  Data files
#
mkdir $dest/data
cp -av data/*.lmp $dest/data || true
cp -av data/*.wad $dest/data || true
cp -av data/*.pak $dest/data || true

mkdir $dest/data/doom1_boss
mkdir $dest/data/doom2_boss

cp -av data/doom1_boss/*.* $dest/data/doom1_boss
cp -av data/doom2_boss/*.* $dest/data/doom2_boss

#
#  Documentation
#
cp -av *.txt $dest

rm -f $dest/LOGS.txt
rm -f $dest/CONFIG.txt

mkdir $dest/doc
cp -av doc/*.* $dest/doc

#
# all done
#
echo "------------------------------------"
echo "All done."

