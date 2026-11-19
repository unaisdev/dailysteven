#!/usr/bin/env bash
set -euo pipefail
export MSYS_NO_PATHCONV=1
ROOT="C:/Users/Unai/Desktop/Proyectos/dailymaximo"
docker run --rm -v "$ROOT:/src" -w /src/pokeemerald-expansion dailymaximo-build bash -lc '
set -e
# Ensure our sources are current
cp -f /src/overlay/src/dailymaximo.c src/dailymaximo.c
cp -f /src/overlay/src/dailymaximo_team.c src/dailymaximo_team.c
cp -f /src/overlay/include/dailymaximo.h include/dailymaximo.h
# Rebuild any empty/broken objects from the first broken compile wave
find build/firered/src -name "*.o" -size -1000c -print -delete | head -50 || true
# Mgba print support
ls -la src/isagbprint.c build/firered/src/isagbprint.o 2>/dev/null || true
make BUILD=firered -j$(nproc)
'
ls -la "$ROOT/web/game/dailymaximo.gba.gz" 2>/dev/null || ls -la /c/Users/Unai/Desktop/Proyectos/dailymaximo/web/game/ 2>/dev/null || true
