#!/usr/bin/env python3
"""Genera los gráficos en castellano de la ficha del Pokémon (rótulos y tipos) en overlay/graphics/.

Se ejecuta a mano (necesita Pillow) cuando se cambia algún rótulo; los PNG resultantes se guardan en el repositorio
y scripts/apply-dailymaximo.sh los copia sobre pokeemerald-expansion/graphics/ al compilar.

    python tools/make_es_graphics.py "<ruta a pokeemerald-expansion>"

Qué hace:
  * graphics/summary_screen/tiles.png: sustituye PROFILE, ABILITY, MOVES, DESCRIPTION, TRAINER MEMO, ITEM, RIBBON,
    STATS y EFFECT por PERFIL, HABILIDAD, ATAQUES, DESCRIPCION, ENTRENADOR, OBJETO, CINTA, ESTAD. y EFECTO,
    con el mismo estilo (letra blanca de 5 px con sombra) y el mismo fondo.
  * graphics/types/<tipo>.png: los iconos de tipo (FIRE -> FUEGO, etc.).
  * graphics/battle_interface/hpbar.png: el rótulo «HP» de las barras de vida pasa a «PS» (mismo estilo de bloque
    con degradado naranja y mismo tamaño: 5 x 4 px por letra).
  * graphics/fonts/latin_small.png y latin_normal.png: el glifo «Lv» (nivel) pasa a «Nv.». Hay que subir también su
    ancho en src/fonts.c (lo hace scripts/patch-dailymaximo-hooks.py con los mismos números de aquí abajo).
"""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "pokeemerald-expansion"
OUT = ROOT / "overlay" / "graphics"

# --- Fuente 5x7 (una fila = 5 caracteres) -----------------------------------------------------------------------
F5 = {
    "A": [".XXX.", "X...X", "X...X", "XXXXX", "X...X", "X...X", "X...X"],
    "B": ["XXXX.", "X...X", "X...X", "XXXX.", "X...X", "X...X", "XXXX."],
    "C": [".XXX.", "X...X", "X....", "X....", "X....", "X...X", ".XXX."],
    "D": ["XXXX.", "X...X", "X...X", "X...X", "X...X", "X...X", "XXXX."],
    "E": ["XXXXX", "X....", "X....", "XXXX.", "X....", "X....", "XXXXX"],
    "F": ["XXXXX", "X....", "X....", "XXXX.", "X....", "X....", "X...."],
    "G": [".XXX.", "X...X", "X....", "X.XXX", "X...X", "X...X", ".XXX."],
    "H": ["X...X", "X...X", "X...X", "XXXXX", "X...X", "X...X", "X...X"],
    "I": ["XXXXX", "..X..", "..X..", "..X..", "..X..", "..X..", "XXXXX"],
    "J": ["..XXX", "...X.", "...X.", "...X.", "...X.", "X..X.", ".XX.."],
    "L": ["X....", "X....", "X....", "X....", "X....", "X....", "XXXXX"],
    "M": ["X...X", "XX.XX", "X.X.X", "X...X", "X...X", "X...X", "X...X"],
    "N": ["X...X", "XX..X", "X.X.X", "X..XX", "X...X", "X...X", "X...X"],
    "O": [".XXX.", "X...X", "X...X", "X...X", "X...X", "X...X", ".XXX."],
    "P": ["XXXX.", "X...X", "X...X", "XXXX.", "X....", "X....", "X...."],
    "Q": [".XXX.", "X...X", "X...X", "X...X", "X.X.X", "X..X.", ".XX.X"],
    "R": ["XXXX.", "X...X", "X...X", "XXXX.", "X.X..", "X..X.", "X...X"],
    "S": [".XXXX", "X....", "X....", ".XXX.", "....X", "....X", "XXXX."],
    "T": ["XXXXX", "..X..", "..X..", "..X..", "..X..", "..X..", "..X.."],
    "U": ["X...X", "X...X", "X...X", "X...X", "X...X", "X...X", ".XXX."],
    "V": ["X...X", "X...X", "X...X", "X...X", ".X.X.", ".X.X.", "..X.."],
    "Y": ["X...X", "X...X", ".X.X.", "..X..", "..X..", "..X..", "..X.."],
    ".": [".....", ".....", ".....", ".....", ".....", ".....", "..X.."],
    " ": [".....", ".....", ".....", ".....", ".....", ".....", "....."],
}

# --- Fuente 4x7 para los iconos de tipo (la I es más estrecha) --------------------------------------------------
F4 = {
    "A": [".XX.", "X..X", "X..X", "XXXX", "X..X", "X..X", "X..X"],
    "B": ["XXX.", "X..X", "X..X", "XXX.", "X..X", "X..X", "XXX."],
    "C": [".XXX", "X...", "X...", "X...", "X...", "X...", ".XXX"],
    "D": ["XXX.", "X..X", "X..X", "X..X", "X..X", "X..X", "XXX."],
    "E": ["XXXX", "X...", "X...", "XXX.", "X...", "X...", "XXXX"],
    "F": ["XXXX", "X...", "X...", "XXX.", "X...", "X...", "X..."],
    "G": [".XXX", "X...", "X...", "X.XX", "X..X", "X..X", ".XXX"],
    "H": ["X..X", "X..X", "X..X", "XXXX", "X..X", "X..X", "X..X"],
    "I": ["XXX", ".X.", ".X.", ".X.", ".X.", ".X.", "XXX"],
    "L": ["X...", "X...", "X...", "X...", "X...", "X...", "XXXX"],
    "M": ["X..X", "XXXX", "XXXX", "X..X", "X..X", "X..X", "X..X"],
    "N": ["X..X", "XX.X", "XX.X", "X.XX", "X.XX", "X..X", "X..X"],
    "O": [".XX.", "X..X", "X..X", "X..X", "X..X", "X..X", ".XX."],
    "P": ["XXX.", "X..X", "X..X", "XXX.", "X...", "X...", "X..."],
    "Q": [".XX.", "X..X", "X..X", "X..X", "X.XX", "X..X", ".XXX"],
    "R": ["XXX.", "X..X", "X..X", "XXX.", "X.X.", "X..X", "X..X"],
    "S": [".XXX", "X...", "X...", ".XX.", "...X", "...X", "XXX."],
    "T": ["XXXX", ".XX.", ".XX.", ".XX.", ".XX.", ".XX.", ".XX."],
    "U": ["X..X", "X..X", "X..X", "X..X", "X..X", "X..X", ".XX."],
    "V": ["X..X", "X..X", "X..X", "X..X", ".XX.", ".XX.", ".XX."],
    "Y": ["X..X", "X..X", ".XX.", ".XX.", ".XX.", ".XX.", ".XX."],
    ".": ["..", "..", "..", "..", "..", "..", ".X"],
}

SHADOW = ((1, 0), (0, 1), (1, 1))


def glyph_pixels(rows: list[str]) -> set[tuple[int, int]]:
    return {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c == "X"}


def draw_text(px, x0: int, y0: int, text: str, font: dict, pitch_extra: int, white: int, shadow: int, w_img: int) -> int:
    """Dibuja `text` con su sombra; devuelve el ancho usado. Las letras no pisan el fondo salvo donde dibujan."""
    x = x0
    for ch in text:
        rows = font[ch]
        pts = glyph_pixels(rows)
        for (gx, gy) in pts:
            for dx, dy in SHADOW:
                sx, sy = x + gx + dx, y0 + gy + dy
                if (gx + dx, gy + dy) not in pts and 0 <= sx < w_img and sy < y0 + 8:
                    px[sx, sy] = shadow
        for (gx, gy) in pts:
            px[x + gx, y0 + gy] = white
        x += len(rows[0]) + pitch_extra
    return x - x0 - pitch_extra


# --- 1. Rótulos de la ficha -------------------------------------------------------------------------------------
# nombre -> (x de la primera tile, y, nº de tiles disponibles, texto nuevo)
LABELS = {
    "PROFILE": (0, 64, 8, "PERFIL"),
    "ABILITY": (0, 72, 9, "HABILIDAD"),
    "MOVES": (0, 96, 8, "ATAQUES"),
    "DESCRIPTION": (0, 104, 10, "DESCRIPCION"),
    "TRAINER MEMO": (0, 112, 9, "ENTRENADOR"),
    "ITEM": (72, 64, 7, "OBJETO"),
    "RIBBON": (72, 72, 7, "CINTA"),
    "STATS": (72, 80, 7, "ESTAD."),
    "EFFECT": (80, 96, 6, "EFECTO"),
}


def bg_column_map(px, x0: int, x1: int, y0: int) -> dict[int, int]:
    """Para cada x de [x0, x1): columna de referencia sin letras (misma paridad, por el trama de puntos del fondo)."""
    glyph_cols = set()
    for x in range(x0, x1):
        for y in range(y0, y0 + 8):
            if px[x, y] in (2, 6):
                glyph_cols.add(x)
    clean = [x for x in range(x0, x1) if x not in glyph_cols]
    ref: dict[int, int] = {}
    for x in range(x0, x1):
        cands = [c for c in clean if (c - x) % 2 == 0]
        if not cands:
            raise SystemExit(f"sin fondo de referencia en x={x} y={y0}")
        ref[x] = min(cands, key=lambda c: abs(c - x))
    return ref


def make_tiles() -> None:
    src = GAME / "graphics/summary_screen/tiles.png"
    im = Image.open(src)
    assert im.mode == "P"
    px = im.load()
    for name, (x0, y0, ntiles, text) in LABELS.items():
        x1 = x0 + ntiles * 8
        width = sum(len(F5[c][0]) + 1 for c in text) - 1 + 2
        if width > ntiles * 8:
            raise SystemExit(f"«{text}» no cabe en {ntiles} tiles ({width}px)")
        ref = bg_column_map(px, x0, x1, y0)
        snapshot = {(x, y): px[ref[x], y] for x in range(x0, x1) for y in range(y0, y0 + 8)}
        for (x, y), v in snapshot.items():
            px[x, y] = v
        draw_text(px, x0 + 2, y0, text, F5, 1, 2, 6, x1)
    dst = OUT / "summary_screen/tiles.png"
    dst.parent.mkdir(parents=True, exist_ok=True)
    im.save(dst, optimize=False)
    print("  ", dst.relative_to(ROOT))


# --- 2. Iconos de tipo ------------------------------------------------------------------------------------------
TYPES = {
    "bug": "BICHO", "dark": "SINIES", "dragon": "DRAGON", "electric": "ELECTR", "fairy": "HADA", "fight": "LUCHA",
    "fire": "FUEGO", "flying": "VOLAD.", "ghost": "FANTAS", "grass": "PLANTA", "ground": "TIERRA", "ice": "HIELO",
    "normal": "NORMAL", "poison": "VENENO", "psychic": "PSIQUI", "rock": "ROCA", "steel": "ACERO", "water": "AGUA",
}


def make_types() -> None:
    for tname, text in TYPES.items():
        src = GAME / f"graphics/types/{tname}.png"
        im = Image.open(src)
        px = im.load()
        fill = px[0, 3]  # color de relleno del icono (la columna 0 no lleva letras)
        for y in range(4, 12):
            for x in range(0, 32):
                px[x, y] = fill
        width = sum(len(F4[c][0]) + 1 for c in text) - 1
        x0 = max(1, (32 - width) // 2)
        draw_text(px, x0, 4, text, F4, 1, 0xF, 0xE, 32)
        dst = OUT / f"types/{tname}.png"
        dst.parent.mkdir(parents=True, exist_ok=True)
        im.save(dst, optimize=False)
    print(f"   {len(TYPES)} iconos de tipo en overlay/graphics/types/")


# --- 3. «HP» -> «PS» en la barra de vida -------------------------------------------------------------------------
# Cada letra: 5 columnas x 4 filas; "X" = pintado. Los colores salen del degradado del original (filas 7, 7, 8, 9).
HP_LETTERS = {
    "P": ["XXXXX", "XXX.X", "XXXXX", "XXX.."],
    "S": ["XXXXX", "XXX..", "..XXX", "XXXXX"],
}
HP_ROW_COLOR = [7, 7, 8, 9]
HP_BG = 6
HP_SLOTS = (11, 17)  # x de la primera columna de cada letra en hpbar.png (la H y la P originales)


def make_hpbar() -> None:
    src = GAME / "graphics/battle_interface/hpbar.png"
    im = Image.open(src)
    assert im.mode == "P"
    px = im.load()
    for x0, ch in zip(HP_SLOTS, "PS"):
        for dy in range(4):
            for dx in range(6):  # 6 columnas: la P original ocupaba 5 y hay 1 de margen
                px[x0 + dx, 2 + dy] = HP_BG
        for dy, row in enumerate(HP_LETTERS[ch]):
            for dx, c in enumerate(row):
                if c == "X":
                    px[x0 + dx, 2 + dy] = HP_ROW_COLOR[dy]
    dst = OUT / "battle_interface/hpbar.png"
    dst.parent.mkdir(parents=True, exist_ok=True)
    im.save(dst, optimize=False)
    print("  ", dst.relative_to(ROOT))


# --- 4. «Lv» -> «Nv.» en las dos fuentes --------------------------------------------------------------------------
# Glifos 0x34 (CHAR_LV) y 0x105 (CHAR_LV_2) de cada fuente. Ancho nuevo: pequeña 9 («Nv.100» mide 24 px, igual que antes «Lv100»; con 10 se corta en el rival, igual que
# antes cabía «Lv100»), normal 14. Mantener sincronizado con FONT_LV_WIDTHS de scripts/patch-dailymaximo-hooks.py.
FONT_LV_WIDTHS = {"latin_small": 9, "latin_normal": 14}
LV_GLYPHS = (0x34, 0x105)
# v compacta de la fuente pequeña (4 columnas, filas 7-11 de la celda): 1 = tinta, 2 = sombra, 3 = fondo
SMALL_V = {7: "1212", 8: "1212", 9: "1212", 10: "2122", 11: "3223"}


def glyph_origin(gid: int) -> tuple[int, int]:
    return (gid % 16) * 16, (gid // 16) * 16


def make_fonts() -> None:
    for name, width in FONT_LV_WIDTHS.items():
        src = GAME / f"graphics/fonts/{name}.png"
        im = Image.open(src)
        px = im.load()
        ox_n, oy_n = glyph_origin(0xC8)   # N
        ox_v, oy_v = glyph_origin(0xEA)   # v
        ox_d, oy_d = glyph_origin(0xAD)   # .
        n_cells = [[px[ox_n + x, oy_n + y] for x in range(16)] for y in range(16)]
        v_cells = [[px[ox_v + x, oy_v + y] for x in range(16)] for y in range(16)]
        d_cells = [[px[ox_d + x, oy_d + y] for x in range(16)] for y in range(16)]
        small = name == "latin_small"
        rows = range(1, 13) if small else range(0, 15)
        cell = [[0] * 16 for _ in range(16)]
        for y in rows:
            for x in range(width):
                cell[y][x] = 3
        if small:
            for y in rows:                      # N: 5 columnas (4 de tinta + sombra)
                for x in range(5):
                    cell[y][x] = n_cells[y][x]
            for y, r in SMALL_V.items():        # v compacta, columnas 5..8
                for i, c in enumerate(r):
                    cell[y][5 + i] = int(c)
            for y in (10,):                     # punto: 1 px en la línea base, separado de la v
                cell[y][8] = 1
        else:
            for y in rows:                      # N (6 columnas), v (6 columnas desde x=6), punto (3 columnas desde x=12)
                for x in range(6):
                    cell[y][x] = n_cells[y][x]
                    cell[y][6 + x] = v_cells[y][x]
                for x in range(2):
                    cell[y][12 + x] = d_cells[y][x]
        for gid in LV_GLYPHS:
            ox, oy = glyph_origin(gid)
            for y in range(16):
                for x in range(16):
                    px[ox + x, oy + y] = cell[y][x]
        dst = OUT / f"fonts/{name}.png"
        dst.parent.mkdir(parents=True, exist_ok=True)
        im.save(dst, optimize=False)
        print("  ", dst.relative_to(ROOT))


def preview() -> None:
    t = Image.open(OUT / "summary_screen/tiles.png").convert("RGB").crop((0, 64, 128, 120))
    t = t.resize((t.width * 4, t.height * 4), Image.NEAREST)
    names = list(TYPES)
    sheet = Image.new("RGB", (4 * 100, 5 * 52), (255, 255, 255))
    for i, n in enumerate(names):
        ic = Image.open(OUT / f"types/{n}.png").convert("RGB").resize((96, 48), Image.NEAREST)
        sheet.paste(ic, ((i % 4) * 100, (i // 4) * 52))
    t.save(OUT.parent / "preview_tiles.png")
    sheet.save(OUT.parent / "preview_types.png")


if __name__ == "__main__":
    make_tiles()
    make_types()
    make_hpbar()
    make_fonts()
    if "--preview" in sys.argv:
        preview()
