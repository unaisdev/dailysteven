#!/usr/bin/env bash
set -euo pipefail
export MSYS_NO_PATHCONV=1
docker run --rm \
  -v "C:/Users/Unai/Desktop/Proyectos/dailymaximo:/src" \
  -w /src/pokeemerald-expansion \
  dailymaximo-build \
  bash -lc '
rm -f build/firered/src/agb_flash.o
make BUILD=firered build/firered/src/agb_flash.o -j1 V=1 2>&1 | tail -40
echo SIZE=$(wc -c < build/firered/src/agb_flash.o)
arm-none-eabi-nm build/firered/src/agb_flash.o | head -30
head -80 src/agb_flash.c
'
