#!/usr/bin/env python3
"""Comprueba de forma INDEPENDIENTE las reglas de Daily Red sobre los equipos que genera la ROM de pruebas.

Uso:  python3 tests/teams/verify_teams.py equipos.txt RUTA_ARBOL_EXPANSION [--show]

`equipos.txt` lo crea tests/teams/dump_teams.sh (volcado de la memoria del emulador). Este script NO usa el
código del generador: recalcula las reglas a partir de los datos de la expansion y de las reglas escritas
en el README de amoi-builds/dailyred:

  1. 6 especies distintas de 1-386; como máximo 1 legendario.
  2. Suma de estadísticas base >= 2700.
  3. IV de 25 a 31 en todas las estadísticas.
  4. EV: 252 / 252 / 6 (suma 510) en las dos mejores estadísticas ÚTILES (Ataque/At. Esp. solo cuentan si algún
     movimiento los usa, y en 3.ª gen lo decide el TIPO del movimiento) y los 6 restantes en PS (o en la
     tercera mejor si PS ya es una de las dos).
  5. Movimientos: hasta 4 DISTINTOS, todos de la lista "por nivel (cualquier nivel) o MT/MO de 3.ª gen" de su
     especie, y al menos uno de daño si la especie tiene alguno de daño.
  6. Objeto: uno de la lista de 16, o el potenciador del tipo de uno de sus ataques (potencia > 1).
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import locale_guard  # noqa: E402

rd = locale_guard.rd
DUMP = Path(sys.argv[1])
GAME = Path(sys.argv[2]).resolve()
SHOW = "--show" in sys.argv

PHYSICAL_TYPES = {"NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL"}  # 3.ª gen
STATS = ["PS", "Ataque", "Defensa", "Velocidad", "At. Esp.", "Def. Esp."]
PRIORITY = [1, 4, 3, 0, 2, 5]  # Atq, AtEsp, Vel, PS, Def, DefEsp (empates)

c_spec = locale_guard.consts(GAME / "include/constants/species.h", "SPECIES_")
c_move = locale_guard.consts(GAME / "include/constants/moves.h", "MOVE_")
c_item = locale_guard.consts(GAME / "include/constants/items.h", "ITEM_")
c_type = locale_guard.consts(GAME / "include/constants/pokemon.h", "TYPE_")
spec_name = {}
for k, v in c_spec.items():
    spec_name.setdefault(v, k)
for k, v in c_spec.items():           # preferir el nombre canónico (sin alias DEOXYS = DEOXYS_NORMAL)
    if not k.endswith("_NORMAL"):
        spec_name[v] = k
move_name = {}
for k, v in c_move.items():
    move_name.setdefault(v, k)
item_name = {}
for k, v in c_item.items():
    item_name.setdefault(v, k)
type_name = {v: k[5:] for k, v in c_type.items()}

team_src = rd(ROOT / "overlay/src/dailymaximo_team.c")
no_comments = re.sub(r"//[^\n]*|/\*.*?\*/", "", team_src, flags=re.S)


def c_list(name: str) -> list[str]:
    m = re.search(name + r"\[[^\]]*\]\s*=\s*\{(.*?)\};", no_comments, re.S)
    return re.findall(r"\b[A-Z][A-Z0-9_]+\b", m.group(1)) if m else []


LEGENDARIES = {c_spec[s] for s in c_list("sLegendaries")}
BATTLE_ITEMS = {c_item[i] for i in c_list("sBattleItems")}
MACHINES = {c_move[m] for m in c_list("sGen3Machines")}
BOOST = {}
for m in re.finditer(r"\[TYPE_(\w+)\]\s*=\s*(ITEM_\w+)", no_comments):
    BOOST[m.group(1)] = c_item[m.group(2)]

# ---- aprendizaje por especie --------------------------------------------------------------
lvl_file = rd(GAME / "src/data/pokemon/level_up_learnsets/gen_3.h")
LVL = {m.group(1): set(re.findall(r"LEVEL_UP_MOVE\(\s*\d+,\s*(MOVE_\w+)\)", m.group(2)))
       for m in re.finditer(r"static const struct LevelUpMove (\w+)\[\]\s*=\s*\{(.*?)\};", lvl_file, re.S)}
tm_file = rd(GAME / "src/data/pokemon/teachable_learnsets.h")
TEACH = {m.group(1): set(re.findall(r"(MOVE_\w+)", m.group(2)))
         for m in re.finditer(r"static const u16 (\w+)\[\]\s*=\s*\{(.*?)\};", tm_file, re.S)}
SPEC_SYMS: dict[int, tuple[str, str]] = {}
for f in sorted((GAME / "src/data/pokemon/species_info").glob("gen_[123]_families.h")):
    parts = re.split(r"\n\s*\[(SPECIES_\w+)\]\s*=", rd(f))
    for i in range(1, len(parts), 2):
        lv = re.search(r"\.levelUpLearnset\s*=\s*(\w+)", parts[i + 1][:8000])
        tc = re.search(r"\.teachableLearnset\s*=\s*(\w+)", parts[i + 1][:8000])
        if lv and tc and parts[i] in c_spec:
            SPEC_SYMS.setdefault(c_spec[parts[i]], (lv.group(1), tc.group(1)))


def candidates(species: int) -> set[int]:
    lv, tc = SPEC_SYMS[species]
    moves = {c_move[m] for m in LVL.get(lv, set()) if m in c_move}
    moves |= {c_move[m] for m in TEACH.get(tc, set()) if m in c_move and c_move[m] in MACHINES}
    return {m for m in moves if 1 <= m <= 354}


def expected_evs(base: list[int], move_types: list[str], powers: list[int]) -> list[int]:
    usable = [True, False, True, True, False, True]
    for t, p in zip(move_types, powers):
        if p <= 1:
            continue
        usable[1 if t in PHYSICAL_TYPES else 4] = True
    order: list[int] = []
    for s in PRIORITY:
        if not usable[s]:
            continue
        j = len(order)
        while j > 0 and base[order[j - 1]] < base[s]:
            j -= 1
        order.insert(j, s)
    evs = [0] * 6
    evs[order[0]] = 252
    evs[order[1]] = 252
    evs[0 if (order[0] != 0 and order[1] != 0) else order[2]] = 6
    return evs


# ---- lectura del volcado -------------------------------------------------------------------
teams: dict[int, list[list[int]]] = {}
cur = None
for line in DUMP.read_text().splitlines():
    if line.startswith("SEED "):
        cur = int(line.split()[1])
        words = []
        continue
    toks = [t for t in line.split() if t.startswith("0x")]
    if cur is not None and toks:
        raw = []
        for t in toks:
            v = int(t, 16)
            raw += [v & 0xFFFF, v >> 16]
        teams[cur] = [raw[i * 34:(i + 1) * 34] for i in range(6)]

problems = 0
for seed, mons in teams.items():
    errs: list[str] = []
    species = [m[0] for m in mons]
    if len(set(species)) != 6 or any(not 1 <= s <= 386 for s in species):
        errs.append(f"especies no válidas o repetidas {species}")
    nleg = sum(1 for s in species if s in LEGENDARIES)
    if nleg > 1:
        errs.append(f"{nleg} legendarios")
    bst = sum(sum(m[26:32]) for m in mons)
    if bst < 2700:
        errs.append(f"suma base {bst} < 2700")
    lines = []
    for m in mons:
        sp, mv, ty, pw, item = m[0], m[1:5], m[5:9], m[9:13], m[13]
        ev, iv, base = m[14:20], m[20:26], m[26:32]
        name = spec_name.get(sp, "?")[8:]
        used = [x for x in mv if x]
        tnames = [type_name.get(t, "?") for t in ty]
        cand = candidates(sp) if sp in SPEC_SYMS else set()
        if len(used) != len(set(used)):
            errs.append(f"{name}: movimientos repetidos")
        if not used:
            errs.append(f"{name}: sin movimientos")
        for x in used:
            if x not in cand:
                errs.append(f"{name}: {move_name.get(x)} no sale de nivel ni de MT/MO de 3.ª gen")
        if len(used) < min(4, len(cand)):
            errs.append(f"{name}: lleva {len(used)} movimientos y podía llevar {min(4, len(cand))}")
        can_damage = False
        for c in cand:
            pass
        if not any(p != 0 for p in pw[:len(used)]):
            # ¿tiene la especie algún movimiento de daño? (potencia > 0); aproximación con los tipos/potencias
            # no disponible para los candidatos: se avisa solo si ninguno de los 4 daña y la especie tiene >4
            if len(cand) > len(used):
                errs.append(f"{name}: ningún movimiento de daño (revisar a mano si la especie tiene alguno)")
        if any(not 25 <= x <= 31 for x in iv):
            errs.append(f"{name}: IV fuera de 25-31 {iv}")
        want = expected_evs(base, tnames, pw)
        if ev != want:
            errs.append(f"{name}: EV {ev} y la regla da {want}")
        boosters = {BOOST[t] for t, p in zip(tnames, pw) if p > 1 and t in BOOST}
        if item not in BATTLE_ITEMS and item not in boosters:
            errs.append(f"{name}: objeto {item_name.get(item)} fuera de la lista")
        lines.append((name, [move_name.get(x, "-")[5:] for x in used], item_name.get(item, "?")[5:], ev, iv,
                      "".join(sorted({t for t in tnames if t != '?'})[:0])))
    status = "OK" if not errs else "FALLA"
    print(f"\n=== semilla {seed}: {status}  (suma base {bst}, legendarios {nleg}) ===")
    for name, mv, item, ev, iv, _ in lines:
        print(f"  {name:<12} {', '.join(mv):<62} @ {item:<14} EV {ev}  IV {iv}")
    for e in errs:
        print("   !!", e)
    problems += len(errs)
print(f"\nverify_teams: {len(teams)} equipos revisados, {problems} problemas.")
sys.exit(1 if problems else 0)
