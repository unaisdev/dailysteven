#!/usr/bin/env bash
# Copia el overlay de Daily Steven (C, cabeceras, escena de Bruno) dentro de pokeemerald-expansion/
# y lo engancha en Hall of Legends.
#
# Orden (importante): 1) overlay, 2) traducción (locale/es), 3) ganchos de Daily Steven.
# Los ganchos buscan textos que ya están en castellano, por eso la traducción va antes.
#
#   scripts/apply-dailymaximo.sh                 # todo
#   scripts/apply-dailymaximo.sh --overlay-only  # solo copia los archivos del overlay (compilación rápida)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/pokeemerald-expansion"

if [ ! -d "$GAME" ]; then
  echo "pokeemerald-expansion/ missing. Run scripts/get-expansion.sh first."
  exit 1
fi

cp -f "$ROOT/overlay/include/dailymaximo.h" "$GAME/include/dailymaximo.h"
cp -f "$ROOT/overlay/include/dailymax_texts.h" "$GAME/include/dailymax_texts.h"
mkdir -p "$GAME/include/constants"
cp -f "$ROOT/overlay/include/constants/dailymaximo.h" "$GAME/include/constants/dailymaximo.h"
cp -f "$ROOT/overlay/src/dailymaximo.c" "$GAME/src/dailymaximo.c"
cp -f "$ROOT/overlay/src/dailymaximo_team.c" "$GAME/src/dailymaximo_team.c"
mkdir -p "$GAME/data/scripts"
cp -f "$ROOT/overlay/data/dailymax_bruno.inc" "$GAME/data/scripts/dailymax_bruno.inc"
mkdir -p "$GAME/test/battle"
cp -f "$ROOT/overlay/test/gen3_surf_earthquake.c" "$GAME/test/battle/gen3_surf_earthquake.c"

if [ "${1:-}" = "--overlay-only" ]; then
  echo "Daily Steven overlay files copied (overlay-only)."
  exit 0
fi

PY="${PYTHON:-python3}"

# 1) Traducción primero.
if [[ -f "$ROOT/locale/es/SOURCE.json" ]]; then
  PYTHON="$PY" "$ROOT/scripts/apply-locale.sh"
fi

# 2) Ganchos después.
"$PY" "$ROOT/scripts/patch-dailymaximo-hooks.py" "$GAME"

echo "Daily Steven overlay applied."
