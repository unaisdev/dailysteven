#!/usr/bin/env python3
"""Comprobación de nombres y textos en castellano. Falla (código 1) si algo alcanzable queda mal.

Uso:  scripts/check-names.py RUTA_AL_ARBOL_DE_LA_EXPANSION [--sample]

Qué comprueba, para todo lo ALCANZABLE en el juego diario (solo 1.ª-3.ª generación):
  * movimientos 1-354, habilidades 1-77, especies 1-386, naturalezas, tipos y los objetos que usa el generador:
    - el nombre de cada identificador es el nombre oficial de España de la tabla de referencia
      (locale/es, que viene de Acimut pret_europe, es decir de Rojo Fuego España) y ese nombre NO está vacío;
    - ningún nombre se repite dentro de su categoría y ningún nombre de movimiento coincide con el de una
      habilidad, especie, objeto ... (así se detecta "HIDROBOMBA" mostrado como "HUMEDAD");
    - cabe en su límite de longitud y solo usa caracteres que existen en la fuente del juego.
  * textos de combate y de menús: cada texto traducido lleva exactamente los mismos códigos {...} que el
    original de la expansion (así se detecta "usó {B_BUFF2}" donde el movimiento va en {B_BUFF3}, o los
    colores de la lista de movimientos). Las pocas excepciones deliberadas están en INTENTIONAL.

--sample imprime una muestra (identificador -> nombre que lleva la ROM) para revisarla a ojo.
"""
from __future__ import annotations

import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import locale_guard  # noqa: E402

ROOT = HERE.parent
GAME = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 and not sys.argv[1].startswith("--") else ROOT / "pokeemerald-expansion"
SAMPLE = "--sample" in sys.argv
LOCALE = ROOT / "locale" / "es"
ERRORS: list[str] = []

# Textos cuyos códigos se cambian a propósito en scripts/patch-dailymaximo-hooks.py o en UI_OVERRIDES.
INTENTIONAL = {
    "sText_TwoTrainersWantToBattle", "sText_TwoTrainersSentPkmn", "sText_Trainer2SentOutPkmn",
    "sText_Trainer2WithdrewPkmn", "sText_Trainer1SentOutPkmn2", "sText_Trainer1WithdrewPkmn",
}


rd = locale_guard.rd
consts = locale_guard.consts


def fail(msg: str) -> None:
    ERRORS.append(msg)


def unescape(s: str) -> str:
    return s.replace('\\"', '"').replace("\\\\", "\\")


# ---------- nombres del árbol ---------------------------------------------------------------
def tree_names(path: Path, key_prefix: str, field: str) -> dict[str, str]:
    text = rd(path)
    out: dict[str, str] = {}
    parts = re.split(rf"\n\s*\[({key_prefix}\w+)\]\s*=", text)
    for i in range(1, len(parts), 2):
        m = re.search(
            rf'{field}\s*=\s*(?:_\(|COMPOUND_STRING\(|ITEM_NAME\()"((?:\\.|[^"\\])*)"\)', parts[i + 1][:6000]
        )
        out[parts[i]] = unescape(m.group(1)) if m else ""
    return out


# ---------- tablas de referencia (locale/es) ------------------------------------------------
def ref_table(path: Path, key_prefix: str) -> dict[str, str]:
    text = rd(path)
    out: dict[str, str] = {}
    for m in re.finditer(rf'\[({key_prefix}\w+)\]\s*=\s*_\(((?:\s*"(?:\\.|[^"\\])*")+)\s*\)', text):
        out[m.group(1)] = unescape("".join(re.findall(r'"((?:\\.|[^"\\])*)"', m.group(2))))
    return out


def charmap_chars() -> set[str]:
    chars: set[str] = set()
    for m in re.finditer(r"^'(\\.|.)'\s*=", rd(GAME / "charmap.txt"), re.M):
        chars.add(m.group(1))
    for m in re.finditer(r"^(\S)\s*=\s*[0-9A-Fa-f]{2}", rd(GAME / "charmap.txt"), re.M):
        chars.add(m.group(1))
    chars |= set(" '")
    return chars


CHARS = charmap_chars()


def check_category(label: str, ids: dict[str, int], reachable, tree: dict[str, str], ref: dict[str, str], limit: int):
    """ids: símbolo->número (para resolver alias). reachable(num)->bool. Devuelve {num: (símbolo, nombre)}."""
    # reference por número
    ref_by_num: dict[int, str] = {}
    for sym, name in ref.items():
        if sym in ids:
            ref_by_num.setdefault(ids[sym], name)
    tree_by_num: dict[int, tuple[str, str]] = {}
    for sym, name in tree.items():
        if sym in ids:
            tree_by_num.setdefault(ids[sym], (sym, name))
    result: dict[int, tuple[str, str]] = {}
    for num in sorted(set(ref_by_num) | {n for n in tree_by_num if reachable(n)}):
        if not reachable(num):
            continue
        sym, got = tree_by_num.get(num, ("?", ""))
        want = ref_by_num.get(num)
        if want is None:
            continue  # sin referencia: no se puede comparar (p. ej. ??? / huecos)
        result[num] = (sym, got)
        if not got.strip():
            fail(f"{label} {num} ({sym}): NOMBRE VACÍO (esperado «{want}»)")
        elif got != want:
            fail(f"{label} {num} ({sym}): sale «{got}» y debería salir «{want}»")
        if len(got) > limit:
            fail(f"{label} {num} ({sym}): «{got}» mide {len(got)} y el límite es {limit}")
        bad = sorted({c for c in got if c not in CHARS})
        if bad:
            fail(f"{label} {num} ({sym}): caracteres que la fuente no tiene: {bad}")
    dup = defaultdict(list)
    for num, (sym, got) in result.items():
        dup[got].append(sym)
    for name, syms in dup.items():
        if len(syms) > 1 and name.strip():
            fail(f"{label}: el nombre «{name}» está repetido en {syms}")
    return result


def main() -> int:
    c_moves = consts(GAME / "include/constants/moves.h", "MOVE_")
    c_abil = consts(GAME / "include/constants/abilities.h", "ABILITY_")
    c_spec = consts(GAME / "include/constants/species.h", "SPECIES_")
    c_item = consts(GAME / "include/constants/items.h", "ITEM_")
    c_nat = consts(GAME / "include/constants/pokemon.h", "NATURE_")
    c_type = consts(GAME / "include/constants/pokemon.h", "TYPE_")

    moves = check_category(
        "MOVIMIENTO", c_moves, lambda n: 1 <= n <= 354,
        tree_names(GAME / "src/data/moves_info.h", "MOVE_", r"\.name"),
        ref_table(LOCALE / "src/data/text/move_names.h", "MOVE_"), 16)
    abil = check_category(
        "HABILIDAD", c_abil, lambda n: 1 <= n <= 77,
        tree_names(GAME / "src/data/abilities.h", "ABILITY_", r"\.name"),
        ref_table(LOCALE / "src/data/text/abilities.h", "ABILITY_"), 16)
    spec_tree = {k: v for f in (GAME / "src/data/pokemon/species_info").glob("gen_[123]_families.h")
                 for k, v in tree_names(f, "SPECIES_", r"\.speciesName").items()}
    um = re.search(r'#define UNOWN_MISC_INFO.*?\.speciesName\s*=\s*_\("([^"]*)"\)',
                   rd(GAME / "src/data/pokemon/species_info/gen_2_families.h"), re.S)
    if um and not spec_tree.get("SPECIES_UNOWN"):
        spec_tree["SPECIES_UNOWN"] = um.group(1)  # el nombre de Unown está dentro de una macro
    spec = check_category(
        "ESPECIE", c_spec, lambda n: 1 <= n <= 386,
        spec_tree,
        ref_table(LOCALE / "src/data/text/species_names.h", "SPECIES_"), 12)

    # objetos: los que puede llevar un Pokémon del generador (overlay/src/dailymaximo_team.c)
    team_src = rd(ROOT / "overlay/src/dailymaximo_team.c")
    pool = set(re.findall(r"\bITEM_[A-Z0-9_]+\b", team_src)) - {"ITEM_NONE"}
    import json
    ref_items = {x["itemId"]: x["english"].replace("\ufffd", "é")
                 for x in json.loads(rd(LOCALE / "src/data/items.json"))["items"]}
    pool_nums = {c_item[i] for i in pool if i in c_item}
    items = check_category(
        "OBJETO", c_item, lambda n: n in pool_nums,
        tree_names(GAME / "src/data/items.h", "ITEM_", r"\.name"), ref_items, 20)
    for i in sorted(pool):
        if i in c_item and i not in ref_items and not any(c_item.get(k) == c_item[i] for k in ref_items if k in c_item):
            fail(f"OBJETO {i}: no hay nombre de referencia en castellano (objeto fuera de 3.ª gen o sin traducir)")

    # naturalezas y tipos
    nat_tree = tree_names(GAME / "src/pokemon.c", "NATURE_", r"\.name")
    nat_ref = {}
    text_n = rd(LOCALE / "src/data/text/nature_names.h")
    locals_ = {m.group(1): m.group(2) for m in re.finditer(r'const u8 (\w+)\[\]\s*=\s*_\("((?:\\.|[^"\\])*)"\)', text_n)}
    for m in re.finditer(r"\[(NATURE_\w+)\]\s*=\s*(\w+)", text_n):
        if m.group(2) in locals_:
            nat_ref[m.group(1)] = locals_[m.group(2)]
    nat = check_category("NATURALEZA", c_nat, lambda n: 0 <= n <= 24, nat_tree, nat_ref, 12)
    type_ref_text = rd(LOCALE / "patches/type_names.h")
    type_names = re.findall(r'\[(TYPE_\w+)\]\s*=\s*_\("((?:\\.|[^"\\])*)"\)', type_ref_text)
    type_ref = {k: unescape(v) for k, v in type_names}
    typ = check_category(
        "TIPO", c_type, lambda n: 1 <= n <= 18,
        tree_names(GAME / "src/data/types_info.h", "TYPE_", r"\.name"), type_ref, 8)

    # cruces entre categorías: un nombre de movimiento no puede ser el de una habilidad/especie/objeto/tipo...
    others = {"HABILIDAD": abil, "ESPECIE": spec, "OBJETO": items, "TIPO": typ, "NATURALEZA": nat}
    mv_names = {name: sym for _, (sym, name) in moves.items()}
    for cat, tab in others.items():
        for _, (sym, name) in tab.items():
            if name in mv_names and cat != "TIPO" and cat != "NATURALEZA":
                fail(f"CRUCE: el movimiento {mv_names[name]} y {cat} {sym} se llaman igual «{name}»")

    # textos: mismos códigos que el original
    for rel in ("src/battle_message.c", "src/strings.c"):
        orig = locale_guard.original_strings(GAME, rel)
        cur = rd(GAME / rel)
        now = {}
        for m in locale_guard._STR.finditer(cur):
            now[m.group(1)] = locale_guard.join_pieces(m.group(2))
        for m in locale_guard._ENTRY.finditer(cur):
            now[m.group(1)] = locale_guard.join_pieces(m.group(2))
        for sym, o in orig.items():
            n = now.get(sym)
            if n is None or sym in INTENTIONAL:
                continue
            if locale_guard.reconcile(o, n) != n:
                fail(f"TEXTO {rel.split('/')[-1]} {sym}: códigos distintos al original  {locale_guard.tokens(o)} -> {locale_guard.tokens(n)}")

    if SAMPLE:
        def show(title, table, wanted):
            print(f"\n{title}")
            for sym in wanted:
                for num, (s, name) in table.items():
                    if s == sym:
                        print(f"  {s:<28} -> {name}")
        show("MOVIMIENTOS (identificador -> nombre en la ROM)", moves, [
            "MOVE_HYDRO_PUMP", "MOVE_SURF", "MOVE_EARTHQUAKE", "MOVE_PSYCHIC", "MOVE_ICE_BEAM", "MOVE_FLAMETHROWER",
            "MOVE_THUNDERBOLT", "MOVE_SHADOW_BALL", "MOVE_SWORDS_DANCE", "MOVE_SPLASH", "MOVE_PROTECT", "MOVE_TOXIC",
            "MOVE_SOLAR_BEAM", "MOVE_DRAGON_CLAW", "MOVE_ROCK_SLIDE", "MOVE_AERIAL_ACE", "MOVE_FACADE", "MOVE_REST",
            "MOVE_BRICK_BREAK", "MOVE_METEOR_MASH", "MOVE_BLAZE_KICK", "MOVE_GIGA_DRAIN"])
        show("HABILIDADES", abil, [
            "ABILITY_DAMP", "ABILITY_STURDY", "ABILITY_LEVITATE", "ABILITY_INTIMIDATE", "ABILITY_BLAZE", "ABILITY_TORRENT",
            "ABILITY_OVERGROW", "ABILITY_SWIFT_SWIM", "ABILITY_NATURAL_CURE", "ABILITY_CLEAR_BODY", "ABILITY_ROCK_HEAD"])
        show("ESPECIES", spec, ["SPECIES_BULBASAUR", "SPECIES_PIKACHU", "SPECIES_METAGROSS", "SPECIES_BLAZIKEN",
                                "SPECIES_GARDEVOIR", "SPECIES_ALTARIA", "SPECIES_AGGRON", "SPECIES_DEOXYS"])
        show("OBJETOS", items, ["ITEM_LEFTOVERS", "ITEM_CHOICE_BAND", "ITEM_CHARCOAL", "ITEM_MYSTIC_WATER", "ITEM_SILK_SCARF"])
        show("NATURALEZAS", nat, ["NATURE_ADAMANT", "NATURE_MODEST", "NATURE_JOLLY"])
        show("TIPOS", typ, ["TYPE_FIRE", "TYPE_WATER", "TYPE_GHOST", "TYPE_DARK"])
        print(f"\nRevisados: {len(moves)} movimientos, {len(abil)} habilidades, {len(spec)} especies, "
              f"{len(items)} objetos, {len(nat)} naturalezas, {len(typ)} tipos.")

    if ERRORS:
        print(f"\ncheck-names: {len(ERRORS)} PROBLEMAS", file=sys.stderr)
        for e in ERRORS[:80]:
            print("  - " + e, file=sys.stderr)
        if len(ERRORS) > 80:
            print(f"  ... y {len(ERRORS) - 80} más", file=sys.stderr)
        return 1
    print("check-names: OK (nombres y textos en castellano coherentes).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
