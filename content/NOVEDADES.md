# Novedades pendientes (jugador)

Hay que rellenar bullets ANTES de lanzar el workflow de release (a mano).
Solo líneas `- …` entran en el modal Novedades. CI falla si van vacíos.
Tras publicar, se vacía en una PR normal (ejecutando scripts/clear-novedades.sh); no hay commits automáticos.

**Público:** castellano claro, sin jerga. Nada de tags, SHA, CI ni nombres técnicos.
Si no lo entendería alguien que solo quiere jugar, no lo pongas.

<!-- Ejemplo:
- Arreglado: el combate del día se reiniciaba si cerrabas a medias.
- GOLD ya no usa el mismo equipo dos días seguidos.
-->
