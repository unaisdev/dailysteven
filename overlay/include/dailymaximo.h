#ifndef GUARD_DAILYMAXIMO_H
#define GUARD_DAILYMAXIMO_H

#include "global.h"

struct DailyMaxRomConfig
{
    u8 magic[16];
    u32 seed;
    u16 wins;
    u16 battles;
    u8 daily;
    u8 padding[3];
};

bool32 DailyMax_IsActive(void);
u32 DailyMax_GetSeed(void);
bool32 DailyMax_IsDailyMode(void);
u8 DailyMax_GetPartnerCharacter(void);

void CB2_DailyMax_StartFromCopyright(void);
void CB2_DailyMax_StartFromTitle(void);

void DailyMax_GenerateTeam(struct Pokemon *party, u32 *rngState);
void DailyMax_MaybeApplyStatus(struct Pokemon *party, u32 seed);
void DailyMax_RebuildPlayerParty(void);

void DailyMax_OnBattleStart(void);
void DailyMax_OnBattleEnd(void);
void DailyMax_FinishDaily(void);
u16 DailyMax_IsDailyActive(void);
void DailyMax_SetupRoom(void);
void DailyMax_BufferSpeech(void);
void DailyMax_BufferResult(void);

#endif // GUARD_DAILYMAXIMO_H
