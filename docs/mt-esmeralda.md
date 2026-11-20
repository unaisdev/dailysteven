# Datos de aprendizaje del jugador: fuente Esmeralda

El generador del equipo del jugador usa los datos de **Pokémon Esmeralda** (pret/pokeemerald), porque los rivales y el juego
se basan en Esmeralda. La regla de Daily Red no cambia: hasta 4 movimientos distintos al azar entre los de nivel (cualquier
nivel) y las 50 MT + 8 MO de 3.ª gen, sin tutores ni huevo, y al menos uno de daño. Solo cambia la fuente de los datos.

- Referencia: `tests/reference/emerald_learnsets.json` (commit de pokeemerald dentro). Se regenera con
  `python3 tools/emerald_reference.py --emerald ~/refs/pokeemerald --game <expansion> --write`.
- Tabla que usa el juego: `overlay/include/dailymax_emerald.h` (generada de la referencia).
- Comprobación al compilar: `scripts/check-tmhm.py` (MT/MO y movimientos por nivel de las 386 especies).
- Independiente del generador: `tests/teams/verify_teams.py` usa la misma referencia.
- La tabla de Rojo Fuego que se hizo en la ronda 6A se archivó en `tests/reference/archivo/` (para comparar con
  `tools/frlg_reference.py`). **Las MT/MO de Esmeralda y de Rojo Fuego son idénticas en las 386 especies.**

- Fuente: pret/pokeemerald @ `9103e529c2fbdd7add4a1ff3350c6c1f92112c42`.
- Especies 1-386 comparadas: 386.
- **MT/MO**: especies cuya compatibilidad de Esmeralda difiere de la expansion moderna: **252**.
- **Por nivel**: especies cuyas listas (nivel + movimiento) difieren entre Esmeralda y la expansion en GEN_3: **15** (de ellas, 14 con distinto CONJUNTO de movimientos; el resto solo cambia el nivel).
- El generador usa SOLO Esmeralda para ambas cosas (tablas en `overlay/include/dailymax_emerald.h`); el resto del juego sigue con la expansion.

## Esmeralda frente a la tabla de Rojo Fuego de la ronda 6A: 0 especies cambian

Especie: MT que Esmeralda AÑADE | MT que Esmeralda QUITA respecto a Rojo Fuego


## MT/MO: Esmeralda frente a la expansion moderna (primeras 30)

Especie: MT que Esmeralda tiene y la expansion no | MT que la expansion tiene y Esmeralda no

- BULBASAUR: +- | -LIGHT_SCREEN, REFLECT, SAFEGUARD
- IVYSAUR: +- | -LIGHT_SCREEN, REFLECT, ROAR, SAFEGUARD
- VENUSAUR: +- | -LIGHT_SCREEN, REFLECT, SAFEGUARD
- CHARMANDER: +- | -REFLECT, ROAR, ROCK_TOMB
- CHARMELEON: +- | -REFLECT, ROAR, ROCK_TOMB
- CHARIZARD: +- | -REFLECT, ROCK_TOMB, SANDSTORM, SOLAR_BEAM
- SQUIRTLE: +- | -REFLECT, ROCK_TOMB
- WARTORTLE: +- | -REFLECT, ROCK_TOMB
- BLASTOISE: +- | -REFLECT, ROCK_TOMB
- BUTTERFREE: +- | -REFLECT
- BEEDRILL: +- | -FLASH, REFLECT
- PIDGEY: +- | -REFLECT
- PIDGEOTTO: +- | -REFLECT
- PIDGEOT: +- | -REFLECT
- RATTATA: +- | -SLUDGE_BOMB
- RATICATE: +- | -SLUDGE_BOMB
- EKANS: +- | -ROCK_TOMB
- ARBOK: +- | -ROCK_TOMB
- PIKACHU: +- | -CALM_MIND, REFLECT, SURF, THIEF
- RAICHU: +- | -CALM_MIND, REFLECT, SAFEGUARD, SURF
- SANDSHREW: +- | -SAFEGUARD
- SANDSLASH: +- | -SAFEGUARD
- NIDORAN_F: +- | -REFLECT
- NIDORINA: +- | -REFLECT
- NIDOQUEEN: +- | -REFLECT
- NIDORAN_M: +- | -REFLECT
- NIDORINO: +- | -REFLECT
- NIDOKING: +- | -REFLECT
- CLEFAIRY: +- | -ROCK_SMASH, SKILL_SWAP, THIEF
- CLEFABLE: +- | -ROCK_SMASH, SKILL_SWAP, THIEF

## Por nivel: Esmeralda frente a la expansion GEN_3 (primeras 30)

- KRABBY: solo Esmeralda [(12, 'ICEGRIP')] | solo expansion [(12, 'ISEGRIP')]
- KINGLER: solo Esmeralda [(1, 'ICEGRIP'), (12, 'ICEGRIP')] | solo expansion [(1, 'ISEGRIP'), (12, 'ISEGRIP')]
- HITMONLEE: solo Esmeralda [(26, 'IJUMPKICK')] | solo expansion [(26, 'IGHJUMPKICK')]
- PINSIR: solo Esmeralda [(1, 'ICEGRIP')] | solo expansion [(1, 'ISEGRIP')]
- UNOWN: solo Esmeralda [(1, 'IDDENPOWER')] | solo expansion []
- MAKUHITA: solo Esmeralda [(31, 'MELLINGSALT')] | solo expansion [(31, 'MELLINGSALTS')]
- HARIYAMA: solo Esmeralda [(33, 'MELLINGSALT')] | solo expansion [(33, 'MELLINGSALTS')]
- MAWILE: solo Esmeralda [(21, 'ICEGRIP')] | solo expansion [(21, 'ISEGRIP')]
- MEDITITE: solo Esmeralda [(32, 'IJUMPKICK')] | solo expansion [(32, 'IGHJUMPKICK')]
- MEDICHAM: solo Esmeralda [(32, 'IJUMPKICK')] | solo expansion [(32, 'IGHJUMPKICK')]
- CORPHISH: solo Esmeralda [(10, 'ICEGRIP')] | solo expansion [(10, 'ISEGRIP')]
- CRAWDAUNT: solo Esmeralda [(1, 'ICEGRIP'), (10, 'ICEGRIP')] | solo expansion [(1, 'ISEGRIP'), (10, 'ISEGRIP')]
- CASTFORM: solo Esmeralda [(1, 'ACKLE'), (10, 'MBER'), (10, 'OWDERSNOW'), (10, 'ATERGUN'), (20, 'AIL')] | solo expansion []
- JIRACHI: solo Esmeralda [(5, 'EST'), (30, 'EST')] | solo expansion [(17, 'EST')]
- DEOXYS: solo Esmeralda [(1, 'EER'), (1, 'RAP'), (5, 'IGHTSHADE'), (10, 'OUBLETEAM'), (15, 'NOCKOFF')] | solo expansion []
