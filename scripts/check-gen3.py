#!/usr/bin/env python3
"""Validador de 3.ª generación. Falla (código 1) si entra algo de 4.ª gen o posterior donde no debe.

Uso:  scripts/check-gen3.py RUTA_AL_ARBOL_DE_LA_EXPANSION

Comprueba:
  1. Configuración del motor: GEN_LATEST = GEN_3, y que ninguna opción de combate/objetos/Pokémon quede en una
     generación posterior a la 3.ª (salvo cosas del mapa del mundo, irrelevantes aquí). Las ayudas modernas
     (eficacia, objetivos, MOVE INFO, última Poké Ball, afecto, correr, reaprender movimientos, colores de
     naturaleza) tienen que estar apagadas.
  2. Equipos de los rivales (overlay/data/dailymax_rivals.party Y lo que quedó en el árbol): STEVEN 6
     (Aura 0-2 + Blasco 3-5), AURA/BLASCO 3; especie 1-386; movimientos 1-354; habilidad 1-77; objetos Gen3.
  3. Generador del jugador (overlay/src/dailymaximo_team.c): toda especie, movimiento u objeto que nombra es
     de 3.ª gen, y la lista de MT/MO tiene 50 + 8 movimientos distintos.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import locale_guard  # noqa: E402

ROOT = HERE.parent
GAME = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "pokeemerald-expansion"
ERRORS: list[str] = []
rd = locale_guard.rd


def fail(msg: str) -> None:
    ERRORS.append(msg)


def define_value(path: Path, name: str) -> str | None:
    m = re.search(rf"^#define\s+{name}\s+(\S+)", rd(path), re.M)
    return m.group(1) if m else None


def check_config() -> None:
    cfg = GAME / "include/config"
    if define_value(cfg / "general.h", "GEN_LATEST") != "GEN_3":
        fail(f"CONFIG GEN_LATEST vale {define_value(cfg / 'general.h', 'GEN_LATEST')} y debe ser GEN_3")
    expected = {
        "species_enabled.h": {"P_GEN_4_POKEMON": "FALSE", "P_GEN_4_CROSS_EVOS": "FALSE", "P_GEN_5_POKEMON": "FALSE",
                              "P_GEN_6_POKEMON": "FALSE", "P_GEN_7_POKEMON": "FALSE", "P_GEN_8_POKEMON": "FALSE",
                              "P_GEN_9_POKEMON": "FALSE", "P_MEGA_EVOLUTIONS": "FALSE", "P_REGIONAL_FORMS": "FALSE"},
        "battle.h": {"B_RUN_TRAINER_BATTLE": "FALSE", "B_AFFECTION_MECHANICS": "FALSE",
                     "B_SHOW_EFFECTIVENESS": "SHOW_EFFECTIVENESS_NEVER", "B_SHOW_TARGETS": "FALSE",
                     "B_SHOW_MOVE_DESCRIPTION": "FALSE", "B_LAST_USED_BALL": "FALSE",
                     "B_SHOW_CATEGORY_ICON": "FALSE", "B_TRAINERS_KNOCK_OFF_ITEMS": "FALSE",
                     "B_PHYSICAL_SPECIAL_SPLIT": "GEN_LATEST", "B_UPDATED_MOVE_DATA": "GEN_LATEST",
                     "B_UPDATED_TYPE_MATCHUPS": "GEN_LATEST", "B_CRIT_MULTIPLIER": "GEN_LATEST",
                     "B_SLEEP_TURNS": "GEN_LATEST", "B_PARALYSIS_SPEED": "GEN_LATEST", "B_STURDY": "GEN_LATEST",
                     "B_ABILITY_WEATHER": "GEN_LATEST", "B_FAINT_SWITCH_IN": "GEN_LATEST"},
        "summary_screen.h": {"P_SUMMARY_SCREEN_MOVE_RELEARNER": "FALSE", "P_SUMMARY_SCREEN_NATURE_COLORS": "FALSE",
                             "P_SUMMARY_SCREEN_RENAME": "FALSE"},
        "pokemon.h": {"P_UPDATED_TYPES": "GEN_LATEST", "P_UPDATED_STATS": "GEN_LATEST",
                      "P_UPDATED_ABILITIES": "GEN_LATEST", "P_LVL_UP_LEARNSETS": "GEN_LATEST"},
    }
    for fname, items in expected.items():
        for name, want in items.items():
            got = define_value(cfg / fname, name)
            if got != want:
                fail(f"CONFIG {fname}: {name} vale {got} y debe ser {want}")
    # Nada fijado a mano en una generación posterior (salvo el mapa del mundo: OW_*, y las definiciones GEN_*).
    for f in sorted(cfg.glob("*.h")):
        if f.name == "test.h":
            continue
        for m in re.finditer(r"^#define\s+(\w+)\s+(GEN_[4-9]\w*|GEN_CHAMPIONS)\b", rd(f), re.M):
            if m.group(1).startswith(("OW_", "GEN_")):
                continue
            fail(f"CONFIG {f.name}: {m.group(1)} está fijado en {m.group(2)} (debe ser GEN_3 o GEN_LATEST)")


def name_to_const(prefix: str, name: str) -> str:
    n = re.sub(r"\s*\([MF]\)\s*$", "", name.strip())
    n = re.sub(r"[^A-Za-z0-9]+", "_", n).strip("_").upper()
    return prefix + n


def parse_party(text: str) -> dict[str, list[dict]]:
    out: dict[str, list[dict]] = {}
    for m in re.finditer(r"=== (TRAINER_HOL_\w+) ===\n(.*?)(?=\n=== |\Z)", text, re.S):
        trainer, body = m.group(1), m.group(2)
        mons = []
        for chunk in re.split(r"\n\s*\n", body.strip())[1:]:
            lines = [l.rstrip() for l in chunk.strip().split("\n")]
            if not lines or lines[0].startswith(("Name:", "Class:")):
                continue
            mon = {"species": lines[0], "item": None, "ability": None, "moves": []}
            if " @ " in lines[0]:
                mon["species"], mon["item"] = [x.strip() for x in lines[0].split(" @ ", 1)]
            for l in lines[1:]:
                if l.startswith("Ability:"):
                    mon["ability"] = l.split(":", 1)[1].strip()
                elif l.startswith("- "):
                    mon["moves"].append(l[2:].strip())
            mons.append(mon)
        out[trainer] = mons
    return out


def check_party(label: str, text: str) -> None:
    c_spec = locale_guard.consts(GAME / "include/constants/species.h", "SPECIES_")
    c_move = locale_guard.consts(GAME / "include/constants/moves.h", "MOVE_")
    c_abil = locale_guard.consts(GAME / "include/constants/abilities.h", "ABILITY_")
    c_item = locale_guard.consts(GAME / "include/constants/items.h", "ITEM_")
    items_json = json.loads(rd(ROOT / "locale/es/src/data/items.json"))["items"]
    gen3_items = {c_item[i["itemId"]] for i in items_json if i["itemId"] in c_item}
    parties = parse_party(text)
    for trainer in ("TRAINER_HOL_STEVEN", "TRAINER_HOL_MAY", "TRAINER_HOL_WALLY"):
        mons = parties.get(trainer)
        if mons is None:
            fail(f"RIVALES ({label}): falta {trainer}")
            continue
        # STEVEN: 6 = trío matchup Aura (0-2) + trío matchup Blasco (3-5). Compañeros: exactamente 3.
        want = 6 if trainer == "TRAINER_HOL_STEVEN" else 3
        if len(mons) != want:
            fail(f"RIVALES ({label}) {trainer}: tiene {len(mons)} Pokémon y deben ser exactamente {want}")
        for mon in mons:
            sc = name_to_const("SPECIES_", mon["species"])
            if sc not in c_spec or not 1 <= c_spec[sc] <= 386:
                fail(f"RIVALES ({label}) {trainer}: especie «{mon['species']}» no es de 1.ª-3.ª gen ({sc})")
            if mon["ability"]:
                ac = name_to_const("ABILITY_", mon["ability"])
                if ac not in c_abil or not 1 <= c_abil[ac] <= 77:
                    fail(f"RIVALES ({label}) {trainer} {mon['species']}: habilidad «{mon['ability']}» no es de 3.ª gen")
            for mv in mon["moves"]:
                mc = name_to_const("MOVE_", mv)
                if mc not in c_move or not 1 <= c_move[mc] <= 354:
                    fail(f"RIVALES ({label}) {trainer} {mon['species']}: movimiento «{mv}» no es de 3.ª gen")
            if mon["item"]:
                ic = name_to_const("ITEM_", mon["item"])
                if ic not in c_item or c_item[ic] not in gen3_items:
                    fail(f"RIVALES ({label}) {trainer} {mon['species']}: objeto «{mon['item']}» no es de 3.ª gen")


def check_generator() -> None:
    src = rd(ROOT / "overlay/src/dailymaximo_team.c")
    src_nc = re.sub(r"//[^\n]*|/\*.*?\*/", "", src, flags=re.S)
    c_spec = locale_guard.consts(GAME / "include/constants/species.h", "SPECIES_")
    c_move = locale_guard.consts(GAME / "include/constants/moves.h", "MOVE_")
    c_item = locale_guard.consts(GAME / "include/constants/items.h", "ITEM_")
    items_json = json.loads(rd(ROOT / "locale/es/src/data/items.json"))["items"]
    gen3_items = {c_item[i["itemId"]] for i in items_json if i["itemId"] in c_item}
    allowed = {"SPECIES_NONE", "SPECIES_GEN3_END", "MOVE_NONE", "ITEM_NONE", "MOVES_COUNT_GEN3"}
    for s in sorted(set(re.findall(r"\bSPECIES_[A-Z0-9_]+\b", src_nc)) - allowed):
        if s not in c_spec or not 1 <= c_spec[s] <= 386:
            fail(f"GENERADOR: {s} no es de 1.ª-3.ª gen")
    for m in sorted(set(re.findall(r"\bMOVE_[A-Z0-9_]+\b", src_nc)) - allowed):
        if m not in c_move or not 1 <= c_move[m] <= 354:
            fail(f"GENERADOR: {m} no es de 3.ª gen")
    for i in sorted(set(re.findall(r"\bITEM_[A-Z0-9_]+\b", src_nc)) - allowed):
        if i not in c_item or c_item[i] not in gen3_items:
            fail(f"GENERADOR: {i} no es un objeto de 3.ª gen")
    block = re.search(r"sGen3Machines\[\]\s*=\s*\{(.*?)\};", src_nc, re.S)
    machines = re.findall(r"\bMOVE_[A-Z0-9_]+\b", block.group(1)) if block else []
    if len(machines) != 58 or len(set(machines)) != 58:
        fail(f"GENERADOR: sGen3Machines tiene {len(machines)} movimientos ({len(set(machines))} distintos) y deben ser 58 (50 MT + 8 MO)")


def main() -> int:
    check_config()
    check_party("overlay", rd(ROOT / "overlay/data/dailymax_rivals.party"))
    tree_party = GAME / "src/data/trainers_frlg.party"
    if tree_party.exists():
        check_party("árbol", rd(tree_party))
    check_generator()
    if ERRORS:
        print(f"check-gen3: {len(ERRORS)} PROBLEMAS", file=sys.stderr)
        for e in ERRORS:
            print("  - " + e, file=sys.stderr)
        return 1
    print("check-gen3: OK (configuración, rivales y generador solo con contenido de 3.ª gen).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
