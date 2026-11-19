#include "global.h"
#include "dailymaximo.h"
#include "constants/dailymaximo.h"
#include "hall_of_legends.h"
#include "constants/hall_of_legends.h"
#include "main.h"
#include "malloc.h"
#include "m4a.h"
#include "menu.h"
#include "new_game.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "save.h"
#include "load_save.h"
#include "script.h"
#include "string_util.h"
#include "event_data.h"
#include "party_menu.h"
#include "battle.h"

struct DailyMaxRomConfig const gDailyMaxRomConfig __attribute__((used, aligned(4))) =
{
    .magic = {'D', 'A', 'I', 'L', 'Y', 'M', 'A', 'X', '-', 'C', 'O', 'N', 'F', '-', 'V', '1'},
};

struct DailyMaxStatus
{
    u8 magic[16];
    u32 seed;
    u8 phase;
    u8 result;
    u8 padding[2];
};

static const u8 sStatusMagic[16] = {'D', 'A', 'I', 'L', 'Y', 'M', 'A', 'X', '-', 'S', 'T', 'A', 'T', 'U', 'S', '1'};
static EWRAM_DATA struct DailyMaxStatus sStatus = {0};
static EWRAM_DATA u32 sTeamSeed = 0;
static EWRAM_DATA bool32 sDailyMaxActive = FALSE;

static const u8 sText_PlayerName[] = _("LEAF");

static u32 GetInjectedSeed(void)
{
    return *(const vu32 *)&gDailyMaxRomConfig.seed;
}

static bool32 IsDailyGame(void)
{
    return *(const vu8 *)&gDailyMaxRomConfig.daily != 0;
}

static void InitStatus(u32 seed)
{
    memcpy(sStatus.magic, sStatusMagic, sizeof(sStatus.magic));
    sStatus.seed = seed;
    sStatus.phase = DAILYMAX_PHASE_EXPLORING;
    sStatus.result = DAILYMAX_RESULT_NONE;
}

bool32 DailyMax_IsActive(void)
{
    return sDailyMaxActive;
}

u32 DailyMax_GetSeed(void)
{
    return GetInjectedSeed();
}

bool32 DailyMax_IsDailyMode(void)
{
    return IsDailyGame();
}

u8 DailyMax_GetPartnerCharacter(void)
{
    u32 seed = GetInjectedSeed();
    if (seed == 0)
        seed = sTeamSeed;
    return (seed & 1) == 0 ? HOL_CHAR_MAY : HOL_CHAR_WALLY;
}

// Solo player vs Máximo + partner. TAG keeps opponent slot 1 + 3+3 iconic teams;
// players[1] stays NONE so HoL_PlayerHasPartner is false (you control all 6).
static void SetupVersusTagRun(void)
{
    memset(&gHoLRun, 0, sizeof(gHoLRun));
    gHoLRun.format = HOL_FORMAT_TAG;
    gHoLRun.runType = HOL_RUN_VERSUS;
    gHoLRun.players[0] = HOL_CHAR_RED;
    gHoLRun.players[1] = HOL_CHAR_NONE;
    gHoLRun.opponents[0] = HOL_CHAR_STEVEN;
    gHoLRun.opponents[1] = DailyMax_GetPartnerCharacter();
    gHoLRun.room = 0;
    gHoLRun.active = TRUE;
    sDailyMaxActive = TRUE;
}

// Called from HoL_InitNewRun after NewGameInitData zeros the party.
void DailyMax_RebuildPlayerParty(void)
{
    u32 rng = sTeamSeed;

    DailyMax_GenerateTeam(gParties[B_TRAINER_PLAYER], &rng);
    DailyMax_MaybeApplyStatus(gParties[B_TRAINER_PLAYER], sTeamSeed);
    CalculatePlayerPartyCount();
}

static void PreparePartyAndStart(void)
{
    SetupVersusTagRun();
#if IS_FRLG
    StringCopy(gSaveBlock1Ptr->rivalName, gHoLCharacters[HOL_CHAR_BLUE].name);
#endif
    StringCopy(gSaveBlock2Ptr->playerName, sText_PlayerName);
    gSaveBlock2Ptr->playerGender = FEMALE;
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SET;
    SetMainCallback2(CB2_NewGame);
}

void CB2_DailyMax_StartFromCopyright(void)
{
    u32 seed = GetInjectedSeed();

    SeedRng(seed);
    sTeamSeed = seed;
    InitStatus(seed);
    if (IsDailyGame())
        gSoftResetDisabled = TRUE;

    SetSaveBlocksPointers(GetSaveBlocksPointersBaseOffset());
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    LoadGameSave(SAVE_NORMAL);
    InitHeap(gHeap, HEAP_SIZE);
    Sav2_ClearSetDefault();
    SetPokemonCryStereo(gSaveBlock2Ptr->optionsSound);
    PreparePartyAndStart();
}

void CB2_DailyMax_StartFromTitle(void)
{
    sTeamSeed = Random32();
    InitStatus(0);
    Sav2_ClearSetDefault();
    PreparePartyAndStart();
}

void DailyMax_OnBattleStart(void)
{
    sStatus.phase = DAILYMAX_PHASE_BATTLE;
}

void DailyMax_OnBattleEnd(void)
{
    sStatus.result = gSpecialVar_Result ? DAILYMAX_RESULT_WON : DAILYMAX_RESULT_LOST;
    sStatus.phase = DAILYMAX_PHASE_RESULT;
}

void DailyMax_FinishDaily(void)
{
    sStatus.phase = DAILYMAX_PHASE_FINISHED;
}

u16 DailyMax_IsDailyActive(void)
{
    return DailyMax_IsActive();
}
