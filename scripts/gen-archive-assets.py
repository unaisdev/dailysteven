"""Generate archive metadata from the expansion build used for the content pack."""

import ast
import gzip
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


root = Path(__file__).resolve().parents[1]
game = root / "pokeemerald-expansion"
web = root / "web/game"
raw = (game / "pokefirered.gba").read_bytes()
if raw != gzip.decompress((web / "dailymaximo.gba.gz").read_bytes()):
    raise SystemExit("El archivo comprimido no corresponde a la ROM recién compilada")

nm = shutil.which("arm-none-eabi-nm") or root / ".toolchain/arm-none-eabi/bin/arm-none-eabi-nm"
listing = subprocess.check_output([nm, "-S", game / "pokefirered.elf"], text=True)
symbols = {}
expected_sizes = {"gParties": 2400, "gPartiesCount": 4, "gDailyMaxArchive": 1288}
for line in listing.splitlines():
    fields = line.split()
    if len(fields) != 4 or fields[-1] not in expected_sizes:
        continue
    address, size, _, name = fields
    address, size = int(address, 16), int(size, 16)
    if size != expected_sizes[name] or not 0x02000000 <= address <= 0x02040000 - size:
        raise SystemExit(f"Layout de RAM inesperado para {name}: {line}")
    symbols[name] = address
if symbols.keys() != expected_sizes.keys():
    raise SystemExit(f"Faltan símbolos en el ELF: {expected_sizes.keys() - symbols.keys()}")

# Use the charmap from the same build. Several bytes have typographic aliases.
charmap = {}
for line in (game / "charmap.txt").read_text(encoding="utf-8").splitlines():
    match = re.match(r"^('(?:[^'\\]|\\.)+')\s*=\s*([0-9A-F]{2})(?:\s|$)", line)
    if match:
        charmap.setdefault(str(int(match[2], 16)), ast.literal_eval(match[1]))

archive = {
    "format": "steven-expansion-v1",
    "romSha256": hashlib.sha256(raw).hexdigest(),
    # The expansion exposes gParties and gPartiesCount; the player's index is 0,
    # enforced by the static assertion in dailymaximo.c.
    "gPlayerParty": symbols["gParties"],
    "gPlayerPartyCount": symbols["gPartiesCount"],
    "gDailyMaxArchive": symbols["gDailyMaxArchive"],
    "charmap": charmap,
}
(web / "archive").mkdir(exist_ok=True)
(web / "archive/symbols.json").write_text(
    json.dumps(archive, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n"
)
