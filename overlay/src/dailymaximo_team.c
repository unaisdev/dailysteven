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

// Team generator RNG (ISO_RANDOMIZE1) — independent of game Random().
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

static const u16 sLegendaries[] =
{
    SPECIES_ARTICUNO, SPECIES_ZAPDOS, SPECIES_MOLTRES, SPECIES_MEWTWO, SPECIES_MEW,
    SPECIES_RAIKOU, SPECIES_ENTEI, SPECIES_SUICUNE, SPECIES_LUGIA, SPECIES_HO_OH, SPECIES_CELEBI,
    SPECIES_REGIROCK, SPECIES_REGICE, SPECIES_REGISTEEL, SPECIES_LATIAS, SPECIES_LATIOS,
    SPECIES_KYOGRE, SPECIES_GROUDON, SPECIES_RAYQUAZA, SPECIES_JIRACHI, SPECIES_DEOXYS,
};

static const u16 sBattleItems[] =
{
    ITEM_LEFTOVERS, ITEM_SHELL_BELL, ITEM_CHOICE_BAND, ITEM_QUICK_CLAW,
    ITEM_KINGS_ROCK, ITEM_SCOPE_LENS, ITEM_BRIGHTPOWDER, ITEM_FOCUS_BAND,
    ITEM_WHITE_HERB, ITEM_MENTAL_HERB, ITEM_LUM_BERRY, ITEM_SITRUS_BERRY,
    ITEM_CHESTO_BERRY, ITEM_LIECHI_BERRY, ITEM_SALAC_BERRY, ITEM_PETAYA_BERRY,
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

static bool32 IsEligibleSpecies(u16 species)
{
    if (species == SPECIES_NONE || species > SPECIES_GEN3_END)
        return FALSE;
#if defined(SPECIES_OLD_UNOWN_B) && defined(SPECIES_OLD_UNOWN_Z)
    if (species >= SPECIES_OLD_UNOWN_B && species <= SPECIES_OLD_UNOWN_Z)
        return FALSE;
#endif
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

    slot = TeamRandom(rng) % PARTY_SIZE;
    SWAP(species[0], species[slot], temp);
}

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
    static const u8 sEVStatPriority[NUM_STATS] = {
        STAT_ATK, STAT_SPATK, STAT_SPEED, STAT_HP, STAT_DEF, STAT_SPDEF
    };

    base[STAT_HP] = info->baseHP;
    base[STAT_ATK] = info->baseAttack;
    base[STAT_DEF] = info->baseDefense;
    base[STAT_SPEED] = info->baseSpeed;
    base[STAT_SPATK] = info->baseSpAttack;
    base[STAT_SPDEF] = info->baseSpDefense;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move == MOVE_NONE || gMovesInfo[move].power <= 1)
            continue;
        if (gMovesInfo[move].category == DAMAGE_CATEGORY_PHYSICAL)
            usable[STAT_ATK] = TRUE;
        else if (gMovesInfo[move].category == DAMAGE_CATEGORY_SPECIAL)
            usable[STAT_SPATK] = TRUE;
    }

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

    if (count == 0)
        return;
    evs[order[0]] = MAX_STAT_EVS;
    if (count > 1)
        evs[order[1]] = MAX_STAT_EVS;
    if (count > 2)
    {
        stat = (order[0] != STAT_HP && order[1] != STAT_HP) ? STAT_HP : order[2];
        evs[stat] = MAX_TOTAL_EVS - 2 * MAX_STAT_EVS;
    }

    for (stat = 0; stat < NUM_STATS; stat++)
        SetMonData(mon, MON_DATA_HP_EV + stat, &evs[stat]);
}

static void AssignRandomItem(struct Pokemon *mon, u32 *rng)
{
    u16 item = sBattleItems[TeamRandom(rng) % ARRAY_COUNT(sBattleItems)];
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
}

static void CreateTeamMon(struct Pokemon *mon, u16 species, u32 *rng)
{
    CreateMon(mon, species, MAX_LEVEL, TeamRandom32(rng), OTID_STRUCT_PLAYER_ID);
}

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
    SetMonData(mon, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &fateful);
    SetMonData(mon, MON_DATA_MET_LOCATION, &metLocation);
    CalculateMonStats(mon);
    AssignRandomItem(mon, rng);
}

static bool32 IsValidTeamMon(struct Pokemon *mon)
{
    if (!IsEligibleSpecies(GetMonData(mon, MON_DATA_SPECIES)))
        return FALSE;
    if (GetMonData(mon, MON_DATA_LEVEL) != MAX_LEVEL)
        return FALSE;
    if (GetMonData(mon, MON_DATA_MAX_HP) == 0
     || GetMonData(mon, MON_DATA_HP) != GetMonData(mon, MON_DATA_MAX_HP))
        return FALSE;
    if (GetMonData(mon, MON_DATA_MOVE1) == MOVE_NONE)
        return FALSE;
    if (GetMonData(mon, MON_DATA_HELD_ITEM) == ITEM_NONE)
        return FALSE;
    return TRUE;
}

static void SetFourMoves(struct Pokemon *mon, u16 a, u16 b, u16 c, u16 d)
{
    SetMonMoveSlot(mon, a, 0);
    SetMonMoveSlot(mon, b, 1);
    SetMonMoveSlot(mon, c, 2);
    SetMonMoveSlot(mon, d, 3);
}

// Practice seeds 900001 / 900002: fast Surf / Earthquake lead + Wobbuffet ally.
// 31 IVs + Spe EVs + +Spe nature so the lead outspeeds Steven's Metagross.
static void FinishGen3ProbeMon(struct Pokemon *mon, u16 item, bool32 investSpeed)
{
    u8 i;
    u8 iv = MAX_PER_STAT_IVS;
    u16 speEv = investSpeed ? MAX_STAT_EVS : 0;
    u16 atkEv = investSpeed ? MAX_STAT_EVS : 0; // SpA or Atk filled below
    u16 hpEv = investSpeed ? (MAX_TOTAL_EVS - 2 * MAX_STAT_EVS) : 0;
    u16 maxHp;

    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_IV + i, &iv);

    if (investSpeed)
    {
        SetMonData(mon, MON_DATA_SPEED_EV, &speEv);
        if (gMovesInfo[GetMonData(mon, MON_DATA_MOVE1)].category == DAMAGE_CATEGORY_PHYSICAL)
            SetMonData(mon, MON_DATA_ATK_EV, &atkEv);
        else
            SetMonData(mon, MON_DATA_SPATK_EV, &atkEv);
        SetMonData(mon, MON_DATA_HP_EV, &hpEv);
    }

    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    CalculateMonStats(mon);
    maxHp = GetMonData(mon, MON_DATA_MAX_HP);
    SetMonData(mon, MON_DATA_HP, &maxHp);
}

static bool32 TryBuildGen3ProbeTeam(struct Pokemon *party, u32 seed)
{
    u8 i;

    if (seed != DAILYMAX_SEED_TEST_SURF && seed != DAILYMAX_SEED_TEST_EARTHQUAKE)
        return FALSE;

    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);

    if (seed == DAILYMAX_SEED_TEST_SURF)
    {
        // Timid Starmie: Surf first vs Metagross / Gardevoir; ally must stay full HP.
        CreateMonWithIVs(&party[0], SPECIES_STARMIE, MAX_LEVEL, NATURE_TIMID, OTID_STRUCT_PLAYER_ID, MAX_PER_STAT_IVS);
        SetFourMoves(&party[0], MOVE_SURF, MOVE_SPLASH, MOVE_SPLASH, MOVE_SPLASH);
        FinishGen3ProbeMon(&party[0], ITEM_FOCUS_BAND, TRUE);
    }
    else
    {
        // Jolly Dugtrio: Earthquake first; ally must take damage.
        CreateMonWithIVs(&party[0], SPECIES_DUGTRIO, MAX_LEVEL, NATURE_JOLLY, OTID_STRUCT_PLAYER_ID, MAX_PER_STAT_IVS);
        SetFourMoves(&party[0], MOVE_EARTHQUAKE, MOVE_SPLASH, MOVE_SPLASH, MOVE_SPLASH);
        FinishGen3ProbeMon(&party[0], ITEM_FOCUS_BAND, TRUE);
    }

    CreateMonWithIVs(&party[1], SPECIES_WOBBUFFET, MAX_LEVEL, NATURE_BOLD, OTID_STRUCT_PLAYER_ID, MAX_PER_STAT_IVS);
    SetFourMoves(&party[1], MOVE_SPLASH, MOVE_NONE, MOVE_NONE, MOVE_NONE);
    FinishGen3ProbeMon(&party[1], ITEM_LEFTOVERS, FALSE);

    for (i = 2; i < PARTY_SIZE; i++)
    {
        CreateMonWithIVs(&party[i], SPECIES_MAGIKARP, MAX_LEVEL, NATURE_HARDY, OTID_STRUCT_PLAYER_ID, MAX_PER_STAT_IVS);
        SetFourMoves(&party[i], MOVE_SPLASH, MOVE_NONE, MOVE_NONE, MOVE_NONE);
        FinishGen3ProbeMon(&party[i], ITEM_NONE, FALSE);
    }
    return TRUE;
}

void DailyMax_GenerateTeam(struct Pokemon *party, u32 *rngState)
{
    u16 species[PARTY_SIZE];
    u8 i, attempt;

    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);

    if (TryBuildGen3ProbeTeam(party, *rngState))
        return;

    PickTeamSpecies(species, rngState);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        for (attempt = 0; attempt < MAX_MON_ATTEMPTS; attempt++)
        {
            CreateTeamMon(&party[i], species[i], rngState);
            GiveMonInitialMoveset(&party[i]);
            FinishTeamMon(&party[i], rngState);
            if (IsValidTeamMon(&party[i]))
                break;
        }
        if (attempt == MAX_MON_ATTEMPTS)
        {
            CreateTeamMon(&party[i], species[i], rngState);
            GiveMonInitialMoveset(&party[i]);
            FinishTeamMon(&party[i], rngState);
        }
    }
}

// Optional challenge status from a reserved seed bit (disabled in v1).
void DailyMax_MaybeApplyStatus(struct Pokemon *party, u32 seed)
{
    u32 status;
    u8 slot;

    (void)party;
    (void)seed;
    (void)status;
    (void)slot;
    // v1: never apply. Hook kept for docs/status-al-entrar.md experiments.
    // Example when enabling:
    // if (!(seed & DAILYMAX_SEED_STATUS_BIT)) return;
    // slot = (seed >> 9) % PARTY_SIZE;
    // status = STATUS1_BURN;
    // SetMonData(&party[slot], MON_DATA_STATUS, &status);
}
