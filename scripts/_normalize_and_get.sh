#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
git config core.autocrlf false
rm -rf pokeemerald-expansion
PY="$(command -v python3 || true)"
if [[ -z "$PY" || "$PY" == *WindowsApps* ]]; then
  for c in \
    "/c/Users/Unai/AppData/Local/Programs/Python/Python313/python.exe" \
    "/c/Users/Unai/AppData/Local/Programs/Python/Python311/python.exe"
  do
    [ -x "$c" ] && PY="$c" && break
  done
fi
"$PY" - <<'PY'
from pathlib import Path
p = Path('patches/hall-of-legends.patch')
data = p.read_bytes().replace(b'\r\n', b'\n')
p.write_bytes(data)
print('normalized patch bytes', len(data))
PY
# Prefer real CPython for apply scripts too
export PATH="$(dirname "$PY"):$PATH"
bash scripts/get-expansion.sh
