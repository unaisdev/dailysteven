# Fase 1 — informe daily-core

**Fecha:** 19/11/2026  
**Deps:** DEC-0 cerrado; Fase 0 puerta cerrada (Surf/EQ verificados).

## Qué cambió

| ID | Cambio |
|----|--------|
| GAP-F1-01 | Sin cambio de código: DEC-0.2 confirma `DAILYMAX-CONF-V1` / `DAILYMAX-STATUS1` / `dailymax-conf-v1+status-v1`. Adaptador launcher = GAP-F4-06. |
| GAP-F1-02 | `AssignRandomMoveset`: pool = level-up (≤100) + teachable (TM/HM/tutor) + egg; 4 moves al azar; ≥1 daño si el pool lo permite. |
| GAP-F1-03 | `AssignRandomItem`: `sBattleItems` siempre elegibles; type boosters solo si hay un move de daño de ese tipo. |
| GAP-F1-04 | `MIN_TEAM_BASE_STAT_TOTAL 2700` marcado provisional (comentario + este doc). Recalibración = GAP-F3-02 / RI-16. |
| GAP-F1-05 | Sin cambio: el día se consume al entrar en `DAILYMAX_PHASE_BATTLE` (`DailyMax_OnBattleStart` → web `markBattleStarted`). Práctica `?seed=` no escribe stats. |

## Cómo probar

```text
bash scripts/_docker_build.sh   # Windows
node scripts/serve.mjs
http://localhost:8080/?seed=3   # equipo generado (no sonda)
```

**Build:** OK 19/11/2026 — `web/game/dailymaximo.gba.gz` regenerada (`dailymaximo_team.o` enlazado).

En el menú de equipo / resumen: movimientos que no son solo level-up final (p.ej. TMs) y objetos que pueden ser potenciadores de tipo.

Sondas Gen3 siguen: `?seed=900001` / `900002`.

## Qué falta (P2)

- GAP-F1-06 party menu SUMMARY/SWITCH/CANCEL forzado en daily
- GAP-F1-07 separar constantes de contenido del núcleo

## Discrepancias

- El pool teachable de la expansión puede incluir moves posteriores a Gen3; el filtro legal Gen3 de rivales/jugador es GAP-F2-04.
- BST 2700 ≠ 2850 del DOCX a propósito hasta simulación.
