"""Search GBA-encoded Spanish/English fragments inside the ROM."""
from __future__ import annotations

import gzip
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CHARMAP = ROOT / "pokeemerald-expansion/charmap.txt"
ROMS = [
    ROOT / "web/game/dailymaximo.gba.gz",
    ROOT / "pokeemerald-expansion/pokefirered.gba",
    ROOT.parent / "hortelanos-daily/games/dailysteven/rom/dailymaximo.gba.gz",
]


def load_charmap() -> dict[str, int]:
    """Map single ASCII-ish chars and common glyphs to bytes (simplified)."""
    cmap: dict[str, bytes] = {}
    text = CHARMAP.read_text(encoding="utf-8", errors="replace")
    # Lines like: A = BB
    for m in re.finditer(r"^(.+?)\s*=\s*([0-9A-Fa-f]{2})(?:\s|$)", text, re.M):
        key = m.group(1).strip()
        val = bytes([int(m.group(2), 16)])
        if len(key) == 1:
            cmap[key] = val
        elif key.startswith("'") and key.endswith("'") and len(key) == 3:
            cmap[key[1]] = val
    # Digraphs / named
    for m in re.finditer(r"^(.+?)\s*=\s*((?:[0-9A-Fa-f]{2}\s*)+)", text, re.M):
        key = m.group(1).strip()
        vals = bytes(int(x, 16) for x in m.group(2).split())
        if key.startswith("'") and key.endswith("'"):
            cmap[key[1:-1]] = vals
        else:
            cmap[key] = vals
    return cmap


def encode(s: str, cmap: dict[str, bytes]) -> bytes | None:
    out = bytearray()
    i = 0
    while i < len(s):
        # Try longest named match greedily for multi-byte (Á etc. as names)
        matched = None
        for n in range(min(8, len(s) - i), 0, -1):
            chunk = s[i : i + n]
            if chunk in cmap:
                matched = cmap[chunk]
                i += n
                break
        if matched is None:
            return None
        out.extend(matched)
    return bytes(out)


def read_rom(path: Path) -> bytes:
    raw = path.read_bytes()
    if raw[:2] == b"\x1f\x8b":
        raw = gzip.decompress(raw)
    return raw


def main() -> None:
    cmap = load_charmap()
    # Ensure basic letters
    for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ ":
        if ch not in cmap:
            print("missing char", ch)
    needles = [
        "DESTRUCTOR",
        "LUCHA",
        "MOCHILA",
        "PARTIDA NUEVA",
        "POUND",
        "FIGHT",
        "BAG",
        "NEW GAME",
        "HUIDA",
        "RUN",
        "FUERTE",
        "HARDY",
    ]
    encoded = {}
    for n in needles:
        b = encode(n, cmap)
        encoded[n] = b
        print(f"encode {n!r} -> {b.hex() if b else None}")

    for path in ROMS:
        if not path.exists():
            print("MISSING", path)
            continue
        rom = read_rom(path)
        print("==", path.name, len(rom))
        for n, b in encoded.items():
            if not b:
                continue
            print(f"  {n}: {rom.find(b)}")


if __name__ == "__main__":
    main()
