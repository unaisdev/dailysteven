#!/usr/bin/env python3
r"""Extrae de POKÉMON ESMERALDA (pret/pokeemerald) la compatibilidad con MT/MO y los movimientos por nivel de las
386 especies, y los convierte en la referencia y en la tabla que usa el generador de equipos del jugador.

  python3 tools/emerald_reference.py --emerald ~/refs/pokeemerald --game ~/dailysteven-build/pokeemerald-expansion
          [--write]            reescribe tests/reference/emerald_learnsets.json y overlay/include/dailymax_emerald.h
          [--report FICHERO]   guarda el informe de diferencias (Markdown)
          [--frlg RUTA]        (opcional) compara también con la tabla de Rojo Fuego de la ronda 6A

Por qué Esmeralda: los rivales y el juego se basan en Esmeralda, así que el jugador usa sus datos. La regla de Daily Red
no cambia (hasta 4 movimientos al azar entre nivel + MT/MO, sin tutores ni huevo); solo cambia la fuente.

Archivos que lee (pokeemerald): src/data/pokemon/tmhm_learnsets.h, level_up_learnsets.h, level_up_learnset_pointers.h,
include/constants/tms_hms.h. El commit se guarda en la referencia.
scripts/check-tmhm.py comprueba al compilar que overlay/include/dailymax_emerald.h coincide con la referencia.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN3_LAST = 386  # DEOXYS en la expansion


# Movimientos que la expansion llama distinto que pokeemerald (mismo movimiento)
MOVE_ALIASES = {"MOVE_FAINT_ATTACK": "MOVE_FEINT_ATTACK"}


def norm(name: str) -> str:
    """Compara nombres de movimientos ignorando guiones bajos (SMELLINGSALT / SMELLING_SALT)."""
    return name.replace("_", "")


def machines_emerald(em: Path) -> list[str]:
    """Los 58 movimientos de las MT01-MT50 y MO01-MO08, en orden (nombres MOVE_*)."""
    text = (em / "include/constants/tms_hms.h").read_text(encoding="utf-8")
    out: list[str] = []
    for macro in ("FOREACH_TM", "FOREACH_HM"):
        m = re.search(r"#define " + macro + r"\(F\)((?:\s*\\\s*F\(\w+\))+)", text)
        out += ["MOVE_" + x for x in re.findall(r"F\((\w+)\)", m.group(1))]
    assert len(out) == 58, f"se esperaban 58 MT/MO y hay {len(out)}"
    return out


def tmhm_emerald(em: Path) -> dict[str, list[str]]:
    text = (em / "src/data/pokemon/tmhm_learnsets.h").read_text(encoding="utf-8")
    out: dict[str, list[str]] = {}
    for m in re.finditer(r"\[(SPECIES_\w+)\]\s*=\s*\{\s*\.learnset\s*=\s*\{(.*?)\}\s*\}", text, re.S):
        out[m.group(1)] = ["MOVE_" + x for x in re.findall(r"\.(\w+)\s*=\s*TRUE", m.group(2))]
    return out


def levelup_emerald(em: Path) -> dict[str, list[list]]:
    lt = (em / "src/data/pokemon/level_up_learnsets.h").read_text(encoding="utf-8")
    sets: dict[str, list[list]] = {}
    for m in re.finditer(r"static const u16 (\w+)\[\] = \{(.*?)LEVEL_UP_END", lt, re.S):
        sets[m.group(1)] = [[int(a), b] for a, b in re.findall(r"LEVEL_UP_MOVE\(\s*(\d+),\s*(MOVE_\w+)\)", m.group(2))]
    pt = (em / "src/data/pokemon/level_up_learnset_pointers.h").read_text(encoding="utf-8")
    return {sp: sets[sym] for sp, sym in re.findall(r"\[(SPECIES_\w+)\]\s*=\s*(\w+)", pt) if sym in sets}


def game_species(game: Path) -> dict[str, int]:
    text = (game / "include/constants/species.h").read_text(encoding="utf-8")
    out: dict[str, int] = {}
    for name, val in re.findall(r"^\s*(SPECIES_\w+)\s*=\s*(\d+)\s*,", text, re.M):
        if 0 < int(val) <= GEN3_LAST:
            out[name] = int(val)
    for name, target in re.findall(r"^\s*(SPECIES_\w+)\s*=\s*(SPECIES_\w+)\s*,", text, re.M):
        if target in out and name not in out:
            out[name] = out[target]
    return out


def game_moves(game: Path) -> dict[str, str]:
    """norm(nombre) -> nombre MOVE_* de la expansion."""
    text = (game / "include/constants/moves.h").read_text(encoding="utf-8")
    out: dict[str, str] = {}
    for name in re.findall(r"^\s*(MOVE_\w+)\s*(?:=[^,\n]*)?,", text, re.M):
        out.setdefault(norm(name), name)
    return out


def blocks(game: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for f in sorted((game / "src/data/pokemon/species_info").glob("gen_*_families.h")):
        text = f.read_text(encoding="utf-8")
        for m in re.finditer(r"\n    \[(SPECIES_\w+)\] =\s*\n?\s*\{(.*?)(?=\n    \[SPECIES_|\n\};)", text, re.S):
            out[m.group(1)] = m.group(2)
    return out


def game_levelup(game: Path) -> dict[str, list[tuple[int, str]]]:
    text = (game / "src/data/pokemon/level_up_learnsets/gen_3.h").read_text(encoding="utf-8")
    sets = {m.group(1): [(int(a), b) for a, b in re.findall(r"LEVEL_UP_MOVE\(\s*(\d+),\s*(MOVE_\w+)\)", m.group(2))]
            for m in re.finditer(r"static const struct LevelUpMove (\w+)\[\] = \{(.*?)LEVEL_UP_END", text, re.S)}
    out = {}
    for sp, blk in blocks(game).items():
        m = re.search(r"\.levelUpLearnset\s*=\s*(\w+)", blk)
        if m and m.group(1) in sets:
            out[sp] = sets[m.group(1)]
    return out


def game_teachable(game: Path) -> dict[str, set[str]]:
    text = (game / "src/data/pokemon/teachable_learnsets.h").read_text(encoding="utf-8")
    sets = {m.group(1): set(re.findall(r"MOVE_\w+", m.group(2)))
            for m in re.finditer(r"static const u16 (\w+)\[\] = \{(.*?)MOVE_UNAVAILABLE", text, re.S)}
    out = {}
    for sp, blk in blocks(game).items():
        m = re.search(r"\.teachableLearnset\s*=\s*(\w+)", blk)
        if m and m.group(1) in sets:
            out[sp] = sets[m.group(1)]
    return out


def build(em: Path, game: Path):
    species = game_species(game)
    tm = tmhm_emerald(em)
    lv = levelup_emerald(em)
    by_idx: dict[int, str] = {}
    for s, i in species.items():  # un solo nombre por número (DEOXYS = DEOXYS_NORMAL): el que usa pokeemerald
        if i not in by_idx or s in tm:
            by_idx[i] = s
    species = {s: i for i, s in sorted(by_idx.items())}
    missing = [s for s in species if s not in tm or s not in lv]
    assert not missing, f"especies sin datos en pokeemerald: {missing[:5]}"
    gm = game_moves(game)
    machines = machines_emerald(em)

    def conv(mv: str) -> str:
        key = norm(MOVE_ALIASES.get(mv, mv))
        assert key in gm, f"el movimiento {mv} de pokeemerald no existe en la expansion"
        return gm[key]

    machines = [conv(m) for m in machines]
    ref = {
        "fuente": "pret/pokeemerald: src/data/pokemon/tmhm_learnsets.h, level_up_learnsets.h, level_up_learnset_pointers.h",
        "commit": subprocess.run(["git", "-C", str(em), "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip(),
        "maquinas": machines,
        "mt": {s: [conv(m) for m in tm[s]] for s in species},
        "nivel": {s: [[lvl, conv(m)] for lvl, m in lv[s]] for s in species},
    }
    return ref, species


def write_header(ref: dict, species: dict[str, int]) -> None:
    machines = ref["maquinas"]
    out = ["// GENERADO por tools/emerald_reference.py (no editar a mano). Datos de POKÉMON ESMERALDA (pret/pokeemerald @ "
           + ref["commit"][:7] + ")",
           "// para las especies 1-386: compatibilidad con MT/MO y movimientos por nivel. Solo los usa el generador de equipos",
           "// del jugador (overlay/src/dailymaximo_team.c). scripts/check-tmhm.py comprueba al compilar que coincide con",
           "// tests/reference/emerald_learnsets.json.", "",
           "// Bit i = máquina i de sGen3Machines (MT01-MT50, MO01-MO08)",
           "static const u32 sEmeraldMachineMask[SPECIES_DEOXYS + 1][2] =", "{"]
    for sp in species:
        mask = 0
        for mv in ref["mt"][sp]:
            mask |= 1 << machines.index(mv)
        out.append(f"    [{sp}] = {{ 0x{mask & 0xFFFFFFFF:08X}, 0x{mask >> 32:08X} }},")
    out += ["};", "", "// Movimientos por nivel (cualquier nivel), todos seguidos; el de cada especie va de",
            "// sEmeraldLevelUpStart[especie] a sEmeraldLevelUpStart[especie + 1] (sin el nivel: el generador no lo usa)."]
    flat: list[str] = []
    start = [0] * (GEN3_LAST + 2)
    by_idx = {i: s for s, i in species.items()}
    for i in range(GEN3_LAST + 1):
        start[i] = len(flat)
        if i in by_idx:
            flat += [mv for _, mv in ref["nivel"][by_idx[i]]]
    start[GEN3_LAST + 1] = len(flat)
    out.append("static const u16 sEmeraldLevelUpStart[SPECIES_DEOXYS + 2] =")
    out.append("{")
    for i in range(0, len(start), 12):
        out.append("    " + ", ".join(str(x) for x in start[i:i + 12]) + ",")
    out.append("};")
    out.append("")
    out.append("static const u16 sEmeraldLevelUpMoves[] =")
    out.append("{")
    for i in range(0, len(flat), 4):
        out.append("    " + ", ".join(flat[i:i + 4]) + ",")
    out.append("};")
    (ROOT / "overlay/include/dailymax_emerald.h").write_text("\n".join(out) + "\n", encoding="utf-8")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emerald", required=True)
    ap.add_argument("--game", required=True)
    ap.add_argument("--frlg")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--report")
    a = ap.parse_args()
    em, game = Path(a.emerald), Path(a.game)
    ref, species = build(em, game)
    machines = ref["maquinas"]
    order = sorted(species, key=species.get)

    # --- Esmeralda frente a la expansion moderna (MT/MO) y a la de 3.ª gen (por nivel) ---
    et = game_teachable(game)
    tm_diff = []
    for s in order:
        now = {m for m in machines if m in et.get(s, set())}
        want = set(ref["mt"][s])
        if now != want:
            tm_diff.append((s, sorted(want - now), sorted(now - want)))
    el = game_levelup(game)
    lv_diff = []
    for s in order:
        a_ = {(lvl, norm(m)) for lvl, m in ref["nivel"][s]}
        b_ = {(lvl, norm(m)) for lvl, m in el.get(s, [])}
        if a_ != b_:
            lv_diff.append((s, sorted(a_ - b_), sorted(b_ - a_)))
    lv_diff_moves = [s for s in order if {norm(m) for _, m in ref["nivel"][s]} != {norm(m) for _, m in el.get(s, [])}]

    lines = ["# MT/MO y movimientos por nivel del generador del jugador: fuente Esmeralda", "",
             f"- Fuente: pret/pokeemerald @ `{ref['commit']}`.",
             f"- Especies 1-386 comparadas: {len(species)}.",
             f"- **MT/MO**: especies cuya compatibilidad de Esmeralda difiere de la expansion moderna: **{len(tm_diff)}**.",
             f"- **Por nivel**: especies cuyas listas (nivel + movimiento) difieren entre Esmeralda y la expansion en GEN_3: "
             f"**{len(lv_diff)}** (de ellas, {len(lv_diff_moves)} con distinto CONJUNTO de movimientos; el resto solo cambia el nivel).",
             "- El generador usa SOLO Esmeralda para ambas cosas (tablas en `overlay/include/dailymax_emerald.h`); el resto del juego "
             "sigue con la expansion.", ""]
    if a.frlg:
        import importlib.util
        spec = importlib.util.spec_from_file_location("frlg_reference", ROOT / "tools/frlg_reference.py")
        fr_mod = None
        if (ROOT / "tools/frlg_reference.py").is_file():
            fr_mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(fr_mod)
        if fr_mod:
            frlg = Path(a.frlg)
            fnames = fr_mod.machine_names_frlg(frlg)
            ft = fr_mod.tmhm_frlg(frlg)
            fm = fr_mod.machine_moves_frlg(frlg)
            frlg_diff = []
            for s in order:
                f_set = {fm[fnames.index(n)] for n in ft.get(s, [])}
                if f_set != set(ref["mt"][s]):
                    frlg_diff.append((s, sorted(set(ref["mt"][s]) - f_set), sorted(f_set - set(ref["mt"][s]))))
            lines += [f"## Esmeralda frente a la tabla de Rojo Fuego de la ronda 6A: {len(frlg_diff)} especies cambian", "",
                      "Especie: MT que Esmeralda AÑADE | MT que Esmeralda QUITA respecto a Rojo Fuego", ""]
            for s, plus, minus in frlg_diff[:60]:
                lines.append(f"- {s[8:]}: +{', '.join(m[5:] for m in plus) or '-'} | -{', '.join(m[5:] for m in minus) or '-'}")
            lines.append("")
            print(f"RESUMEN esmeralda_vs_frlg: {len(frlg_diff)}")
            for s, plus, minus in frlg_diff[:8]:
                print("  EJ", s[8:], "+", [m[5:] for m in plus], "-", [m[5:] for m in minus])
    lines += ["## MT/MO: Esmeralda frente a la expansion moderna (primeras 30)", "",
              "Especie: MT que Esmeralda tiene y la expansion no | MT que la expansion tiene y Esmeralda no", ""]
    for s, plus, minus in tm_diff[:30]:
        lines.append(f"- {s[8:]}: +{', '.join(m[5:] for m in plus) or '-'} | -{', '.join(m[5:] for m in minus) or '-'}")
    lines += ["", "## Por nivel: Esmeralda frente a la expansion GEN_3 (primeras 30)", ""]
    for s, plus, minus in lv_diff[:30]:
        lines.append(f"- {s[8:]}: solo Esmeralda {[(l, m[5:]) for l, m in plus][:5]} | solo expansion {[(l, m[5:]) for l, m in minus][:5]}")
    report = "\n".join(lines) + "\n"
    if a.report:
        Path(a.report).write_text(report, encoding="utf-8")
    print(f"RESUMEN: mt_difieren_expansion={len(tm_diff)} nivel_difieren_expansion={len(lv_diff)} "
          f"(conjunto distinto: {len(lv_diff_moves)})")

    if a.write:
        (ROOT / "tests/reference").mkdir(parents=True, exist_ok=True)
        (ROOT / "tests/reference/emerald_learnsets.json").write_text(
            json.dumps(ref, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
        write_header(ref, species)
        print("escritos tests/reference/emerald_learnsets.json y overlay/include/dailymax_emerald.h")


if __name__ == "__main__":
    main()
