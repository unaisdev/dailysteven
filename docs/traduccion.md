# Cómo se traduce Daily Steven

La traducción es nuestra (ronda 5). Todo lo que sale en pantalla está en castellano de España.

| Qué | Dónde se edita |
|---|---|
| Escena de la sala de Bruno, frases de combate, final y nombres (STEVEN, AURA, BLASCO, HORTE) | `overlay/include/dailymax_texts.h` |
| Mensajes de combate (esquivar, debilitar, habilidades, estados, clima, estadísticas…) | `locale/es/overrides_battle_es.tsv` |
| Menús, equipo, mochila, mensajes de sistema | `locale/es/overrides_strings_es.tsv` |
| Rótulos de la ficha (PERFIL, HABILIDAD…) e iconos de tipo | `tools/make_es_graphics.py` → genera `overlay/graphics/` |
| La web (portada, avisos, modo práctica) | `web/index.html` y `web/app.js` |
| Hoja para revisar frases | `docs/traduccion-revision.csv` (se regenera con `check-spanish.py --csv`) |

Formato de los `.tsv`: `CLAVE<TAB>texto`. Los códigos `{...}` tienen que ser **idénticos** a los del texto original en inglés
(la compilación falla si no). `\p` = caja nueva, `\n` = salto de línea.

## Comprobaciones al compilar (`scripts/check-spanish.py`)

La compilación **falla** si:
- algún mensaje de combate o de menú sigue igual que el original en inglés (salvo `locale/es/untranslated_allowed.txt`);
- una traducción cambia los códigos o el número de cajas del original;
- un texto contiene palabras inglesas;
- queda el nombre antiguo del rival en cualquier archivo del repositorio, o la escena tiene «PROVISIONAL»;
- los gráficos de `overlay/graphics` no son los que usa el juego.

## Regenerar la hoja de revisión

```
wsl -d Ubuntu -- bash -c "cd /mnt/c/.../dailysteven-ronda4 && python3 scripts/check-spanish.py ~/dailysteven-build/pokeemerald-expansion --csv docs/traduccion-revision.csv"
```

## Glifos, clases y animación (ronda 5B)

- **PS / Nv. / POKéMON**: el rótulo «HP» de la barra de vida y el glifo «Lv» se redibujan en `tools/make_es_graphics.py`
  (`overlay/graphics/battle_interface/hpbar.png`, `overlay/graphics/fonts/latin_*.png`); el ancho del glifo «Nv.» se fija en
  `scripts/patch-dailymaximo-hooks.py` (mismos números que `FONT_LV_WIDTHS` del generador). `{PKMN}` se sustituye por la palabra
  POKéMON en los menús (`scripts/apply_spanish_overrides.py`).
- **Clases de entrenador**: STEVEN sale como «STEVEN de la ELITE FOUR de CAMPEONES», exactamente la fórmula de GOLD en Daily Gold
  (que no era una clase del juego, sino una cadena propia); AURA y BLASCO salen como «ASPIRANTE AURA» / «ASPIRANTE BLASCO».
  Los textos están en `overlay/include/dailymax_texts.h` (`DAILYMAX_TEXT_CLASS_*`) y se enganchan en `battle_message.c`
  desde `patch-dailymaximo-hooks.py` (`DailyMax_GetNameWithClass` en `overlay/src/dailymaximo.c`).
- **Entrada del compañero**: `overlay/data/dailymax_bruno.inc` (puerta, «!», giro de STEVEN, camino, vuelta al frente).

## Pendiente conocido

- La licencia de `web/licenses.txt` está en inglés a propósito (texto legal de terceros).
