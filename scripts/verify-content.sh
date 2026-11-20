#!/usr/bin/env bash
# Verifica el content pack de Daily Steven (manifest + ROM + firmas) ANTES de empaquetar.
# Uso: scripts/verify-content.sh [carpeta-de-staging]
#   Sin argumento mira content/ (manifest) + web/game/ (ROM compilada).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGING="${1:-}"

PYTHON="${PYTHON:-python3}"
command -v "$PYTHON" >/dev/null 2>&1 || PYTHON=python

"$PYTHON" - "$ROOT" "$STAGING" <<'PY'
import gzip, hashlib, json, sys
from pathlib import Path

root = Path(sys.argv[1])
staging = Path(sys.argv[2]) if sys.argv[2] else None
manifest_path = (staging / "game.manifest.json") if staging else (root / "content" / "game.manifest.json")
if not manifest_path.is_file():
    raise SystemExit(f"Missing manifest: {manifest_path}")
m = json.loads(manifest_path.read_text(encoding="utf-8"))

required = ["id", "name", "contentVersion", "protocolId", "rom", "client", "compat"]
missing = [k for k in required if k not in m]
if missing:
    raise SystemExit(f"manifest missing keys: {missing}")
if m["id"] != "dailysteven":
    raise SystemExit(f'expected id "dailysteven", got {m["id"]!r}')
if m["protocolId"] != "dailymax-conf-v1+status-v1":
    raise SystemExit(f"unexpected protocolId {m['protocolId']!r}")
if m["protocolId"] not in m["compat"].get("runnerAdapters", []):
    raise SystemExit("compat.runnerAdapters must list the protocolId")
if m.get("archive") != {"format": "steven-expansion-v1", "symbols": "archive/symbols.json"}:
    raise SystemExit("Falta el archivo histórico de Daily Steven")
if m["client"].get("statsKey") != "hortelano:dailysteven:stats":
    raise SystemExit("client.statsKey must be hortelano:dailysteven:stats (estadisticas aisladas por juego)")

rom_rel = m["rom"]["path"]
magic = m["rom"]["configMagic"].encode()
status_magic = m["rom"].get("statusMagic", "").encode()
rom = (staging / rom_rel) if staging else (root / "web" / "game" / Path(rom_rel).name)
if not rom.is_file():
    raise SystemExit(f"ROM not found: {rom} (compila con scripts/build-wsl.ps1 o con el CI)")
raw = rom.read_bytes()
if raw[:2] != b"\x1f\x8b":
    raise SystemExit(f"{rom.name} no esta comprimida con gzip")
raw = gzip.decompress(raw)
symbols_file = (staging if staging else root / "web" / "game") / m["archive"]["symbols"]
symbols = json.loads(symbols_file.read_text(encoding="utf-8"))
if symbols.get("format") != m["archive"]["format"] or symbols.get("romSha256") != hashlib.sha256(raw).hexdigest():
    raise SystemExit("Los símbolos del archivo no corresponden a la ROM del pack")
for symbol in ("gPlayerParty", "gPlayerPartyCount", "gDailyMaxArchive"):
    if not 0x02000000 <= symbols.get(symbol, 0) < 0x02040000:
        raise SystemExit(f"Símbolo fuera de EWRAM: {symbol}")
if len(raw) < 16 * 1024 * 1024:
    raise SystemExit(f"la ROM descomprimida mide solo {len(raw)} bytes")
if raw.count(magic) != 1:
    raise SystemExit(f"config magic {magic!r}: se esperaba 1 aparicion y hay {raw.count(magic)}")
if status_magic and status_magic not in raw:
    print(f"NOTE status magic {status_magic.decode()} no esta incrustado en la ROM (es un marcador de RAM)")
title = m["client"].get("titleAsset")
if title:
    t = (staging / title) if staging else (root / "web" / "game" / Path(title).name)
    if not t.is_file():
        raise SystemExit(f"title asset not found: {t}")
if staging and not (staging / "licenses.txt").is_file():
    raise SystemExit("falta licenses.txt (creditos y licencias) en el pack")
print(f"OK id={m['id']} version={m['contentVersion']} protocol={m['protocolId']}")
print(f"OK config magic {magic.decode()} en {rom.name} ({len(raw)} bytes descomprimida)")
print(f"OK minLauncherVersion={m['compat'].get('minLauncherVersion')}")
PY
