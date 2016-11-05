#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/zoin.png
ICON_DST=../../src/qt/res/icons/zoin.ico
convert ${ICON_SRC} -resize 16x16 zoin-16.png
convert ${ICON_SRC} -resize 32x32 zoin-32.png
convert ${ICON_SRC} -resize 48x48 zoin-48.png
convert zoin-16.png zoin-32.png zoin-48.png ${ICON_DST}
