#!/usr/bin/env bash
# Recreates pokeemerald-expansion/ from scratch: fetches RHH's
# pokeemerald-expansion at the pinned commit, applies Hall of Legends, then
# Daily Máximo overlay sources and patches.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME_DIR="$ROOT/pokeemerald-expansion"
PATCH="$ROOT/patches/hall-of-legends.patch"
RHH_URL="https://github.com/rh-hideout/pokeemerald-expansion.git"
RHH_COMMIT="8fb02c6f2b81271e795d3f6aca61733fec739016"

if [ -e "$GAME_DIR" ]; then
  echo "pokeemerald-expansion/ already exists. Delete it first to start from a fresh copy."
  exit 1
fi

# Keep LF even on Windows checkouts so git apply and make don't choke.
git -C "$ROOT" config core.autocrlf false || true

git init -q "$GAME_DIR"
git -C "$GAME_DIR" remote add RHH "$RHH_URL"
git -C "$GAME_DIR" config core.autocrlf false
echo ">> Fetching pokeemerald-expansion @ ${RHH_COMMIT:0:10}"
git -C "$GAME_DIR" fetch -q --depth 1 RHH "$RHH_COMMIT"
git -C "$GAME_DIR" checkout -q FETCH_HEAD

PY=python3
if ! command -v python3 >/dev/null 2>&1 || [[ "$(command -v python3)" == *WindowsApps* ]]; then
  for c in \
    "python"
  do
    if command -v "$c" >/dev/null 2>&1 || [ -x "$c" ]; then PY="$c"; break; fi
  done
fi

TMP_PATCH="$(mktemp)"
# CRLF patches from Windows checkouts break git apply; normalize + binary headers.
"$PY" "$ROOT/scripts/normalize-binary-patch.py" < "$PATCH" | tr -d '\r' > "$TMP_PATCH"
echo ">> Applying $(basename "$PATCH")"
git -C "$GAME_DIR" apply --whitespace=nowarn "$TMP_PATCH"
rm -f "$TMP_PATCH"

# Castellano (ES-ES) string tables from Acimut pret_europe overlays (Daily Gold style).
if [[ ! -f "$ROOT/locale/es/SOURCE.json" ]]; then
  echo ">> Generating locale/es from pret_europe"
  "$PY" "$ROOT/scripts/fetch-spanish-locale.py"
fi

# Overlay + traducción + ganchos, en ese orden (lo hace apply-dailymaximo.sh).
echo ">> Applying Daily Steven overlay (overlay -> Spanish locale -> hooks)"
PYTHON="$PY" "$ROOT/scripts/apply-dailymaximo.sh"

[ -n "${HOL_PLAY:-}" ] || echo "Done. Build with: scripts/build.sh"
