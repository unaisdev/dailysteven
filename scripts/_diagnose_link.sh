#!/usr/bin/env bash
set -euo pipefail
export MSYS_NO_PATHCONV=1
docker run --rm \
  -v "C:/Users/Unai/Desktop/Proyectos/dailymaximo:/src" \
  -w /src/pokeemerald-expansion \
  dailymaximo-build \
  bash -lc '
set -x
ls -la build/firered/src/agb_flash*.o || true
nm build/firered/src/agb_flash.o 2>/dev/null | grep -E "EraseFlash|ProgramFlash|ReadFlash" | head || true
grep -n MgbaPrintf include/gba/isagbprint.h include/config.h 2>/dev/null | head || true
grep -n "agb_flash\|libagbsyscall\|LDFLAGS\|OBJS" Makefile | head -40
ls libagbsyscall/*.a libagbsyscall/build 2>/dev/null || true
'
