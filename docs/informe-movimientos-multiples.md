# Auditoría de movimientos múltiples de Daily Steven

## Corrección y revalidación — REALIZADO / PROBADO

Terremoto, Magnitud, Explosión y Autodestrucción ya conservan el daño completo con dos objetivos vivos, tanto si falta el aliado como si falta un rival. El parche de generación de la ROM corrige la regla de daño de tercera generación para los ataques que también pueden alcanzar al aliado.

| Movimiento | Aliado ausente, dos rivales | Aliado vivo, un rival | Uno o tres objetivos vivos |
|---|---|---|---|
| Terremoto | PASA | PASA | PASA |
| Magnitud | PASA | PASA | PASA |
| Explosión | PASA | PASA | PASA |
| Autodestrucción | PASA | PASA | PASA |

Resultado después del arreglo. **136 de 136 pruebas pasan**, incluidas las 46 anteriores y las nueve que reproducían el fallo. Compilación normal y validadores check-names, check-gen3, check-spanish y check-tmhm correctos. ROM en web/game/dailymaximo.gba.gz, con huella abreviada 311b05ce568e7ff9 para la ROM descomprimida.

La compilación y las pruebas se ejecutaron en /home/javier/dailysteven-build-codex-fix. Los registros están en .cache/reference/audit-build-fix.log y .cache/reference/gen3-tests-fix.log.

Archivos modificados para el arreglo, scripts/patch-dailymaximo-hooks.py y content/NOVEDADES.md. El informe, el CSV y el archivo de pruebas son archivos nuevos. Git diff --stat muestra 16 líneas añadidas en los dos archivos ya seguidos por Git.

## Resultados de la auditoría antes de la corrección

Los resultados y el estado de Git de las secciones siguientes corresponden a la auditoría previa al arreglo.

Estado de la revisión REALIZADO y PROBADO. Se han completado las comprobaciones de esta auditoría y las ampliaciones pendientes del informe anterior. Hay un fallo de lógica confirmado, reproducido en cuatro movimientos. La suite completa termina con 127 pruebas que pasan y 9 que fallan, de 136. Las nueve incidencias de prueba corresponden al mismo fallo.

## 1. Referencia y objetivos — REALIZADO / PROBADO

Worktree C:/Users/Javier/orca/dailysteven-codex-movs. Rama codex/movimientos-multiples, creada sin upstream desde 04728b39f65d05d598e7c37949353c9655b39522. Expansion base 8fb02c6f2b, preparada mediante los scripts del proyecto.

Referencia de Esmeralda fijada en pret/pokeemerald 9103e529c2fbdd7add4a1ff3350c6c1f92112c42. Se compararon las 354 entradas originales de src/data/battle_moves.h con las de src/data/moves_info.h del árbol generado. Se resolvieron las condiciones de generación con GEN_3 y los nombres internos equivalentes VICE_GRIP/VISE_GRIP, HI_JUMP_KICK/HIGH_JUMP_KICK, FAINT_ATTACK/FEINT_ATTACK y SMELLING_SALT/SMELLING_SALTS. Además, ocho pruebas comprueban dentro del ejecutable los 354 objetivos de la ROM contra el CSV. Todas pasan.

Hay diez diferencias literales. En Esmeralda todas las entradas siguientes indican usuario. Sus efectos amplían después el alcance. La ROM utiliza categorías más específicas.

| Movimiento | Objetivo en la ROM | Comprobación de combate | Resultado |
|---|---|---|---|
| Niebla | Campo | Elimina aumentos de los cuatro Pokémon | PROBADO, PASA |
| Canto Mortal | Todos | Cuenta atrás, Protección e Insonorizar en usuario y rival | PROBADO, PASA |
| Tormenta Arena | Campo | Daño en ambos bandos y duración de cinco turnos | PROBADO, PASA |
| Campana Cura | Usuario y aliado | Cura seis plazas, respeta Insonorizar y conserva los estados rivales | PROBADO, PASA |
| Danza Lluvia | Campo | Potencia Agua en ambos bandos y dura cinco turnos | PROBADO, PASA |
| Día Soleado | Campo | Debilita Agua en ambos bandos y dura cinco turnos | PROBADO, PASA |
| Granizo | Campo | Daño en ambos bandos y duración de cinco turnos | PROBADO, PASA |
| Chapoteo Lodo | Campo | Reduce Electricidad en ambos bandos y cesa al retirar al usuario | PROBADO, PASA |
| Aromaterapia | Usuario y aliado | Cura seis plazas, incluida Insonorizar, y conserva los estados rivales | PROBADO, PASA |
| Hidrochorro | Campo | Reduce Fuego en ambos bandos y cesa al retirar al usuario | PROBADO, PASA |

Las otras 344 entradas coinciden. Estas diez diferencias no han producido fallos en los comportamientos comprobados. El detalle de las 354 entradas está en movimientos-objetivos.csv.

## 2. Reducción del daño — REALIZADO / PROBADO

Esmeralda aplica la mitad del daño en dobles a los movimientos dirigidos a ambos rivales mientras las dos posiciones rivales estén ocupadas. Usa la marca de ausencia de cada posición. Al terminar un movimiento tramita los debilitados y las sustituciones, y actualiza esa marca. Esto tiene tres consecuencias comprobadas con el código original y con pruebas de la ROM.

- Si solo queda un rival al comenzar el ataque, aplica daño completo.
- Si el primer rival cae durante ese mismo ataque múltiple, el segundo sigue recibiendo daño reducido.
- Si otro Pokémon elimina a un rival antes de que ataque el usuario y su posición queda vacía, el ataque posterior recibe daño completo.

Protección e inmunidades no eliminan una posición del recuento. Las pruebas con Surf, Absorbe Agua y Protección lo confirman. Los movimientos que también golpean al aliado quedan fuera de la reducción de Esmeralda.

La ROM cuenta Pokémon vivos y aplica 0,5 cuando encuentra exactamente dos objetivos. Para los ataques que alcanzan al aliado esto es incorrecto. Con tres objetivos vivos o uno solo las pruebas pasan. Con dos fallan.

La revisión inicial consideró incorrectamente que Esmeralda retiraba la reducción al caer el primer rival durante el mismo ataque. Se corrigió esa expectativa tras revisar CountAliveMonsInBattle, Cmd_end, HandleAction_TryFinish y HandleFaintedMonActions. Las 34 pruebas finales sobre bajas durante el ataque o antes de él pasan. Ese supuesto segundo fallo queda descartado.

## 3. Tabla de comprobaciones — PROBADO

En esta tabla, «los 17» significa Viento Cortante, Ácido, Surf, Ventisca, Hoja Afilada, Rapidez, Burbuja, Avalancha, Nieve Polvo, Viento Hielo, Ciclón, Onda Ígnea, Estallido, Vozarrón, Aire Afilado, Salpicar y Agua Lodosa.

| Grupo o situación | Movimientos probados | Esperado en Esmeralda | Resultado | Veredicto |
|---|---|---|---|---|
| Daño a ambos rivales | Los 17 | Mitad con dos rivales, completo con uno; aliado intacto | Coincide | PASA |
| Cae el primer rival durante el mismo ataque | Los 17 | Conserva la reducción contra el segundo | Coincide | PASA |
| Otro Pokémon elimina antes al rival y deja su posición vacía | Los 17 | Daño completo contra el restante | Coincide | PASA |
| Ataques que alcanzan al aliado, tres objetivos vivos | Terremoto, Magnitud, Explosión, Autodestrucción | Daño completo | Coincide | PASA |
| Los mismos, aliado ausente y dos rivales vivos | Los cuatro | Daño completo | Aplica la mitad | FALLA |
| Los mismos, aliado vivo y un rival | Los cuatro | Daño completo | Aplica la mitad | FALLA |
| Los mismos, aliado ausente y un rival | Los cuatro | Daño completo | Coincide | PASA |
| Estado contra ambos rivales | Gruñido, Malicioso, Látigo, Disparo Demora, Dulce Aroma | Baja correspondiente en ambos, aliado intacto | Coincide | PASA |
| Protección ante estado múltiple | Los cinco anteriores | Protege a quien lo usa, conserva el efecto sobre el otro | Coincide | PASA |
| Estado contra los otros tres | Danza Caos | Confunde a ambos rivales y al aliado | Coincide | PASA |
| Protección y Ritmo Propio | Danza Caos | Impiden confusión en sus usuarios | Coincide | PASA |
| Aleatorios | Golpe, Danza Pétalo, Enfado, Alboroto | Puede elegir cualquiera de los dos rivales, golpea solo a uno | Ambas elecciones forzadas pasan; código usa elección uniforme | PASA |
| Aleatorios con un rival | Los cuatro anteriores | Redirige al único rival vivo aunque la elección inicial sea la plaza vacía | Coincide | PASA |
| Usuario | Danza Espada | Aumenta solo su Ataque | Coincide | PASA |
| Aliado | Refuerzo | Daño del aliado multiplicado por 1,5 | Coincide | PASA |
| Aliado ausente | Refuerzo | Falla | Coincide | PASA |
| Campo y equipo | Los diez movimientos de la tabla anterior | Efectos indicados arriba | Coincide en cada prueba | PASA |
| Campo rival | Púas | Daña las entradas de las dos plazas rivales | Coincide, 50 PS sobre 400 | PASA |
| Protección e inmunidad | Surf frente a Protección y Absorbe Agua | Conserva reducción contra el otro rival vivo | Coincide | PASA |
| Protección y Levitación | Terremoto | Impide el impacto en el protegido o inmune, conserva daño del resto | Coincide | PASA |
| Insonorizar | Gruñido y Vozarrón | Bloquea solo en su portador | Coincide | PASA |
| Humedad | Explosión y Autodestrucción | Impide explosión y sacrificio | Coincide | PASA |
| Redirección | Señuelo | Atrae ataques individuales desde ambos puestos | Coincide | PASA |
| Redirección y caída del usuario | Señuelo | Deja de atraer tras debilitarse | Coincide | PASA |
| Prioridad de redirección | Señuelo frente a Pararrayos, con Impactrueno | Prevalece Señuelo | Coincide | PASA |
| Redirección ante movimientos múltiples | Señuelo con Surf y Gruñido | Conserva ambos objetivos | Coincide | PASA |
| Pararrayos rival | Impactrueno y Onda Trueno | Redirige, sin inmunidad ni aumento especial | Coincide | PASA |
| Pararrayos aliado | Impactrueno | En Esmeralda no atrae el ataque del aliado | Coincide | PASA |
| Objetivo individual | Onda Trueno sin Pararrayos | Paraliza solo al seleccionado | Coincide | PASA |
| Tabla compilada | Movimientos originales 1–354 | Coincide con las 354 entradas auditadas | Coincide | PASA |

Bofetón Lodo es individual. Chapoteo Lodo es el movimiento de campo. Refuerzo figura como usuario en ambas tablas y su efecto actúa sobre el aliado. Disparo Demora baja un nivel de Velocidad en esta generación.

Se neutralizaron los efectos secundarios de Ácido y Ascuas en las pruebas comparativas para medir la reducción sin cambios de Defensa ni quemaduras entre turnos. En los casos de Magnitud se fija Magnitud 7. Las comprobaciones absolutas contemplan la tirada de daño del juego y un margen de dos PS de redondeo. Las elecciones aleatorias se comprobaron fijando ambas ramas, con revisión de la elección uniforme en el código. La comparación de Esmeralda se hizo con sus fuentes fijadas, no mediante ejecución de su cartucho original.

## 4. Fallos y arreglos — FALLIDO / PENDIENTE

Único fallo confirmado de esta auditoría, reducción indebida con exactamente dos objetivos vivos en Terremoto, Magnitud, Explosión y Autodestrucción.

| Movimiento | Aliado ausente, dos rivales | Aliado vivo, un rival | Estado |
|---|---|---|---|
| Terremoto | Falla | Falla | PROBADO / FALLIDO |
| Magnitud | Falla | Falla | PROBADO / FALLIDO |
| Explosión | Falla | Falla | PROBADO / FALLIDO |
| Autodestrucción | Falla | Falla | PROBADO / FALLIDO |

La novena prueba fallida es el caso original de Terremoto en dos turnos, con 170 PS y luego 85 PS. Las ocho pruebas adicionales aíslan las dos disposiciones anteriores para cada movimiento.

Causa en src/battle_util.c, funciones GetMoveTargetCount y GetTargetDamageModifier. Se usa el número de objetivos vivos para decidir el multiplicador, sin excluir en GEN_3 la categoría que alcanza al aliado.

Arreglo propuesto, pendiente de la fase de corrección solicitada por el usuario. En GEN_3, devolver daño completo para TARGET_FOES_AND_ALLY y mantener la reducción de TARGET_BOTH con dos rivales. Incorporarlo mediante el mecanismo de parches del proyecto y volver a ejecutar toda la batería. Esta auditoría solo añade pruebas y documentación.

La revisión solicitada queda REALIZADA. Quedan PENDIENTES el arreglo y su validación posterior. La cobertura comprende los casos enumerados, no una demostración de todas las combinaciones posibles de equipos, habilidades y estados del juego.

## 5. Compilación, archivos y Git — REALIZADO / PROBADO

Compilación normal repetida correctamente en /home/javier/dailysteven-build-codex mediante scripts/build.sh. El trabajo inicial usó scripts/wsl-build.sh con DAILYSTEVEN_WSL_DIR apuntando a esa misma carpeta aislada. Los cuatro validadores check-names, check-gen3, check-spanish y check-tmhm vuelven a dar OK.

Suite scripts/run-gen3-tests.sh ejecutada hasta finalizar. Resultado final 136 pruebas, 127 PASSED y 9 FAILED. Las 46 anteriores pasan. El archivo nuevo aporta 90 pruebas, 81 pasan y 9 fallan. Las pruebas parametrizadas cuentan como una prueba y varias de ellas comprueban múltiples movimientos o situaciones. La salida devuelve código 1 por los fallos reproducidos.

Los mensajes ASSUME failed de archivos generales de la expansion pertenecen a comprobaciones globales de otras mecánicas configuradas para generaciones posteriores. La lista final de pruebas fallidas contiene exclusivamente los nueve casos indicados en esta auditoría.

Archivos nuevos de la revisión

- docs/informe-movimientos-multiples.md
- docs/movimientos-objetivos.csv
- overlay/test/gen3_multiple_targets.c

Salida de git status --short

```text
?? docs/informe-movimientos-multiples.md
?? docs/movimientos-objetivos.csv
?? overlay/test/gen3_multiple_targets.c
```

Git diff --stat devuelve salida vacía porque los tres archivos son nuevos y están sin añadir al índice. Git diff --check no informa de problemas en los archivos seguidos. La rama carece de upstream y permanece en 04728b3.

Las evidencias locales completas están en .cache/reference/gen3-tests.log y .cache/reference/audit-build.log, con copias en /home/javier/dailysteven-build-codex. Estas rutas están fuera del contenido versionado.

Fuentes fijadas de Esmeralda

- Tabla de movimientos, src/data/battle_moves.h, commit 9103e529c2fbdd7add4a1ff3350c6c1f92112c42.
- Daño y recuento de posiciones, src/pokemon.c, líneas 3275–3277, 3326–3328 y función CountAliveMonsInBattle.
- Paso al siguiente objetivo, src/battle_script_commands.c, MOVEEND_NEXT_TARGET. Final de movimiento y ausencias, Cmd_end y Cmd_openpartyscreen.
- Gestión de bajas, src/battle_util.c, HandleAction_TryFinish y HandleFaintedMonActions.
- Campo y equipo, Cmd_healpartystatus y Cmd_trysetperishsong en src/battle_script_commands.c.

Repositorio de referencia https://github.com/pret/pokeemerald/tree/9103e529c2fbdd7add4a1ff3350c6c1f92112c42.
