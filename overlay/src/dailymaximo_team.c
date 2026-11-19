#include "global.h"
#include "dailymaximo.h"
#include "constants/dailymaximo.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "data.h"
#include "move.h"
#include "battle.h"

// Generador del equipo del jugador de Daily Steven.
//
// Copia, regla por regla, el generador de Daily Red (amoi-builds/dailyred, src/dailyred_team.c):
//   - 6 especies distintas de las 386 de 1.ª-3.ª gen, con suma de estadísticas base >= MIN_TEAM_BASE_STAT_TOTAL.
//     1 equipo de cada LEGENDARY_TEAM_ODDS lleva exactamente un legendario; los demás, ninguno.
//   - Nivel 100, personalidad aleatoria (naturaleza, género y habilidad), IV aleatorios de MIN_TEAM_IV a 31 y
//     EV enfocados en sus dos mejores estadísticas (AssignFocusedEVs).
//   - Hasta 4 movimientos DISTINTOS al azar entre los que aprende por nivel (cualquier nivel) o por MT/MO
//     DE 3.ª GEN (lista fija sGen3Machines: 50 MT + 8 MO). Si la especie tiene algún movimiento de daño, al
//     menos uno de los 4 lo es.
//   - Un objeto al azar entre sBattleItems y los potenciadores de tipo de los tipos con los que ataca.
// Usa su propio RNG (ISO_RANDOMIZE1), separado del Random() del juego: el equipo solo depende de la semilla.
//
// Diferencias inevitables con Daily Red (vienen del motor, no de las reglas):
//   - La compatibilidad con MT/MO la da la tabla de la expansion (CanLearnTeachableMove), no la del
//     cartucho original. La lista de movimientos candidatos sí es la de 3.ª gen.
//   - Las especies se numeran como en la expansion (1-386 seguidas), no como en FireRed (con hueco de Unown);
//     el conjunto de especies posibles es el mismo, pero la misma semilla no da el mismo equipo que Red.

#define MAX_CANDIDATE_MOVES  128
#define MAX_TOTAL_EVS        510
#define MAX_STAT_EVS         252
#define MIN_TEAM_IV          25
#define MAX_SPECIES_ATTEMPTS 1000
#define MAX_MON_ATTEMPTS     10
#define MAX_TEAM_ATTEMPTS    1000
#define MIN_TEAM_BASE_STAT_TOTAL 2700
#define LEGENDARY_TEAM_ODDS  8
#define SPECIES_GEN3_END     SPECIES_DEOXYS

#if DAILYMAX_DEBUG
// Solo en la ROM de pruebas (-Debug): volcado del equipo para comprobarlo desde fuera.
// Por Pokémon (34 valores de 16 bits): [0] especie, [1-4] movimientos, [5-8] tipo de cada movimiento,
// [9-12] potencia de cada movimiento, [13] objeto, [14-19] EV y [20-25] IV (PS,At,Df,Vel,AtE,DfE),
// [26-31] estadísticas base (mismo orden), [32-33] personalidad (mitad baja, mitad alta).
EWRAM_DATA u16 gDailyMaxTeamDump[PARTY_SIZE][34] = {0};
#endif

static const u16 sLegendaries[] =
{
    SPECIES_ARTICUNO, SPECIES_ZAPDOS, SPECIES_MOLTRES, SPECIES_MEWTWO, SPECIES_MEW,
    SPECIES_RAIKOU, SPECIES_ENTEI, SPECIES_SUICUNE, SPECIES_LUGIA, SPECIES_HO_OH, SPECIES_CELEBI,
    SPECIES_REGIROCK, SPECIES_REGICE, SPECIES_REGISTEEL, SPECIES_LATIAS, SPECIES_LATIOS,
    SPECIES_KYOGRE, SPECIES_GROUDON, SPECIES_RAYQUAZA, SPECIES_JIRACHI, SPECIES_DEOXYS,
};

// Objetos que ayudan a cualquier Pokémon en un solo combate.
static const u16 sBattleItems[] =
{
    ITEM_LEFTOVERS,
    ITEM_SHELL_BELL,
    ITEM_CHOICE_BAND,
    ITEM_QUICK_CLAW,
    ITEM_KINGS_ROCK,
    ITEM_SCOPE_LENS,
    ITEM_BRIGHT_POWDER,
    ITEM_FOCUS_BAND,
    ITEM_WHITE_HERB,
    ITEM_MENTAL_HERB,
    ITEM_LUM_BERRY,
    ITEM_SITRUS_BERRY,
    ITEM_CHESTO_BERRY,
    ITEM_LIECHI_BERRY,
    ITEM_SALAC_BERRY,
    ITEM_PETAYA_BERRY,
};

// El objeto que potencia un 10 % los movimientos de cada tipo (3.ª gen).
static const u16 sTypeBoostItems[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = ITEM_SILK_SCARF,
    [TYPE_FIGHTING] = ITEM_BLACK_BELT,
    [TYPE_FLYING]   = ITEM_SHARP_BEAK,
    [TYPE_POISON]   = ITEM_POISON_BARB,
    [TYPE_GROUND]   = ITEM_SOFT_SAND,
    [TYPE_ROCK]     = ITEM_HARD_STONE,
    [TYPE_BUG]      = ITEM_SILVER_POWDER,
    [TYPE_GHOST]    = ITEM_SPELL_TAG,
    [TYPE_STEEL]    = ITEM_METAL_COAT,
    [TYPE_MYSTERY]  = ITEM_NONE,
    [TYPE_FIRE]     = ITEM_CHARCOAL,
    [TYPE_WATER]    = ITEM_MYSTIC_WATER,
    [TYPE_GRASS]    = ITEM_MIRACLE_SEED,
    [TYPE_ELECTRIC] = ITEM_MAGNET,
    [TYPE_PSYCHIC]  = ITEM_TWISTED_SPOON,
    [TYPE_ICE]      = ITEM_NEVER_MELT_ICE,
    [TYPE_DRAGON]   = ITEM_DRAGON_FANG,
    [TYPE_DARK]     = ITEM_BLACK_GLASSES,
};

// Los 50 MT y los 8 MO de 3.ª generación. La expansion trae las MT de todas las generaciones; solo estas
// pueden salir como movimiento candidato.
static const u16 sGen3Machines[] =
{
    // MT01-MT50
    MOVE_FOCUS_PUNCH, MOVE_DRAGON_CLAW, MOVE_WATER_PULSE, MOVE_CALM_MIND, MOVE_ROAR,
    MOVE_TOXIC, MOVE_HAIL, MOVE_BULK_UP, MOVE_BULLET_SEED, MOVE_HIDDEN_POWER,
    MOVE_SUNNY_DAY, MOVE_TAUNT, MOVE_ICE_BEAM, MOVE_BLIZZARD, MOVE_HYPER_BEAM,
    MOVE_LIGHT_SCREEN, MOVE_PROTECT, MOVE_RAIN_DANCE, MOVE_GIGA_DRAIN, MOVE_SAFEGUARD,
    MOVE_FRUSTRATION, MOVE_SOLAR_BEAM, MOVE_IRON_TAIL, MOVE_THUNDERBOLT, MOVE_THUNDER,
    MOVE_EARTHQUAKE, MOVE_RETURN, MOVE_DIG, MOVE_PSYCHIC, MOVE_SHADOW_BALL,
    MOVE_BRICK_BREAK, MOVE_DOUBLE_TEAM, MOVE_REFLECT, MOVE_SHOCK_WAVE, MOVE_FLAMETHROWER,
    MOVE_SLUDGE_BOMB, MOVE_SANDSTORM, MOVE_FIRE_BLAST, MOVE_ROCK_TOMB, MOVE_AERIAL_ACE,
    MOVE_TORMENT, MOVE_FACADE, MOVE_SECRET_POWER, MOVE_REST, MOVE_ATTRACT,
    MOVE_THIEF, MOVE_STEEL_WING, MOVE_SKILL_SWAP, MOVE_SNATCH, MOVE_OVERHEAT,
    // MO01-MO08
    MOVE_CUT, MOVE_FLY, MOVE_SURF, MOVE_STRENGTH, MOVE_FLASH,
    MOVE_ROCK_SMASH, MOVE_WATERFALL, MOVE_DIVE,
};

static u16 TeamRandom(u32 *rng)
{
    *rng = ISO_RANDOMIZE1(*rng);
    return *rng >> 16;
}

static u32 TeamRandom32(u32 *rng)
{
    u32 lo = TeamRandom(rng);
    return lo | ((u32)TeamRandom(rng) << 16);
}

// Qué especies pueden salir en el equipo del jugador.
static bool32 IsEligibleSpecies(u16 species)
{
    if (species == SPECIES_NONE || species > SPECIES_GEN3_END)
        return FALSE;
    if (gSpeciesInfo[species].baseHP == 0)
        return FALSE;
    return TRUE;
}

static u16 GetBaseStatTotal(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->baseHP + info->baseAttack + info->baseDefense
         + info->baseSpeed + info->baseSpAttack + info->baseSpDefense;
}

static bool32 IsLegendary(u16 species)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sLegendaries); i++)
    {
        if (sLegendaries[i] == species)
            return TRUE;
    }
    return FALSE;
}

static bool32 IsSpeciesTaken(u16 species, const u16 *taken, u8 takenCount)
{
    u8 i;

    for (i = 0; i < takenCount; i++)
    {
        if (taken[i] == species)
            return TRUE;
    }
    return FALSE;
}

// Una especie no legendaria al azar que aún no está en el equipo.
static u16 PickSpecies(u32 *rng, const u16 *taken, u8 takenCount)
{
    u16 species;
    s32 i;

    for (i = 0; i < MAX_SPECIES_ATTEMPTS; i++)
    {
        species = 1 + TeamRandom(rng) % SPECIES_GEN3_END;
        if (IsEligibleSpecies(species) && !IsLegendary(species) && !IsSpeciesTaken(species, taken, takenCount))
            return species;
    }
    for (species = 1; species <= SPECIES_GEN3_END; species++)
    {
        if (IsEligibleSpecies(species) && !IsLegendary(species) && !IsSpeciesTaken(species, taken, takenCount))
            return species;
    }
    return SPECIES_BULBASAUR;
}

// Decide si el equipo lleva legendario y completa el resto hasta cumplir el mínimo de estadísticas base.
static void PickTeamSpecies(u16 *species, u32 *rng)
{
    u16 best[PARTY_SIZE];
    u32 total, bestTotal = 0;
    s32 attempt;
    u8 i, first = 0, slot;
    u16 temp;

    if (TeamRandom(rng) % LEGENDARY_TEAM_ODDS == 0)
        species[first++] = sLegendaries[TeamRandom(rng) % ARRAY_COUNT(sLegendaries)];

    for (attempt = 0; attempt < MAX_TEAM_ATTEMPTS; attempt++)
    {
        total = 0;
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (i >= first)
                species[i] = PickSpecies(rng, species, i);
            total += GetBaseStatTotal(species[i]);
        }
        if (total >= MIN_TEAM_BASE_STAT_TOTAL)
            break;
        if (total > bestTotal)
        {
            bestTotal = total;
            memcpy(best, species, sizeof(best));
        }
    }
    if (attempt == MAX_TEAM_ATTEMPTS)
        memcpy(species, best, sizeof(best));

    // El legendario (si hay) va a un hueco al azar, no siempre el primero.
    slot = TeamRandom(rng) % PARTY_SIZE;
    SWAP(species[0], species[slot], temp);
}

static void AddCandidateMove(u16 *moves, u8 *count, u16 move)
{
    u8 i;

    if (move == MOVE_NONE || move >= MOVES_COUNT_GEN3 || *count >= MAX_CANDIDATE_MOVES)
        return;
    for (i = 0; i < *count; i++)
    {
        if (moves[i] == move)
            return;
    }
    moves[(*count)++] = move;
}

// Todo lo que la especie puede aprender por nivel (cualquier nivel) o por MT/MO de 3.ª gen.
static u8 GetLearnableMoves(u16 species, u16 *moves)
{
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);
    u8 count = 0;
    u32 i;

    for (i = 0; learnset[i].move != LEVEL_UP_MOVE_END; i++)
        AddCandidateMove(moves, &count, learnset[i].move);
    for (i = 0; i < ARRAY_COUNT(sGen3Machines); i++)
    {
        if (CanLearnTeachableMove(species, sGen3Machines[i]))
            AddCandidateMove(moves, &count, sGen3Machines[i]);
    }
    return count;
}

static bool32 IsDamagingMove(u16 move)
{
    // Los de daño fijo (Sísmico, Furia Dragón...) tienen potencia 1.
    return GetMovePower(move) != 0;
}

static void AssignRandomMoves(struct Pokemon *mon, u32 *rng)
{
    u16 moves[MAX_CANDIDATE_MOVES];
    u8 count, numMoves, numDamaging, i, j;
    u16 temp;

    count = GetLearnableMoves(GetMonData(mon, MON_DATA_SPECIES), moves);
    numMoves = min(count, MAX_MON_MOVES);

    // Barajado parcial de Fisher-Yates: moves[0..numMoves) queda como una selección uniforme sin repetidos.
    for (i = 0; i < numMoves; i++)
    {
        j = i + TeamRandom(rng) % (count - i);
        SWAP(moves[i], moves[j], temp);
    }

    // Si ninguno de los elegidos hace daño, se cambia uno por un movimiento de daño del resto (si lo hay).
    for (i = 0; i < numMoves && !IsDamagingMove(moves[i]); i++)
        ;
    if (i == numMoves)
    {
        numDamaging = 0;
        for (j = numMoves; j < count; j++)
        {
            if (IsDamagingMove(moves[j]))
                numDamaging++;
        }
        if (numDamaging != 0)
        {
            u8 pick = TeamRandom(rng) % numDamaging;

            for (j = numMoves; j < count; j++)
            {
                if (IsDamagingMove(moves[j]) && pick-- == 0)
                    break;
            }
            i = TeamRandom(rng) % numMoves;
            SWAP(moves[i], moves[j], temp);
        }
    }

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, i < numMoves ? moves[i] : MOVE_NONE, i);
}

static void AssignRandomItem(struct Pokemon *mon, u32 *rng)
{
    u16 items[ARRAY_COUNT(sBattleItems) + MAX_MON_MOVES];
    u8 count = 0;
    u8 i, j;
    u16 move, item;

    for (i = 0; i < ARRAY_COUNT(sBattleItems); i++)
        items[count++] = sBattleItems[i];

    // Potenciadores solo de los tipos con los que ataca. Los de daño fijo (potencia 1: Sísmico, Poder Oculto...)
    // no se potencian, así que se saltan.
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move == MOVE_NONE || GetMovePower(move) <= 1)
            continue;
        item = sTypeBoostItems[GetMoveType(move)];
        for (j = 0; j < count && items[j] != item; j++)
            ;
        if (item != ITEM_NONE && j == count)
            items[count++] = item;
    }

    item = items[TeamRandom(rng) % count];
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
}

// Si dos estadísticas empatan, gana la que va primero.
static const u8 sEVStatPriority[NUM_STATS] = {STAT_ATK, STAT_SPATK, STAT_SPEED, STAT_HP, STAT_DEF, STAT_SPDEF};

// Mismo entrenamiento que el de RED/GOLD: 252 EV en cada una de sus dos estadísticas base más altas y los 6
// últimos en PS (o en la tercera mejor si PS es una de las dos). Ataque y At. Esp. solo cuentan si algún
// movimiento los usa, y en 3.ª gen eso lo decide el TIPO del movimiento (no la categoría moderna del
// movimiento). Hace falta tener ya los movimientos.
static void AssignFocusedEVs(struct Pokemon *mon)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[GetMonData(mon, MON_DATA_SPECIES)];
    u8 base[NUM_STATS];
    bool8 usable[NUM_STATS] = {TRUE, FALSE, TRUE, TRUE, FALSE, TRUE};
    u8 order[NUM_STATS];
    u8 evs[NUM_STATS] = {0};
    u8 count = 0;
    u8 i, j, stat;
    u16 move;

    base[STAT_HP] = info->baseHP;
    base[STAT_ATK] = info->baseAttack;
    base[STAT_DEF] = info->baseDefense;
    base[STAT_SPEED] = info->baseSpeed;
    base[STAT_SPATK] = info->baseSpAttack;
    base[STAT_SPDEF] = info->baseSpDefense;

    // Los movimientos de estado y los de daño fijo (potencia 1) no usan ninguna de las dos.
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move == MOVE_NONE || GetMovePower(move) <= 1)
            continue;
        if (gTypesInfo[GetMoveType(move)].damageCategory == DAMAGE_CATEGORY_PHYSICAL)
            usable[STAT_ATK] = TRUE;
        else
            usable[STAT_SPATK] = TRUE;
    }

    // Estadísticas útiles de mayor a menor base (inserción estable; los empates siguen sEVStatPriority).
    for (i = 0; i < NUM_STATS; i++)
    {
        stat = sEVStatPriority[i];
        if (!usable[stat])
            continue;
        for (j = count; j > 0 && base[order[j - 1]] < base[stat]; j--)
            order[j] = order[j - 1];
        order[j] = stat;
        count++;
    }

    evs[order[0]] = MAX_STAT_EVS;
    evs[order[1]] = MAX_STAT_EVS;
    stat = (order[0] != STAT_HP && order[1] != STAT_HP) ? STAT_HP : order[2];
    evs[stat] = MAX_TOTAL_EVS - 2 * MAX_STAT_EVS;

    for (stat = 0; stat < NUM_STATS; stat++)
        SetMonData(mon, MON_DATA_HP_EV + stat, &evs[stat]);
}

// Nivel 100 con personalidad aleatoria y entrenador = el jugador (así obedece sin medallas).
static void CreateTeamMon(struct Pokemon *mon, u16 species, u32 *rng)
{
    CreateMon(mon, species, MAX_LEVEL, TeamRandom32(rng), OTID_STRUCT_PLAYER_ID);
}

// IV, EV, objeto y estadísticas finales, con los movimientos ya puestos.
static void FinishTeamMon(struct Pokemon *mon, u32 *rng)
{
    u8 metLocation = METLOC_FATEFUL_ENCOUNTER;
    u8 fateful = TRUE;
    u8 i, iv;

    for (i = 0; i < NUM_STATS; i++)
    {
        iv = MIN_TEAM_IV + TeamRandom(rng) % (MAX_PER_STAT_IVS - MIN_TEAM_IV + 1);
        SetMonData(mon, MON_DATA_HP_IV + i, &iv);
    }
    AssignFocusedEVs(mon);

    // Mew y Deoxys desobedecen si no son Pokémon de evento.
    SetMonData(mon, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &fateful);
    SetMonData(mon, MON_DATA_MET_LOCATION, &metLocation);

    CalculateMonStats(mon);
    AssignRandomItem(mon, rng);
}

static void CreateDailyTeamMon(struct Pokemon *mon, u16 species, u32 *rng)
{
    CreateTeamMon(mon, species, rng);
    AssignRandomMoves(mon, rng);
    FinishTeamMon(mon, rng);
}

static bool32 IsValidTeamMon(struct Pokemon *mon)
{
    u16 moves[MAX_MON_MOVES];
    u8 i, j;

    if (!IsEligibleSpecies(GetMonData(mon, MON_DATA_SPECIES)))
        return FALSE;
    if (GetMonData(mon, MON_DATA_LEVEL) != MAX_LEVEL)
        return FALSE;
    if (GetMonData(mon, MON_DATA_MAX_HP) == 0 || GetMonData(mon, MON_DATA_HP) != GetMonData(mon, MON_DATA_MAX_HP))
        return FALSE;

    for (i = 0; i < MAX_MON_MOVES; i++)
        moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
    if (moves[0] == MOVE_NONE || GetMonData(mon, MON_DATA_HELD_ITEM) == ITEM_NONE)
        return FALSE;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (moves[i] == MOVE_NONE)
            continue;
        if (GetMonData(mon, MON_DATA_PP1 + i) == 0)
            return FALSE;
        for (j = i + 1; j < MAX_MON_MOVES; j++)
        {
            if (moves[i] == moves[j])
                return FALSE;
        }
    }
    return TRUE;
}

#if DAILYMAX_DEBUG
static void DumpTeam(struct Pokemon *party)
{
    u8 i, k;
    u32 personality;
    u16 move;
    const struct SpeciesInfo *info;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 *d = gDailyMaxTeamDump[i];

        personality = GetMonData(&party[i], MON_DATA_PERSONALITY);
        d[0] = GetMonData(&party[i], MON_DATA_SPECIES);
        info = &gSpeciesInfo[d[0]];
        for (k = 0; k < MAX_MON_MOVES; k++)
        {
            move = GetMonData(&party[i], MON_DATA_MOVE1 + k);
            d[1 + k] = move;
            d[5 + k] = move != MOVE_NONE ? GetMoveType(move) : 0;
            d[9 + k] = move != MOVE_NONE ? GetMovePower(move) : 0;
        }
        d[13] = GetMonData(&party[i], MON_DATA_HELD_ITEM);
        for (k = 0; k < NUM_STATS; k++)
        {
            d[14 + k] = GetMonData(&party[i], MON_DATA_HP_EV + k);
            d[20 + k] = GetMonData(&party[i], MON_DATA_HP_IV + k);
        }
        d[26] = info->baseHP;
        d[27] = info->baseAttack;
        d[28] = info->baseDefense;
        d[29] = info->baseSpeed;
        d[30] = info->baseSpAttack;
        d[31] = info->baseSpDefense;
        d[32] = personality & 0xFFFF;
        d[33] = personality >> 16;
    }
}

// Equipo de prueba para forzar una DERROTA rápida: seis Magikarp (solo en la ROM de pruebas).
static bool32 TryBuildLoseTeam(struct Pokemon *party, u32 seed)
{
    u8 i;

    if (seed != DAILYMAX_DEBUG_SEED_LOSE)
        return FALSE;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        CreateMon(&party[i], SPECIES_MAGIKARP, MAX_LEVEL, 0, OTID_STRUCT_PLAYER_ID);
        SetMonMoveSlot(&party[i], MOVE_SPLASH, 0);
        SetMonMoveSlot(&party[i], MOVE_NONE, 1);
        SetMonMoveSlot(&party[i], MOVE_NONE, 2);
        SetMonMoveSlot(&party[i], MOVE_NONE, 3);
        CalculateMonStats(&party[i]);
    }
    return TRUE;
}
#endif

void DailyMax_GenerateTeam(struct Pokemon *party, u32 *rngState)
{
    u16 species[PARTY_SIZE];
    u8 i, attempt;

    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);

#if DAILYMAX_DEBUG
    if (TryBuildLoseTeam(party, *rngState))
    {
        DumpTeam(party);
        return;
    }
#endif

    PickTeamSpecies(species, rngState);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        for (attempt = 0; attempt < MAX_MON_ATTEMPTS; attempt++)
        {
            CreateDailyTeamMon(&party[i], species[i], rngState);
            if (IsValidTeamMon(&party[i]))
                break;
        }

        // No debería pasar nunca, pero jamás se entrega un Pokémon roto: se usa el conjunto de movimientos
        // por defecto del juego para su nivel.
        if (attempt == MAX_MON_ATTEMPTS)
        {
            CreateTeamMon(&party[i], species[i], rngState);
            GiveMonInitialMoveset(&party[i]);
            FinishTeamMon(&party[i], rngState);
        }
    }
#if DAILYMAX_DEBUG
    DumpTeam(party);
#endif
}

// Estados alterados al empezar el combate (reservado; desactivado). Ver docs/status-al-entrar.md.
void DailyMax_MaybeApplyStatus(struct Pokemon *party, u32 seed)
{
    (void)party;
    (void)seed;
}
