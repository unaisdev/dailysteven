# Fase 2 — contenido Steven

**Fecha:** 19/11/2026 (actualizado tras PR #1 + port pokepaste)

## Qué hay

| ID | Estado |
|----|--------|
| GAP-F2-01…03 | **DONE** vía PR (`dailymax_rivals.party`) + port pokepaste |
| GAP-F2-06 | **DONE** en PR #1 (sala Bruno + puerta) |
| GAP-F2-04 | Parcial — `scripts/check-gen3.py` (Steven admite 6 mons) |
| GAP-F2-05 | Pendiente en **navegador** |
| GAP-F2-07 | Parcial+ — Protección + efectos TEAM (Púas/Reflejo/…) en ES; ~49 STRINGID aún EN |
| GAP-F2-08 | Pendiente |

### Equipos (pokepaste → `overlay/data/dailymax_rivals.party`)

| Matchup | STEVEN (slots) | Compañero |
|---------|----------------|-----------|
| Aura ([paste](https://pokepast.es/30fcfe031e43cb26)) | 0–2 Skarmory, Cradily, Metagross | Blaziken, Swellow, Salamence |
| Blasco ([paste](https://pokepast.es/65e82ca89516f510)) | 3–5 Skarmory, Metagross, Aggron | Altaria, Gardevoir, Milotic |

Gen3 vs paste: Focus Sash → Focus Band; Telepathy → Trace. Blaziken sin EVs → Adamant 4/252/252.

## Cómo probar

```text
powershell -File scripts\build-wsl.ps1
node scripts/serve.mjs
http://localhost:8080/?seed=1   # Blasco
http://localhost:8080/?seed=2   # Aura
```

`900001`/`900002` = ROM `-Debug` (derrota/victoria), no sondas Surf/EQ.
