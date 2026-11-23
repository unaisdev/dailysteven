# Daily Steven

Un combate doble de Pokémon cada día contra **Steven y Aura o Blasco**. Recibes seis Pokémon aleatorios de nivel 100 de las tres primeras generaciones. Quienes juegan en la misma fecha reciben el mismo equipo y se enfrentan a la misma pareja de rivales. Tú controlas a los seis Pokémon y tienes un intento diario.

**Juega en Hortelano’s Daily 2.2.3.** Abre el lanzador y selecciona el cartucho **Daily Steven**. El lanzador descarga el contenido oficial en el primer arranque. Puedes consultar los equipos de días anteriores y volver a jugar esos combates en práctica, sin gastar el intento de hoy. El ranking de Steven sigue pendiente.

[ZIP portable para Windows x64](https://github.com/unaisdev/hortelanos-daily/releases/download/v2.2.3/Hortelanos.Daily_2.2.3_x64-portable.zip) · [Lanzador para Windows, Linux y macOS](https://github.com/unaisdev/hortelanos-daily/releases/latest) · [Pack oficial de Daily Steven](https://github.com/unaisdev/dailysteven/releases/latest)

El juego está en **castellano de España**. Se construye sobre Hall of Legends y pokeemerald-expansion. Los combates los resuelve el motor de la ROM de Game Boy Advance, ejecutada con mGBA. [Hall of Legends](https://github.com/amoi-builds/halloflegends) · [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion) · [mGBA](https://github.com/mgba-emu/mgba)

- [Inicio rápido](#inicio-rápido)
- [Cómo funciona](#cómo-funciona)
- [Generación del equipo](#generación-del-equipo)
- [Equipos rivales](#equipos-rivales)
- [Flujo de desarrollo](#flujo-de-desarrollo)
- [Pruebas y depuración](#pruebas-y-depuración)
- [Jugar en local](#jugar-en-local)
- [Publicación del pack](#publicación-del-pack)
- [Créditos y licencias](#créditos-y-licencias)
- [Limitaciones conocidas](#limitaciones-conocidas)
- [Estructura del repositorio](#estructura-del-repositorio)

## Inicio rápido

### macOS

Para compilar necesitas Git, Python 3 y las herramientas de línea de comandos de Xcode (`xcode-select --install`). El script prepara el compilador ARM, GNU Make y libpng en `.toolchain/`, tanto en Intel como en Apple Silicon. Puedes construir la ROM desde el código fuente sin aportar una ROM comercial.

```sh
bash scripts/play.sh
```

El script prepara el árbol, compila la ROM, descarga el emulador y sirve el juego en `http://localhost:8080`. Con Node.js también puedes ejecutar el proceso paso a paso.

```sh
bash scripts/setup-macos.sh
bash scripts/get-expansion.sh
bash scripts/fetch-vendor.sh
bash scripts/build.sh
node scripts/serve.mjs
```

### Windows con WSL

Prepara Ubuntu en WSL con el compilador ARM y las dependencias del motor. El script sincroniza las fuentes con el árbol de compilación de WSL y copia los resultados al repositorio de Windows.

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-wsl.ps1
node scripts/serve.mjs
```

Puedes añadir `-Clean` para reconstruir desde cero o `-Debug` para generar una ROM de pruebas independiente. La compilación normal deja `web/game/dailymaximo.gba.gz` y `web/game/BUILD-INFO.txt`, con la versión de las fuentes y la huella de la ROM.

### Linux o Docker Desktop en Windows

En Linux, instala Python 3, GNU Make, el compilador ARM y las dependencias que indica la expansión. Después ejecuta `bash scripts/play.sh`. En Windows, con Docker Desktop, prepara primero el árbol con `bash scripts/get-expansion.sh`, compila con `bash scripts/_docker_build.sh` y descarga el emulador con `bash scripts/fetch-vendor.sh`. Puedes servir el resultado con `node scripts/serve.mjs`. [Dependencias de pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion/blob/8fb02c6f2b81271e795d3f6aca61733fec739016/INSTALL.md)

## Cómo funciona

```text
pokeemerald-expansion + Hall of Legends + overlay + traducción al castellano
  → compilación FireRed → pokefirered.gba → dailymaximo.gba.gz
  → el cliente inyecta la semilla y el récord → Nostalgist.js y mGBA
  → el cliente lee el estado de la ROM en RAM → actualiza el historial
```

Emular la GBA conserva el motor de combate, las animaciones, el sonido y el código que usa el hardware de la consola. Nostalgist.js carga la ROM como bytes, envía los controles a mGBA y permite al cliente leer su memoria. El núcleo de mGBA funciona con su BIOS integrada y los archivos se pueden servir desde un servidor web estático. [Nostalgist.js](https://github.com/arianrhodsandlot/nostalgist) · [Licencias del reproductor](web/licenses.txt)

### El reto diario

- **Fecha y semilla.** El cliente calcula un hash FNV-1a de `dailysteven:AAAA-MM-DD` usando el día local. La ROM mezcla la semilla para seleccionar a Aura o Blasco. La misma pareja puede repetirse varios días seguidos.
- **De la aplicación a la ROM.** `gDailyMaxRomConfig` contiene la firma `DAILYMAX-CONF-V1`, la semilla, las victorias y combates acumulados y un indicador de partida diaria. El cliente localiza la firma y escribe esos datos antes de arrancar.
- **De la ROM a la aplicación.** `DAILYMAX-STATUS1` identifica en RAM un bloque con la semilla, la fase (exploración, combate, resultado o final) y el desenlace. El cliente lo consulta y guarda el resultado cuando está disponible. El protocolo del lanzador es `dailymax-conf-v1+status-v1`.
- **Un intento por día.** Puedes salir antes de empezar el combate y volver con el mismo equipo. Tras comenzar, salir, recargar o reiniciar cuenta como derrota. Al terminar la escena final, la aplicación muestra el resultado y la cuenta atrás para el próximo reto.
- **Estadísticas.** Se registran victorias, derrotas, racha de victorias y racha diaria, con la clave `hortelano:dailysteven:stats`. En el lanzador las estadísticas de Steven tienen su propio almacenamiento, separado de Gold y Red. Saltarse un día interrumpe las rachas.
- **Archivo y práctica.** El archivo del lanzador permite ver los Pokémon, sus movimientos y sus estadísticas para fechas anteriores. El modo práctica usa la semilla de la fecha elegida y no registra el resultado. En la web local, `?seed=28` abre una práctica contra Aura y `?seed=113` una contra Blasco.

### Recorrido y código

| Paso | Código |
| --- | --- |
| Con semilla, la ROM entra desde la pantalla de copyright. Sin semilla, pulsa Inicio en el título. | `CB2_DailyMax_StartFromCopyright` y `CB2_DailyMax_StartFromTitle` en [dailymaximo.c](overlay/src/dailymaximo.c). |
| Se crea al jugador HORTE, con texto rápido y estilo de combate fijo de partida, y se genera su equipo. | [dailymaximo.c](overlay/src/dailymaximo.c) y [dailymaximo_team.c](overlay/src/dailymaximo_team.c). |
| Entras en la sala de Bruno. Al hablar con Steven, su compañero llega por la puerta y se coloca a su lado. | [Escena de la sala](overlay/data/dailymax_bruno.inc) y [textos](overlay/include/dailymax_texts.h). |
| Empieza el doble contra el equipo fusionado de seis. El jugador controla a sus dos Pokémon activos y elige también los relevos. | [Equipos](overlay/data/dailymax_rivals.party) y [ganchos del motor](scripts/patch-dailymaximo-hooks.py). |
| La escena registra el resultado, muestra el mensaje final y funde a negro. El cliente vuelve a la pantalla del reto. | [Escena de la sala](overlay/data/dailymax_bruno.inc), [lectura de RAM](web/game-link.js) y [cliente web](web/app.js). |

Se aplican las mecánicas de **tercera generación**. El tipo del ataque determina si es físico o especial, los críticos infligen el doble de daño y las sustituciones tras un KO pueden entrar durante el turno. En dobles, Surf alcanza a los rivales y reduce su daño a la mitad si los dos están activos. Terremoto también alcanza al compañero y mantiene su daño de área completo. El pack 0.1.3 corrige el bloqueo de Carga y conserva su potenciación eléctrica. [Mecánicas de ataques múltiples](docs/informe-movimientos-multiples.md) · [Parche de Carga](https://github.com/unaisdev/dailysteven/pull/7)

## Generación del equipo

El generador vive en [dailymaximo_team.c](overlay/src/dailymaximo_team.c). Tiene su propio estado aleatorio, por lo que una misma semilla genera el mismo equipo con la misma versión de la ROM.

- **Especies.** Elige seis especies distintas entre las 386 de las generaciones I a III. El objetivo es una suma de estadísticas base de **2700 o más**. Si se agota el límite de 1000 intentos, conserva el equipo de mayor suma encontrado.
- **Legendarios y singulares.** El sorteo se hace antes de seleccionar el resto del equipo. Aproximadamente uno de cada ocho equipos lleva uno, situado en un hueco aleatorio.
- **Movimientos.** Escoge hasta cuatro movimientos distintos entre los que la especie aprende por nivel y mediante las 50 MT y 8 MO de tercera generación. Usa las listas de Pokémon Esmeralda. Si la especie puede aprender un movimiento de daño, recibe al menos uno. Las especies con un repertorio corto reciben los disponibles. Los movimientos de huevo y de tutor quedan fuera del sorteo. [Datos de aprendizaje de Esmeralda](docs/mt-esmeralda.md)
- **Nivel, IV y EV.** Todos salen al nivel 100, con personalidad aleatoria e IV de 25 a 31. Se asignan 252 EV a cada una de sus dos estadísticas base útiles más altas, y los 6 restantes a PS o a la tercera si PS ya es una de las dos primeras. Ataque y Ataque Especial solo compiten por los EV si el repertorio usa esa categoría. La personalidad da la naturaleza, el género y la habilidad que correspondan, y admite Pokémon variocolor.
- **Objetos.** Cada Pokémon lleva un objeto de un grupo de opciones de combate, como Restos, Cinta Elegida, Garra Rápida o bayas. Un potenciador de tipo solo entra en el sorteo si ese Pokémon lleva un ataque de ese tipo.
- **Validación.** El motor comprueba nivel, PS, movimientos distintos y PP. Si se agotan los reintentos de un Pokémon, se aplica el repertorio inicial del motor para su nivel.

El mínimo de 2700 permite que una especie débil aparezca acompañada de otras que compensen la suma. En una comprobación de 10.000 semillas con la ROM pública 0.1.2, la mediana fue **2778** y los equipos observados iban de **2700 a 3290**. El BST mide estadísticas base. La cobertura de tipos, la velocidad, los objetos y las combinaciones disponibles en dobles también influyen en cada enfrentamiento. El sorteo puede dar combinaciones poco cómodas, como una Cinta Elegida junto a varios movimientos de estado.

## Equipos rivales

| Pareja | Trío de Steven | Trío del compañero | BST del equipo conjunto |
| --- | --- | --- | ---: |
| Steven y Aura | Skarmory, Cradily y Metagross | Blaziken, Swellow y Salamence | 3120 |
| Steven y Blasco | Skarmory, Metagross y Aggron | Altaria, Gardevoir y Dusclops | 3058 |

| Pokémon | PS | Ataque | Defensa | Velocidad | At. Esp. | Def. Esp. | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Skarmory | 65 | 80 | 140 | 70 | 40 | 70 | 465 |
| Cradily | 86 | 81 | 97 | 43 | 81 | 107 | 495 |
| Metagross | 80 | 135 | 130 | 70 | 95 | 90 | 600 |
| Blaziken | 80 | 120 | 70 | 80 | 110 | 70 | 530 |
| Swellow | 60 | 85 | 60 | 125 | 50 | 50 | 430 |
| Salamence | 95 | 135 | 80 | 100 | 110 | 80 | 600 |
| Aggron | 70 | 110 | 180 | 50 | 60 | 60 | 530 |
| Altaria | 75 | 70 | 90 | 80 | 70 | 105 | 490 |
| Gardevoir | 68 | 65 | 65 | 80 | 125 | 115 | 518 |
| Dusclops | 40 | 70 | 130 | 25 | 60 | 130 | 455 |

Son las estadísticas base usadas por la ROM. Cada rival lleva Pokémon de nivel 100 con movimientos, objetos, naturalezas, habilidades, EV e IV definidos en el archivo de equipos. Los IV son 31 salvo los de Ataque de Skarmory en el equipo con Aura, Gardevoir y Dusclops, que son 0. Los nombres internos de Aura y Blasco son May y Wally. [Definiciones completas de los tres entrenadores](overlay/data/dailymax_rivals.party)

- **Blaziken de Aura** lleva Hierba Blanca.
- **Metagross de Steven** es variocolor los días con Blasco.
- **Dusclops de Blasco** lleva Restos y tiene Presión y naturaleza Serena. Sus EV son 252 en PS, 136 en Defensa y 120 en Defensa Especial. Usa Maldición, Divide Dolor, Fuego Fatuo y Protección.

Los equipos de estas tablas corresponden a `main`. El pack público `content-v0.1.3` contiene los equipos anteriores.

La IA usa la configuración original de Daily Steven. Incluye `Basic Trainer`, `Omniscient`, `Smart Switching`, `Smart Mon Choices`, `PP Stall Prevention`, `Randomize Switchin` y `Prediction`. `HP Aware` está desactivada.

## Flujo de desarrollo

1. Cambia las fuentes de Daily Steven en `overlay/`, los textos de `locale/es/` o los ganchos del motor en `scripts/patch-dailymaximo-hooks.py`.
2. Si ya tienes preparado `pokeemerald-expansion/`, aplica los cambios con `bash scripts/apply-dailymaximo.sh` y compila con `bash scripts/build.sh`. En Windows puedes ejecutar de nuevo `scripts/build-wsl.ps1`.
3. Abre o recarga el servidor local. `scripts/serve.mjs` sirve el contenido con `Cache-Control: no-cache`.
4. Revisa y registra los cambios en las fuentes del repositorio. El árbol generado, la ROM comprimida y los recursos descargados se pueden reconstruir a partir de un clon limpio.

El build fija pokeemerald-expansion en `8fb02c6f2b81271e795d3f6aca61733fec739016`, aplica Hall of Legends, el contenido de Daily Steven y la localización, y usa GCC ARM moderno. El resultado es la ROM de Daily Steven. Puedes abrir `pokeemerald-expansion/pokefirered.gba` en mGBA y pulsar Inicio en el título si arrancas sin semilla. La página de `web/` usa HTML, CSS y módulos JavaScript sin una fase de empaquetado del frontend. [Guía de contribución y publicación](AGENTS.md)

## Pruebas y depuración

Antes de compilar, los validadores comprueban los nombres, la localización, el contenido de tercera generación y las listas de aprendizaje. Las pruebas de mecánicas ejecutan el motor de batalla.

```sh
python3 tests/partner_distribution.py
bash scripts/run-gen3-tests.sh Gen3
bash scripts/verify-content.sh
```

- **Selección del compañero.** La prueba usa semillas de cien años de calendario y comprueba el reparto, la repetición y la independencia del orden de consulta.
- **Combate.** La suite de Gen 3 cubre, entre otros casos, los ataques de área, los relevos al debilitarse un Pokémon y Carga. La publicación de `content-v0.1.3` pasó 137 pruebas de Gen 3. [Ejecución oficial del pack](https://github.com/unaisdev/dailysteven/actions/runs/45020496220)
- **Equipos.** Los scripts de `tests/teams/` vuelcan el equipo de una ROM de depuración y lo validan frente a los datos de referencia. Configura `ROMGZ` y `ELF` con los archivos de la misma compilación de depuración. El volcado requiere mGBA, Xvfb y el compilador ARM; usa una sesión aislada porque el script administra sus propios procesos de emulación. [Volcado](tests/teams/dump_teams.sh) · [Validador](tests/teams/verify_teams.py)
- **Ciclo web.** `tests/web/cycle.mjs` comprueba el arranque, el resultado y el bloqueo del reto hasta el día siguiente en Chrome. Usa Node.js con WebSocket y, si Chrome está en otra ruta, configura la variable `CHROME`. [Prueba de navegador](tests/web/cycle.mjs)

En Windows, `powershell -ExecutionPolicy Bypass -File scripts\build-wsl.ps1 -Debug` genera `web/game/dailymaximo-debug.gba.gz`. Con el servidor en marcha, añade `?debug=1&seed=900001` para la derrota preparada con seis Magikarp, `?debug=1&seed=900002` para la victoria preparada contra Aura o `?debug=1&seed=900003` para su equivalente contra Blasco. Añade `&daily=1` para probar el ciclo diario. Las estadísticas de depuración tienen su propia clave y esos casos se reservan para la ROM de pruebas.

En la consola de la web local, `dailyMaximo.emulator` muestra la instancia del emulador y `dailyMaximo.stats` devuelve las estadísticas. Para reiniciar únicamente el historial de ese servidor local, puedes eliminar la clave `hortelano:dailysteven:stats` de su `localStorage`.

## Jugar en local

Después de compilar y descargar los recursos, `node scripts/serve.mjs` sirve `web/` en `http://localhost:8080`. Pasa un puerto como argumento si quieres otro, por ejemplo `node scripts/serve.mjs 3000`. El navegador guarda las estadísticas por origen, de modo que puertos o perfiles diferentes llevan historiales separados.

- `http://localhost:8080/` abre el reto diario.
- `http://localhost:8080/?seed=28` abre una práctica contra Aura.
- `http://localhost:8080/?seed=113` abre una práctica contra Blasco.

## Publicación del pack

1. Escribe hasta ocho viñetas en castellano en [Novedades](content/NOVEDADES.md) y comprueba que el lanzador admite el protocolo `dailymax-conf-v1+status-v1`.
2. Ejecuta manualmente el workflow **Release content pack** indicando la versión `X.Y.Z`, o crea la etiqueta `content-vX.Y.Z`. El workflow compila desde limpio, ejecuta los validadores y las pruebas de Gen 3, regenera los símbolos del archivo y prepara un borrador.
3. Verifica el ZIP, `SHA256SUMS`, el manifiesto, la correspondencia entre `archive/symbols.json` y la ROM y la compatibilidad con el lanzador. Publica el borrador una vez revisado.
4. Vacía las novedades consumidas mediante una PR normal con `scripts/clear-novedades.sh`.

Un cambio en `main` por sí solo carece de disparador de publicación. Para empaquetar en local, compila y ejecuta `VERSION=X.Y.Z bash scripts/package-content.sh`. El resultado queda en `dist/content/` e incluye `dailysteven-content.zip`, una copia versionada y `SHA256SUMS`. [Workflow oficial](.github/workflows/release-content.yml)

## Créditos y licencias

Daily Steven es un proyecto gratuito de fans e independiente de Nintendo, Creatures Inc., GAME FREAK inc. y The Pokémon Company. Pokémon y sus nombres, personajes y contenidos pertenecen a sus titulares.

- **amoi-builds**, por el formato diario de Daily Red y Hall of Legends. [Daily Red](https://github.com/amoi-builds/dailyred) · [Hall of Legends](https://github.com/amoi-builds/halloflegends)
- **Rom Hacking Hideout y sus colaboradores**, por pokeemerald-expansion, y **pret**, por las descompilaciones de Pokémon Esmeralda y Rojo Fuego. [Créditos de la expansión](https://github.com/rh-hideout/pokeemerald-expansion/blob/8fb02c6f2b81271e795d3f6aca61733fec739016/CREDITS.md) · [pret](https://github.com/pret)
- **Acimut**, por las tablas españolas de Rojo Fuego, rama `pret_europe` en el commit `6651454608dcc61356b60c20154214d773689730`. [Origen de la localización](locale/es/SOURCE.json)
- **Sprites de Hall of Legends**, recopilados en The Spriters Resource. Ethan por FourLeafSunny, Cynthia por lichen, Silver por Nx-Kun, mufasakong y Dragoon, Dawn por Kaori y Barry por Gridiron, Eevee, Zhen Lin, pika y Pipian. [Créditos completos del pack](content/licenses.txt)
- **Nostalgist.js, RetroArch (GPL-3.0) y mGBA (MPL-2.0)**, por el reproductor y el núcleo de emulación. Los créditos de los sprites y del resto de componentes figuran en las licencias del pack. [Licencias del contenido](content/licenses.txt) · [Licencias de la web](web/licenses.txt)

## Limitaciones conocidas

- **Hora local.** El reto cambia a medianoche en cada zona horaria. Dos personas en zonas distintas pueden pasar al siguiente equipo en momentos diferentes.
- **Historial local.** Las estadísticas se guardan en el navegador o en los datos locales del lanzador y dependen del perfil. Borrar esos datos borra también el historial asociado.
- **Reinicio.** La combinación A+B+Inicio+Seleccionar se desactiva durante una partida diaria. El intento queda consumido en cuanto empieza el combate.
- **Archivo.** Se pueden consultar y repetir fechas cerradas, hasta ayer. El equipo de hoy se ve al abrir el reto. Los símbolos del archivo deben corresponder exactamente a la ROM instalada.
- **Audio.** Algunos navegadores esperan una tecla o un clic antes de permitir sonido. Las pruebas automatizadas se centran en el estado del juego y la interfaz.
- **Ranking.** El ranking de Steven está pendiente. Puedes jugar el reto y usar el archivo histórico.

## Estructura del repositorio

| Ruta | Contenido |
| --- | --- |
| [overlay/](overlay/) | C del reto, tablas de aprendizaje, escena, equipos, recursos gráficos y pruebas de batalla. El código mantiene el nombre interno `dailymaximo`. |
| [locale/es/](locale/es/) | Tablas y correcciones en castellano. |
| [patches/](patches/) | Parche de Hall of Legends sobre la versión fijada de la expansión. |
| [scripts/](scripts/) | Preparación del motor, herramientas, compilación, servidor y empaquetado. |
| [web/](web/) | Reproductor web, semilla diaria, comunicación con la ROM y estadísticas. |
| [tests/](tests/) | Pruebas de distribución del compañero, equipos y ciclo web. |
| [content/](content/) | Manifiesto, licencias y novedades del pack. |
| [docs/](docs/) | Especificación, revisiones y documentación técnica del reto. |
