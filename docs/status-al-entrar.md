# Estados alterados al entrar en combate

## ¿Se puede?

**Sí.** En pokeemerald-expansion (y en FireRed) un Pokémon de equipo puede
llevar un estado no volátil antes de que empiece la pelea:

```c
u32 status = STATUS1_BURN; // o POISON, PARALYSIS, FREEZE, SLEEP (+ turnos)
SetMonData(&party[i], MON_DATA_STATUS, &status);
```

También existe el campo de entrenador `startingStatus` en expansion para
aplicarlo al crear el equipo rival.

Hall of Legends **no** lo usa hoy: sus equipos entran limpios.

## Efectos en el daily

| Estado | Efecto práctico |
|--------|-----------------|
| Quemadura | Baja el Atk físico; daño por turno. **Guts / Facade** se benefician. |
| Veneno / tóxico | Daño por turno; con Leftovers cambia el ritmo. |
| Parálisis | Puede impedir atacar; baja Speed. |
| Sueño / congelación | Frustrante en un daily de una sola pelea: no recomendable. |

El generador de equipo del jugador exige **HP lleno**; el status no invalida
eso (`IsValidTeamMon` no mira `MON_DATA_STATUS`).

## Decisión v1

- Rivales (Máximo / Aura / Blasco): **sin** status forzado (fairness).
- Jugador: helper `DailyMax_MaybeApplyStatus` existe pero **no aplica nada**.
- Bit reservado en la semilla: `DAILYMAX_SEED_STATUS_BIT` (`0x100`) para un
  futuro modo reto; desactivado en v1.

Para probar en local, descomenta el ejemplo en `src/dailymaximo_team.c`.
