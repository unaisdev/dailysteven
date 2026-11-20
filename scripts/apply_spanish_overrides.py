#!/usr/bin/env python3
r"""Aplica nuestras traducciones propias (locale/es/overrides_*.tsv) sobre los archivos de texto del juego.

Se ejecuta DESPUÉS de translate_battle_strings_table.py: lo que la traducción de FireRed no pudo cubrir
(mensajes que solo existen en la expansion, o con códigos distintos) lo traducimos aquí a mano.

Regla de oro (la misma que locale_guard): una traducción NO puede cambiar los códigos {...} del texto original de la
expansion. Si los cambia, este script FALLA (y con él la compilación). El número de cajas (\p) y de \l tampoco.
"""
from __future__ import annotations

import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import locale_guard as lg  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
GAME = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "pokeemerald-expansion")
LOCALE = ROOT / "locale" / "es"

# archivo del juego -> ficheros .tsv que lo traducen
TARGETS = {
    "src/battle_message.c": ["overrides_battle_es.tsv"],
    "src/strings.c": ["overrides_strings_es.tsv"],
    "src/battle_main.c": ["overrides_classes_es.tsv"],
}

PIECES = r'(?:\s*"(?:\\.|[^"\\])*")+'


def load_tsv(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for n, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if "\t" not in line:
            raise SystemExit(f"{path.name}:{n}: falta el TAB entre clave y texto")
        key, text = line.split("\t", 1)
        key = key.strip()
        if key in out:
            raise SystemExit(f"{path.name}:{n}: clave repetida {key}")
        out[key] = text
    return out


def code_signature(text: str):
    r"""Códigos {...}, nº de \p y nº de \l: lo que NO puede cambiar una traducción."""
    return Counter(lg.tokens(text)), text.count("\\p"), text.count("\\l")


def entry_pattern(key: str) -> tuple[re.Pattern, str, str]:
    """(patrón, prefijo del texto, sufijo) de la entrada `key` en el código C."""
    if key.startswith("TRAINER_CLASS_"):
        return re.compile(r"(\[" + key + r"\]\s*=\s*\{\s*)_\(" + PIECES + r"\s*\)"), '_("', '")'
    if key.startswith("STRINGID_"):
        return re.compile(r"(\[" + key + r"\]\s*=\s*)COMPOUND_STRING\(" + PIECES + r"\s*\)"), 'COMPOUND_STRING("', '")'
    return re.compile(r"((?:static )?const u8 " + key + r"\[[^\]]*\]\s*=\s*)_\(" + PIECES + r"\s*\)"), '_("', '")'


def main() -> None:
    total = 0
    for rel, files in TARGETS.items():
        path = GAME / rel
        if not path.is_file():
            continue
        entries: dict[str, str] = {}
        for f in files:
            p = LOCALE / f
            if p.is_file():
                entries.update(load_tsv(p))
        if not entries:
            continue
        text = path.read_text(encoding="utf-8")
        originals = lg.original_strings(GAME, rel)
        errors: list[str] = []
        done = 0
        for key, es in entries.items():
            pat, pre, post = entry_pattern(key)
            m = pat.search(text)
            if not m:
                errors.append(f"{key}: no existe en {rel} (o está enlazada a otra constante: traduce la constante)")
                continue
            orig = originals.get(key)
            if orig is None:  # constante añadida por nuestros parches: se compara con su texto actual
                orig = lg.join_pieces(m.group(0))
            if code_signature(orig) != code_signature(es):
                errors.append(f"{key}: códigos distintos al original.\n    original:   {orig}\n    traducción: {es}")
                continue
            text = text[: m.start()] + m.group(1) + pre + es + post + text[m.end():]
            done += 1
        if errors:
            print(f"ERROR en las traducciones propias de {rel}:", file=sys.stderr)
            for e in errors:
                print("  -", e, file=sys.stderr)
            raise SystemExit(1)
        path.write_text(text, encoding="utf-8")
        print(f"  {rel}: {done} traducciones propias aplicadas")
        total += done
    print(f"  traducciones propias: {total}")
    replace_pkmn_glyph()


# Textos de menús donde {PKMN} saldría como el símbolo «PKMN» del juego en inglés: en el Rojo Fuego español sale
# la palabra POKéMON entera. (Es la única excepción a «no cambiar los códigos {...}», y solo para estas constantes.)
PKMN_WORD = ["gText_DoWhatWithPokemon", "gText_CantSelectSamePkmn", "gText_PkmnForSwap", "gText_SamePkmnInPartyAlready"]
# Texto completo cuando la expansion los dejó en inglés o con el símbolo (ya con la palabra POKéMON)
PKMN_FULL = {
    "gText_CantSelectSamePkmn": "No puedes elegir el mismo POKéMON.",
    "gText_PkmnForSwap": "POKéMON A CAMBIO",
    "gText_SamePkmnInPartyAlready": "Ya tienes ese POKéMON en el equipo.",
}


def replace_pkmn_glyph() -> None:
    path = GAME / "src/strings.c"
    if not path.is_file():
        return
    text = path.read_text(encoding="utf-8")
    n = 0
    for name in PKMN_WORD:
        pat = re.compile(r"(const u8 " + name + r"\[[^\]]*\]\s*=\s*_\()([^;]*?)(\);)")
        m = pat.search(text)
        if not m:
            raise SystemExit(f"{name}: no existe en src/strings.c")
        new = ('"' + PKMN_FULL[name] + '"') if name in PKMN_FULL else m.group(2).replace("{PKMN}", "POKéMON")
        if new != m.group(2):
            text = text[: m.start(2)] + new + text[m.end(2):]
            n += 1
    path.write_text(text, encoding="utf-8")
    print(f"  {{PKMN}} -> POKéMON en {n} textos de menú")


if __name__ == "__main__":
    main()
