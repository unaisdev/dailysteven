#!/usr/bin/env python3
r"""Comprueba que los datos de aprendizaje del generador de equipos del jugador son los de POKÉMON ESMERALDA.

  python3 scripts/check-tmhm.py [pokeemerald-expansion]

La compilación FALLA si:
  1. overlay/include/dailymax_emerald.h no coincide, especie por especie (1-386), con tests/reference/emerald_learnsets.json
     (la referencia, extraída de pret/pokeemerald con tools/emerald_reference.py) en MT/MO ni en movimientos por nivel;
  2. la lista sGen3Machines de overlay/src/dailymaximo_team.c no es la de las 50 MT + 8 MO de 3.ª gen, en su orden;
  3. el generador vuelve a usar las tablas de la expansion (CanLearnTeachableMove / GetSpeciesLevelUpLearnset);
  4. el árbol de la expansion tiene una copia distinta del .h (no se aplicó el overlay);
  5. si hay un checkout de pokeemerald a mano (variable DAILYSTEVEN_EMERALD o ~/refs/pokeemerald), la referencia tampoco
     coincide con sus archivos (el commit comprobado es el guardado en la referencia).
"""
from __future__ import annotations

import json
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "pokeemerald-expansion"
errors: list[str] = []


def fail(msg: str) -> None:
    errors.append(msg)


def main() -> None:
    ref = json.loads((ROOT / "tests/reference/emerald_learnsets.json").read_text(encoding="utf-8"))
    machines: list[str] = ref["maquinas"]
    mt: dict[str, list[str]] = ref["mt"]
    lvl: dict[str, list[list]] = ref["nivel"]
    if len(machines) != 58 or len(mt) != 386 or len(lvl) != 386:
        fail(f"la referencia debería tener 58 máquinas y 386 especies (tiene {len(machines)}, {len(mt)} y {len(lvl)})")

    header = (ROOT / "overlay/include/dailymax_emerald.h").read_text(encoding="utf-8")
    # --- MT/MO ---
    masks = {m.group(1): int(m.group(2), 16) | (int(m.group(3), 16) << 32)
             for m in re.finditer(r"\[(SPECIES_\w+)\] = \{ 0x([0-9A-F]{8}), 0x([0-9A-F]{8}) \}", header)}
    for sp, mvs in mt.items():
        want = 0
        for mv in mvs:
            want |= 1 << machines.index(mv)
        got = masks.get(sp)
        if got is None:
            fail(f"dailymax_emerald.h: falta {sp} en la tabla de MT/MO")
        elif got != want:
            have = [machines[i] for i in range(58) if got >> i & 1]
            fail(f"dailymax_emerald.h: MT/MO de {sp} no coinciden con Esmeralda. Sobran {sorted(set(have) - set(mvs))}, "
                 f"faltan {sorted(set(mvs) - set(have))}")
    for sp in masks:
        if sp not in mt:
            fail(f"dailymax_emerald.h: {sp} no está en la referencia")

    # --- movimientos por nivel ---
    m = re.search(r"sEmeraldLevelUpStart\[[^\]]*\] =\s*\{(.*?)\};", header, re.S)
    starts = [int(x) for x in re.findall(r"\d+", m.group(1))] if m else []
    m = re.search(r"sEmeraldLevelUpMoves\[\] =\s*\{(.*?)\};", header, re.S)
    flat = re.findall(r"MOVE_\w+", m.group(1)) if m else []
    species_ids = {}
    for sp in mt:
        species_ids[sp] = None
    # el índice de cada especie sale del orden de la tabla de máscaras (están ordenadas por número de especie)
    order = list(masks)
    if len(starts) != 388:
        fail(f"dailymax_emerald.h: sEmeraldLevelUpStart debería tener 388 entradas (tiene {len(starts)})")
    else:
        pos = 0
        for idx in range(1, 387):
            sp = order[idx - 1]
            got = flat[starts[idx]:starts[idx + 1]]
            want = [mv for _, mv in lvl.get(sp, [])]
            if got != want:
                fail(f"dailymax_emerald.h: movimientos por nivel de {sp} no coinciden con Esmeralda "
                     f"(sobran {sorted(set(got) - set(want))}, faltan {sorted(set(want) - set(got))})")
            pos += 1

    team = (ROOT / "overlay/src/dailymaximo_team.c").read_text(encoding="utf-8")
    m = re.search(r"sGen3Machines\[\] =\s*\{(.*?)\};", team, re.S)
    listed = re.findall(r"MOVE_\w+", m.group(1)) if m else []
    if listed != machines:
        fail("dailymaximo_team.c: sGen3Machines no es la lista de 50 MT + 8 MO de 3.ª gen (mismo orden que Esmeralda). "
             f"Difiere en: {[(i, a, b) for i, (a, b) in enumerate(zip(listed, machines)) if a != b][:5]}")
    for bad in ("CanLearnTeachableMove", "GetSpeciesLevelUpLearnset"):
        if bad in team:
            fail(f"dailymaximo_team.c: el generador del jugador vuelve a usar {bad} (tabla de la expansion)")
    for need in ("CanLearnEmeraldMachine", "sEmeraldLevelUpMoves"):
        if need not in team:
            fail(f"dailymaximo_team.c: el generador no usa los datos de Esmeralda ({need})")

    copy = GAME / "include/dailymax_emerald.h"
    if copy.is_file() and copy.read_bytes().replace(b"\r\n", b"\n") != (ROOT / "overlay/include/dailymax_emerald.h").read_bytes().replace(b"\r\n", b"\n"):
        fail("include/dailymax_emerald.h del juego no es el de overlay/ (¿no se aplicó apply-dailymaximo.sh?)")

    em = Path(os.environ.get("DAILYSTEVEN_EMERALD", str(Path.home() / "refs/pokeemerald")))
    if (em / "src/data/pokemon/tmhm_learnsets.h").is_file():
        sys.path.insert(0, str(ROOT / "tools"))
        import emerald_reference as er  # noqa: E402
        fresh, _ = er.build(em, GAME)
        for key in ("maquinas", "mt", "nivel"):
            if fresh[key] != ref[key]:
                fail(f"la referencia ({key}) no coincide con el pokeemerald de {em} (commit {fresh['commit'][:7]}; "
                     f"la referencia usa {ref['commit'][:7]}). Regenera con tools/emerald_reference.py --write")
        note = f"y con {em} @ {fresh['commit'][:7]}"
    else:
        note = "(sin checkout de pokeemerald a mano: solo se comprobó contra tests/reference/emerald_learnsets.json)"

    if errors:
        print(f"check-tmhm: {len(errors)} problema(s):", file=sys.stderr)
        for e in errors[:30]:
            print("  -", e, file=sys.stderr)
        raise SystemExit(1)
    print(f"check-tmhm OK: 386 especies (58 MT/MO y movimientos por nivel) de Esmeralda @ {ref['commit'][:7]} coinciden "
          f"con la referencia {note}.")


if __name__ == "__main__":
    main()
