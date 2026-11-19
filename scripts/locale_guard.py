"""Protección de la traducción: una cadena traducida NUNCA puede cambiar los "códigos" que lleva dentro.

Los textos de pokefirered (de donde sale la traducción) y los de pokeemerald-expansion no usan los mismos
códigos. Ejemplos reales que ya nos fallaron:

  * "¡{B_ATK_NAME_WITH_PREFIX} usó\\n{B_BUFF2}"   -> en la expansion el nombre del movimiento va en {B_BUFF3};
    {B_BUFF2} tenía otra cosa (a veces el nombre de una HABILIDAD, "HUMEDAD") o estaba vacío.
  * gText_MoveInterfaceDynamicColors              -> la traducción fijaba los colores 13/14/15 en vez de los
    colores dinámicos del tipo del movimiento, y algunos nombres de movimiento salían invisibles.

Regla: se compara cada traducción con el texto ORIGINAL de la expansion (git HEAD del árbol, antes de nada).
  - mismos códigos {...}                  -> vale
  - mismos códigos en otro orden          -> vale (el castellano ordena las frases de otro modo)
  - solo cambia el número de {B_BUFFn}    -> se reasigna al del original (si el número de buffers coincide)
  - rol de combatiente distinto (ATK/DEF/SCR/EFF) en el mismo tipo de código
    (NAME_WITH_PREFIX, ABILITY…)          -> se reasigna al del original (mismo nº y mismo grupo)
  - cualquier otra diferencia             -> NO se traduce esa cadena (se queda el original) y se anota

Ej.: FireRed «¡{B_DEF_NAME_WITH_PREFIX} se ha protegido!» vs expansion
«{B_SCR_NAME_WITH_PREFIX} protected itself!» -> se acepta como «¡{B_SCR_NAME_WITH_PREFIX} se ha protegido!».
"""
from __future__ import annotations

import re
import subprocess
from collections import Counter
from pathlib import Path

TOKEN = re.compile(r"\{[^{}]*\}")
BUFF = re.compile(r"\{B_BUFF\d\}")

# FireRed y expansion a menudo nombran al mismo combatiente con otro prefijo
# (DEF vs SCR, …) o con/sin la variante «2» (posesivo / minúsculas).
SYNONYM_GROUPS: tuple[frozenset[str], ...] = (
    frozenset(
        {
            "{B_ATK_NAME_WITH_PREFIX}",
            "{B_DEF_NAME_WITH_PREFIX}",
            "{B_SCR_NAME_WITH_PREFIX}",
            "{B_EFF_NAME_WITH_PREFIX}",
            "{B_ATK_NAME_WITH_PREFIX2}",
            "{B_DEF_NAME_WITH_PREFIX2}",
            "{B_SCR_NAME_WITH_PREFIX2}",
            "{B_EFF_NAME_WITH_PREFIX2}",
        }
    ),
    frozenset({"{B_ATK_ABILITY}", "{B_DEF_ABILITY}", "{B_SCR_ABILITY}"}),
)


def tokens(s: str) -> list[str]:
    return TOKEN.findall(s)


FORMAT_PREFIXES = ("PALETTE", "COLOR", "TEXT_COLORS", "BACKGROUND", "HIGHLIGHT", "SHADOW", "FONT", "SIZE",
                   "MIN_LETTER", "CLEAR", "SKIP", "PAUSE", "WAIT", "PLAY_", "RESET_")


def is_format(tok: str) -> bool:
    """Códigos que solo dan formato (color, fuente, pausa); no insertan texto."""
    return tok[1:].startswith(FORMAT_PREFIXES)


def synonym_group(tok: str) -> frozenset[str] | None:
    for g in SYNONYM_GROUPS:
        if tok in g:
            return g
    return None


def _remap_pairs(text: str, pairs: list[tuple[str, str]]) -> str:
    """Sustituye tokens ES->ORIG sin pisar sustituciones intermedias."""
    fixed = text
    for i, (e, _) in enumerate(pairs):
        fixed = fixed.replace(e, f"\x00{i}\x00", 1)
    for i, (_, o) in enumerate(pairs):
        fixed = fixed.replace(f"\x00{i}\x00", o, 1)
    return fixed


def reconcile(orig: str, es: str) -> str | None:
    """Devuelve la traducción corregida, o None si no se puede conciliar con el original.

    Los códigos que INSERTAN texto ({B_ATK_NAME_WITH_PREFIX}, {B_BUFF3}, {STR_VAR_1}...) tienen que ser los del
    original. Los de solo formato (colores, fuentes) pueden diferir, SALVO si el original usa colores DINÁMICOS
    ({TEXT_COLORS DYNAMIC_COLOR4 ...}: los pone el juego según el tipo del movimiento), que se exigen idénticos.
    """
    ot, et = tokens(orig), tokens(es)
    if Counter(ot) == Counter(et):
        return es
    if any("DYNAMIC_COLOR" in t for t in ot):
        return None
    ot = [t for t in ot if not is_format(t)]
    et = [t for t in et if not is_format(t)]
    if Counter(ot) == Counter(et):
        return es

    o_syn = [t for t in ot if synonym_group(t)]
    e_syn = [t for t in et if synonym_group(t)]
    if len(o_syn) != len(e_syn):
        return None
    for e, o in zip(e_syn, o_syn):
        if synonym_group(e) != synonym_group(o):
            return None

    o_rest = Counter(t for t in ot if not synonym_group(t) and not BUFF.fullmatch(t))
    e_rest = Counter(t for t in et if not synonym_group(t) and not BUFF.fullmatch(t))
    if o_rest != e_rest:
        return None

    o_b = sorted(set(t for t in ot if BUFF.fullmatch(t)))
    e_b = sorted(set(t for t in et if BUFF.fullmatch(t)))
    if len(o_b) != len(e_b):
        return None

    pairs: list[tuple[str, str]] = list(zip(e_syn, o_syn)) + list(zip(e_b, o_b))
    fixed = _remap_pairs(es, pairs) if pairs else es
    if Counter(t for t in tokens(fixed) if not is_format(t)) != Counter(ot):
        return None
    return fixed


def git_original(game: Path, relpath: str) -> str | None:
    """Texto original (sin HoL ni traducción) de un archivo, desde el git del árbol de la expansion."""
    try:
        return subprocess.check_output(
            ["git", "-C", str(game), "show", f"HEAD:{relpath}"], stderr=subprocess.DEVNULL
        ).decode("utf-8", errors="replace")
    except Exception:
        return None


_STR = re.compile(r'(?:static )?const u8 (\w+)\[[^\]]*\]\s*=\s*_\(((?:\s*"(?:\\.|[^"\\])*")+)\s*\)\s*;', re.S)
_ENTRY = re.compile(r'\[(STRINGID_\w+)\]\s*=\s*COMPOUND_STRING\(((?:\s*"(?:\\.|[^"\\])*")+)\s*\)', re.S)
_PIECE = re.compile(r'"((?:\\.|[^"\\])*)"')


def join_pieces(raw: str) -> str:
    return "".join(_PIECE.findall(raw))


def original_strings(game: Path, relpath: str) -> dict[str, str]:
    """{simbolo: texto original} de las cadenas `const u8 X[] = _("...")` y de gBattleStringsTable."""
    text = git_original(game, relpath)
    out: dict[str, str] = {}
    if not text:
        return out
    for m in _STR.finditer(text):
        out[m.group(1)] = join_pieces(m.group(2))
    for m in _ENTRY.finditer(text):
        out[m.group(1)] = join_pieces(m.group(2))
    return out


def rd(p: Path) -> str:
    return p.read_text(encoding="utf-8", errors="replace")


def consts(path: Path, prefix: str) -> dict[str, int]:
    text = rd(path)
    raw: dict[str, str] = {}
    for m in re.finditer(rf"\b({prefix}\w+)\s*=\s*([\w()+\- ]+?)\s*[,\n]", text):
        raw[m.group(1)] = m.group(2).strip()
    for m in re.finditer(rf"#define\s+({prefix}\w+)\s+([\w()+\-]+)", text):
        raw.setdefault(m.group(1), m.group(2).strip())
    out: dict[str, int] = {}

    def val(name: str, depth: int = 0) -> int | None:
        if name in out:
            return out[name]
        v = raw.get(name)
        if v is None or depth > 20:
            return None
        v = v.strip("()")
        if re.fullmatch(r"\d+", v):
            out[name] = int(v)
        elif re.fullmatch(r"0x[0-9A-Fa-f]+", v):
            out[name] = int(v, 16)
        elif v in raw:
            r = val(v, depth + 1)
            if r is None:
                return None
            out[name] = r
        else:
            return None
        return out[name]

    for k in list(raw):
        val(k)
    return out


def canonical_keys(mapping: dict, const_path: Path, prefix: str, target_text: str) -> dict:
    """Cambia las claves de la traducción (nombres de FireRed) por las que usa la expansion para el MISMO número.
    Ej.: FireRed MOVE_HI_JUMP_KICK == expansion MOVE_HIGH_JUMP_KICK (ambos 136). Sin esto el nombre oficial
    no se aplicaba y se quedaba otro (de PokeAPI) distinto."""
    ids = consts(const_path, prefix)
    tree_keys = set(re.findall(rf"\n\s*\[({prefix}\w+)\]\s*=", target_text))
    by_id: dict = {}
    for k in tree_keys:
        if k in ids:
            by_id.setdefault(ids[k], k)
    out: dict = {}
    for k, v in mapping.items():
        if k in tree_keys:
            out[k] = v
        elif k in ids and ids[k] in by_id:
            out.setdefault(by_id[ids[k]], v)
    # lo que ya coincidía por nombre manda sobre el alias
    for k, v in mapping.items():
        if k in tree_keys:
            out[k] = v
    return out
