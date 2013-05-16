#!/bin/bash
set -e

if [ ! -d vpo_lib ]; then
  echo "Run this script from the top level."
  exit 1
fi

echo "Creating source package...."

cd ..

src=vis-explorer
dest=visexp-X.XX-source

mkdir $dest

#
#  Source code
#
mkdir $dest/src
cp -av $src/src/*.[chr]* $dest/src
cp -av $src/Makefile* $dest/

mkdir $dest/vpo_lib
cp -av $src/vpo_lib/*.[chr]* $dest/vpo_lib

mkdir $dest/misc
cp -av $src/misc/*.* $dest/misc
mkdir $dest/misc/debian
cp -av $src/misc/debian/* $dest/misc/debian

mkdir $dest/obj_linux
mkdir $dest/obj_win32

mkdir $dest/obj_linux/vpo_lib
mkdir $dest/obj_win32/vpo_lib

#
#  Data files
#

#
#  Documentation
#
cp -av $src/*.txt $dest

## mkdir $dest/docs
## cp -av $src/docs/*.* $dest/docs

#
# all done
#
echo "------------------------------------"
echo "All done."

