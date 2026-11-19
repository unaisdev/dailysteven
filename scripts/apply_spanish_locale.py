#!/usr/bin/env python3
"""Apply locale/es (Acimut pret_europe Spanish FireRed tables) onto pokeemerald-expansion.

Mirrors Daily Gold's apply-locale.sh, adapted for expansion file layout:
species/moves/abilities/types live in *Info structs, not pret tables.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import locale_guard  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
GUARD_SKIPPED: list[str] = []  # cadenas que NO se traducen porque cambiarían los códigos del original
GAME = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "pokeemerald-expansion")
LOCALE = ROOT / "locale" / "es"

STR_LIT = re.compile(
    r'((?:static )?const u8 )(\w+)(\[\] = )(_\((?:\"(?:\\.|[^\"\\])*\"\s*)+\));',
    re.S,
)

# pret_europe → expansion battle placeholder renames
PLACEHOLDER_REMAP = {
    "B_SCR_ACTIVE_NAME_WITH_PREFIX": "B_SCR_NAME_WITH_PREFIX",
    "B_SCR_ACTIVE_ABILITY": "B_SCR_ABILITY",
    "B_ACTIVE_NAME_WITH_PREFIX": "B_BUFF1",
    "FONT_NORMAL_COPY_1": "FONT_NORMAL",
    "FONT_NORMAL_COPY_2": "FONT_NORMAL",
}


def die(msg: str) -> None:
    raise SystemExit(f"apply_spanish_locale: {msg}")


def literal_inner(es: str) -> str:
    """Turn _('FOO') or _('FO' 'O') into FOO."""
    if es.startswith("_(") and es.endswith(")"):
        parts = re.findall(r'\"((?:\\.|[^\"\\])*)\"', es[2:-1])
        return "".join(parts)
    return es


UNSUPPORTED_CHARS = {
    "\u2025": "...",  # ‥
    "\u2026": "...",  # …
    "\u2014": "-",  # —
    "\u2013": "-",  # –
}


def sanitize_chars(lit: str) -> str:
    for bad, good in UNSUPPORTED_CHARS.items():
        lit = lit.replace(bad, good)
    return lit


def normalize_es_literal(lit: str, valid_controls: set[str]) -> str | None:
    """Remap FR placeholders; return None if still unknown tokens (skip string)."""
    lit = sanitize_chars(lit.rstrip().rstrip(";"))
    for old, new in PLACEHOLDER_REMAP.items():
        lit = lit.replace("{" + old + "}", "{" + new + "}")
        # Also bare control words used as {FONT_X} without braces remap above
    for tok in re.findall(r"\{([A-Z][A-Z0-9_]*)\b", lit):
        if tok not in valid_controls:
            return None
    return lit


def load_valid_placeholders(charmap: Path) -> set[str]:
    text = charmap.read_text(encoding="utf-8", errors="replace")
    # Any control code name usable inside {_(\"{CODE}...\")} strings.
    return set(re.findall(r"^([A-Z][A-Z0-9_]*)\s*=", text, re.M))


def load_symbol_map_from_headers(paths: list[Path]) -> dict[str, str]:
    mapping: dict[str, str] = {}
    for path in paths:
        text = path.read_text(encoding="utf-8")
        for m in STR_LIT.finditer(text):
            mapping[m.group(2)] = m.group(4)
    return mapping


def patch_string_literals(
    path: Path,
    mapping: dict[str, str],
    label: str,
    valid_placeholders: set[str] | None = None,
    originals: dict[str, str] | None = None,
) -> int:
    text = path.read_text(encoding="utf-8")
    # Clean accidental double-semicolons from earlier runs.
    text = re.sub(r"(\"\)|\)\));;+", r"\1;", text)
    replaced = 0
    skipped = 0

    def repl(m: re.Match[str]) -> str:
        nonlocal replaced, skipped
        name = m.group(2)
        if name not in mapping:
            return m.group(0)
        lit = mapping[name]
        if valid_placeholders is not None:
            norm = normalize_es_literal(lit, valid_placeholders)
            if norm is None:
                skipped += 1
                return m.group(0)
            lit = norm
        else:
            lit = lit.rstrip().rstrip(";")
        if originals and name in originals:
            inner = literal_inner(lit)
            fixed = locale_guard.reconcile(originals[name], inner)
            if fixed is None:
                GUARD_SKIPPED.append(f"{label}: {name}")
                skipped += 1
                return m.group(0)
            if fixed != inner:
                lit = '_("' + fixed + '")'
        replaced += 1
        return f"{m.group(1)}{name}{m.group(3)}{lit};"

    new = STR_LIT.sub(repl, text)
    if replaced or ";;" in path.read_text(encoding="utf-8"):
        path.write_text(new, encoding="utf-8")
    else:
        path.write_text(new, encoding="utf-8")
    extra = f", skipped {skipped}" if skipped else ""
    print(f"  {label}: {replaced} string literals{extra}")
    return replaced


def patch_indexed_names(path: Path, mapping: dict[str, str], wrap: str, label: str, field: str = r"\.name") -> int:
    """Replace .name = _("English") / COMPOUND_STRING("English") inside [KEY] blocks."""
    text = path.read_text(encoding="utf-8")
    replaced = 0
    # FireRed y la expansion no siempre llaman igual a la misma cosa (MOVE_HI_JUMP_KICK / MOVE_HIGH_JUMP_KICK):
    # se casan por NÚMERO, no por nombre del identificador.
    const_files = {"MOVE_": "moves.h", "ABILITY_": "abilities.h", "SPECIES_": "species.h",
                   "ITEM_": "items.h", "NATURE_": "pokemon.h"}
    first = next(iter(mapping), "")
    pm = re.match(r"[A-Z]+_", first)
    if pm and pm.group(0) in const_files:
        mapping = locale_guard.canonical_keys(
            mapping, GAME / "include/constants" / const_files[pm.group(0)], pm.group(0), text)
    for key, es in mapping.items():
        literal = literal_inner(es.rstrip().rstrip(";"))
        if not literal or "$" in literal:
            continue
        lit_esc = literal.replace("\\", "\\\\").replace('"', '\\"')
        replacement = wrap.format(lit_esc)
        pat = re.compile(
            rf"(\[{re.escape(key)}\]\s*=\s*\{{(?:(?!\n\s*\[).)*?{field}\s*=\s*)"
            rf"(?:_\(\"[^\"]*\"\)|COMPOUND_STRING\(\"(?:\\.|[^\"\\])*\"\)|ITEM_NAME\(\"(?:\\.|[^\"\\])*\"\))"
            rf"(\s*,)",
            re.S,
        )

        def repl(m: re.Match[str], rep: str = replacement) -> str:
            return f"{m.group(1)}{rep}{m.group(2)}"

        text2, n = pat.subn(repl, text, count=1)
        if n:
            replaced += n
            text = text2
    if replaced:
        path.write_text(text, encoding="utf-8")
    print(f"  {label}: {replaced} names")
    return replaced


def load_table_map(path: Path, key_prefix: str) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    out: dict[str, str] = {}
    for m in re.finditer(
        rf"\[({key_prefix}\w+)\]\s*=\s*(_\((?:\"(?:\\.|[^\"\\])*\"\s*)+\))",
        text,
    ):
        out[m.group(1)] = m.group(2)
    return out


def load_nature_map(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    locals_map: dict[str, str] = {}
    for m in STR_LIT.finditer(text):
        locals_map[m.group(2)] = m.group(4)
    out: dict[str, str] = {}
    for m in re.finditer(r"\[(NATURE_\w+)\]\s*=\s*(\w+)", text):
        sym = m.group(2)
        if sym in locals_map:
            out[m.group(1)] = locals_map[sym]
    return out


def patch_eu_nick_order(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    if "ES: name then" in text:
        print("  battle_message.c: EU nick order already applied")
        return

    en_macro = """#define HANDLE_NICKNAME_STRING_CASE(battler)                            \\
    if (!IsOnPlayerSide(battler))                                       \\
    {                                                                   \\
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)                     \\
            toCpy = sText_FoePkmnPrefix;                                \\
        else                                                            \\
            toCpy = sText_WildPkmnPrefix;                               \\
        while (*toCpy != EOS)                                           \\
        {                                                               \\
            dst[dstID] = *toCpy;                                        \\
            dstID++;                                                    \\
            toCpy++;                                                    \\
        }                                                               \\
    }                                                                   \\
    GetBattlerNick(battler, text);                                      \\
    toCpy = text;"""

    eu_macro = """#define HANDLE_NICKNAME_STRING_CASE(battler)                            \\
    GetBattlerNick(battler, text);                                      \\
    toCpy = text;                                                       \\
    if (!IsOnPlayerSide(battler))                                       \\
    {                                                                   \\
        while (*toCpy != EOS)                                           \\
        {                                                               \\
            dst[dstID] = *toCpy;                                        \\
            dstID++;                                                    \\
            toCpy++;                                                    \\
        }                                                               \\
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)                     \\
            toCpy = sText_FoePkmnPrefix;                                \\
        else                                                            \\
            toCpy = sText_WildPkmnPrefix;                               \\
    } /* ES: name then \" enemigo\"/\" salvaje\" */"""

    en_lower = """#define HANDLE_NICKNAME_STRING_LOWERCASE(battler)                       \\
    if (!IsOnPlayerSide(battler))                       \\
    {                                                                   \\
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)                     \\
            toCpy = sText_FoePkmnPrefixLower;                           \\
        else                                                            \\
            toCpy = sText_WildPkmnPrefixLower;                          \\
        while (*toCpy != EOS)                                           \\
        {                                                               \\
            dst[dstID] = *toCpy;                                        \\
            dstID++;                                                    \\
            toCpy++;                                                    \\
        }                                                               \\
    }                                                                   \\
    GetBattlerNick(battler, text);                                      \\
    toCpy = text;"""

    eu_lower = """#define HANDLE_NICKNAME_STRING_LOWERCASE(battler)                       \\
    GetBattlerNick(battler, text);                                      \\
    toCpy = text;                                                       \\
    if (!IsOnPlayerSide(battler))                                       \\
    {                                                                   \\
        while (*toCpy != EOS)                                           \\
        {                                                               \\
            dst[dstID] = *toCpy;                                        \\
            dstID++;                                                    \\
            toCpy++;                                                    \\
        }                                                               \\
        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)                     \\
            toCpy = sText_FoePkmnPrefixLower;                           \\
        else                                                            \\
            toCpy = sText_WildPkmnPrefixLower;                          \\
    } /* ES: name then prefix */"""

    changed = []
    if en_macro not in text:
        die("HANDLE_NICKNAME_STRING_CASE macro not found")
    text = text.replace(en_macro, eu_macro, 1)
    changed.append("HANDLE_NICKNAME_STRING_CASE")

    if en_lower in text:
        text = text.replace(en_lower, eu_lower, 1)
        changed.append("HANDLE_NICKNAME_STRING_LOWERCASE")

    text = re.sub(
        r'static const u8 sText_WildPkmnPrefix\[\] = _\("[^"]*"\);',
        'static const u8 sText_WildPkmnPrefix[] = _(" salvaje");',
        text,
        count=1,
    )
    text = re.sub(
        r'static const u8 sText_FoePkmnPrefix\[\] = _\("[^"]*"\);',
        'static const u8 sText_FoePkmnPrefix[] = _(" enemigo");',
        text,
        count=1,
    )
    text = re.sub(
        r'static const u8 sText_WildPkmnPrefixLower\[\] = _\("[^"]*"\);',
        'static const u8 sText_WildPkmnPrefixLower[] = _(" salvaje");',
        text,
        count=1,
    )
    text = re.sub(
        r'static const u8 sText_FoePkmnPrefixLower\[\] = _\("[^"]*"\);',
        'static const u8 sText_FoePkmnPrefixLower[] = _(" enemigo");',
        text,
        count=1,
    )
    changed.append("foe/wild prefixes")

    path.write_text(text, encoding="utf-8")
    print(f"  battle_message.c: EU nick order ({', '.join(changed)})")


# Official Spanish battle abbreviations (fit TYPE_NAME_LENGTH == 8).
TYPE_NAMES_ES = {
    "TYPE_NONE": "---",
    "TYPE_NORMAL": "NORMAL",
    "TYPE_FIGHTING": "LUCHA",
    "TYPE_FLYING": "VOLAD.",
    "TYPE_POISON": "VENENO",
    "TYPE_GROUND": "TIERRA",
    "TYPE_ROCK": "ROCA",
    "TYPE_BUG": "BICHO",
    "TYPE_GHOST": "FANT.",
    "TYPE_STEEL": "ACERO",
    "TYPE_MYSTERY": "(?)",
    "TYPE_FIRE": "FUEGO",
    "TYPE_WATER": "AGUA",
    "TYPE_GRASS": "PLANTA",
    "TYPE_ELECTRIC": "ELÉCT.",
    "TYPE_PSYCHIC": "PSÍQ.",
    "TYPE_ICE": "HIELO",
    "TYPE_DRAGON": "DRAGÓN",
    "TYPE_DARK": "SINIE.",
    "TYPE_FAIRY": "HADA",
    "TYPE_STELLAR": "ASTRAL",
}


def patch_types(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    replaced = 0
    for key, name in TYPE_NAMES_ES.items():
        lit = name.replace('"', '\\"')
        pat = re.compile(
            rf"(\[{re.escape(key)}\]\s*=\s*\{{(?:(?!\n\s*\[TYPE_).)*?\.name\s*=\s*)_\(\"[^\"]*\"\)",
            re.S,
        )
        text2, n = pat.subn(rf'\1_("{lit}")', text, count=1)
        if n:
            replaced += n
            text = text2
    path.write_text(text, encoding="utf-8")
    print(f"  types_info.h: {replaced} type names")


def c_str_lit(s: str) -> str:
    """Build a C _(\"...\") literal with escapes."""
    esc = (
        s.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
    )
    return f'_("{esc}")'


# Expansion Emerald symbols not present (or differently named) in FireRed ES headers.
UI_OVERRIDES = {
    "gText_Cancel": c_str_lit("SALIR"),
    "gText_Cancel2": c_str_lit("SALIR"),
    "gText_Cancel3": c_str_lit("SALIR"),
    "gText_Cancel4": c_str_lit("SALIR"),
    "gText_Cancel5": c_str_lit("SALIR"),
    "gText_PkmnInfo": c_str_lit("INFO. POKéMON"),
    "gText_OTSlash": c_str_lit("EO/"),
    "gText_TypeSlash": c_str_lit("TIPO/"),
    "gText_IDNumber2": c_str_lit("{ID}{NO}."),
    "gText_XNature": c_str_lit("Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}"),
    "gText_XNatureMetSomewhereAt": c_str_lit(
        "Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}.\n"
        "Conseguido en algún lugar\n"
        "con {LV_2}{DYNAMIC 0}{DYNAMIC 3}{DYNAMIC 1}."
    ),
    "gText_XNatureHatchedSomewhereAt": c_str_lit(
        "Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}.\n"
        "Abierto en algún lugar\n"
        "con {LV_2}{DYNAMIC 0}{DYNAMIC 3}{DYNAMIC 1}."
    ),
    "gText_XNatureMetAtYZ": c_str_lit(
        "Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}.\n"
        "Conseguido en {DYNAMIC 0}{DYNAMIC 4}{DYNAMIC 1}\n"
        "con {LV_2}{DYNAMIC 0}{DYNAMIC 3}{DYNAMIC 1}."
    ),
    "gText_XNatureHatchedAtYZ": c_str_lit(
        "Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}.\n"
        "Abierto en {DYNAMIC 0}{DYNAMIC 4}{DYNAMIC 1}\n"
        "con {LV_2}{DYNAMIC 0}{DYNAMIC 3}{DYNAMIC 1}."
    ),
    "gText_XNatureObtainedInTrade": c_str_lit(
        "Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}.\n"
        "Conseguido por intercambio."
    ),
    "gText_XNatureFatefulEncounter": c_str_lit(
        "Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}.\n"
        "Conseguido de forma curiosa\n"
        "con {LV_2}{DYNAMIC 0}{DYNAMIC 3}{DYNAMIC 1}."
    ),
    "gText_XNatureProbablyMetAt": c_str_lit(
        "Naturaleza {DYNAMIC 0}{DYNAMIC 2}{DYNAMIC 1}{DYNAMIC 5}.\n"
        "Conseguido, según parece,\n"
        "en {DYNAMIC 0}{DYNAMIC 4}{DYNAMIC 1}\n"
        "con {LV_2}{DYNAMIC 0}{DYNAMIC 3}{DYNAMIC 1}."
    ),
    "gText_PickNextCancel": c_str_lit("{DPAD_UPDOWN}ELEGIR {A_BUTTON}SIG. {B_BUTTON}SALIR"),
    "gText_PickCancel": c_str_lit("{DPAD_UPDOWN}ELEGIR {A_BUTTON}{B_BUTTON}SALIR"),
    "gText_AButtonExit": c_str_lit("{A_BUTTON}SALIR"),
    "gText_NextBack": c_str_lit("{A_BUTTON}SIG. {B_BUTTON}ATRÁS"),
    "gText_Power": c_str_lit("POT."),
    "gText_Accuracy2": c_str_lit("PREC."),
    "gText_Appeal": c_str_lit("EXHIB."),
    "gText_Jam": c_str_lit("INHIB."),
}


PARTY_MENU_ACTIONS_ES = {
    "SUMMARY": "DATOS",
    "SWITCH": "CAMBIAR",
    "SHIFT": "CAMBIAR",
    "SEND OUT": "SACAR",
    "ENTER": "ENTRAR",
    "NO ENTRY": "NO ENTRA",
    "STORE": "GUARDAR",
    "ITEM": "OBJETO",
    "TAKE": "QUITAR",
    "MOVE": "MOVER",
    "MAIL": "CARTA",
    "READ": "LEER",
    "Change form": "Cambiar forma",
    "Change Ability": "Cambiar habil.",
}


def patch_ui_overrides(path: Path) -> int:
    text = path.read_text(encoding="utf-8")
    replaced = 0
    for name, lit in UI_OVERRIDES.items():
        # DOTALL + non-greedy: also repairs prior runs that wrote raw newlines inside _("...").
        pat = re.compile(
            rf"(const u8 {re.escape(name)}\[\].*?=\s*)(_\([\s\S]*?\));",
        )
        def repl(m: re.Match[str], lit: str = lit) -> str:
            return f"{m.group(1)}{lit};"

        text2, n = pat.subn(repl, text, count=1)
        if n:
            replaced += n
            text = text2
    path.write_text(text, encoding="utf-8")
    print(f"  strings.c overrides: {replaced}")
    return replaced


def patch_party_menu_actions(path: Path) -> int:
    text = path.read_text(encoding="utf-8")
    replaced = 0
    for en, es in PARTY_MENU_ACTIONS_ES.items():
        esc_en = re.escape(en)
        esc_es = es.replace("\\", "\\\\").replace('"', '\\"')
        pat = re.compile(rf'COMPOUND_STRING\("{esc_en}"\)')
        text2, n = pat.subn(f'COMPOUND_STRING("{esc_es}")', text)
        if n:
            replaced += n
            text = text2
    path.write_text(text, encoding="utf-8")
    print(f"  party_menu.h actions: {replaced}")
    return replaced


def camel_to_move_key(camel: str) -> str:
    """Pound -> MOVE_POUND, KarateChop -> MOVE_KARATE_CHOP, DoubleSlap -> MOVE_DOUBLE_SLAP."""
    out: list[str] = []
    for i, ch in enumerate(camel):
        if ch.isupper() and i > 0 and (camel[i - 1].islower() or (i + 1 < len(camel) and camel[i + 1].islower())):
            out.append("_")
        out.append(ch.upper())
    return "MOVE_" + "".join(out)


def read_text_auto(path: Path) -> str:
    raw = path.read_bytes()
    for enc in ("utf-8", "utf-8-sig", "cp1252", "latin-1"):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", errors="replace")


def load_move_descriptions(path: Path) -> dict[str, str]:
    text = read_text_auto(path)
    out: dict[str, str] = {}
    for m in STR_LIT.finditer(text):
        name = m.group(2)
        if not name.startswith("gMoveDescription_"):
            continue
        camel = name[len("gMoveDescription_") :]
        key = camel_to_move_key(camel)
        out[key] = literal_inner(m.group(4).rstrip().rstrip(";"))
    return out


def load_ability_descriptions(path: Path) -> dict[str, str]:
    text = read_text_auto(path)
    locals_map = {
        m.group(2): literal_inner(m.group(4).rstrip().rstrip(";")) for m in STR_LIT.finditer(text)
    }
    out: dict[str, str] = {}
    for m in re.finditer(r"\[(ABILITY_\w+)\]\s*=\s*(\w+)", text):
        sym = m.group(2)
        if sym in locals_map and "Description" in sym:
            out[m.group(1)] = locals_map[sym]
    return out


def compound_string_body(text: str) -> str:
    """Build COMPOUND_STRING(...) with adjacent quoted lines (no raw newlines inside quotes)."""
    lit = sanitize_chars(text).replace("\r\n", "\n").replace("\r", "\n")
    # Parsed C literals keep \n as two chars; turn into real newlines for wrapping.
    lit = lit.replace("\\n", "\n").replace("\\p", "\n")
    # Prefer existing line breaks; otherwise soft-wrap ~28 chars for GBA textbox.
    if "\n" not in lit:
        words = lit.split(" ")
        lines: list[str] = []
        cur = ""
        for w in words:
            trial = w if not cur else f"{cur} {w}"
            if len(trial) > 28 and cur:
                lines.append(cur)
                cur = w
            else:
                cur = trial
        if cur:
            lines.append(cur)
    else:
        lines = [ln.strip() for ln in lit.split("\n") if ln.strip()]
    if not lines:
        return 'COMPOUND_STRING("")'

    def esc_line(line: str) -> str:
        # No backslash-doubling of controls — lines are plain text now.
        return line.replace("\\", "\\\\").replace('"', '\\"')

    if len(lines) == 1:
        return f'COMPOUND_STRING("{esc_line(lines[0])}")'
    # Adjacent string literals (NO commas) — COMPOUND_STRING is a macro.
    chunks = []
    for i, line in enumerate(lines):
        if i < len(lines) - 1:
            chunks.append(f'"{esc_line(line)}\\n"')
        else:
            chunks.append(f'"{esc_line(line)}"')
    joined = "\n            ".join(chunks)
    return f"COMPOUND_STRING(\n            {joined})"


def patch_info_descriptions(path: Path, mapping: dict[str, str], key_prefix: str, label: str) -> int:
    """Replace .description = COMPOUND_STRING(...) inside each [KEY] block only."""
    text = path.read_text(encoding="utf-8")
    replaced = 0
    desc_pat = re.compile(
        r"(\.description\s*=\s*)COMPOUND_STRING\(\s*(?:\"(?:\\.|[^\"\\])*\"\s*)+\)",
    )
    for key, es in mapping.items():
        if not key.startswith(key_prefix):
            continue
        lit = sanitize_chars(es)
        if not lit or "$" in lit:
            continue
        body = compound_string_body(lit)
        start = text.find(f"[{key}]")
        if start < 0:
            continue
        # Limit edits to this entry: next sibling key or EOF.
        nxt = re.search(rf"\n    \[{re.escape(key_prefix)}\w+\]", text[start + 1 :])
        end = start + 1 + nxt.start() if nxt else len(text)
        block = text[start:end]

        def repl(m: re.Match[str], b: str = body) -> str:
            return f"{m.group(1)}{b}"

        new_block, n = desc_pat.subn(repl, block, count=1)
        if n:
            text = text[:start] + new_block + text[end:]
            replaced += n
    if replaced:
        path.write_text(text, encoding="utf-8")
    print(f"  {label}: {replaced} descriptions")
    return replaced


def patch_battle_move_stat_labels(path: Path) -> None:
    """Battle move info box: CAT/PWR/ACC (7-byte buffers) → Spanish abbreviations."""
    text = path.read_text(encoding="utf-8")
    repls = [
        ('u8 cat_desc[7] = _("CAT: ");', 'u8 cat_desc[7] = _("CAT: ");'),  # keep short
        ('u8 pwr_desc[7] = _("PWR: ");', 'u8 pwr_desc[7] = _("POT: ");'),
        ('u8 acc_desc[7] = _("ACC: ");', 'u8 acc_desc[7] = _("PRE: ");'),
    ]
    n = 0
    for old, new in repls:
        if old in text and new not in text:
            text = text.replace(old, new, 1)
            n += 1
        elif new in text:
            n += 1
    path.write_text(text, encoding="utf-8")
    print(f"  battle_controller_player.c: {n} move-stat labels")


# Expansion leftovers (Z-Moves / typos) not covered by PokeAPI slug mapping.
EXTRA_MOVE_NAMES_ES = {
    "MOVE_VICE_GRIP": "AGARRE",
    "MOVE_VISE_GRIP": "AGARRE",
    "MOVE_BREAKNECK_BLITZ": "CARRERA ARROLLADORA",
    "MOVE_SUPERSONIC_SKYSTRIKE": "PICADO SUPERSÓNICO",
    "MOVE_ACID_DOWNPOUR": "DILUVIO CORROSIVO",
    "MOVE_TECTONIC_RAGE": "BARRENA TELÚRICA",
    "MOVE_CONTINENTAL_CRUSH": "APLAST. GIGALÍTICO",
    "MOVE_CORKSCREW_CRASH": "RIZO MACHACADOR",
    "MOVE_INFERNO_OVERDRIVE": "HECATOMBE PÍRICA",
    "MOVE_HYDRO_VORTEX": "HIDROVÓRTICE",
    "MOVE_BLOOM_DOOM": "MEGATÓN FLORAL",
    "MOVE_GIGAVOLT_HAVOC": "GIGAVOLTIO DESTR.",
    "MOVE_SHATTERED_PSYCHE": "DISRUPTOR PSÍQUICO",
    "MOVE_SUBZERO_SLAMMER": "CRIOALIENTO DESOL.",
    "MOVE_DEVASTATING_DRAKE": "DRACOESTRELLA FUG.",
    "MOVE_BLACK_HOLE_ECLIPSE": "AGUJERO NEGRO ANIQ.",
    "MOVE_TWINKLE_TACKLE": "ARRUMACO SIDERAL",
}


def load_full_move_es() -> tuple[dict[str, str], dict[str, str]]:
    """Merge FireRed locale names/descs with PokeAPI full Gen1–9 table."""
    names: dict[str, str] = {}
    descs: dict[str, str] = {}
    full_path = LOCALE / "moves_full_es.json"
    if full_path.is_file():
        full = json.loads(full_path.read_text(encoding="utf-8"))
        for key, entry in full.items():
            if "name" in entry:
                names[key] = c_str_lit(entry["name"])
            if "description" in entry:
                descs[key] = entry["description"]
    # Prefer official FireRed Spanish names where present.
    fr_names = load_table_map(LOCALE / "src/data/text/move_names.h", "MOVE_")
    fr_names.pop("MOVE_NONE", None)
    names.update(fr_names)
    fr_descs = load_move_descriptions(LOCALE / "src/move_descriptions.c")
    descs.update(fr_descs)
    for key, name in EXTRA_MOVE_NAMES_ES.items():
        names[key] = c_str_lit(name)
    return names, descs


def main() -> None:
    if not (GAME / "src").is_dir():
        die(f"missing {GAME} — run scripts/get-expansion.sh first")
    if not (LOCALE / "SOURCE.json").is_file():
        die(f"missing {LOCALE} — run scripts/fetch-spanish-locale.py first")

    print(">> Applying Spanish (es-ES) locale to pokeemerald-expansion")
    valid = load_valid_placeholders(GAME / "charmap.txt")

    ui_map = load_symbol_map_from_headers(
        [
            LOCALE / "src/data/text/strings_1_es.h",
            LOCALE / "src/data/text/strings_2_es.h",
            LOCALE / "src/data/text/strings_3_es.h",
            LOCALE / "src/data/text/quest_log_es.h",
            LOCALE / "src/data/text/teachy_tv_es.h",
        ]
    )
    patch_string_literals(
        GAME / "src/strings.c", ui_map, "strings.c",
        originals=locale_guard.original_strings(GAME, "src/strings.c"),
    )
    patch_ui_overrides(GAME / "src/strings.c")
    # Expansion charmap rejects some punctuation that appears in leftover JP strings.
    strings_path = GAME / "src/strings.c"
    stext = sanitize_chars(strings_path.read_text(encoding="utf-8"))
    strings_path.write_text(stext, encoding="utf-8")

    patch_party_menu_actions(GAME / "src/data/party_menu.h")
    patch_battle_move_stat_labels(GAME / "src/battle_controller_player.c")

    battle_map = json.loads((LOCALE / "battle_strings_es.json").read_text(encoding="utf-8"))
    for hdr in (
        LOCALE / "src/data/text/battle_message_1_es.h",
        LOCALE / "src/data/text/battle_message_2_es.h",
    ):
        battle_map.update(load_symbol_map_from_headers([hdr]))
    patch_string_literals(
        GAME / "src/battle_message.c",
        battle_map,
        "battle_message.c",
        valid_placeholders=valid,
        originals=locale_guard.original_strings(GAME, "src/battle_message.c"),
    )
    patch_eu_nick_order(GAME / "src/battle_message.c")

    moves, move_descs = load_full_move_es()
    moves.pop("MOVE_NONE", None)
    # Los nombres oficiales de España (FireRed) mandan sobre los de PokeAPI, casados por NÚMERO de movimiento
    # (FireRed MOVE_HI_JUMP_KICK == expansion MOVE_HIGH_JUMP_KICK).
    fr_official = load_table_map(LOCALE / "src/data/text/move_names.h", "MOVE_")
    fr_official.pop("MOVE_NONE", None)
    fr_official = locale_guard.canonical_keys(
        fr_official, GAME / "include/constants/moves.h", "MOVE_", (GAME / "src/data/moves_info.h").read_text(encoding="utf-8"))
    moves.update(fr_official)
    patch_indexed_names(
        GAME / "src/data/moves_info.h",
        moves,
        wrap='COMPOUND_STRING("{}")',
        label="moves_info.h",
    )
    patch_info_descriptions(
        GAME / "src/data/moves_info.h",
        move_descs,
        key_prefix="MOVE_",
        label="moves_info.h descs",
    )

    abilities = load_table_map(LOCALE / "src/data/text/abilities.h", "ABILITY_")
    # Name table uses _("..."); drop description-pointer rows if any slipped in.
    abilities = {k: v for k, v in abilities.items() if v.startswith("_(")}
    patch_indexed_names(
        GAME / "src/data/abilities.h",
        abilities,
        wrap='_("{}")',
        label="abilities.h",
    )
    ability_descs = load_ability_descriptions(LOCALE / "src/data/text/abilities.h")
    patch_info_descriptions(
        GAME / "src/data/abilities.h",
        ability_descs,
        key_prefix="ABILITY_",
        label="abilities.h descs",
    )

    natures = load_nature_map(LOCALE / "src/data/text/nature_names.h")
    patch_indexed_names(
        GAME / "src/pokemon.c",
        natures,
        wrap='COMPOUND_STRING("{}")',
        label="pokemon.c natures",
    )

    patch_types(GAME / "src/data/types_info.h")

    # Especies (1-386) y objetos de 1.ª-3.ª gen: nombres oficiales de España.
    species = load_table_map(LOCALE / "src/data/text/species_names.h", "SPECIES_")
    species.pop("SPECIES_NONE", None)
    for fam in sorted((GAME / "src/data/pokemon/species_info").glob("gen_[123]_families.h")):
        patch_indexed_names(fam, species, wrap='_("{}")', label=fam.name, field=r"\.speciesName")
    unown = GAME / "src/data/pokemon/species_info/gen_2_families.h"
    utxt = unown.read_text(encoding="utf-8")
    utxt = utxt.replace('.speciesName = _("Unown")', '.speciesName = _("UNOWN")')
    unown.write_text(utxt, encoding="utf-8")
    items_json = json.loads((LOCALE / "src/data/items.json").read_text(encoding="utf-8"))["items"]
    items = {}
    for it in items_json:
        nm = it["english"].replace("�", "é")  # el json trae 'POK? BALL' con la é rota
        if it["itemId"] != "ITEM_NONE" and nm and "?" not in nm:
            items[it["itemId"]] = c_str_lit(nm)
    patch_indexed_names(GAME / "src/data/items.h", items, wrap='ITEM_NAME("{}")', label="items.h names")

    # Expansion battle dialogue lives in gBattleStringsTable (COMPOUND_STRING), not sText_*.
    import subprocess

    subprocess.check_call(
        [sys.executable, str(ROOT / "scripts" / "translate_battle_strings_table.py"), str(GAME)]
    )

    cache = ROOT / ".cache"
    cache.mkdir(exist_ok=True)
    (cache / "locale-skipped.txt").write_text(chr(10).join(GUARD_SKIPPED) + chr(10), encoding="utf-8")
    print(f"  guard: {len(GUARD_SKIPPED)} cadenas se dejan en el idioma original (códigos distintos); lista en .cache/locale-skipped.txt")
    print("Done. Locale es-ES applied.")


if __name__ == "__main__":
    main()
