#!/usr/bin/env bash
# Copies Daily Máximo overlay sources into pokeemerald-expansion/ and patches
# Hall of Legends hooks (title → daily boot, skip BuildPlayerParty, cinematic).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/pokeemerald-expansion"

if [ ! -d "$GAME" ]; then
  echo "pokeemerald-expansion/ missing. Run scripts/get-expansion.sh first."
  exit 1
fi

cp -f "$ROOT/overlay/include/dailymaximo.h" "$GAME/include/dailymaximo.h"
mkdir -p "$GAME/include/constants"
cp -f "$ROOT/overlay/include/constants/dailymaximo.h" "$GAME/include/constants/dailymaximo.h"
cp -f "$ROOT/overlay/src/dailymaximo.c" "$GAME/src/dailymaximo.c"
cp -f "$ROOT/overlay/src/dailymaximo_team.c" "$GAME/src/dailymaximo_team.c"
mkdir -p "$GAME/test/battle"
cp -f "$ROOT/overlay/test/gen3_surf_earthquake.c" "$GAME/test/battle/gen3_surf_earthquake.c"

PY="${PYTHON:-python3}"
"$PY" "$ROOT/scripts/patch-dailymaximo-hooks.py" "$GAME"

# Keep Spanish overlays in sync when re-applying on an existing tree.
if [[ -f "$ROOT/locale/es/SOURCE.json" ]]; then
  PYTHON="$PY" "$ROOT/scripts/apply-locale.sh"
fi

echo "Daily Máximo overlay applied."
