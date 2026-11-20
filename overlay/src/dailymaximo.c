#include "global.h"
#include "dailymaximo.h"
#include "dailymaximo_partner.h"
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
#include "dailymax_texts.h"
#include "field_name_box.h"
#include "line_break.h"
#include "text.h"
#include "constants/flags.h"
#include "constants/opponents.h"
#include "constants/species.h"

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

// El jugador se llama HORTE en todo (guardado, combate, textos del motor). El nombre está en dailymax_texts.h.
static const u8 sText_PlayerName[] = _(DAILYMAX_NAME_PLAYER);

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
    return DailyMax_SeedSelectsBlasco(seed) ? HOL_CHAR_WALLY : HOL_CHAR_MAY;
}

const u8 *DailyMax_GetStevenIconicSlots(void)
{
    static const u8 sAuraSlots[3] = {0, 1, 2};
    static const u8 sBlascoSlots[3] = {3, 4, 5};

    return DailyMax_GetPartnerCharacter() == HOL_CHAR_WALLY ? sBlascoSlots : sAuraSlots;
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
    gSaveBlock2Ptr->playerGender = MALE;
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

// ---------------------------------------------------------------------------
// Escena de entrada en la sala de Bruno (ver data/scripts/dailymax_bruno.inc).
// Los textos viven TODOS en include/dailymax_texts.h.

// Flag solo de esta ROM: oculta los Pokémon del "escaparate" de la sala para
// que no se vea el equipo del compañero (es una sorpresa).
#define FLAG_DAILYMAX_HIDE_TEAM  FLAG_0x0B3

// Casilla de la puerta del fondo de la sala de Bruno por donde entra el compañero.
#define DAILYMAX_PARTNER_DOOR_X  6
#define DAILYMAX_PARTNER_DOOR_Y  2

#define DAILYMAX_TEXT_WIDTH      208  // caja de mensajes normal
#define DAILYMAX_TEXT_LINES      2

// Mantener sincronizado con data/dailymax_bruno.inc
enum
{
    DAILYMAX_SPEECH_BOSS_1,
    DAILYMAX_SPEECH_BOSS_2,
    DAILYMAX_SPEECH_BOSS_3,
    DAILYMAX_SPEECH_PARTNER_ENTER,
    DAILYMAX_SPEECH_AFTER_1,
    DAILYMAX_SPEECH_AFTER_2,
};

static bool32 PartnerIsBlasco(void)
{
    return DailyMax_GetPartnerCharacter() == HOL_CHAR_WALLY;
}

// Llamado desde HoL_SetupRoom (ON_TRANSITION de la sala), tras colocar a los entrenadores.
void DailyMax_SetupRoom(void)
{
    u32 i;

    if (!DailyMax_IsActive())
        return;
    // Solo el jefe está en la sala. El compañero aparece luego por la puerta del fondo.
    FlagSet(FLAG_HOL_HIDE_TRAINER_B);
    SetObjEventTemplateCoords(LOCALID_HOL_TRAINER_B, DAILYMAX_PARTNER_DOOR_X, DAILYMAX_PARTNER_DOOR_Y);
    FlagSet(FLAG_DAILYMAX_HIDE_TEAM);
    for (i = 0; i < HOL_TEAM_DISPLAY_COUNT; i++)
        SetObjEventTemplateFlagId(LOCALID_HOL_MON_1 + i, FLAG_DAILYMAX_HIDE_TEAM);
}

// Clase de entrenador que se muestra en combate (solo para los tres rivales de Daily Steven; NULL para el resto).
const u8 *DailyMax_GetClassName(u16 trainerId)
{
    static const u8 sClassBoss[] = _(DAILYMAX_TEXT_CLASS_BOSS);
    static const u8 sClassPartner[] = _(DAILYMAX_TEXT_CLASS_PARTNER);

    if (trainerId == TRAINER_HOL_STEVEN)
        return sClassBoss;
    if (trainerId == TRAINER_HOL_MAY || trainerId == TRAINER_HOL_WALLY)
        return sClassPartner;
    return NULL;
}

// «clase + nombre» tal como sale en los mensajes de combate. STEVEN: «STEVEN de la ELITE FOUR de CAMPEONES» (como GOLD);
// AURA / BLASCO: «ASPIRANTE AURA» / «ASPIRANTE BLASCO». Devuelve un texto fijo (no cabría en el búfer de 32 de battle_message.c).
const u8 *DailyMax_GetNameWithClass(u16 trainerId)
{
    static const u8 sBoss[] = _(DAILYMAX_TEXT_BOSS_WITH_CLASS);
    static const u8 sAura[] = _(DAILYMAX_TEXT_CLASS_PARTNER " " DAILYMAX_NAME_AURA);
    static const u8 sBlasco[] = _(DAILYMAX_TEXT_CLASS_PARTNER " " DAILYMAX_NAME_BLASCO);

    switch (trainerId)
    {
    case TRAINER_HOL_STEVEN: return sBoss;
    case TRAINER_HOL_MAY:    return sAura;
    case TRAINER_HOL_WALLY:  return sBlasco;
    default:                 return NULL;
    }
}

// VAR_0x8004 = DAILYMAX_SPEECH_*. Deja el texto en gStringVar4 y el nombre del que habla en la caja.
void DailyMax_BufferSpeech(void)
{
    static const u8 *const sBossTalk[] =
    {
        COMPOUND_STRING(DAILYMAX_TEXT_BOSS_TALK_1),
        COMPOUND_STRING(DAILYMAX_TEXT_BOSS_TALK_2),
        COMPOUND_STRING(DAILYMAX_TEXT_BOSS_TALK_3),
    };
    static const u8 *const sPartnerEnter[] =
    {
        COMPOUND_STRING(DAILYMAX_TEXT_AURA_ENTER),
        COMPOUND_STRING(DAILYMAX_TEXT_BLASCO_ENTER),
    };
    static const u8 *const sAfter1[] =
    {
        COMPOUND_STRING(DAILYMAX_TEXT_AURA_AFTER_1),
        COMPOUND_STRING(DAILYMAX_TEXT_BLASCO_AFTER_1),
    };
    static const u8 *const sAfter2[] =
    {
        COMPOUND_STRING(DAILYMAX_TEXT_AURA_AFTER_2),
        COMPOUND_STRING(DAILYMAX_TEXT_BLASCO_AFTER_2),
    };
    u32 id = gSpecialVar_0x8004;
    u32 partner = PartnerIsBlasco();
    const u8 *text;
    const u8 *speaker;
    const u8 *bossName = gHoLCharacters[HOL_CHAR_STEVEN].name;
    const u8 *partnerName = gHoLCharacters[DailyMax_GetPartnerCharacter()].name;

    switch (id)
    {
    case DAILYMAX_SPEECH_BOSS_1:
    case DAILYMAX_SPEECH_BOSS_2:
    case DAILYMAX_SPEECH_BOSS_3:
        text = sBossTalk[id - DAILYMAX_SPEECH_BOSS_1];
        speaker = bossName;
        break;
    case DAILYMAX_SPEECH_PARTNER_ENTER:
        text = sPartnerEnter[partner];
        speaker = partnerName;
        break;
    case DAILYMAX_SPEECH_AFTER_1:
        text = sAfter1[partner];
        speaker = partner ? bossName : partnerName;   // AURA habla primero; con BLASCO habla STEVEN
        break;
    default:
        text = sAfter2[partner];
        speaker = partner ? partnerName : bossName;   // AURA: contesta STEVEN; BLASCO: cierra BLASCO
        break;
    }
    StringExpandPlaceholders(gStringVar4, text);
    BreakStringAutomatic(gStringVar4, DAILYMAX_TEXT_WIDTH, DAILYMAX_TEXT_LINES, FONT_NORMAL, SHOW_SCROLL_PROMPT);
    gSpeakerName = speaker;
}

// Mensaje final: titular + récord + despedida, según el resultado ya guardado por DailyMax_OnBattleEnd.
void DailyMax_BufferResult(void)
{
    static const u8 *const sResult[] =
    {
        COMPOUND_STRING(DAILYMAX_TEXT_RESULT_LOST "\p" DAILYMAX_TEXT_RECORD "\p" DAILYMAX_TEXT_OUTRO_LOST),
        COMPOUND_STRING(DAILYMAX_TEXT_RESULT_WON "\p" DAILYMAX_TEXT_RECORD "\p" DAILYMAX_TEXT_OUTRO_WON),
    };
    bool32 won = sStatus.result == DAILYMAX_RESULT_WON;
    u32 wins = *(const vu16 *)&gDailyMaxRomConfig.wins;
    u32 battles = *(const vu16 *)&gDailyMaxRomConfig.battles;

    // La web guarda el récord anterior en la ROM; aquí se suma este combate.
    wins += won ? 1 : 0;
    battles += 1;
    ConvertIntToDecimalStringN(gStringVar1, wins, STR_CONV_MODE_LEFT_ALIGN, 5);
    ConvertIntToDecimalStringN(gStringVar2, battles, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringExpandPlaceholders(gStringVar4, sResult[won]);
    BreakStringAutomatic(gStringVar4, DAILYMAX_TEXT_WIDTH, DAILYMAX_TEXT_LINES, FONT_NORMAL, SHOW_SCROLL_PROMPT);
    gSpeakerName = NULL;
}

#if DAILYMAX_DEBUG
// ROM de pruebas: con la semilla DAILYMAX_DEBUG_SEED_WIN los rivales empiezan con 1 PS (victoria segura).
// Se llama al final de HoL_CreateOpponentParties, antes de que empiece el combate.
// Volcado (solo ROM de pruebas): especies de los equipos rivales A (STEVEN) y B (compañero) al empezar el combate.
EWRAM_DATA u16 gDailyMaxFoeDump[12] = {0};

void DailyMax_DebugOnOpponents(void)
{
    u32 i;
    u16 hp = 1;
    u32 sleep = 7;  // contador de sueño (7 turnos)

    for (i = 0; i < PARTY_SIZE; i++)
    {
        gDailyMaxFoeDump[i] = GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES);
        gDailyMaxFoeDump[6 + i] = GetMonData(&gParties[B_TRAINER_OPPONENT_B][i], MON_DATA_SPECIES);
    }

    if (GetInjectedSeed() != DAILYMAX_DEBUG_SEED_WIN && GetInjectedSeed() != DAILYMAX_DEBUG_SEED_WIN2)
        return;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        // El primer ataque de mi equipo pasa a ser PLACAJE (si no, un RUGIDO al azar atasca la victoria de prueba).
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE)
        {
            u16 tackle = MOVE_TACKLE;
            u8 tacklePp = 35;
            SetMonData(&gPlayerParty[i], MON_DATA_MOVE1, &tackle);
            SetMonData(&gPlayerParty[i], MON_DATA_PP1, &tacklePp);
        }
        // Cada rival lleva UN solo Pokémon: la victoria de prueba dura dos ataques.
        if (i >= 1)
        {
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][i]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_B][i]);
            continue;
        }
        // 1 PS y dormidos (así no te atacan y no hay cambios forzados): victoria segura y fácil de automatizar.
        if (GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES) != SPECIES_NONE)
        {
            SetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_HP, &hp);
            SetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_STATUS, &sleep);
        }
        if (GetMonData(&gParties[B_TRAINER_OPPONENT_B][i], MON_DATA_SPECIES) != SPECIES_NONE)
        {
            SetMonData(&gParties[B_TRAINER_OPPONENT_B][i], MON_DATA_HP, &hp);
            SetMonData(&gParties[B_TRAINER_OPPONENT_B][i], MON_DATA_STATUS, &sleep);
        }
    }
}
#endif
