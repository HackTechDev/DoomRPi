#!/bin/sh

yadex map01.wad
bsp map01.wad -o tmp.wad
chocolate-doom -file tmp.wad
