# Veredicto de pruebas (19/11/2026)

Pruebas en el navegador sobre `web/game/dailymaximo.gba.gz` recompilada tras Gen3 + textos. Semillas de práctica `?seed=1` y `?seed=2`.

## Qué pasó y se corrigió

| Hallazgo | Veredicto | Acción |
|----------|-----------|--------|
| Intro en inglés: «You are challenged by CHAMPION STEVEN and CHAMPION WALLY» | **Mal** | Textos de dobles pasados a castellano; nombres de combate de MAY/WALLY → **AURA** / **BLASCO** en `trainers_frlg.party` + `trainers_frlg.h` |
| Tras el arreglo: «¡STEVEN y BLASCO quieren combatir!» (`seed=1`) | **OK** | Capturado en pantalla |
| Tras el arreglo: «¡STEVEN y AURA quieren combatir!» (`seed=2`) | **OK** | Capturado en pantalla |
| «¡STEVEN envió a Metagross!» | **OK** | Visto con Blasco y con Aura |
| Blasco saca Gardevoir; Aura saca Blaziken | **OK** | Equipos provisionales HoL; `seed & 1` elige compañero |
| Combate doble 1 vs 2, dos sprites al inicio | **OK** | |
| Debilitar y cambiar (menú de equipo, «¡Adelante, Masquerain!») | **OK** | |
| Portada Daily Steven, práctica sin guardar | **OK** | |

## Gen 3

Con `GEN_LATEST GEN_3` en el árbol compilado:

| Caso | Esperado | Estado |
|------|----------|--------|
| Surf a dos rivales, 50 %, sin aliado | `TARGET_BOTH` + modificador 0.5 | **OK en partida** (`?seed=900001` Starmie): aliado Wobbuffet sin daño |
| Terremoto al campo, aliado incluido, sin esa reducción | `TARGET_FOES_AND_ALLY` + modificador 1.0 si ≥3 | **OK en partida** (`?seed=900002` Dugtrio): aliado pierde PS; combate sigue |
| Freeze «usó TERREMOTO» (ES) | Texto de ataque con nombre del movimiento | **Corregido** (`sText_AttackerUsedX` → `{B_BUFF3}`) |
| Crítico ×2 | `B_CRIT_MULTIPLIER` vía Gen3 | **Config OK** |
| Físico/especial por tipo | `B_PHYSICAL_SPECIAL_SPLIT` | **Config OK** |
| Acero resiste Fantasma/Siniestro | `STL_RS` con matchups Gen3 | **Config OK** |
| Protección 50 % al repetir | tabla Gen3/4 | **Config OK** |
| Cambio al caer en el acto | `B_FAINT_SWITCH_IN` Gen3 | **Config OK** |
| Sin icono de categoría en resumen | `B_SHOW_CATEGORY_ICON FALSE` | **Config OK**; no abierto el resumen en esta sesión |

## Pendiente / fuera de esta ronda

- Sala de Bruno + cinemática (DEC-0.3): sigue la sala del campeón y el combate al instante.
- Frase exacta «¡BLASCO/AURA envió a …!» en frame: el Pokémon sale; el subtítulo se saltó al avanzar el texto.
- Moves ilegales Gen4 en partidos HoL (Gallade / Iron Head / Roost): validador Fase 2.
- Pruebas dirigidas de IA (Fase 3).

## Cómo reproducir

```text
node scripts/serve.mjs
http://localhost:8080/?seed=1        # Blasco + Gardevoir
http://localhost:8080/?seed=2        # Aura + Blaziken
http://localhost:8080/?seed=900001   # sonda Surf (Starmie)
http://localhost:8080/?seed=900002   # sonda Terremoto (Dugtrio)
```
