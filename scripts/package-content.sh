#!/usr/bin/env bash
# Empaqueta el content pack de Daily Steven para Hortelano's Daily / GitHub Releases.
#
# Salida (dist/content/):
#   dailysteven-content.zip           <- nombre que espera el catalogo del lanzador
#   dailysteven-content-vX.Y.Z.zip    <- copia versionada
#   SHA256SUMS                        <- sumas de comprobacion
#
# Uso: [VERSION=0.1.0] scripts/package-content.sh
#   VERSION (sin prefijo) sustituye contentVersion SOLO dentro del zip; no se toca content/game.manifest.json.
#   Necesita la ROM compilada en web/game/dailymaximo.gba.gz y el sprite web/game/maximo.png.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PYTHON="${PYTHON:-python3}"
command -v "$PYTHON" >/dev/null 2>&1 || PYTHON=python

ROM_SRC="$ROOT/web/game/dailymaximo.gba.gz"
TITLE_SRC="$ROOT/web/game/maximo.png"
[ -f "$TITLE_SRC" ] || TITLE_SRC="$ROOT/content/assets/maximo.png"
OUT_DIR="$ROOT/dist/content"
STAGING="$OUT_DIR/staging"

[ -f "$ROM_SRC" ] || { echo "Falta la ROM $ROM_SRC: compila primero." >&2; exit 1; }
[ -f "$TITLE_SRC" ] || { echo "Falta el sprite del titulo (web/game/maximo.png o content/assets/maximo.png)." >&2; exit 1; }

"$ROOT/scripts/verify-content.sh"

rm -rf "$STAGING"
mkdir -p "$STAGING/rom" "$STAGING/assets"
cp "$ROM_SRC" "$STAGING/rom/dailymaximo.gba.gz"
cp "$TITLE_SRC" "$STAGING/assets/maximo.png"
cp "$ROOT/content/licenses.txt" "$STAGING/licenses.txt"
"$PYTHON" - "$ROOT/content/game.manifest.json" "$STAGING/game.manifest.json" "${VERSION:-}" <<'PY'
import json, sys
src, dst, ver = sys.argv[1:4]
m = json.load(open(src, encoding="utf-8"))
if ver:
    m["contentVersion"] = ver.lstrip("v")
open(dst, "w", encoding="utf-8", newline="\n").write(json.dumps(m, indent=2, ensure_ascii=False) + "\n")
print("contentVersion en el pack:", m["contentVersion"])
PY

"$ROOT/scripts/verify-content.sh" "$STAGING"

ID=dailysteven
VER=$("$PYTHON" -c "import json,sys; print(json.load(open(sys.argv[1], encoding='utf-8'))['contentVersion'])" "$STAGING/game.manifest.json")
"$PYTHON" - "$STAGING" "$OUT_DIR" "$ID" "$VER" <<'PY'
import hashlib, shutil, sys, zipfile
from pathlib import Path
staging, out, ident, ver = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3], sys.argv[4]
upload = out / f"{ident}-content.zip"
versioned = out / f"{ident}-content-v{ver}.zip"
for p in (upload, versioned, out / "SHA256SUMS", out / f"SHA256SUMS-{ident}.txt"):
    p.unlink(missing_ok=True)
# Zip reproducible: orden fijo, fecha fija, game.manifest.json en la raiz.
with zipfile.ZipFile(upload, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for f in sorted(staging.rglob("*")):
        if f.is_file():
            zi = zipfile.ZipInfo(f.relative_to(staging).as_posix(), date_time=(2026, 1, 1, 0, 0, 0))
            zi.compress_type = zipfile.ZIP_DEFLATED
            zi.external_attr = 0o644 << 16
            z.writestr(zi, f.read_bytes())
shutil.copyfile(upload, versioned)
lines = []
for p in (upload, versioned):
    lines.append(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}")
(out / "SHA256SUMS").write_text("\n".join(lines) + "\n", encoding="utf-8")
(out / f"SHA256SUMS-{ident}.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
print("\n".join(lines))
PY
echo "Pack listo en $OUT_DIR"
