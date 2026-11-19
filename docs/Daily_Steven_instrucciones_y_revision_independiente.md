# Daily Steven — instrucciones para el agente

Nov 17, 2026 · @Horte

> **Historial y procedencia de este documento**  
> **Versión original:** 17/11/2026, `@Horte`. El contenido de las secciones originales se conserva como especificación inicial, incluidas sus decisiones, pendientes y posibles discrepancias.  
> **Conversión a Markdown y revisión independiente:** 19/11/2026, **segundo agente revisor (ChatGPT/GPT-6)**, diferente del agente que redactó la primera versión.  
> **Importante:** el **Anexo A** recoge observaciones, verificaciones y propuestas de este segundo agente. **No sustituye ni modifica automáticamente** las decisiones del autor original. Ante diferencias, el equipo deberá confirmarlas antes de aplicarlas en código.  
> **Estado de la revisión:** documental y de código fuente; **no se ha ejecutado una compilación ni una ROM de prueba** para esta revisión.

---

## Contexto y objetivo

Hay que crear **Daily Steven**, un tercer juego para el launcher Hortelano's Daily: un combate doble diario contra STEVEN y un compañero que cambia cada día (AURA o WALLY). Este documento es la especificación completa para el agente que lo implemente. Todas las decisiones de diseño están cerradas, salvo los equipos, que se entregarán aparte.

- **Daily Red / Daily Gold (ya existen):** un combate individual al día contra un jefe, al estilo Wordle. El jugador recibe 6 Pokémon aleatorios de nivel 100, iguales para todos ese día. Están hechos sobre [pret/pokefirered](https://github.com/pret/pokefirered) (el código del FireRed original) más un parche. Repo de referencia: unaisdev/dailygold (carpeta local dailylance). Su README explica toda la arquitectura.

- **Hall of Legends** ([amoi-builds/halloflegends](https://github.com/amoi-builds/halloflegends)): otro proyecto, hecho sobre [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion) en modo FireRed. Ya trae combates contra dos entrenadores a la vez, a Steven, Aura (May en inglés), Wally y Cynthia, la sala de Bruno y una IA de combate fuerte.

- **Daily Steven = la parte "Daily" de Daily Gold + el motor y el contenido de Hall of Legends.**

- Después vendrá **Daily Cynthia** con mecánicas de 4.ª generación. Por eso la base que se construya aquí tiene que ser reutilizable.

Regla para el agente: si algo de este documento choca con lo que encuentres en el código, para y pregunta. No inventes reglas de juego.

## Decisiones ya tomadas

Estos requisitos están cerrados; no se cambian sin consultar.

| Tema                     | Decisión                                                                                                                                                                                                                                                                                            |
|--------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Generación               | Todo hasta la **3.ª generación**: especies n.º 1–386, movimientos, habilidades y objetos de 3.ª gen, y mecánicas de combate de 3.ª gen.                                                                                                                                                             |
| Formato                  | **Combate doble**: el jugador solo, con sus 6 Pokémon, contra **dos entrenadores**: STEVEN siempre y un compañero.                                                                                                                                                                                  |
| Compañero                | **AURA o WALLY**, elegido al azar cada día a partir de la semilla diaria.                                                                                                                                                                                                                           |
| Equipos rivales          | Cada entrenador lleva **3 Pokémon: 2 fijos + 1 variante** elegida entre 2 opciones. Hay 2 tríos posibles por entrenador y 8 combates distintos (2 compañeros × 2 tríos de Steven × 2 tríos del compañero). La elección sale de la semilla.                                                          |
| Equipos (contenido)      | Inspirados en los de Hall of Legends; **los entregará el equipo**. Mientras tanto, usar equipos provisionales marcados como tales.                                                                                                                                                                  |
| Equipos fusionados       | Tras el primer envío, los 3 + 3 forman **un solo equipo de 6**. Cuando cae uno, puede salir **cualquiera** de los 6, sea de quien sea (igual que hace Hall of Legends).                                                                                                                             |
| Escenario                | **Sala de Bruno** de la Liga Pokémon.                                                                                                                                                                                                                                                               |
| Escena                   | Al entrar, **solo está Steven**. El jugador habla con él y Steven dice su frase. Entonces el compañero **entra por la puerta del fondo** (la que lleva a la sala siguiente, detrás de Steven), camina hasta **ponerse a su lado**, mira al jugador, dice **una última frase** y empieza el combate. |
| Inicio del combate       | Salen los **dos sprites** de los rivales. Mensaje "STEVEN y AURA quieren combatir" (o WALLY).                                                                                                                                                                                                       |
| Envíos                   | Cada mensaje nombra al **dueño real** del Pokémon: "STEVEN envió a METAGROSS", "AURA envió a BLAZIKEN". También en los cambios tras caer uno.                                                                                                                                                       |
| Final del combate        | Salen los **dos sprites** y **cada uno dice su propia frase** (de victoria o de derrota).                                                                                                                                                                                                           |
| Portada (web y launcher) | Muestra **solo a Steven**. No se revela el compañero ni sus Pokémon hasta que el jugador habla con Steven dentro del juego.                                                                                                                                                                         |
| Producto                 | **Juego nuevo** en el launcher: repo propio, content pack propio y estadísticas propias. Daily Red y Daily Gold no se tocan.                                                                                                                                                                        |
| Jugador                  | Igual que en Daily Gold: un combate al día, equipo aleatorio por semilla, abandonar a mitad cuenta como derrota y modo práctica con ?seed=.                                                                                                                                                         |

## Arquitectura elegida

La base es **pokeemerald-expansion en modo FireRed**, partiendo de Hall of Legends, con la capa Daily de Daily Gold llevada encima. **No** se hace sobre pret/pokefirered como Daily Gold.

**Por qué:**

- **IA:** la IA de dobles del FireRed original está vacía. Su guion AI_DoubleBattle es solo end y elige el objetivo a cara o cruz. La IA de la expansion es la que usan los hacks difíciles con código abierto.

- **Combate contra dos entrenadores:** FireRed no lo tiene. La expansion sí lo tiene (BATTLE_TYPE_TWO_OPPONENTS), y Hall of Legends ya resuelve la fusión de equipos y los textos.

- **Daily Cynthia** necesitará 4.ª gen, que solo es viable con la expansion. La capa Daily se porta una sola vez y sirve para los dos juegos.

**Cómo se consigue la 3.ª generación:**

- En include/config/general.h, GEN_LATEST debe ser GEN_3. Hall of Legends usa GEN_4. De ese valor dependen unas 238 opciones de include/config/battle.h: físico/especial según el tipo (B_PHYSICAL_SPECIAL_SPLIT), críticos ×2, tabla de tipos antigua, datos de movimientos, Protección, el momento del cambio tras caer un Pokémon, el daño a varios objetivos, etc.

- En include/config/species_enabled.h, desactivar las especies de 4.ª gen en adelante.

- Revisar también los ajustes de objetos (I\_\*) y de Pokémon (P\_\*) para que no se cuele nada posterior a la 3.ª gen.

- Asumido y aceptado: es una imitación configurable de la 3.ª gen, no el motor original. Puede haber diferencias mínimas (redondeos de 1–2 PS, casos raros).

**Cómo se organiza el código:**

- Igual que Daily Gold y Hall of Legends: el repo **no guarda el motor**, solo un parche sobre un commit fijado (Hall of Legends usa 8fb02c6 de la expansion) y scripts que descargan, parchean y compilan.

- La capa Daily debe quedar **separada del contenido de Steven** (archivos y constantes propios), para reutilizarla en Daily Cynthia cambiando solo configuración y contenido.

## Fase 0: prueba de viabilidad

Antes de construir nada, hay que confirmar que la base funciona. Esta fase se hace en una carpeta aparte y su resultado es un informe corto, no código para entregar.

1.  Clonar Hall of Legends y compilarlo tal cual (scripts/get-expansion.sh y scripts/build.sh) **en Windows**, que es donde trabaja el equipo. Sus scripts están pensados para macOS; adaptar lo necesario (Git Bash, WSL o devkitARM) y documentar cómo.

2.  Cambiar GEN_LATEST a GEN_3, limitar las especies a la 1.ª–3.ª gen y volver a compilar.

3.  Arrancar un combate doble contra dos entrenadores y comprobar a mano esta lista de mecánicas de 3.ª gen:

    - Un movimiento de tipo Lucha usa Ataque y uno de tipo Fuego usa Ataque Especial, sea cual sea el movimiento (físico/especial según el tipo).

    - Un crítico hace ×2.

    - Acero resiste los ataques de tipo Fantasma y Siniestro.

    - Terremoto y Surf hacen la mitad de daño cuando golpean a dos objetivos.

    - Cuando cae un Pokémon, el sustituto sale en el momento, no al final del turno.

    - Usar Protección dos veces seguidas falla el 50 % de las veces.

    - El menú resumen no muestra el icono de categoría física/especial de cada movimiento.

4.  Estimar cuánto cuesta portar la capa Daily (fase 1).

**Criterio de salida:** compila en Windows, la lista de mecánicas se cumple y no hay ningún bloqueo. Si algo falla, parar y avisar antes de seguir.

## Fase 1: llevar la capa Daily al nuevo motor

El objetivo es que la base nueva se comporte como Daily Gold de cara a la web y al launcher, sin que el launcher cambie. La referencia es el parche patches/pokefirered-dailyred.patch de Daily Gold: todos sus cambios llevan el comentario Daily Red: o Daily Gold:. Son unas 1.000 líneas propias.

| Pieza de Daily Gold                                     | Qué hace                                                                                                                                                                       | Qué hacer en la base nueva                                                                                                                                                    |
|---------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| src/dailyred.c                                          | Arranque directo sin intro ni título; nombre del jugador; texto rápido; estilo de combate SET bloqueado; especiales DailyRed_On\* que avisan a la web; bloque de estado en RAM | Portar. Quitar de Hall of Legends la pantalla de elección de personaje, las rondas del Alto Mando, el kit de curación y los créditos: se arranca directo en la sala de Bruno. |
| src/dailyred_team.c                                     | Equipo aleatorio del jugador a partir de la semilla                                                                                                                            | Portar adaptando a las estructuras de datos de la expansion (aprendizajes por nivel, MT/MO, tutores, movimientos huevo). Mantener todas las reglas (ver abajo).               |
| src/dailyred_boss.c + enganche en CreateNPCTrainerParty | Equipo, EV, naturalezas y habilidades del jefe                                                                                                                                 | Sustituir por el montaje diario de Steven + compañero (fase 2). En la expansion los EV y naturalezas de los entrenadores ya se pueden definir sin enganches.                  |
| Mapa DailyRed_Summit + include/constants/dailyred.h     | Sala sin salidas y punto de inicio                                                                                                                                             | Rehacer sobre la sala de Bruno (fase 2).                                                                                                                                      |
| Menús (start_menu.c, party_menu.c, option_menu.c)       | Menú reducido: POKéMON / OPTION / EXIT; equipo: SUMMARY / SWITCH / CANCEL                                                                                                      | Portar.                                                                                                                                                                       |
| Sin derrota total ni mochila                            | Perder no manda al Centro Pokémon; no hay objetos de la mochila                                                                                                                | Usar las opciones de la expansion que ya usa Hall of Legends: B_FLAG_NO_WHITEOUT y B_VAR_NO_BAG_USE.                                                                          |

**Contrato con la web y el launcher (no cambiar):**

- Estructura de configuración en la ROM con la marca DAILYRED-CONF-V2: semilla u32, victorias u16, combates u16 y modo diario u8. La web la busca por la marca y escribe en ella antes de arrancar (web/rom-config.js).

- Bloque de estado en RAM con la marca DAILYRED-STATUS1: magic16, semilla u32, fase u8 (exploración, combate, resultado, fin) y resultado u8. La web lo lee dos veces por segundo (web/game-link.js).

- Mismo protocolId: dailyred-conf-v2+status-v1. Así el launcher no necesita un adaptador nuevo. El compilador no debe eliminar ni mover esas estructuras.

- El día cuenta como jugado al empezar el combate. Cerrar durante el combate cuenta como derrota. Tras los mensajes finales, fundido a negro y espera.

**Reglas del equipo del jugador (mantener):**

- 6 especies distintas de la 1.ª–3.ª gen, nivel 100.

- Suma mínima de estadísticas base: hoy 2850. **Recalibrar** contra el total de los dos rivales (3 + 3).

- 1 de cada 8 equipos lleva exactamente un legendario.

- Movimientos al azar entre todos los que puede aprender, con al menos uno de daño si es posible.

- IV entre 25 y 31; EV concentrados en las 2 mejores estadísticas (AssignFocusedEVs).

- Un objeto equipado de la lista sBattleItems o un potenciador del tipo de algún ataque que lleve.

- Generador aleatorio propio, que depende solo de la semilla y no de Random().

## Fase 2: contenido de Daily Steven

Esta fase monta el combate del día sobre la base de la fase 1. Casi todo tiene ya un modelo en Hall of Legends: copiar su enfoque, no reinventarlo.

**2.1 Combinación del día**

- A partir de la semilla diaria se sacan 3 elecciones de 2 opciones: compañero (AURA/WALLY), variante de Steven y variante del compañero.

- Usar un flujo aleatorio propio derivado de la semilla, distinto del que genera el equipo del jugador. Así cambiar una cosa no altera la otra.

- La web **no** calcula ni muestra esta combinación.

- En modo práctica (?seed=) y sin semilla, la combinación sale de la misma semilla que el equipo del jugador.

**2.2 Datos de los equipos**

- Un único archivo de datos, fácil de editar, con: Steven (2 fijos + 2 opciones de variante), AURA (ídem) y WALLY (ídem).

- Por Pokémon: especie, nivel 100, IV, EV, naturaleza, habilidad, objeto, 4 movimientos y si es variocolor.

- Añadir una comprobación al compilar o en los tests: todo debe existir en 3.ª gen y cada movimiento debe poder aprenderlo esa especie.

- Hasta que lleguen los equipos definitivos, usar los tríos de Hall of Legends adaptados y marcados como **PROVISIONAL**: Steven (Metagross, Skarmory, Aggron), Aura (Blaziken, Tropius, Pelipper) y Wally (Gardevoir, Altaria y un sustituto de Gallade).

**2.3 Montaje del combate**

- Seguir src/hall_of_legends.c de Hall of Legends:

  - HoL_CreateOpponentParties: cada rival crea sus 3 Pokémon.

  - HoL_MergeOpponentTeams: tras el primer envío, el equipo de B pasa a los huecos 3–5 de A.

  - gHoLOpponentsMerged: lo consultan los guiones de fin de combate y los mensajes.

  - HoL_IsTrainerBMon: dice de quién es cada Pokémon para el mensaje de envío.

- Mensajes de envío según el dueño real, también en los cambios después de caer un Pokémon.

- Sprites de los dos rivales al empezar y al terminar, y una frase de cada uno al final. Hall of Legends ya lo hace así.

- Sprites de Steven, May y Wally: los propios de la expansion (Hall of Legends usa TRAINER_PIC_STEVEN, TRAINER_PIC_MAY, TRAINER_PIC_WALLY y OBJ_EVENT_GFX_STEVEN, OBJ_EVENT_GFX_MAY_NORMAL, OBJ_EVENT_GFX_WALLY). No hay que importar PNG.

**2.4 Mapa y escena**

1.  Sala de Bruno (Hall of Legends: data/maps/PokemonLeague_BrunosRoom_Frlg), sin salidas para el jugador.

2.  Al entrar solo está Steven, mirando al jugador. El compañero existe en el mapa pero oculto con un flag. Su sprite se elige con VAR_OBJ_GFX_ID_0 antes de que aparezca.

3.  El jugador habla con Steven y Steven dice su frase de presentación.

4.  Se abre la puerta del fondo (detrás de Steven) con su sonido. Aparece el compañero en la puerta, camina hasta la casilla de al lado de Steven y se gira hacia el jugador.

5.  El compañero dice su última frase y empieza el combate.

6.  Tras el combate se cuenta el récord igual que en Daily Gold. En práctica, el compañero vuelve a ocultarse y la sala queda como al principio para el siguiente combate.

**2.5 Textos**

- En castellano, con nombres en mayúsculas como en Daily Gold: "STEVEN y AURA quieren combatir", "STEVEN envió a METAGROSS", etc.

- Frases de presentación, de llegada y de victoria o derrota: tomar como base las de Hall of Legends traducidas, marcadas como provisionales.

- Juntar todos los textos en un solo archivo para que el equipo los revise.

## Fase 3: IA de dobles

Se usa la IA de la expansion con la misma configuración que Hall of Legends. **No** se escribe una IA propia desde cero.

- En Steven, AURA y WALLY, activar las mismas opciones que Hall of Legends pone a sus entrenadores: *Basic Trainer, Omniscient, Smart Switching, Smart Mon Choices, PP Stall Prevention, Randomize Switchin* y *Prediction*. Están en src/data/trainers_frlg.party del parche de Hall of Legends; la documentación está en [AI flags](https://rh-hideout.github.io/pokeemerald-expansion/tutorials/ai_flags.html).

- La lógica propia de dobles (AI_FLAG_DOUBLE_BATTLE) se activa sola en los combates dobles.

- Tras la fusión de equipos el combate pertenece al entrenador A (Steven). Comprobar que la IA sigue activa en los dos Pokémon rivales y que los Pokémon del compañero no pierden su configuración.

- No portar los ajustes de IA específicos de Gold (Gyarados, Umbreon, Typhlosion). Eran para otro equipo y otro motor.

- Solo si las pruebas muestran fallos claros con estos equipos, añadir reglas concretas. Documentar cada una con el caso que corrige.

**Comportamientos que hay que ver en las pruebas:**

- No usa Terremoto, Surf o Explosión si dañan a su compañero, salvo que este sea inmune (por ejemplo, Skarmory o Altaria con Terremoto).

- Remata: si uno de los Pokémon del jugador está a punto de caer, al menos un rival va a por él.

- No elige objetivo a cara o cruz.

- Cambia de Pokémon con sentido y, cuando cae uno, saca un sustituto adecuado entre los 6.

## Fase 4: web, launcher y publicación

El juego se publica igual que Daily Gold, pero como juego independiente. Copiar de Daily Gold web/, content/, docs/, .github/workflows/ y los scripts de empaquetado, y cambiar lo siguiente.

| Qué                   | Daily Gold (referencia)    | Daily Steven                                                               |
|-----------------------|----------------------------|----------------------------------------------------------------------------|
| Repo                  | unaisdev/dailygold         | Nuevo, propuesto unaisdev/dailysteven (confirmar nombre)                   |
| id del manifest       | dailygold                  | dailysteven                                                                |
| Nombre                | Daily Gold                 | Daily Steven                                                               |
| Clave de estadísticas | hortelano:dailygold:stats  | hortelano:dailysteven:stats                                                |
| Clave de silencio     | hortelano:dailygold:muted  | hortelano:dailysteven:muted                                                |
| Prefijo de semilla    | dailyoro:                  | dailysteven: (para que el equipo del día no coincida con el de Daily Gold) |
| bossLabel             | GOLD                       | STEVEN                                                                     |
| Imagen de portada     | assets/gold.png            | assets/steven.png: sprite de combate de Steven exportado de la expansion   |
| Zip del content pack  | dailygold-content.zip      | dailysteven-content.zip                                                    |
| protocolId            | dailyred-conf-v2+status-v1 | **Igual**, sin cambios                                                     |

**Portada (web y launcher):** solo Steven. Quitar los iconos del equipo del jefe que pinta Daily Gold (web/game/team/) y cualquier texto que nombre al compañero o a sus Pokémon.

**Launcher:** añadir la entrada del juego en el catálogo de [Hortelano's Daily](https://github.com/unaisdev/hortelanos-daily) con repo, asset y tagPrefix: "content-v", igual que la de Daily Gold. El protocolo no cambia, así que no hace falta un adaptador nuevo.

**Releases:** el mismo flujo que Daily Gold (docs/CONTENT_RELEASES.md, docs/RELEASE_NOTES.md y AGENTS.md): cada push a main publica un Release y exige bullets para el jugador en content/NOVEDADES.md. Primera versión: content-v0.1.0.

## Pruebas y criterios de aceptación

El trabajo está terminado cuando todas estas casillas pasan con pruebas automatizadas, salvo las que digan "a mano". Portar los tests de Daily Gold (tests/smoke.mjs, tests/site.mjs, tests/teams.mjs): ejecutan la ROM real en Chrome sin interfaz y leen la RAM desde los estados guardados del emulador.

**Juego**

- [ ] Compila en Windows desde cero con los scripts del repo.

- [ ] Arranca directamente en la sala de Bruno, con solo Steven visible.

- [ ] Una misma semilla da siempre el mismo compañero, los mismos tríos y el mismo equipo del jugador; semillas distintas dan combinaciones distintas.

- [ ] Sobre muchas semillas aparecen las 8 combinaciones en proporciones parecidas.

- [ ] La escena ocurre en orden: frase de Steven, puerta, el compañero camina a su lado, frase del compañero y combate.

- [ ] El combate es doble, contra dos entrenadores, y salen los dos sprites al empezar y al terminar.

- [ ] Cada mensaje de envío nombra al dueño real del Pokémon, también en los cambios tras caer uno.

- [ ] Tras caer un Pokémon rival puede salir cualquiera de los que quedan de los 6.

- [ ] Al final cada rival dice su frase.

- [ ] Los equipos rivales en combate coinciden exactamente con el archivo de datos (especie, objeto, movimientos, EV, IV, naturaleza, habilidad).

- [ ] El equipo del jugador cumple todas las reglas de la fase 1.

- [ ] Victoria y derrota se procesan bien: no hay derrota total, se actualiza el récord y aparece un equipo nuevo en práctica.

- [ ] Las mecánicas de 3.ª gen de la fase 0 se cumplen; no aparece ninguna especie, movimiento, habilidad ni objeto posterior a la 3.ª gen.

- [ ] A mano: la IA se comporta como pide la fase 3 en al menos 10 combates de prueba.

**Web y launcher**

- [ ] La web encuentra DAILYRED-CONF-V2 en la ROM y DAILYRED-STATUS1 en la RAM.

- [ ] Pasan las comprobaciones de npm run site: un combate al día, cerrar durante el combate cuenta como derrota, rachas, cuenta atrás, práctica sin guardar y vista móvil.

- [ ] La portada muestra solo a Steven y no revela al compañero.

- [ ] El launcher descarga, verifica e instala el content pack, y Daily Red y Daily Gold siguen funcionando.

## Pendiente, reglas y glosario

**Pendiente de decidir o entregar** (usar lo provisional indicado y no bloquearse):

- [ ] **Equipos definitivos** de Steven, AURA y WALLY: los entrega el equipo. Provisional: los de la fase 2.2.

- [ ] **Textos definitivos**. Provisional: los de Hall of Legends traducidos.

- [ ] **Personaje del jugador**: nombre y sprites. Provisional: los mismos que Daily Gold.

- [ ] **Música y transición** de entrada al combate. Provisional: la música de Campeón, como Daily Gold.

- [ ] **Cuándo cuenta el día como jugado.** Propuesta: al hablar con Steven, antes de que aparezca el compañero, y no al empezar el combate como en Daily Gold. Así nadie puede ver quién es el compañero, cerrar y volver sin que cuente. Confirmar.

- [ ] **Nombre del repo**. Propuesto: unaisdev/dailysteven.

- [ ] **Nuevo mínimo de estadísticas** del equipo del jugador. Proponer un valor con una simulación, como hizo Daily Gold.

**Reglas para el agente**

- No tocar los repos de Daily Red ni de Daily Gold.

- Nada posterior a la 3.ª generación: ni especies, ni movimientos, ni habilidades, ni objetos.

- No cambiar el protocolo con la web y el launcher (marcas, formato y protocolId).

- Al guardar el parche, no meter archivos compilados (el de Daily Gold arrastró una ROM por error) ni cambios que sean solo de saltos de línea de Windows: configurar core.autocrlf false en la copia del motor.

- Marcar todos los cambios en archivos del motor con un comentario Daily Steven:, como hace Daily Gold.

- Si algo de este documento choca con el código o no está claro, preguntar antes de decidir una regla de juego.

- En el README, citar como base Daily Red y Hall of Legends (de amoi-builds) y pokeemerald-expansion, con sus créditos de sprites.

**Glosario para quien no juega a Pokémon**

| Término                   | Qué es                                                                                                                                      |
|---------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| Generación (gen)          | Cada etapa de juegos. 3.ª gen = Rubí, Zafiro, Esmeralda, Rojo Fuego y Verde Hoja: 386 especies.                                             |
| Mecánicas                 | Las reglas del combate: cómo se calcula el daño, los críticos, los tipos, el orden de los turnos, etc.                                      |
| Físico / especial         | Si un ataque usa Ataque y Defensa o Ataque Especial y Defensa Especial. En 3.ª gen lo decide el tipo del ataque; desde la 4.ª, cada ataque. |
| Combate doble             | Dos Pokémon por bando en el campo a la vez.                                                                                                 |
| Equipos fusionados        | Los 3 + 3 Pokémon de los dos rivales se tratan como un equipo de 6.                                                                         |
| Lead                      | El primer Pokémon que saca cada entrenador.                                                                                                 |
| IV / EV                   | Valores ocultos que suben las estadísticas de un Pokémon. IV: 0–31 por estadística. EV: hasta 255 por estadística y 510 en total.           |
| Naturaleza                | Sube un 10 % una estadística y baja otra.                                                                                                   |
| Habilidad                 | Efecto pasivo propio de cada especie (por ejemplo, Levitación hace inmune a Terremoto).                                                     |
| Variocolor (shiny)        | Versión de distinto color; solo estética.                                                                                                   |
| Semilla                   | Número sacado de la fecha. Hace que todos tengan el mismo combate ese día.                                                                  |
| Decompilación / expansion | Código C que compila el juego de GBA. pret/pokefirered es el original; pokeemerald-expansion es una versión ampliada y configurable.        |
| AURA / WALLY              | AURA es May en inglés; WALLY es Blasco en la versión española. Se usan los nombres AURA y WALLY.                                            |
---

# Anexo A. Auditoría técnica independiente del documento original

> **Autor de este anexo:** **segundo agente revisor (ChatGPT/GPT-6)**; no es el agente que elaboró las instrucciones del 17/11/2026.  
> **Fecha de la revisión:** 19/11/2026.  
> **Finalidad:** advertir errores comprobables, contradicciones internas, dependencias técnicas y pruebas que conviene añadir **antes de solicitar a Codex una implementación completa**.  
> **Alcance:** lectura del documento original; inspección del código publicado de [Daily Gold](https://github.com/unaisdev/dailygold), [Hall of Legends](https://github.com/amoi-builds/halloflegends) y configuración del commit `8fb02c6f2b81271e795d3f6aca61733fec739016` de [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion); contraste documental de mecánicas de 3.ª generación. **No hay validación mediante compilación o combate ejecutado**.  
> **Cómo interpretar este anexo:** «corrección propuesta» no significa «decisión aprobada». Si una propuesta contradice un requisito declarado «cerrado», debe presentarse al equipo responsable para su confirmación. No inventar equipos, movimientos, dificultades ni reglas para rellenar huecos.

## A.1. Resumen ejecutivo de la revisión

La estrategia de **usar Hall of Legends sobre pokeemerald-expansion en modo FireRed**, junto con una capa Daily separada, es **coherente** y merece conservarse. En el parche de Hall of Legends se han localizado las funciones de creación y fusión de equipos rivales y el seguimiento del dueño de cada Pokémon. Sin embargo, el documento todavía **no es una especificación libre de contradicciones**. Hay que corregir reglas de combate mal expresadas, confirmar el instante en que se consume el intento diario, aislar las dependencias internas de Hall of Legends y adaptar componentes web que usan estructuras de `pret/pokefirered`.

**Prioridades:**

1. **Crítica — reglas de combate:** corregir el caso Terremoto/Surf en la fase 0 y en la fase 3; verificar comportamiento con ROM real.
2. **Crítica — consumo de intento:** resolver y sincronizar el evento que bloquea el día antes de revelar al compañero.
3. **Alta — estado interno de Hall of Legends:** asegurar que los hooks de creación y fusión funcionan al retirar su selector de personajes.
4. **Alta — compatibilidad con la web:** preservar el contrato binario y adaptar previsualización y archivo histórico.
5. **Alta — compatibilidad estricta de 3.ª generación:** validar todos los datos de equipos y los ajustes del motor, no solo `GEN_LATEST`.
6. **Media — contenido provisional y localización:** preparar variantes para las ocho combinaciones y una traducción española verificable.

## A.2. Hallazgos y propuestas de corrección

### RI-01. Terremoto y Surf: la prueba de daño múltiple es incorrecta

- **Ubicación original:** «Fase 0: prueba de viabilidad», punto 3, y «Fase 3: IA de dobles».
- **Tipo:** error de regla documentada; **corrección propuesta**, no autorizada automáticamente.
- **Texto original:** «Terremoto y Surf hacen la mitad de daño cuando golpean a dos objetivos».
- **Problema:** en **3.ª generación**, la reducción del **50 %** afecta a movimientos que alcanzan a **los dos rivales** (como Surf) cuando hay varios objetivos; **no** a movimientos que golpean a **todos los demás Pokémon del campo**, como Terremoto. Terremoto también golpea al aliado; en esa generación Surf golpea a ambos adversarios, **no al aliado**.
- **Propuesta de sustitución para la fase 0 (sujeta a aprobación):** «En combates dobles de 3.ª generación, Surf golpea a ambos rivales y tiene reducción del 50 % al afectar a más de un objetivo; no golpea al aliado. Terremoto golpea a los demás Pokémon del campo, incluido el aliado, sin esa reducción general. Verificar ambos comportamientos con casos de prueba reproducibles».
- **Propuesta para la fase 3:** evaluar daño al compañero con **Terremoto y Explosión**; **no** calificar Surf como ataque que daña al aliado en 3.ª generación. Verificar las excepciones de inmunidad y protección en la ROM construida.
- **Fuentes de referencia:** [Bulbapedia — Double Battle](https://bulbapedia.bulbagarden.net/wiki/Double_Battle), [Bulbapedia — Damage (Generation III)](https://bulbapedia.bulbagarden.net/wiki/Damage), [Bulbapedia — Surf](https://bulbapedia.bulbagarden.net/wiki/Surf).

### RI-02. Contradicción sobre cuándo se consume el intento diario

- **Ubicación original:** «Fase 1: contrato con la web» frente a «Pendiente: cuándo cuenta el día como jugado».
- **Tipo:** contradicción **interna** del documento.
- **Problema:** la fase 1 fija el inicio del combate como punto de consumo, mientras que los pendientes proponen hacerlo al **hablar con Steven**, antes de mostrar a AURA o WALLY.
- **Propuesta del segundo agente:** consumir el intento **al activar el diálogo que compromete al jugador y precede a la revelación**, antes de la llegada visible del segundo entrenador. Si se cierra entonces el navegador o la ROM, registrar derrota; el modo práctica permanece excluido.
- **Detalle de implementación:** la web de Daily Gold usa `PHASE.BATTLE` y `markBattleStarted` como condición de bloqueo. El protocolo define solo fases numéricas `EXPLORING=0`, `BATTLE=1`, `RESULT=2`, `FINISHED=3`; mantener los valores y los desplazamientos. Si se reutiliza `PHASE.BATTLE` para señalar el compromiso previo al combate, **documentar explícitamente su nueva semántica** y revisar todos los consumidores. No añadir un campo ni cambiar `protocolId` sin coordinación con el launcher.
- **Pruebas obligatorias propuestas:** recarga antes de hablar (no consume), cierre tras activar la conversación pero antes de la puerta (sí consume), cierre durante el combate (sí consume), práctica (no almacena), victoria o derrota (solo una anotación del resultado).
- **Estado:** **pendiente de aprobación del equipo**, porque el documento lo declara decisión abierta.

### RI-03. `GEN_LATEST = GEN_3` es necesario, pero no suficiente

- **Ubicación original:** «Arquitectura elegida: cómo se consigue la 3.ª generación», y «Fase 0».
- **Tipo:** riesgo de implementación.
- **Hallazgo en código:** el parche de Hall of Legends cambia `GEN_LATEST` a `GEN_4`; la expansión incluye múltiples configuraciones adicionales (`B_*`, `P_*`, `I_*`) y banderas de especies disponibles.
- **Propuesta:** auditar y documentar de forma separada la configuración de: categorías físicas/especiales por tipo, críticos, tabla de tipos, movimientos, sus efectos y objetivos, habilidades, objetos, estadísticas, tipos, movimientos aprendibles y turnos de sustitución. Verificar que no se mantienen mejoras posteriores a tercera generación.
- **Criterio:** no marcar la fase 0 como aprobada por el simple hecho de que compile. **Registrar casos concretos y resultados observados** en el juego.

### RI-04. Icono físico/especial del menú resumen

- **Ubicación original:** «Fase 0», última comprobación del punto 3.
- **Tipo:** requisito cuya implementación no puede suponerse automática.
- **Hallazgo documental:** `B_PHYSICAL_SPECIAL_SPLIT` controla la categoría de daño en la expansión, pero el comentario de configuración indica que **el icono de la pantalla de resumen reflejará la categoría**. No demuestra que el icono vaya a ocultarse al seleccionar Gen 3.
- **Propuesta:** conservar el requisito visual original y crear **una prueba separada de interfaz**. Si se necesita cambiar el menú resumen, hacerlo de forma explícita y aislada; no asumir que `GEN_LATEST` lo resuelve.

### RI-05. Dependencias de estado de Hall of Legends al fusionar equipos

- **Ubicación original:** «Fase 1» y «Fase 2.3».
- **Tipo:** riesgo técnico alto, verificado mediante lectura del parche.
- **Hallazgo:** `HoL_CreateOpponentParties` y `HoL_MergeOpponentTeams` dependen de condiciones del modo Hall of Legends, como `HoL_IsActive()`. La fusión copia equipo y estados del entrenador B a huecos de A y desactiva `BATTLE_TYPE_TWO_OPPONENTS`, conservando un indicador (`gHoLOpponentsMerged`) para mensajes y resultados.
- **Riesgo:** eliminar el selector y la inicialización originales de Hall of Legends sin revisar esos estados puede impedir crear/fusionar equipos o romper las frases de ambos rivales.
- **Propuesta:** adaptar los hooks a una condición «Daily Steven activo» o a una abstracción compartida compatible, sin depender accidentalmente de la pantalla de selección. Probar el primer envío, las sustituciones, el dueño de cada Pokémon, ambos sprites al inicio/final y ambos textos de resultado. Revisar que la IA del entrenador B no pierda su configuración al fusionarse.
- **Referencia:** [Parche Hall of Legends](https://github.com/amoi-builds/halloflegends/blob/main/patches/hall-of-legends.patch).

### RI-06. Contrato binario de ROM/RAM: conservar estructura, no solo los nombres

- **Ubicación original:** «Fase 1: contrato con la web» y «Fase 4».
- **Tipo:** requisito de compatibilidad que necesita pruebas adicionales.
- **Hallazgo:** Daily Gold identifica la configuración por `DAILYRED-CONF-V2`; el lector web de estado asume los desplazamientos de `DAILYRED-STATUS1` y lee `phase`/`result` a posiciones conocidas respecto de la marca.
- **Propuesta:** conservar **marcas, orden, tamaños, alineación, little-endian, códigos de fases y resultado**, además del `protocolId`. Se pueden mover direcciones absolutas entre compilaciones si el mecanismo de búsqueda por marca funciona; lo crítico es el diseño binario y las referencias actualizadas.
- **Pruebas:** inyectar una semilla y leerla desde la ROM real; extraer el bloque de RAM real; verificar que la web detecta correctamente explorar, compromiso/inicio, resultado y fin.
- **Referencias:** [Daily Gold — rom-config.js](https://github.com/unaisdev/dailygold/blob/main/web/rom-config.js), [game-link.js](https://github.com/unaisdev/dailygold/blob/main/web/game-link.js).

### RI-07. Estadísticas, silencio y cachés: web independiente frente al launcher

- **Ubicación original:** «Fase 4: web, launcher y publicación».
- **Tipo:** posible contaminación de datos entre juegos.
- **Hallazgo:** el manifest de Daily Gold usa `hortelano:dailygold:stats`, pero `web/daily.js` contiene la clave local `dailyred-stats`. Son **entornos/configuraciones diferentes** y ambos requieren revisión.
- **Propuesta:** usar claves únicas de Daily Steven **tanto en el manifest del launcher como en la web autónoma**; hacer lo mismo con silencio, cachés y archivo histórico. Mantener distintos prefijos de semilla. Las pruebas deben verificar el aislamiento frente a Daily Red/Gold.
- **Referencias:** [manifest de Daily Gold](https://github.com/unaisdev/dailygold/blob/main/content/game.manifest.json), [daily.js](https://github.com/unaisdev/dailygold/blob/main/web/daily.js).

### RI-08. Archivo histórico y previsualización no son transferibles sin revisar

- **Ubicación original:** «Fase 4» y «Pruebas y criterios de aceptación».
- **Tipo:** requisito omitido en el detalle de la migración.
- **Hallazgo en Daily Gold:** `web/archive-party.js` extrae `gPlayerParty`/`gPlayerPartyCount` de estados del emulador; `scripts/gen-archive-assets.mjs` obtiene símbolos y tablas de la compilación `pret/pokefirered`; `web/team-preview.js` replica en JavaScript la selección de especies.
- **Riesgo:** `pokeemerald-expansion` puede emplear otras disposiciones de datos, identificadores y rutas de compilación; una copia directa podría producir previsualizaciones incorrectas aunque el juego se abra.
- **Propuesta:** adaptar el decodificador de Pokémon, los símbolos del ELF, los identificadores/especies y las tablas de nombres, iconos y habilidades. Comparar, para numerosas semillas, la previsualización de la web con el equipo real de la ROM. No revelar la identidad ni el equipo del **compañero rival** mediante la vista previa.
- **Referencias:** [archive-party.js](https://github.com/unaisdev/dailygold/blob/main/web/archive-party.js), [gen-archive-assets.mjs](https://github.com/unaisdev/dailygold/blob/main/scripts/gen-archive-assets.mjs), [team-preview.js](https://github.com/unaisdev/dailygold/blob/main/web/team-preview.js).

### RI-09. Las plantillas provisionales no definen las ocho variantes

- **Ubicación original:** «Decisiones ya tomadas», «Fase 2.2» y «Pruebas y criterios de aceptación».
- **Tipo:** especificación provisional incompleta.
- **Problema:** la regla **2 fijos + 1 variante entre 2** exige **cuatro perfiles por entrenador** (dos fijos + dos alternativos). La lista provisional contiene solo **tres especies** para cada uno y no define las dos variantes.
- **Propuesta:** distinguir «trío provisional para una primera prueba de combate» de «dos tríos provisionales para validar las ocho combinaciones». Antes de dar la fase 2 por terminada, disponer de todas las variantes (o anotar que el criterio aún no se cumple). No completar equipos definitivos por iniciativa del agente.

### RI-10. Filtrado real de equipos de Hall of Legends según Gen 3

- **Ubicación original:** «Fase 2.2», «Fase 3» y regla de tercera generación.
- **Tipo:** incompatibilidades verificables en datos de referencia.
- **Hallazgo:** en los equipos originales de Hall of Legends aparecen, por ejemplo, **Cabeza de Hierro** en Aggron y **Ida y Vuelta/Respiro** en Pelipper, movimientos posteriores a la 3.ª generación; también aparece Gallade, especie de cuarta generación.
- **Propuesta:** el equipo **provisional adaptado** debe superar un verificador de disponibilidad en 3.ª gen y aprendizaje legal de cada movimiento por especie. Aplicar a objetos, habilidades y versiones de tipos/estadísticas el mismo filtro. No confundir «especie disponible en 3.ª gen» con «combinación de movimientos legal en 3.ª gen».
- **Nota:** mantener recursos de Gen 4 en la base común puede ser útil para Daily Cynthia; lo que debe impedirse es que resulten utilizables o seleccionables en Daily Steven.

### RI-11. Localización de castellano de España

- **Ubicación original:** «Fase 1: menús», «Fase 2.5» y «Fase 4».
- **Tipo:** trabajo técnico adicional no detallado.
- **Problema:** Daily Gold incorpora una localización sobre `pret/pokefirered`, mientras Hall of Legends y el nuevo motor tienen otros mensajes, tablas y código.
- **Propuesta:** crear inventario y pruebas de traducción española para **movimientos, habilidades, objetos, naturalezas, estados, menús, pantallas de combate, presentación y frases finales**. Emplear la terminología oficial española de tercera generación cuando corresponda. Mantener **AURA y WALLY** por decisión expresa del documento original.
- **Criterio:** no marcar «español completado» por traducir únicamente los mensajes de Steven/AURA/WALLY.

### RI-12. IA de dobles: pruebas dirigidas y no solo diez combates genéricos

- **Ubicación original:** «Fase 3» y «Pruebas».
- **Tipo:** mejora de criterios de aceptación.
- **Propuesta:** además de al menos diez combates manuales, establecer casos reproducibles de inmunidad del aliado a Terremoto, daño propio con Explosión, elección de objetivos, remates, dos Pokémon debilitados a la vez, sustituciones entre los seis disponibles y selección de movimientos ante Protección. Usar las banderas de IA de Hall of Legends como base y solo añadir reglas específicas si se demuestra un fallo.
- **Precaución:** una IA «fuerte» no implica decisiones perfectas en todos los estados; documentar resultados reproducibles y evitar requisitos subjetivos absolutos como «siempre remata» sin especificar el escenario.

### RI-13. Distinguir núcleo Daily, motor y contenido de Steven

- **Ubicación original:** «Arquitectura», «Fase 1» y «Fase 2».
- **Tipo:** recomendación de mantenibilidad para Daily Cynthia.
- **Propuesta de organización lógica (no obliga a tres repositorios):**
  1. **Núcleo Daily compartido:** semilla, selección de equipo del jugador, estados, protocolo, resultados, modo práctica.
  2. **Motor adaptable:** pokeemerald-expansion, combates dobles, IA, configuración y pruebas de generación.
  3. **Contenido específico de juego:** entrenadores, variantes, mapa, escena, textos, portada y música de Daily Steven; después, contenidos propios de Daily Cynthia.
- **Criterio:** evitar constantes como `STEVEN` o rutas del mapa de Bruno en el núcleo reutilizable, salvo configuraciones de un juego concreto.

### RI-14. Arranque sin semilla y compatibilidad con entorno Windows

- **Ubicación original:** «Fase 0» y «Fase 1».
- **Tipo:** comportamientos que deben documentarse.
- **Propuesta:** especificar el arranque con semilla `0` y sin la web, así como el paso de práctica a nueva partida, sin resucitar el selector original. Probar un entorno concreto de Windows (por ejemplo, WSL si el equipo lo acepta) con instrucciones reproducibles sobre herramienta ARM, `libpng`, rutas y scripts. Evitar cambios de salto de línea y binarios dentro del parche, tal como pide el original.
- **Nota:** en el README de Hall of Legends existen instrucciones para Linux/Windows mediante WSL; aun así, **no se verificó aquí una compilación real en Windows**.

### RI-15. Publicación y aislamiento de Daily Red / Daily Gold

- **Ubicación original:** «Fase 4» y «Reglas para el agente».
- **Tipo:** mejora del flujo de publicación.
- **Propuesta:** no modificar el catálogo del launcher hasta tener un content pack de Steven validado y pruebas satisfactorias. Después, añadir la entrada nueva y ejecutar pruebas de regresión de Daily Red y Daily Gold. Verificar nombres de assets, checksum, versión `content-v*`, compatibilidad con el adaptador existente y que el único retrato en portada es el de Steven.
- **Precaución de distribución:** revisar derechos, licencias y atribuciones de motor, sprites y otros recursos antes de publicar; compilar una ROM no supone por sí solo autorización para distribuirla.

### RI-16. Dificultad y transparencia de los límites técnicos

- **Ubicación original:** «Reglas del equipo del jugador», «Pruebas» y «Portada».
- **Tipo:** recomendaciones de equilibrio y producto.
- **Propuesta:** no elegir aún un nuevo mínimo de estadísticas base. Simular múltiples semillas y las ocho combinaciones con diferentes perfiles de equipo; evaluar sinergias, inmunidades, ataques de área y protección antes de proponer un umbral. Evitar limitar el estudio a una simple proporción respecto de los 2850 puntos originales.
- **Límite del diseño:** con estadísticas locales y una ROM descargable, «un intento al día» es una regla **del navegador/cliente**, no una garantía antitrampas global. Ocultar el compañero en la portada y la web evita spoilers de interfaz, pero no puede considerarse secreto absoluto frente a ingeniería inversa de una ROM descargada. **No se propone cambiar este diseño sin autorización**.

## A.3. Matriz de decisión antes de entregar instrucciones definitivas a Codex

| Punto | Situación | Acción propuesta por el segundo agente | ¿Requiere aprobación? |
|---|---|---|---|
| Mecánica Terremoto/Surf | Error de especificación respecto a Gen 3 | Corregir tests y expectativas de IA (RI-01) | **Sí**, porque modifica texto original |
| Consumo del intento | Contradicción fase 1 / pendientes | Consumir al iniciar el diálogo previo a la revelación (RI-02) | **Sí, expresamente pendiente** |
| Base técnica | Estrategia viable a nivel de arquitectura | Mantener Hall of Legends + expansion, fijando commit y pruebas (RI-03, RI-05) | No cambiar sin consulta |
| Icono físico/especial | Comportamiento no garantizado por `GEN_LATEST` | Añadir test visual y adaptación si falla (RI-04) | No, mantiene requisito original |
| Protocolo ROM/RAM | Esquema existente reutilizable | Validar offsets, tamaños, fases y lectura real (RI-06) | No, mantiene requisito original |
| Almacenamiento de datos | Claves distintas en web y launcher | Aislar todas las claves/cachés de Steven (RI-07) | No, mantiene independencia original |
| Archivo y previsualización | Copia directa de código antiguo no garantizada | Adaptar y verificar con RAM de la nueva ROM (RI-08) | No, completa pruebas |
| Equipos provisionales | No están definidas todas las variantes | Mantener provisional o pedir datos; no inventar (RI-09, RI-10) | **Sí**, si se decide contenido nuevo |
| Traducción | El motor no hereda automáticamente todos los textos | Inventario es-ES y pruebas (RI-11) | No, mantiene requisito original |
| IA | Diez pruebas manuales no cubren todos los casos | Añadir pruebas dirigidas (RI-12) | No, aumenta cobertura |
| Daily Cynthia | Reutilización todavía conceptual | Núcleo/motor/contenido desacoplados (RI-13) | No alterar diseño sin consulta |
| Equilibrio | BST mínima pendiente | Simulación antes de proponer valor (RI-16) | **Sí, decisión pendiente** |

## A.4. Pruebas adicionales propuestas por el segundo agente

Estas casillas son **complementarias** a las pruebas de aceptación del documento original. No constan realizadas en esta revisión.

### A.4.1. Motor y reglas Gen 3

- [ ] Registrar commit exacto de la expansión, hash del parche y comando de compilación.
- [ ] Confirmar daños, objetivos e interacción con aliados de Surf/Terremoto en dobles, según RI-01.
- [ ] Confirmar crítico ×2, tipos originales, Protección, orden de sustitución, físico/especial por tipo y ausencia de generaciones posteriores.
- [ ] Verificar visualmente el menú resumen y su iconografía de categoría de movimiento.
- [ ] Verificar por programa las especies, movimientos, habilidades, objetos y aprendizajes posibles de todos los equipos rivales.

### A.4.2. Escena, fusión y modo diario

- [ ] Demostrar con RAM o trazas que los equipos se fusionan y conservan seis candidatos disponibles.
- [ ] Comprobar envío inicial y sustituciones, nombres del dueño, sprites y frases de cada entrenador.
- [ ] Probar cierre antes de hablar, durante la llegada del compañero, durante combate y al final.
- [ ] Comprobar todas las ocho combinaciones con semillas reproducibles **una vez que existan las variantes**.
- [ ] Comprobar que el modo práctica no altera el récord y reinicia la sala sin dejar sprites visibles indebidamente.

### A.4.3. Web, datos y publicación

- [ ] Validar el formato binario de `DAILYRED-CONF-V2` y `DAILYRED-STATUS1` con la ROM compilada de Steven.
- [ ] Confirmar claves independientes de estadísticas, silencio, cachés y previsualizaciones respecto de Red y Gold.
- [ ] Comparar especies previsualizadas con el equipo real para un conjunto amplio de semillas.
- [ ] Verificar archivo de equipos anteriores con datos reales de la nueva ROM y textos correctos de es-ES.
- [ ] Verificar content pack, checksums, licencia/atribución y que el launcher no rompe Red/Gold.

## A.5. Orden de trabajo recomendado al agente implementador

**La estructura de cinco fases del original se mantiene.** El segundo agente propone añadir puertas de validación:

1. **Fase 0 — Viabilidad:** adaptar Windows, compilar Hall of Legends sin cambios, configurar tercera generación en copia aislada, ejecutar comprobaciones de RI-01/RI-03/RI-04 y emitir informe. **No avanzar si no pasa**.
2. **Fase 1 — Núcleo Daily:** portar semillas, generación, estado y arranque; mantener compatibilidad con la web. Resolver con el equipo RI-02 antes de declarar terminada esta fase.
3. **Fase 2 — Daily Steven jugable:** fijar los estados necesarios de Hall of Legends, montar la escena y los equipos, validar fusión/sprites/diálogos; dejar explícitamente provisionales las variantes que aún no hayan sido entregadas.
4. **Fase 3 — IA y equilibrio:** utilizar la IA de la expansión, añadir casos dirigidos y **proponer**, no imponer, un nuevo mínimo BST mediante simulaciones.
5. **Fase 4 — Web, launcher y publicación:** adaptar localización, previsualización, archivo, almacenamiento y contenido. Integrar en el launcher **solo después** de superar pruebas y regresión.

**Entregables por fase:** breve informe de qué se ha cambiado, cómo reproducir las pruebas, qué pasó, qué falta, y si existe alguna discrepancia que exige decisión humana. No afirmar que una condición está verificada sin un resultado ejecutado.

## A.6. Fuentes consultadas y limitaciones

**Código fuente público inspeccionado para esta auditoría:**

- [Daily Gold — README](https://github.com/unaisdev/dailygold/blob/main/README.md).
- [Daily Gold — manifest](https://github.com/unaisdev/dailygold/blob/main/content/game.manifest.json).
- [Daily Gold — estado web](https://github.com/unaisdev/dailygold/blob/main/web/game-link.js) y [partida diaria](https://github.com/unaisdev/dailygold/blob/main/web/daily.js).
- [Daily Gold — configuración de ROM](https://github.com/unaisdev/dailygold/blob/main/web/rom-config.js).
- [Daily Gold — archivo histórico](https://github.com/unaisdev/dailygold/blob/main/web/archive-party.js), [generación de recursos](https://github.com/unaisdev/dailygold/blob/main/scripts/gen-archive-assets.mjs) y [previsualización](https://github.com/unaisdev/dailygold/blob/main/web/team-preview.js).
- [Hall of Legends — README](https://github.com/amoi-builds/halloflegends/blob/main/README.md), [script de obtención](https://github.com/amoi-builds/halloflegends/blob/main/scripts/get-expansion.sh) y [parche](https://github.com/amoi-builds/halloflegends/blob/main/patches/hall-of-legends.patch).
- [pokeemerald-expansion — configuración de batalla, commit fijado](https://github.com/rh-hideout/pokeemerald-expansion/blob/8fb02c6f2b81271e795d3f6aca61733fec739016/include/config/battle.h), [general](https://github.com/rh-hideout/pokeemerald-expansion/blob/8fb02c6f2b81271e795d3f6aca61733fec739016/include/config/general.h), [especies](https://github.com/rh-hideout/pokeemerald-expansion/blob/8fb02c6f2b81271e795d3f6aca61733fec739016/include/config/species_enabled.h) y [Pokémon](https://github.com/rh-hideout/pokeemerald-expansion/blob/8fb02c6f2b81271e795d3f6aca61733fec739016/include/config/pokemon.h).

**Mecánicas contrastadas en documentación externa:** [Bulbapedia — Double Battle](https://bulbapedia.bulbagarden.net/wiki/Double_Battle), [Damage](https://bulbapedia.bulbagarden.net/wiki/Damage), [Surf](https://bulbapedia.bulbagarden.net/wiki/Surf) y [Protect](https://bulbapedia.bulbagarden.net/wiki/Protect); la referencia principal para la implementación debe seguir siendo el comportamiento comprobado de tercera generación.

**Limitaciones y advertencias de procedencia:** esta es una **segunda revisión independiente**, no una corrección avalada por el agente autor del documento original ni una comprobación experimental de la ROM. No consta compilación en Windows, ejecución en emulador ni validación de las ocho combinaciones durante esta auditoría. Cuando se trabaje con código que pueda haber cambiado después del 19/11/2026, comprobar el estado real de los repositorios y anotar el commit utilizado.

---

*Fin del anexo de revisión independiente. El documento original se ha preservado en las secciones precedentes; las propuestas del segundo agente requieren evaluación y aceptación explícita cuando afecten a decisiones de diseño.*
