#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/pokeemerald-expansion"
mkdir -p "$ROOT/web/game" "$ROOT/content/assets"
gzip -9 -n -c "$GAME/pokefirered.gba" > "$ROOT/web/game/dailymaximo.gba.gz"
STEVEN="$GAME/graphics/trainers/front_pics/steven.png"
if [ -f "$STEVEN" ]; then
  cp -f "$STEVEN" "$ROOT/web/game/maximo.png"
  cp -f "$STEVEN" "$ROOT/content/assets/maximo.png"
fi
ls -la "$ROOT/web/game/"
echo "ROM bytes: $(wc -c < "$ROOT/web/game/dailymaximo.gba.gz")"
# Confirm config magic is in the ROM
python3 - <<'PY'
import gzip, pathlib
rom = gzip.open(pathlib.Path('web/game/dailymaximo.gba.gz'), 'rb').read()
magic = b'DAILYMAX-CONF-V1'
print('config magic offset', rom.find(magic))
print('status magic offset', rom.find(b'DAILYMAX-STATUS1'))
PY
