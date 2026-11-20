#!/usr/bin/env python3
r"""Extrae de pokefirered (la base de Daily Red) la compatibilidad con MT/MO y los movimientos por nivel, y la compara
con la expansion.

  python3 tools/frlg_reference.py --frlg ~/dailyred/pokefirered --game ~/dailysteven-build/pokeemerald-expansion
          [--write]      reescribe tests/reference/frlg_tmhm.json y overlay/include/dailymax_tmhm.h
          [--report F]   guarda el informe de diferencias (Markdown)

La referencia es pret/pokefirered @ 037335f (el commit que fija Daily Red). La tabla se guarda en
tests/reference/frlg_tmhm.json (referencia) y se convierte en overlay/include/dailymax_tmhm.h (lo que usa el generador
de equipos del jugador). scripts/check-tmhm.py comprueba al compilar que ambos coinciden.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN3_LAST = 386  # DEOXYS


def norm(name: str) -> str:
    """Quita guiones bajos: pokefirered y la expansion no siempre escriben igual los nombres de movimientos."""
    return name.replace("_", "")


def machine_moves_frlg(frlg: Path) -> list[str]:
    text = (frlg / "src/data/party_menu.h").read_text(encoding="utf-8")
    m = re.search(r"static const u16 sTMHMMoves\[\] =\s*\{(.*?)\};", text, re.S)
    moves = re.findall(r"MOVE_\w+", m.group(1))
    assert len(moves) == 58, f"se esperaban 58 MT/MO y hay {len(moves)}"
    return moves


def machine_names_frlg(frlg: Path) -> list[str]:
    """TM01_FOCUS_PUNCH ... HM08_DIVE en el orden de los bits."""
    text = (frlg / "include/constants/items.h").read_text(encoding="utf-8")
    names = re.findall(r"#define ITEM_((?:TM|HM)\d\d_\w+)\s+ITEM_(?:TM|HM)\d\d\b", text)
    tm = sorted([n for n in names if n.startswith("TM")])
    hm = sorted([n for n in names if n.startswith("HM")])
    return tm + hm


def tmhm_frlg(frlg: Path) -> dict[str, list[str]]:
    text = (frlg / "src/data/pokemon/tmhm_learnsets.h").read_text(encoding="utf-8")
    out: dict[str, list[str]] = {}
    for m in re.finditer(r"\[(SPECIES_\w+)\]\s*=\s*TMHM_LEARNSET\(", text):
        depth, i = 1, m.end()
        while depth:
            depth += {"(": 1, ")": -1}.get(text[i], 0)
            i += 1
        out[m.group(1)] = re.findall(r"TMHM\((\w+)\)", text[m.end():i - 1])
    return out


def levelup_frlg(frlg: Path) -> dict[str, list[tuple[int, str]]]:
    lt = (frlg / "src/data/pokemon/level_up_learnsets.h").read_text(encoding="utf-8")
    sets: dict[str, list[tuple[int, str]]] = {}
    for m in re.finditer(r"static const u16 (\w+)\[\] = \{(.*?)LEVEL_UP_END", lt, re.S):
        sets[m.group(1)] = [(int(a), b) for a, b in re.findall(r"LEVEL_UP_MOVE\(\s*(\d+),\s*(MOVE_\w+)\)", m.group(2))]
    pt = (frlg / "src/data/pokemon/level_up_learnset_pointers.h").read_text(encoding="utf-8")
    out = {}
    for sp, sym in re.findall(r"\[(SPECIES_\w+)\]\s*=\s*(\w+)", pt):
        out[sp] = sets[sym]
    return out


def gen3_species(game: Path) -> dict[str, int]:
    text = (game / "include/constants/species.h").read_text(encoding="utf-8")
    out = {}
    for name, val in re.findall(r"^\s*(SPECIES_\w+)\s*=\s*(\d+)\s*,", text, re.M):
        if 0 < int(val) <= GEN3_LAST:
            out[name] = int(val)
    for name, target in re.findall(r"^\s*(SPECIES_\w+)\s*=\s*(SPECIES_\w+)\s*,", text, re.M):  # p. ej. DEOXYS = DEOXYS_NORMAL
        if target in out and name not in out:
            out[name] = out[target]
    return out


def exp_species_blocks(game: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for f in sorted((game / "src/data/pokemon/species_info").glob("gen_*_families.h")):
        text = f.read_text(encoding="utf-8")
        for m in re.finditer(r"\n    \[(SPECIES_\w+)\] =\s*\n?\s*\{(.*?)(?=\n    \[SPECIES_|\n\};)", text, re.S):
            out[m.group(1)] = m.group(2)
    return out


def exp_levelup(game: Path) -> dict[str, list[tuple[int, str]]]:
    sets: dict[str, list[tuple[int, str]]] = {}
    text = (game / "src/data/pokemon/level_up_learnsets/gen_3.h").read_text(encoding="utf-8")
    for m in re.finditer(r"static const struct LevelUpMove (\w+)\[\] = \{(.*?)LEVEL_UP_END", text, re.S):
        sets[m.group(1)] = [(int(a), b) for a, b in re.findall(r"LEVEL_UP_MOVE\(\s*(\d+),\s*(MOVE_\w+)\)", m.group(2))]
    out = {}
    for sp, blk in exp_species_blocks(game).items():
        m = re.search(r"\.levelUpLearnset\s*=\s*(\w+)", blk)
        if m and m.group(1) in sets:
            out[sp] = sets[m.group(1)]
    return out


def exp_teachable(game: Path) -> dict[str, set[str]]:
    text = (game / "src/data/pokemon/teachable_learnsets.h").read_text(encoding="utf-8")
    sets: dict[str, set[str]] = {}
    for m in re.finditer(r"static const u16 (\w+)\[\] = \{(.*?)MOVE_UNAVAILABLE", text, re.S):
        sets[m.group(1)] = set(re.findall(r"MOVE_\w+", m.group(2)))
    out = {}
    for sp, blk in exp_species_blocks(game).items():
        m = re.search(r"\.teachableLearnset\s*=\s*(\w+)", blk)
        if m and m.group(1) in sets:
            out[sp] = sets[m.group(1)]
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--frlg", required=True)
    ap.add_argument("--game", required=True)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--report")
    a = ap.parse_args()
    frlg, game = Path(a.frlg), Path(a.game)

    moves = machine_moves_frlg(frlg)
    names = machine_names_frlg(frlg)
    assert len(names) == 58, f"nombres de MT/MO: {len(names)}"
    for nm, mv in zip(names, moves):
        assert norm(nm.split("_", 1)[1]) == norm(mv[5:]), f"{nm} no coincide con {mv}"
    species = gen3_species(game)
    tm = tmhm_frlg(frlg)
    by_idx: dict[int, str] = {}
    for s, i in species.items():  # un solo nombre por número (DEOXYS y DEOXYS_NORMAL son el 386): el que usa pokefirered
        if i not in by_idx or s in tm:
            by_idx[i] = s
    species = {s: i for i, s in by_idx.items()}
    missing = [s for s in species if s not in tm]
    assert not missing, f"especies sin entrada en tmhm_learnsets.h de pokefirered: {missing[:5]}"
    table = {s: sorted(names.index(n) for n in tm[s]) for s in sorted(species, key=species.get)}

    commit = (frlg / ".git/HEAD").read_text().strip() if (frlg / ".git").is_dir() else ""
    ref = {
        "fuente": "pret/pokefirered @ 037335f4c725d7c9aecdac87066f2002b4bd7e14 (el que fija Daily Red): src/data/pokemon/tmhm_learnsets.h",
        "maquinas": moves,
        "especies": {s: [moves[i] for i in idx] for s, idx in table.items()},
    }

    # --- comparación con la expansion ---
    et = exp_teachable(game)
    diffs = []
    for s in sorted(species, key=species.get):
        now = {mv for mv in moves if mv in et.get(s, set())}
        want = set(ref["especies"][s])
        if now != want:
            diffs.append((s, sorted(want - now), sorted(now - want)))
    fl, el = levelup_frlg(frlg), exp_levelup(game)
    ldiffs = []
    for s in sorted(species, key=species.get):
        a_ = {(lv, norm(mv)) for lv, mv in fl.get(s, [])}
        b_ = {(lv, norm(mv)) for lv, mv in el.get(s, [])}
        if a_ != b_:
            ldiffs.append((s, sorted(a_ - b_), sorted(b_ - a_)))

    lines = ["# MT/MO y movimientos por nivel: Rojo Fuego (Daily Red) frente a la expansion", "",
             f"- Especies 1-386 comparadas: {len(species)}.",
             f"- Especies cuya compatibilidad con MT/MO difiere de Rojo Fuego en la expansion: **{len(diffs)}**.",
             f"- Especies cuyos movimientos por nivel difieren entre Rojo Fuego y la expansion (GEN_3): **{len(ldiffs)}**.", ""]
    lines += ["## MT/MO: ejemplos (especie: Rojo Fuego puede y la expansion no | la expansion puede y Rojo Fuego no)", ""]
    for s, only_frlg, only_exp in diffs[:40]:
        lines.append(f"- {s[8:]}: +{', '.join(m[5:] for m in only_frlg) or '-'} | -{', '.join(m[5:] for m in only_exp) or '-'}")
    lines += ["", "## Por nivel: especies que difieren (primeras 40)", ""]
    for s, only_frlg, only_exp in ldiffs[:40]:
        lines.append(f"- {s[8:]}: solo Rojo Fuego {only_frlg[:6]} | solo expansion {only_exp[:6]}")
    report = "\n".join(lines) + "\n"
    print(report[:3000])
    if a.report:
        Path(a.report).write_text(report, encoding="utf-8")
    print(f"RESUMEN: mt_difieren={len(diffs)} nivel_difieren={len(ldiffs)}")

    if a.write:
        (ROOT / "tests/reference").mkdir(parents=True, exist_ok=True)
        (ROOT / "tests/reference/frlg_tmhm.json").write_text(json.dumps(ref, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
        write_header(moves, table)
        print("escritos tests/reference/frlg_tmhm.json y overlay/include/dailymax_tmhm.h")


def write_header(moves: list[str], table: dict[str, list[int]]) -> None:
    out = ["// GENERADO por tools/frlg_reference.py (no editar a mano). Compatibilidad con MT/MO de ROJO FUEGO (pret/pokefirered",
           "// @ 037335f, la base de Daily Red) para las especies 1-386. Bit i = máquina i de sGen3Machines (MT01-MT50, MO01-MO08).",
           "// scripts/check-tmhm.py comprueba al compilar que coincide con tests/reference/frlg_tmhm.json.", "",
           "static const u32 sFrlgMachineMask[SPECIES_DEOXYS + 1][2] =", "{"]
    for sp, idx in table.items():
        mask = 0
        for i in idx:
            mask |= 1 << i
        out.append(f"    [{sp}] = {{ 0x{mask & 0xFFFFFFFF:08X}, 0x{mask >> 32:08X} }},")
    out.append("};")
    (ROOT / "overlay/include/dailymax_tmhm.h").write_text("\n".join(out) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
