#!/usr/bin/env python3
r"""Comprobador de traducción de Daily Steven: la compilación FALLA si algún texto alcanzable sigue en inglés,
si una traducción cambia los códigos {...} del original o si queda algún «MÁXIMO».

  python3 scripts/check-spanish.py pokeemerald-expansion            # comprueba (lo llama build.sh)
  python3 scripts/check-spanish.py pokeemerald-expansion --csv docs/traduccion-revision.csv   # genera la hoja de revisión

Qué comprueba:
  1. Mensajes de combate (src/battle_message.c): TODAS las entradas de la tabla gBattleStringsTable y todas las
     constantes sText_*/gText_*. Ninguna puede ser idéntica al original en inglés (salvo la lista blanca
     locale/es/untranslated_allowed.txt, que debe estar vacía o justificada) ni contener palabras inglesas.
  2. Los códigos {...} de cada traducción coinciden con los del original (mismos códigos, mismo nº de \p).
  3. Textos de menús/ficha/equipo/mochila (src/strings.c): todos los que referencian los archivos que se
     ven en Daily Steven tienen que estar traducidos.
  4. Escena y final (overlay/include/dailymax_texts.h): sin «MÁXIMO», sin PROVISIONAL, sin palabras inglesas,
     frases de combate dentro del búfer (máx. 200 caracteres) y los 4 nombres definitivos.
  5. Ficha de datos: sin página de concursos (lo vigila también check-gen3.py).
  5a. Descripciones de los 354 movimientos de tercera generación.
  5b. Glifos y clases: «PS» (no HP) en barras y menús, «Nv.» (no Lv), POKéMON (no el símbolo PKMN) y las clases de
      entrenador (STEVEN: «ELITE FOUR de CAMPEONES» como GOLD; AURA y BLASCO: «ASPIRANTE»; nunca «CAMPEÓN»).
  6. En todo el repositorio no queda «MÁXIMO».
"""
from __future__ import annotations

import csv
import glob
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import locale_guard as lg  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
args = [a for a in sys.argv[1:] if not a.startswith("--")]
GAME = Path(args[0] if args else ROOT / "pokeemerald-expansion")
CSV_OUT = None
if "--csv" in sys.argv:
    CSV_OUT = Path(sys.argv[sys.argv.index("--csv") + 1])
    args = [a for a in args if a != str(CSV_OUT)]
    GAME = Path(args[0] if args else ROOT / "pokeemerald-expansion")
LOCALE = ROOT / "locale" / "es"

# Palabras que NO existen en castellano: si aparecen en un texto, ese texto está (en parte) en inglés.
ENGLISH_WORDS = re.compile(
    r"\b(the|The|its|Its|was|were|and|your|Your|you|You|have|are|with|from|that|this|their|there|will|would|"
    r"fainted|avoided|attack|used|but|into|can't|cannot|don't|won't|didn't|isn't|doesn't|it's|It's|which|"
    r"nothing|happened|affected|raised|lowered|restored|health|opposing|ally|enemy|sent|came|back|switch|"
    r"wild|trainer|battle|lost|appeared|about|after|before|because|while|when|again|already|all|any|more|"
    r"some|than|them|they|what|who|how|why|not|out|up|down|off|over)\b"
)
# Falsos positivos legítimos: nombres propios o siglas dentro de textos en castellano.
SPANISH_HINT = re.compile(r"[¡¿áéíóúñÁÉÍÓÚÑ]|\b(el|la|los|las|de|del|un|una|que|se|su|sus|ha|han|es|con|por|para|al|en|no|lo|te)\b")

errors: list[str] = []


def fail(msg: str) -> None:
    errors.append(msg)


def plain(text: str) -> str:
    return re.sub(r"\\[a-z]", " ", re.sub(r"\{[^{}]*\}", " ", text))


def has_letters(text: str) -> bool:
    return bool(re.search(r"[A-Za-zÁÉÍÓÚÑáéíóúñ]{3,}", plain(text)))


def non_format_tokens(text: str) -> Counter:
    # {PKMN} se ignora: en los menús se sustituye por la palabra POKéMON (como en el Rojo Fuego español).
    return Counter(t for t in lg.tokens(text) if not lg.is_format(t) and t != "{PKMN}")


def parse_symbols(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for m in lg._STR.finditer(text):
        out[m.group(1)] = lg.join_pieces(m.group(2))
    for m in lg._ENTRY.finditer(text):
        out[m.group(1)] = lg.join_pieces(m.group(2))
    return out


def table_symbol_map(text: str) -> dict[str, str]:
    """STRINGID_X -> constante a la que apunta (entradas sin COMPOUND_STRING propio)."""
    m = re.search(r"gBattleStringsTable\[STRINGID_COUNT\]\s*=\s*\{(.*?)\n\};", text, re.S)
    out: dict[str, str] = {}
    if m:
        for e in re.finditer(r"\[(STRINGID_\w+)\]\s*=\s*(?!COMPOUND_STRING)(\w+)\s*,", m.group(1)):
            out[e.group(1)] = e.group(2)
    return out


def load_allowed() -> dict[str, str]:
    out: dict[str, str] = {}
    p = LOCALE / "untranslated_allowed.txt"
    if p.is_file():
        for line in p.read_text(encoding="utf-8").splitlines():
            if line.strip() and not line.lstrip().startswith("#"):
                k, _, why = line.partition("\t")
                out[k.strip()] = why.strip()
    return out


def load_tsv(name: str) -> dict[str, str]:
    p = LOCALE / name
    out: dict[str, str] = {}
    if p.is_file():
        for line in p.read_text(encoding="utf-8").splitlines():
            if line.strip() and not line.lstrip().startswith("#") and "\t" in line:
                k, t = line.split("\t", 1)
                out[k.strip()] = t
    return out


def check_move_descriptions() -> int:
    rel = "src/data/moves_info.h"
    current = (GAME / rel).read_text(encoding="utf-8")
    original = lg.git_original(GAME, rel) or ""
    ids = lg.gen3_move_ids(GAME / "include/constants/moves.h")
    last = ids["MOVE_PSYCHO_BOOST"]

    def fields(text: str) -> dict[str, list[str]]:
        return {
            key: [block[start:end].strip() for start, end in lg.move_description_spans(block)]
            for key, block in lg.MOVE_INFO_ENTRY.findall(text)
            if key in ids
        }

    expected = fields(original)
    actual = fields(current)
    for label, entries in (("original", expected), ("generado", actual)):
        if {ids[key] for key in entries} != set(range(1, last + 1)):
            fail(f"{rel}: tabla de movimientos de tercera generación incompleta ({label})")
    for key, variants in actual.items():
        if not variants or len(variants) != len(expected.get(key, [])):
            fail(f"{rel}: {key} tiene variantes de descripción ausentes")
        for field in variants:
            if not re.fullmatch(r'COMPOUND_STRING\(\s*(?:"(?:\\.|[^"\\])*"\s*)+\),', field):
                fail(f"{rel}: {key} conserva una variante condicional sin traducir")
            elif field in expected.get(key, []) or ENGLISH_WORDS.search(plain(lg.join_pieces(field))):
                fail(f"{rel}: {key} tiene una descripción sin traducir")
    return len(actual)


# ----------------------------------------------------------------------------------------------------------------
def check_file(rel: str, tsv: dict[str, str], allowed: dict[str, str], only: set[str] | None):
    """Comprueba un archivo de textos del juego. Devuelve (actual, original) por clave."""
    path = GAME / rel
    cur = parse_symbols(path.read_text(encoding="utf-8"))
    orig = lg.original_strings(GAME, rel)
    for key, es in tsv.items():
        if key in cur and cur[key] != es:
            fail(f"{rel}: {key} no tiene la traducción de locale/es ({cur[key][:60]}…): ¿se aplicó apply_spanish_overrides.py?")
    for key, text in cur.items():
        if only is not None and key not in only:
            continue
        o = orig.get(key)
        if o is None:
            o = text  # constante añadida por nuestros parches (no existe en la expansion)
            if key in tsv:
                continue  # ya la traduce locale/es y se comprueba arriba
        if text == o and has_letters(text) and key not in allowed:
            fail(f"{rel}: {key} SIN TRADUCIR: «{text[:90]}»")
            continue
        if text != o and key not in allowed and non_format_tokens(text) != non_format_tokens(o):
            fail(f"{rel}: {key} cambia los códigos del original.\n    original:   {o}\n    traducción: {text}")
        if text != o and text.count("\\p") != o.count("\\p") and key not in allowed:
            fail(f"{rel}: {key} cambia el nº de cajas (\\p) del original.\n    original:   {o}\n    traducción: {text}")
        if key not in allowed:
            words = ENGLISH_WORDS.findall(plain(text))
            if words:
                fail(f"{rel}: {key} contiene palabras inglesas {sorted(set(words))}: «{text[:100]}»")
    return cur, orig


def referenced_names(patterns: list[str]) -> set[str]:
    names: set[str] = set()
    for pat in patterns:
        for f in glob.glob(str(GAME / pat)):
            names |= set(re.findall(r"\b(?:g|s)Text_\w+", Path(f).read_text(encoding="utf-8", errors="replace")))
    return names


# Archivos de código que se ven en Daily Steven; todo texto de strings.c que usen debe estar en castellano.
VISIBLE_SOURCES = [
    "src/battle_*.c", "src/pokemon_summary_screen.c", "src/party_menu.c", "src/item_menu.c", "src/item_use.c",
    "src/menu.c", "src/menu_helpers.c", "src/start_menu.c", "src/field_message_box.c", "src/main_menu.c",
    "src/hall_of_legends.c", "src/script_menu.c",
]


# Literales de texto escritos directamente en el código C del combate y de la ficha (no pasan por las tablas).
C_LITERAL_FILES = [
    "src/battle_interface.c", "src/battle_controller_player.c", "src/battle_main.c", "src/battle_script_commands.c",
    "src/battle_util.c", "src/pokemon_summary_screen.c",
]


def check_c_literals() -> None:
    for rel in C_LITERAL_FILES:
        p = GAME / rel
        if not p.is_file():
            continue
        text = re.sub(r"//[^\n]*", "", p.read_text(encoding="utf-8"))
        for m in re.finditer(r'(?:_|COMPOUND_STRING)\(((?:\s*"(?:\\.|[^"\\])*")+)\s*\)', text):
            lit = lg.join_pieces(m.group(1))
            if re.fullmatch(r"[\s'{}A-Za-z_0-9]*", lit) and not ENGLISH_WORDS.search(plain(lit)) and "'s" not in lit:
                continue
            if ENGLISH_WORDS.search(plain(lit)) or re.search(r"\w'[sS]\b", lit):
                fail(f"{rel}: texto escrito en el código con inglés: «{lit[:80]}»")


def check_graphics() -> None:
    """Rótulos y tipos de la ficha en castellano (los dibuja tools/make_es_graphics.py en overlay/graphics)."""
    for f in ["summary_screen/tiles.png", "battle_interface/hpbar.png", "fonts/latin_small.png", "fonts/latin_normal.png"] + [f"types/{t}.png" for t in (
            "bug", "dark", "dragon", "electric", "fight", "fire", "flying", "ghost", "grass", "ground", "ice",
            "normal", "poison", "psychic", "rock", "steel", "water")]:
        a = ROOT / "overlay/graphics" / f
        b = GAME / "graphics" / f
        if not a.is_file():
            fail(f"gráficos: falta overlay/graphics/{f} (ejecuta tools/make_es_graphics.py)")
        elif b.is_file() and a.read_bytes() != b.read_bytes():
            fail(f"gráficos: graphics/{f} del juego no es el de overlay/graphics (¿no se aplicó el overlay?)")


def check_dailymax_texts() -> dict[str, str]:
    p = ROOT / "overlay/include/dailymax_texts.h"
    text = p.read_text(encoding="utf-8")
    defs: dict[str, str] = {}
    for m in re.finditer(r"#define\s+(DAILYMAX_(?:TEXT|NAME)_\w+)((?:[ \t]*\\\n|[ \t]+\"[^\n]*\n?|[ \t]*\"[^\n]*\n?)+)", text):
        pieces = re.findall(r'"((?:\\.|[^"\\])*)"', m.group(2))
        defs[m.group(1)] = "".join(pieces)
    names = {k: v for k, v in defs.items() if k.startswith("DAILYMAX_NAME_")}
    want = {"DAILYMAX_NAME_BOSS": "STEVEN", "DAILYMAX_NAME_AURA": "AURA", "DAILYMAX_NAME_BLASCO": "BLASCO", "DAILYMAX_NAME_PLAYER": "HORTE"}
    for k, v in want.items():
        if names.get(k) != v:
            fail(f"dailymax_texts.h: {k} debe ser «{v}» (vale «{names.get(k)}»)")
    if re.search(r"PROVISIONAL", text, re.I):
        fail("dailymax_texts.h: todavía pone PROVISIONAL")
    for k, v in defs.items():
        if k.startswith("DAILYMAX_TEXT_"):
            if not v:
                fail(f"dailymax_texts.h: {k} está vacío o no se ha podido leer")
            words = ENGLISH_WORDS.findall(plain(v))
            if words:
                fail(f"dailymax_texts.h: {k} contiene palabras inglesas {sorted(set(words))}")
            if ("_DEFEAT" in k or "_VICTORY" in k) and len(v) > 200:
                fail(f"dailymax_texts.h: {k} mide {len(v)} caracteres y el búfer de combate admite ~250 bytes (máx. 200)")
    return defs


def check_glyphs_and_classes(cur_s: dict[str, str], visible_keys: set[str]) -> None:
    """«HP» -> «PS», «Lv» -> «Nv.», «PKMN» -> POKéMON y clases de entrenador (STEVEN como GOLD; ASPIRANTE)."""
    if cur_s.get("gText_HP3") != "PS":
        fail(f"strings.c: gText_HP3 debe ser «PS» (vale «{cur_s.get('gText_HP3')}»)")
    if "HP" in (cur_s.get("gText_MaxHP") or ""):
        fail("strings.c: gText_MaxHP todavía lleva «HP»")
    for key in ("gText_DoWhatWithPokemon", "gText_CantSelectSamePkmn", "gText_PkmnForSwap", "gText_SamePkmnInPartyAlready"):
        v = cur_s.get(key, "")
        if "{PKMN}" in v or "POKéMON" not in v:
            fail(f"strings.c: {key} debe llevar la palabra POKéMON y no el símbolo {{PKMN}} (vale «{v}»)")
    for key, v in cur_s.items():
        if key not in visible_keys:
            continue
        if re.search(r"\bHP\b", plain(v)) and key not in ("gText_NextFusionMon",):
            fail(f"strings.c: {key} lleva «HP» (en castellano es PS): «{v[:60]}»")
    # ancho de los glifos Nv. en src/fonts.c
    fonts = (GAME / "src/fonts.c").read_text(encoding="utf-8")
    for table, want in (("gFontSmallLatinGlyphWidths", 9), ("gFontNormalLatinGlyphWidths", 14)):
        m = re.search(table + r"\[\] = \{(.*?)\n\};", fonts, re.S)
        nums = [int(x) for x in re.findall(r"\d+", m.group(1))] if m else []
        if len(nums) != 512 or nums[0x34] != want or nums[0x105] != want:
            fail(f"fonts.c: el glifo Nv. de {table} debería medir {want} px (¿no se aplicó patch-dailymaximo-hooks.py?)")
    # clases de entrenador
    texts = (ROOT / "overlay/include/dailymax_texts.h").read_text(encoding="utf-8")
    for macro, want in (("DAILYMAX_TEXT_CLASS_BOSS", "ELITE FOUR de CAMPEONES"), ("DAILYMAX_TEXT_CLASS_PARTNER", "ASPIRANTE")):
        m = re.search(r'#define\s+' + macro + r'\s+"([^"]*)"', texts)
        if not m or m.group(1) != want:
            fail(f"dailymax_texts.h: {macro} debe ser «{want}» (vale «{m.group(1) if m else 'no existe'}»)")
    if re.search(r"DAILYMAX_TEXT_(CLASS|BOSS_WITH)\w*[ \t]+[^\n]*CAMPE[ÓO]N\b", texts):
        fail("dailymax_texts.h: «CAMPEÓN» delante del nombre de un rival")
    bmsg = (GAME / "src/battle_message.c").read_text(encoding="utf-8")
    if "DailyMax_GetNameWithClass" not in bmsg or "DailyMax_GetClassName" not in bmsg:
        fail("battle_message.c: falta el enganche de las clases de entrenador (patch-dailymaximo-hooks.py)")
    for sym in ("sText_TwoTrainersWantToBattle", "sText_TwoTrainersSentPkmn", "sText_Trainer2SentOutPkmn",
                "sText_Trainer1SentOutPkmn2", "sText_Trainer1WithdrewPkmn", "sText_Trainer2WithdrewPkmn"):
        m = re.search(r"const u8 " + sym + r"\[\] = _\(((?:\s*\"(?:\\.|[^\"\\])*\")+)\)", bmsg)
        if not m or "_NAME_WITH_CLASS" not in m.group(1):
            fail(f"battle_message.c: {sym} debe mostrar la clase del entrenador ({{B_TRAINERx_NAME_WITH_CLASS}})")
    dm = (GAME / "src/dailymaximo.c").read_text(encoding="utf-8") if (GAME / "src/dailymaximo.c").is_file() else ""
    if "ELITE FOUR de CAMPEONES" not in dm.replace('" DAILYMAX', "") and "DAILYMAX_TEXT_BOSS_WITH_CLASS" not in dm:
        fail("dailymaximo.c: no define la clase de STEVEN")


def check_no_maximo() -> None:
    skip_parts = ("pokeemerald-expansion", ".git", "vendor", ".cache", "node_modules", "game", "out")
    for f in ROOT.rglob("*"):
        if not f.is_file() or any(part in skip_parts for part in f.relative_to(ROOT).parts):
            continue
        if f.suffix.lower() not in (".c", ".h", ".inc", ".py", ".sh", ".js", ".mjs", ".html", ".css", ".md", ".json", ".txt", ".tsv", ".csv", ".party", ".ps1"):
            continue
        if f.name == "check-spanish.py":
            continue
        try:
            t = f.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        if "MÁXIMO" in t or "Daily Máximo" in t:
            fail(f"{f.relative_to(ROOT)}: aparece «MÁXIMO» (el rival se llama STEVEN)")


# ----------------------------------------------------------------------------------------------------------------
ZONE_HINTS = [
    (r"^(sText|gText)_(Trainer|Link|InGame|HoL|TwoLink|Opponent|PlayerLost|PlayerBattled|PlayerDefeated|TwoInGame|Legendary|JustALittle|BeCareful|PkmnSwitch|Wild)", "Combate: entrenadores / sacar y retirar"),
    (r"^gText_(BattleMenu|Battle(Yes|Catch|Switch)|MoveInterface|WhichMove|WhatWill|Judg|Win|Loss|Draw|Safari)", "Combate: menús de elección"),
    (r"^gText_(Menu|Saving)", "Menú de inicio / guardado"),
    (r"^gText_(Summary|Switch|Egg|Odd|Peculiar)", "Ficha / equipo"),
    (r"^gText_(Pkmn|ExpShare|Throw|Item|XsY|NoMore|BagFull|SpAtk|SpDef)", "Equipo / mochila"),
    (r"^STRINGID_(STAT|ATTACKERABILITY|SCRIPTINGABILITY|TARGETABILITY|USINGITEM)", "Combate: cambios de estadísticas"),
    (r"^STRINGID_(.*FAINT|BATTLERFAINTED)", "Combate: Pokémon debilitado"),
    (r"^STRINGID_(.*AVOID|.*MISS|.*EFFECTIVE|.*AFFECT|CRITICAL)", "Combate: resultado de un ataque"),
    (r"^STRINGID_(.*SUN|.*RAIN|.*HAIL|.*SNOW|.*SAND|.*FOG|.*TERRAIN|.*GRAVITY|.*ROOM|.*WIND)", "Combate: clima y campo"),
    (r"^STRINGID_(.*POISON|.*BURN|.*PARALY|.*FROZEN|.*FROST|.*SLEEP|.*ASLEEP|.*CONFUS|.*INFATU|.*WOKE|.*CURED)", "Combate: estados"),
    (r"^STRINGID_(PRESSURE|PKMNTRACED|INTIM|.*ABILITY|.*ENTERS|.*ACTIVATES)", "Combate: habilidades"),
    (r"^STRINGID_", "Combate: mensaje del turno"),
]


STRINGS_ONLY = ("Menú de inicio", "Ficha / equipo", "Equipo / mochila")


def where(key: str, from_strings: bool = False) -> str:
    for pat, label in ZONE_HINTS:
        if label.startswith(STRINGS_ONLY) and not from_strings:
            continue
        if re.search(pat, key):
            return label
    return "Combate: menús y mensajes" if not from_strings else "Menús del juego"


def write_csv(rows: list[list[str]]) -> None:
    CSV_OUT.parent.mkdir(parents=True, exist_ok=True)
    with CSV_OUT.open("w", encoding="utf-8-sig", newline="") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["identificador", "texto original (inglés)", "traducción actual", "dónde sale", "origen de la traducción"])
        w.writerows(rows)
    print(f"  hoja de revisión: {CSV_OUT} ({len(rows)} filas)")


WEB_TEXTS = [
    ("web/index.html · portada", "Wins / Losses / Win streak / Day streak", "Victorias / Derrotas / Racha de victorias / Días seguidos"),
    ("web/index.html · portada", "Play", "Jugar"),
    ("web/index.html · portada", "Practice run: results aren't saved.", "Modo práctica: los resultados no se guardan."),
    ("web/index.html · carga", "Downloading the game", "Descargando el juego"),
    ("web/app.js · carga", "Starting the emulator", "Iniciando el emulador"),
    ("web/app.js · carga", "{n} of {total} MB", "{n} de {total} MB"),
    ("web/index.html · error", "The game didn't start / Try again", "El juego no ha arrancado / Reintentar"),
    ("web/app.js · error", "Check your connection and try again.", "Comprueba tu conexión e inténtalo de nuevo."),
    ("web/app.js · resultado del día", "You beat STEVEN today!", "¡Has vencido a STEVEN hoy!"),
    ("web/app.js · resultado del día", "STEVEN won today.", "STEVEN ha ganado hoy."),
    ("web/app.js · cuenta atrás", "Next battle in hh:mm:ss", "Próximo combate en hh:mm:ss"),
    ("web/index.html · barra", "Mute / Unmute", "Silenciar / Activar sonido"),
    ("web/index.html · barra", "Full screen / Exit full screen", "Pantalla completa / Salir de pantalla completa"),
    ("web/index.html · guía", "Keyboard: Walk, move the cursor / A button: talk, confirm / B button: back, cancel / Start: open the menu", "Teclado: Andar, mover el cursor / Botón A: hablar, aceptar / Botón B: volver, cancelar / Start: abrir el menú"),
    ("web/index.html · guía", "On a phone or tablet, use the buttons under the screen.", "En el móvil o la tablet, usa los botones bajo la pantalla."),
    ("web/index.html · pie", "Licenses / Source", "Licencias / Código"),
    ("web/app.js · error", "This browser can't run the game. Use a recent version of Chrome, Edge, Firefox or Safari.", "Este navegador no puede ejecutar el juego. Usa una versión reciente de Chrome, Edge, Firefox o Safari."),
]


def main() -> None:
    allowed = load_allowed()
    rows: list[list[str]] = []

    # 1-2. Combate
    battle_tsv = load_tsv("overrides_battle_es.tsv")
    cur_b, orig_b = check_file("src/battle_message.c", battle_tsv, allowed, None)
    symmap = table_symbol_map((GAME / "src/battle_message.c").read_text(encoding="utf-8"))

    # 3. strings.c: solo lo que referencian los archivos visibles
    strings_tsv = load_tsv("overrides_strings_es.tsv")
    visible = referenced_names(VISIBLE_SOURCES)
    cur_s = parse_symbols((GAME / "src/strings.c").read_text(encoding="utf-8"))
    only = {k for k in cur_s if k in visible or k in strings_tsv}
    cur_s, orig_s = check_file("src/strings.c", strings_tsv, allowed, only)

    # 3b. Clases de entrenador (CHAMPION -> CAMPEÓN, etc.)
    bm = (GAME / "src/battle_main.c").read_text(encoding="utf-8")
    for key, es in load_tsv("overrides_classes_es.tsv").items():
        m = re.search(r"\[" + key + r"\]\s*=\s*\{\s*_\(\"([^\"]*)\"\)", bm)
        if not m or m.group(1) != es:
            fail(f"src/battle_main.c: la clase {key} no está traducida a «{es}» (vale «{m.group(1) if m else 'no existe'}»)")

    # 4. Escena y final, literales del código C y gráficos
    defs = check_dailymax_texts()
    move_descriptions = check_move_descriptions()
    check_c_literals()
    check_graphics()
    check_glyphs_and_classes(cur_s, only | set(strings_tsv))

    # 6. MÁXIMO
    check_no_maximo()

    if CSV_OUT:
        for key, text in cur_b.items():
            o = orig_b.get(key, text)
            if text != o or key in battle_tsv:
                src = "propia (locale/es/overrides_battle_es.tsv)" if key in battle_tsv else "FireRed oficial (Acimut)"
                rows.append([key, o, text, where(key), src])
        for key, text in cur_s.items():
            o = orig_s.get(key, text)
            if key in strings_tsv:
                rows.append([key, o, text, where(key, True), "propia (locale/es/overrides_strings_es.tsv)"])
        spots = {
            "DAILYMAX_TEXT_BOSS_TALK_1": "Sala de Bruno: STEVEN habla (caja 1)", "DAILYMAX_TEXT_BOSS_TALK_2": "Sala de Bruno: STEVEN habla (caja 2)",
            "DAILYMAX_TEXT_BOSS_TALK_3": "Sala de Bruno: STEVEN habla (caja 3)", "DAILYMAX_TEXT_AURA_ENTER": "Sala de Bruno: AURA entra",
            "DAILYMAX_TEXT_BLASCO_ENTER": "Sala de Bruno: BLASCO entra", "DAILYMAX_TEXT_AURA_AFTER_1": "Sala de Bruno: AURA junto a STEVEN",
            "DAILYMAX_TEXT_AURA_AFTER_2": "Sala de Bruno: STEVEN responde a AURA", "DAILYMAX_TEXT_BLASCO_AFTER_1": "Sala de Bruno: STEVEN habla a BLASCO",
            "DAILYMAX_TEXT_BLASCO_AFTER_2": "Sala de Bruno: BLASCO responde", "DAILYMAX_TEXT_BOSS_DEFEAT": "Combate: STEVEN pierde",
            "DAILYMAX_TEXT_BOSS_VICTORY": "Combate: STEVEN gana", "DAILYMAX_TEXT_AURA_DEFEAT": "Combate: AURA pierde",
            "DAILYMAX_TEXT_AURA_VICTORY": "Combate: AURA gana", "DAILYMAX_TEXT_BLASCO_DEFEAT": "Combate: BLASCO pierde",
            "DAILYMAX_TEXT_BLASCO_VICTORY": "Combate: BLASCO gana", "DAILYMAX_TEXT_RESULT_WON": "Final: titular al ganar",
            "DAILYMAX_TEXT_RESULT_LOST": "Final: titular al perder", "DAILYMAX_TEXT_RECORD": "Final: récord", "DAILYMAX_TEXT_OUTRO_WON": "Final: despedida al ganar",
            "DAILYMAX_TEXT_OUTRO_LOST": "Final: despedida al perder",
        }
        for k, v in defs.items():
            if k.startswith("DAILYMAX_TEXT_"):
                rows.append([k, "(texto definitivo del proyecto, escrito en castellano)", v, spots.get(k, "Escena / final"), "definitivo (overlay/include/dailymax_texts.h)"])
        gfx = [("PROFILE", "PERFIL"), ("ABILITY", "HABILIDAD"), ("TRAINER MEMO", "ENTRENADOR"), ("ITEM", "OBJETO"), ("RIBBON", "CINTA"),
               ("STATS", "ESTAD."), ("MOVES", "ATAQUES"), ("DESCRIPTION", "DESCRIPCION"), ("EFFECT", "EFECTO")]
        for en, es in gfx:
            rows.append(["gráfico summary_screen/tiles.png", en, es, "Ficha del Pokémon (rótulo dibujado)", "propia (tools/make_es_graphics.py)"])
        for t, es in (("BUG", "BICHO"), ("DARK", "SINIES"), ("DRAGON", "DRAGON"), ("ELECTRIC", "ELECTR"), ("FIGHT", "LUCHA"), ("FIRE", "FUEGO"),
                      ("FLYING", "VOLAD."), ("GHOST", "FANTAS"), ("GRASS", "PLANTA"), ("GROUND", "TIERRA"), ("ICE", "HIELO"), ("NORMAL", "NORMAL"),
                      ("POISON", "VENENO"), ("PSYCHIC", "PSIQUI"), ("ROCK", "ROCA"), ("STEEL", "ACERO"), ("WATER", "AGUA")):
            rows.append(["gráfico types/" + t.lower() + ".png", t, es, "Ficha del Pokémon (icono de tipo)", "propia (tools/make_es_graphics.py)"])
        for where_, en, es in WEB_TEXTS:
            rows.append(["web", en, es, where_, "propia (web/)"])
        write_csv(rows)

    if errors:
        print(f"check-spanish: {len(errors)} problema(s) de traducción:", file=sys.stderr)
        for e in errors[:80]:
            print("  -", e, file=sys.stderr)
        if len(errors) > 80:
            print(f"  … y {len(errors) - 80} más", file=sys.stderr)
        raise SystemExit(1)
    print(f"check-spanish OK: {len(cur_b)} textos de combate, {len(only)} de menús, {len(defs)} de escena/final, "
          f"{move_descriptions} descripciones de movimientos; "
          f"traducciones propias: {len(battle_tsv) + len(strings_tsv)}; sin ningún «MÁXIMO».")


if __name__ == "__main__":
    main()
