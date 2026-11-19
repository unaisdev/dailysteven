#!/usr/bin/env python3
"""Translate expansion gBattleStringsTable by STRINGID using Acimut ES FireRed texts."""
from __future__ import annotations

import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import locale_guard  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
GAME = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "pokeemerald-expansion")
LOCALE = ROOT / "locale" / "es"
CACHE = ROOT / ".cache" / "pret_europe"
EUROPE_URL = "https://github.com/Acimut/pokefirered.git"
EUROPE_COMMIT = "6651454608dcc61356b60c20154214d773689730"

STR_LIT = re.compile(
    r'(?:static )?const u8 (\w+)\[\] = (_\((?:\"(?:\\.|[^\"\\])*\"\s*)+\));',
    re.S,
)

PLACEHOLDER_REMAP = {
    "B_SCR_ACTIVE_NAME_WITH_PREFIX": "B_SCR_NAME_WITH_PREFIX",
    "B_SCR_ACTIVE_ABILITY": "B_SCR_ABILITY",
    "B_ACTIVE_NAME_WITH_PREFIX": "B_BUFF1",
}

SPARSE = [
    "/src/battle_message.c",
    "/src/data/text/battle_message_1_en.h",
    "/src/data/text/battle_message_2_en.h",
    "/src/data/text/battle_message_1_es.h",
    "/src/data/text/battle_message_2_es.h",
]


def run(cmd: list[str]) -> None:
    subprocess.check_call(cmd)


def ensure_cache() -> None:
    need = CACHE / "src/data/text/battle_message_1_en.h"
    if need.is_file():
        return
    if CACHE.exists():
        shutil.rmtree(CACHE)
    CACHE.mkdir(parents=True)
    run(["git", "init", "-q", str(CACHE)])
    run(["git", "-C", str(CACHE), "remote", "add", "origin", EUROPE_URL])
    run(["git", "-C", str(CACHE), "config", "core.sparseCheckout", "true"])
    (CACHE / ".git/info/sparse-checkout").write_text("\n".join(SPARSE) + "\n")
    print(f">> Fetching pret_europe @{EUROPE_COMMIT[:10]} EN/ES battle strings")
    run(["git", "-C", str(CACHE), "fetch", "-q", "--depth", "1", "origin", EUROPE_COMMIT])
    run(["git", "-C", str(CACHE), "checkout", "-q", "FETCH_HEAD"])


def parse_literals(text: str) -> dict[str, str]:
    return {m.group(1): m.group(2) for m in STR_LIT.finditer(text)}


def inner_string(lit: str) -> str:
    lit = lit.strip().rstrip(";")
    if not (lit.startswith("_(") and lit.endswith(")")):
        return lit
    parts = re.findall(r'\"((?:\\.|[^\"\\])*)\"', lit[2:-1])
    return "".join(parts)


def remap_placeholders(s: str) -> str:
    for old, new in PLACEHOLDER_REMAP.items():
        s = s.replace("{" + old + "}", "{" + new + "}")
    return s


def c_escape(s: str) -> str:
    """Write a C string body. Keep charmap controls (\n \p \l …) as a single backslash."""
    # Real newlines (from broken prior runs) → \n control.
    s = s.replace("\r\n", "\n").replace("\r", "\n")
    out: list[str] = []
    i = 0
    while i < len(s):
        ch = s[i]
        if ch == "\n":
            out.append("\\n")
            i += 1
            continue
        if ch == "\t":
            out.append("\\t")
            i += 1
            continue
        if ch == '"':
            out.append('\\"')
            i += 1
            continue
        if ch == "\\" and i + 1 < len(s):
            nxt = s[i + 1]
            # Already-decoded C escapes / pret charmap controls — keep one backslash.
            if nxt in "npl\"\\0123456789" or nxt.isalpha():
                out.append("\\")
                out.append(nxt)
                i += 2
                continue
            out.append("\\\\")
            i += 1
            continue
        if ch == "\\":
            out.append("\\\\")
            i += 1
            continue
        out.append(ch)
        i += 1
    return "".join(out)


def load_stringid_to_stext(battle_c: str) -> dict[str, str]:
    """STRINGID_FOO -> sText_Bar from FireRed gBattleStringsTable."""
    out: dict[str, str] = {}
    for m in re.finditer(
        r"\[(STRINGID_\w+)\s*-\s*BATTLESTRINGS_TABLE_START\]\s*=\s*(sText_\w+)",
        battle_c,
    ):
        out[m.group(1)] = m.group(2)
    return out


def main() -> None:
    path = GAME / "src/battle_message.c"
    if not path.is_file():
        raise SystemExit(f"missing {path}")

    ensure_cache()

    en = {}
    es = {}
    for lang, bucket in (("en", en), ("es", es)):
        for name in (f"battle_message_1_{lang}.h", f"battle_message_2_{lang}.h"):
            p = CACHE / "src/data/text" / name
            if not p.is_file() and lang == "es":
                p = LOCALE / "src/data/text" / name
            if p.is_file():
                bucket.update(parse_literals(p.read_text(encoding="utf-8")))

    id_to_sym = load_stringid_to_stext(
        (CACHE / "src/battle_message.c").read_text(encoding="utf-8")
    )
    print(f"  FireRed STRINGID map: {len(id_to_sym)}, EN lits: {len(en)}, ES lits: {len(es)}")

    # STRINGID -> Spanish inner text
    id_to_es: dict[str, str] = {}
    for sid, sym in id_to_sym.items():
        if sym not in es:
            continue
        id_to_es[sid] = remap_placeholders(inner_string(es[sym]))

    # Manuales: expansion-only, o FireRed con códigos de equipo distintos (TEAM vs PREFIX).
    # Deben usar EXACTAMENTE los {...} del original de expansion (p. ej. B_*_TEAM*).
    # Sobrescriben FireRed cuando este no pasa locale_guard (efectos de campo en inglés).
    manual = {
        "STRINGID_ITSUPEREFFECTIVE": "¡Es muy eficaz!",
        "STRINGID_NOTVERYEFFECTIVE": "No es muy eficaz...",
        "STRINGID_ONEHITKO": "¡K.O. en 1 golpe!",
        "STRINGID_CRITICALHIT": "¡Un golpe crítico!",
        "STRINGID_ANHITXTIMESBINUSE": "¡El Pokémon recibió {B_BUFF1} golpe{B_BUFF2}!",
        # Pantallas / velo / púas (Gen3) — {B_*_TEAM*} → «tu equipo» / «el equipo enemigo»
        "STRINGID_PKMNRAISEDDEF": "¡REFLEJO aumentó la DEFENSA de {B_ATK_TEAM2}!",
        "STRINGID_PKMNRAISEDSPDEF": "¡PANTALLA LUZ aumentó la DEF. ESP. de {B_ATK_TEAM2}!",
        "STRINGID_PKMNAURORAVEIL": "¡VELO AURORA fortaleció a {B_ATK_TEAM2}!",
        "STRINGID_PKMNCOVEREDBYVEIL": "¡{B_ATK_TEAM1} se protegió con VELO SAGRADO!",
        "STRINGID_PKMNSAFEGUARDEXPIRED": "¡{B_ATK_TEAM1} ya no está protegido por VELO SAGRADO!",
        "STRINGID_SPIKESSCATTERED": "¡Se han esparcido PÚAS alrededor de {B_DEF_TEAM2}!",
        "STRINGID_PKMNSXWOREOFF": "¡Se agotó el efecto de {B_BUFF1} en {B_ATK_TEAM1}!",
        "STRINGID_REFLECTWOREOFF": "¡{B_DEF_TEAM1} ya no tiene REFLEJO!",
        "STRINGID_LIGHTSCREENWOREOFF": "¡{B_DEF_TEAM1} ya no tiene PANTALLA LUZ!",
        "STRINGID_AURORAVEILWOREOFF": "¡{B_DEF_TEAM1} ya no tiene VELO AURORA!",
        # Hazards / efectos de campo posteriores (misma familia TEAM)
        "STRINGID_POISONSPIKESSCATTERED": "¡Se han esparcido PÚAS TÓXICAS alrededor de {B_DEF_TEAM2}!",
        "STRINGID_POINTEDSTONESFLOAT": "¡Flotan piedras puntiagudas sobre {B_DEF_TEAM2}!",
        "STRINGID_STICKYWEBUSED": "¡Se ha tendido una RED VISCOSA alrededor de {B_DEF_TEAM2}!",
        "STRINGID_PROTECTEDTEAM": "¡{B_CURRENT_MOVE} protege ahora a {B_ATK_TEAM2}!",
        "STRINGID_TAILWINDBLEW": "¡Sopla un VIENTO AFÍN a favor de {B_ATK_TEAM2}!",
        "STRINGID_TAILWINDENDS": "¡Se ha agotado el VIENTO AFÍN de {B_ATK_TEAM1}!",
        "STRINGID_SHIELDEDFROMCRITICALHITS": "¡CONJURO ocultó a {B_ATK_TEAM2} de los críticos!",
        "STRINGID_LUCKYCHANTENDS": "¡Se agotó el CONJURO de {B_ATK_TEAM1}!",
        "STRINGID_UNNERVEENTERS": "¡{B_EFF_TEAM1} está demasiado nervioso para comer Bayas!",
        "STRINGID_TOXICSPIKESABSORBED": "¡Desaparecieron las PÚAS TÓXICAS de alrededor de {B_EFF_TEAM2}!",
        "STRINGID_SPIKESDISAPPEAREDFROMTEAM": "¡Desaparecieron las PÚAS de alrededor de {B_ATK_TEAM2}!",
        "STRINGID_TOXICSPIKESDISAPPEAREDFROMTEAM": "¡Desaparecieron las PÚAS TÓXICAS de alrededor de {B_ATK_TEAM2}!",
        "STRINGID_STICKYWEBDISAPPEAREDFROMTEAM": "¡Desapareció la RED VISCOSA de alrededor de {B_ATK_TEAM2}!",
        "STRINGID_STEALTHROCKDISAPPEAREDFROMTEAM": "¡Desaparecieron las piedras puntiagudas de {B_ATK_TEAM2}!",
        "STRINGID_SHARPSTEELFLOATS": "¡Flotan trozos de acero afilado alrededor de {B_DEF_TEAM2}!",
        "STRINGID_SHARPSTEELDISAPPEAREDFROMTEAM": "¡Desaparecieron los trozos de acero de {B_ATK_TEAM2}!",
        "STRINGID_TEAMTRAPPEDWITHVINES": "¡Los Pokémon de {B_EFF_TEAM1} quedaron atrapados por enredaderas!",
        "STRINGID_TEAMCAUGHTINVORTEX": "¡Los Pokémon de {B_EFF_TEAM1} quedaron atrapados en un torbellino!",
        "STRINGID_TEAMSURROUNDEDBYFIRE": "¡Los Pokémon de {B_EFF_TEAM1} quedaron rodeados de fuego!",
        "STRINGID_TEAMSURROUNDEDBYROCKS": "¡Los Pokémon de {B_EFF_TEAM1} quedaron rodeados de rocas!",
        "STRINGID_ARAINBOWAPPEAREDONSIDE": "¡Apareció un arcoíris sobre {B_EFF_TEAM2}!",
        "STRINGID_THERAINBOWDISAPPEARED": "¡Desapareció el arcoíris de {B_ATK_TEAM2}!",
        "STRINGID_SEAOFFIREENVELOPEDSIDE": "¡Un mar de fuego envolvió a {B_EFF_TEAM2}!",
        "STRINGID_THESEAOFFIREDISAPPEARED": "¡Desapareció el mar de fuego de {B_ATK_TEAM2}!",
        "STRINGID_SWAMPENVELOPEDSIDE": "¡Un pantano envolvió a {B_EFF_TEAM2}!",
        "STRINGID_THESWAMPDISAPPEARED": "¡Desapareció el pantano de {B_ATK_TEAM2}!",
    }
    id_to_es.update(manual)

    text = path.read_text(encoding="utf-8")
    m = re.search(
        r"(const u8 \*const gBattleStringsTable\[STRINGID_COUNT\]\s*=\s*\{)(.*?)(\n\};)",
        text,
        re.S,
    )
    if not m:
        raise SystemExit("gBattleStringsTable not found")

    body = m.group(2)
    replaced = 0
    guard_skipped: list[str] = []
    originals = locale_guard.original_strings(GAME, "src/battle_message.c")

    def repl_entry(em: re.Match[str]) -> str:
        nonlocal replaced
        sid = em.group(1)
        if sid not in id_to_es:
            return em.group(0)
        es_text = id_to_es[sid]
        if not es_text:
            return em.group(0)
        if sid in originals:
            fixed = locale_guard.reconcile(originals[sid], es_text)
            if fixed is None:
                guard_skipped.append(sid)
                return em.group(0)
            es_text = fixed
        replaced += 1
        return f'[{sid}] = COMPOUND_STRING("{c_escape(es_text)}")'

    body2 = re.sub(
        r"\[(STRINGID_\w+)\]\s*=\s*COMPOUND_STRING\((?:\s*\"(?:\\.|[^\"\\])*\"\s*)+\)",
        repl_entry,
        body,
    )

    text = text[: m.start(2)] + body2 + text[m.end(2) :]
    path.write_text(text, encoding="utf-8")
    print(f"  gBattleStringsTable: translated {replaced} entries, {len(guard_skipped)} left in original language (different codes)")
    cache = ROOT / ".cache"
    cache.mkdir(exist_ok=True)
    with open(cache / "locale-skipped.txt", "a", encoding="utf-8") as f:
        f.write(chr(10).join("table: " + x for x in guard_skipped) + chr(10))


if __name__ == "__main__":
    main()
