# Pending — handoff Daily Steven

**Última actualización:** 20/11/2026 (fix ES efectos de campo / TEAM + Your-Opposing)  
**Para quién:** siguiente compi / subagente. No reabrir DEC-0 ni OPEN-P* sin decisión nueva.

Fuente de verdad del backlog: [`gap-analisis-daily-steven.md`](gap-analisis-daily-steven.md).  
Spec + RI: [`Daily_Steven_instrucciones_y_revision_independiente.md`](Daily_Steven_instrucciones_y_revision_independiente.md).

---

## Dónde estamos

| Fase | Estado |
|------|--------|
| DOC / DEC-0 | **Hecho** |
| **PR #1** (`trabajo/ronda-1`) | **Mergeada en main** — Bruno+cinemática, HORTE, final tipo Red, generador estilo Red (MT/MO Gen3), check-gen3/names, locale_guard, ROM `-Debug` |
| Fase 0 Gen3 | Cubierta por PR + config; Surf/EQ observados en sesión previa |
| Port post-PR | Pokepastes definitivos en `.party`, docs, fix `B_BUFF3` ES, Steven 6 slots (Aura/Blasco) |
| Fase 3–4 | **Pendiente** (ver abajo) |

**Siguiente ID típico:** `GAP-F2-05` (battery RI-05 en navegador) o `GAP-F4-01` / smoke tests.

---

## Hecho (no repetir)

### De la PR #1
- Escena sala Bruno + compañero por puerta; consumo día al combate
- Final victoria/derrota tipo Daily Red; textos en `dailymax_texts.h`
- HORTE (Spenser); Gen3 estricta + ayudas modernas off; `check-gen3.py` / `check-names.py` / `locale_guard.py`
- Generador jugador: level + 58 MT/MO Gen3, type boosters, BST 2700
- Debug ROM: `?debug=1&seed=900001` derrota (Magikarp), `900002` victoria (rivales 1 PS)
- Build: `scripts\build-wsl.ps1`

### Port encima (esta sesión)
- Rivales pokepaste en `overlay/data/dailymax_rivals.party` (Steven 6 mons: Aura 0–2 / Blasco 3–5)
- `DailyMax_GetStevenIconicSlots` + gancho en `HoL_CreateOpponentParties`
- Fix ES freeze movimiento: `sText_AttackerUsedX` → `{B_BUFF3}` (`locale/es/...`)
- Fix ES Protección mixta: `locale_guard` remapea DEF↔SCR (y roles afines) en `gBattleStringsTable`
- Fix ES efectos de campo: `sText_Your*`/`Opposing*` + manuals TEAM (Púas, pantallas, hazards)
- Fix ES ficha habilidades: `UI_OVERRIDES` (HABIL. POKéMON, PS/ATAQUE/…, PUNTOS EXP.)
- Docs: `pending.md`, informes fase 0/1/2, veredicto, gap, `AGENTS.md`, `NOVEDADES`

| Matchup | STEVEN | Compañero | Paste |
|---------|--------|-----------|-------|
| Aura (`seed` par / `?seed=2`) | Skarmory, Cradily, Metagross | Blaziken, Swellow, Salamence | https://pokepast.es/30fcfe031e43cb26 |
| Blasco (`seed` impar / `?seed=1`) | Skarmory, Metagross, Aggron | Altaria, Gardevoir, Milotic | https://pokepast.es/65e82ca89516f510 |

Ajustes Gen3 vs paste: Focus Sash → Focus Band; Telepathy → Trace.

---

## Pendiente por ID

### Fase 1 P2
| ID | Qué |
|----|-----|
| GAP-F1-06 | Party menu SUMMARY/SWITCH/CANCEL |
| GAP-F1-07 | Separar constantes contenido vs núcleo |

### Fase 2 restos
| ID | Pri | Qué | Notas |
|----|-----|-----|-------|
| GAP-F2-05 | P0 | Battery RI-05 en **navegador** | PR probó mGBA desktop; falta web + día bloqueado |
| GAP-F2-04 | P0 | Validador | Ya existe `check-gen3.py`; ampliar si hace falta |
| GAP-F2-07 | P1 | Textos ES | Parcial+: efectos TEAM (Púas/Reflejo/…) + Your/Opposing ES; quedan ~49 STRINGID irreconciliables (habilidades vía BUFF, etc.) |
| GAP-F2-08 | P2 | Reset práctica in-room | Con escena Bruno ya hay; ¿reinicio sin reload? |

### Fase 3
| ID | Qué |
|----|-----|
| GAP-F3-01 | Casos IA dirigidos |
| GAP-F3-02 | Proponer BST (sigue 2700 provisional) |

### Fase 4
| ID | Qué |
|----|-----|
| GAP-F4-01 | Claves stats/muted RI-07 |
| GAP-F4-04 | Tests smoke/site |
| GAP-F4-02 | Archive / team-preview |
| GAP-F4-03 | Inventario locale |
| GAP-F4-05 | CI release + NOVEDADES |
| GAP-F4-06 | Pack launcher + regresión Red/Gold |

---

## Reglas rápidas

1. Leer este archivo + gap + informe de fase.
2. No pisar `900001`/`900002`: son derrota/victoria de la ROM `-Debug`, **no** sondas Surf/EQ.
3. Editar rivales solo en `overlay/data/dailymax_rivals.party` (Steven = 6).
4. Antes de push: `content/NOVEDADES.md`.
5. Compilar: `powershell -File scripts\build-wsl.ps1` (o Docker/`_docker_build.sh`).

### Prompt mínimo

```text
Repo: dailymaximo
Lee docs/pending.md y docs/gap-analisis-daily-steven.md
Trabaja SOLO los IDs: …
No toques OPEN-P* ni DEC-0.
Al terminar: actualizar pending.md + gap + informe + NOVEDADES si aplica.
```
