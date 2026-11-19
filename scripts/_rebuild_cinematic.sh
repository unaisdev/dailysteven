#!/usr/bin/env bash
set -euo pipefail
export MSYS_NO_PATHCONV=1
ROOT="C:/Users/Unai/Desktop/Proyectos/dailymaximo"
# Keep overlay copies in sync for headers/sources we own
cp -f "$ROOT/overlay/include/dailymaximo.h" "$ROOT/pokeemerald-expansion/include/dailymaximo.h"
cp -f "$ROOT/overlay/src/dailymaximo.c" "$ROOT/pokeemerald-expansion/src/dailymaximo.c"
cp -f "$ROOT/overlay/src/dailymaximo_team.c" "$ROOT/pokeemerald-expansion/src/dailymaximo_team.c"

# Re-apply Spanish locale (idempotent on already-patched symbols).
if [[ -f "$ROOT/locale/es/SOURCE.json" ]]; then
  PY=python3
  if ! command -v python3 >/dev/null 2>&1 || [[ "$(command -v python3)" == *WindowsApps* ]]; then
    PY=python
  fi
  "$PY" "$ROOT/scripts/apply_spanish_locale.py" "$ROOT/pokeemerald-expansion"
fi

docker run --rm -v "$ROOT:/src" -w /src/pokeemerald-expansion dailymaximo-build \
    bash -lc 'make BUILD=firered -j$(nproc)'

mkdir -p "$ROOT/web/game"
gzip -9 -n -c "$ROOT/pokeemerald-expansion/pokefirered.gba" > "$ROOT/web/game/dailymaximo.gba.gz"
ls -la "$ROOT/web/game/dailymaximo.gba.gz"
echo Rebuild done.
