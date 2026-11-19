#!/usr/bin/env python3
"""Build locale/es/moves_full_es.json from PokeAPI (official ES names + flavor text)."""
from __future__ import annotations

import json
import re
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "locale" / "es" / "moves_full_es.json"
CACHE = ROOT / ".cache" / "pokeapi_moves"


def get_json(url: str) -> dict | None:
    CACHE.mkdir(parents=True, exist_ok=True)
    key = re.sub(r"[^a-z0-9]+", "_", url.lower())
    path = CACHE / f"{key}.json"
    if path.is_file():
        return json.loads(path.read_text(encoding="utf-8"))
    req = urllib.request.Request(url, headers={"User-Agent": "dailymaximo-locale/1.0"})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            data = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        if e.code == 404:
            print(f"  skip 404 {url}")
            return None
        raise
    path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
    time.sleep(0.05)
    return data


def slug_to_move_key(slug: str) -> str:
    """power-trick -> MOVE_POWER_TRICK; 10-000-000-volt-thunderbolt -> MOVE_..."""
    parts = slug.replace("'", "").split("-")
    return "MOVE_" + "_".join(p.upper() for p in parts if p)


def pick_es_name(names: list[dict]) -> str | None:
    for n in names:
        if n.get("language", {}).get("name") == "es":
            return n.get("name")
    return None


def pick_es_flavor(entries: list[dict]) -> str | None:
    # Prefer recent games; fall back to any Spanish entry.
    prefer = (
        "scarlet-violet",
        "sword-shield",
        "ultra-sun-ultra-moon",
        "sun-moon",
        "omega-ruby-alpha-sapphire",
        "x-y",
        "black-2-white-2",
        "black-white",
        "heartgold-soulsilver",
        "platinum",
        "diamond-pearl",
        "firered-leafgreen",
        "emerald",
        "ruby-sapphire",
    )
    by_game = {}
    for e in entries:
        if e.get("language", {}).get("name") != "es":
            continue
        game = e.get("version_group", {}).get("name", "")
        text = (e.get("flavor_text") or "").replace("\x0c", "\n").replace("\xad", "")
        text = re.sub(r"[ \t]+", " ", text)
        text = re.sub(r"\n+", "\n", text).strip()
        if text:
            by_game[game] = text
    for g in prefer:
        if g in by_game:
            return by_game[g]
    return next(iter(by_game.values()), None)


def main() -> None:
    listing = get_json("https://pokeapi.co/api/v2/move?limit=1000")
    results = listing.get("results", [])
    out: dict[str, dict[str, str]] = {}
    for i, item in enumerate(results, 1):
        slug = item["name"]
        detail = get_json(item["url"])
        if not detail:
            continue
        key = slug_to_move_key(slug)
        name = pick_es_name(detail.get("names", []))
        desc = pick_es_flavor(detail.get("flavor_text_entries", []))
        if name or desc:
            out[key] = {}
            if name:
                # FRLG battle UI uses caps-style move names.
                out[key]["name"] = name.upper()
            if desc:
                out[key]["description"] = desc
        if i % 50 == 0:
            print(f"  {i}/{len(results)} moves…")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(out, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT} ({len(out)} moves)")


if __name__ == "__main__":
    main()
