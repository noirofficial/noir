#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/zoin_testnet.png
ICON_DST=../../src/qt/res/icons/zoin_testnet.ico
convert ${ICON_SRC} -resize 16x16 zoin_testnet-16.png
convert ${ICON_SRC} -resize 32x32 zoin_testnet-32.png
convert ${ICON_SRC} -resize 48x48 zoin_testnet-48.png
convert zoin_testnet-16.png zoin_testnet-32.png zoin_testnet-48.png ${ICON_DST}
