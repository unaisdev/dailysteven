#!/usr/bin/env bash
# Overlay locale/es onto pokeemerald-expansion/ (single castellano ROM).
# Same idea as Daily Gold's apply-locale.sh, adapted for expansion layouts.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/pokeemerald-expansion"
LOCALE="$ROOT/locale/es"

if [[ ! -d "$GAME/src" ]]; then
  echo "Missing $GAME — run scripts/get-expansion.sh first." >&2
  exit 1
fi
if [[ ! -f "$LOCALE/SOURCE.json" ]]; then
  echo "Missing $LOCALE — run scripts/fetch-spanish-locale.py first." >&2
  exit 1
fi

PY="${PYTHON:-python3}"
if ! command -v "$PY" >/dev/null 2>&1 || [[ "$(command -v "$PY")" == *WindowsApps* ]]; then
  for c in \
    "python"
  do
    if command -v "$c" >/dev/null 2>&1 || [ -x "$c" ]; then PY="$c"; break; fi
  done
fi

"$PY" "$ROOT/scripts/apply_spanish_locale.py" "$GAME"
