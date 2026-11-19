# Fase 0 — informe Gen 3

**Fecha:** 19/11/2026  
**Árbol:** `pokeemerald-expansion/` local, después de `scripts/patch-dailymaximo-hooks.py`  
**Compilación:** correcta el 19/11/2026 (clean + incremental). ROM en `web/game/dailymaximo.gba.gz`. Arranque en navegador OK (ver [`veredicto-pruebas.md`](veredicto-pruebas.md)). **Daño Surf/EQ/críticos no medidos en partida**; la config Gen3 sí está activa en el árbol.

## Qué se cambió

El parche Daily, al reaplicarse, deja estos defines (HoL los había dejado en Gen 4):

| Define | Antes | Ahora | Para qué |
|--------|-------|-------|----------|
| `GEN_LATEST` | `GEN_4` | `GEN_3` | Crítico, físico/especial, Protect, cambio al caer, daño en área, tabla de tipos y el resto de `B_*` / `I_*` / `P_*` que siguen `GEN_LATEST` |
| `P_GEN_4_POKEMON` | `TRUE` | `FALSE` | Familias nuevas de 4.ª gen fuera del juego |
| `B_SHOW_CATEGORY_ICON` | `TRUE` | `FALSE` | El resumen no muestra el icono de categoría. `GEN_LATEST` no lo apagaba solo (RI-04) |

`P_GEN_4_CROSS_EVOS` se deja en `TRUE`. Gallade es una evolución cruzada de Ralts y el equipo provisional de Blasco (Hall of Legends) lo usa. Apagarlo borra los datos de la especie y deja el hueco del equipo vacío. Se quita cuando GAP-F2 sustituya ese Pokémon.

## Qué debería hacer el motor con `GEN_LATEST GEN_3`

Según los comentarios de `include/config/battle.h` del propio árbol. Hay que confirmarlo en combate.

| Caso | Esperado (DEC-0.4 + spec) | Flag |
|------|---------------------------|------|
| Surf en dobles | Golpea a los dos rivales, no al aliado, y el daño baja al 50 % | `B_MULTIPLE_TARGETS_DMG`: en Gen 3 el golpe a varios objetivos es 50 %; el que golpea a todo el campo no se reduce |
| Terremoto en dobles | Golpea al resto del campo, aliado incluido, **sin** esa reducción | mismo flag |
| Crítico | ×2, no ×1.5 | `B_CRIT_MULTIPLIER` |
| Lucha / Fuego | El tipo decide si es físico o especial | `B_PHYSICAL_SPECIAL_SPLIT` |
| Acero vs Fantasma y Siniestro | Resistencia de 3.ª gen | `B_UPDATED_TYPE_MATCHUPS` |
| Protección dos veces | Falla la mitad de las veces, no 1/3 | `B_PROTECT_FAILURE_RATE` |
| Al caer uno | El sustituto entra en el acto, no al final del turno | `B_FAINT_SWITCH_IN` |
| Resumen | Sin icono de categoría | `B_SHOW_CATEGORY_ICON FALSE` |

## Lo que `GEN_LATEST` no arregla

Siguen fijados a `GEN_4` a propósito de Hall of Legends y no cambian con esta fase:

- `I_BERRY_PRICE`, `I_EXP_SHARE_ITEM` (`include/config/item.h`)
- `P_SHOW_TERA_TYPE`, `P_TM_LITERACY` (`include/config/pokemon.h`)

Los movimientos que no existían en 3.ª gen (Cabeza de Hierro, Respiro, etc.) siguen en las tablas. El equipo provisional de Hall of Legends todavía puede enseñarlos. Eso es GAP-F2-04 / RI-10, no esta fase.

## Puerta

No dar la Fase 0 por cerrada hasta que una ROM compilada con este árbol cumpla la tabla de arriba. Si la compilación falla, parar y no seguir con contenido.
