#!/usr/bin/env python3
"""Patch Hall of Legends sources for Daily Máximo after the HoL patch is applied."""
from __future__ import annotations

import pathlib
import sys

GAME = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "pokeemerald-expansion")


def force_define(path: pathlib.Path, name: str, value: str, label: str) -> None:
    """Set `#define NAME <token>` once. Skips when the token already matches."""
    import re

    text = path.read_text(encoding="utf-8")
    pat = re.compile(rf"(#define\s+{name}\s+)(\S+)")
    match = pat.search(text)
    if match is None:
        raise SystemExit(f"patch failed: {path}: missing #define {name} for {label}")
    if match.group(2) == value:
        print(f"  skip {label} (already {value})")
        return
    path.write_text(pat.sub(rf"\g<1>{value}", text, count=1), encoding="utf-8")
    print(f"  patched {label} -> {value}")


def replace_all_present(path: pathlib.Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    if old not in text:
        print(f"  skip {label} (not present)")
        return
    path.write_text(text.replace(old, new), encoding="utf-8")
    print(f"  patched {label}")


def replace_once(path: pathlib.Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    if new in text:
        print(f"  skip {label} (already applied)")
        return
    # Find a distinctive slice of `new` that is not part of `old`.
    distinctive = ""
    for line in new.splitlines():
        if line and line not in old:
            distinctive = line
            break
    if old not in text:
        if distinctive and distinctive in text:
            print(f"  skip {label} (already applied)")
            return
        raise SystemExit(f"patch failed: {path}: missing marker for {label}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"  patched {label}")


def main() -> None:
    print(">> Patching Daily Máximo hooks")

    replace_once(
        GAME / "src/title_screen_frlg.c",
        '#include "hall_of_legends.h"',
        '#include "hall_of_legends.h"\n#include "dailymaximo.h"',
        "title include",
    )
    replace_once(
        GAME / "src/title_screen_frlg.c",
        "SetMainCallback2(CB2_InitHallOfLegendsSelect);",
        "SetMainCallback2(CB2_DailyMax_StartFromTitle);",
        "title callback",
    )

    hol = GAME / "src/hall_of_legends.c"
    replace_once(
        hol,
        '#include "hall_of_legends.h"',
        '#include "hall_of_legends.h"\n#include "dailymaximo.h"',
        "hol include",
    )
    replace_once(
        hol,
        '#include "trainer_util.h"',
        '#include "trainer.h"\n#include "trainer_util.h"',
        "hol trainer.h",
    )
    replace_once(
        GAME / "include/hall_of_legends.h",
        """static inline bool32 HoL_PlayerHasPartner(void)
{
    return HoL_IsActive() && gHoLRun.format == HOL_FORMAT_TAG;
}
""",
        """static inline bool32 HoL_PlayerHasPartner(void)
{
    return HoL_IsActive()
        && gHoLRun.format == HOL_FORMAT_TAG
        && gHoLRun.players[1] != HOL_CHAR_NONE;
}
""",
        "player has partner",
    )
    replace_once(
        hol,
        """void HoL_InitNewRun(void)
{
    const struct HoLCharacter *player = &gHoLCharacters[gHoLRun.players[0]];
    u32 i;

    StringCopyN(gSaveBlock2Ptr->playerName, player->name, PLAYER_NAME_LENGTH);
    gSaveBlock2Ptr->playerName[PLAYER_NAME_LENGTH] = EOS;
    gSaveBlock2Ptr->playerGender = player->gender;

    FlagSet(FLAG_SYS_POKEMON_GET);
    for (i = FLAG_BADGE01_GET; i <= FLAG_BADGE08_GET; i++)
        FlagSet(i);
    FlagSet(FLAG_HOL_NO_WHITEOUT);
    gHoLRun.room = 0;

    BuildPlayerParty();
""",
        """void HoL_InitNewRun(void)
{
    const struct HoLCharacter *player = &gHoLCharacters[gHoLRun.players[0]];
    u32 i;

    if (!DailyMax_IsActive())
    {
        StringCopyN(gSaveBlock2Ptr->playerName, player->name, PLAYER_NAME_LENGTH);
        gSaveBlock2Ptr->playerName[PLAYER_NAME_LENGTH] = EOS;
        gSaveBlock2Ptr->playerGender = player->gender;
    }

    FlagSet(FLAG_SYS_POKEMON_GET);
    for (i = FLAG_BADGE01_GET; i <= FLAG_BADGE08_GET; i++)
        FlagSet(i);
    FlagSet(FLAG_HOL_NO_WHITEOUT);
    gHoLRun.room = 0;

    if (DailyMax_IsActive())
        DailyMax_RebuildPlayerParty();
    else
        BuildPlayerParty();
""",
        "HoL_InitNewRun party",
    )
    replace_once(
        hol,
        """void HoL_SpawnPartnerFollower(void)
{
    if (!HoL_IsActive() || !IsTagBattle() || PlayerHasFollowerNPC())
        return;
    CreateFollowerNPC(gHoLCharacters[gHoLRun.players[1]].objGfx, FNPC_ALL_LAND, NULL);
}
""",
        """void HoL_SpawnPartnerFollower(void)
{
    if (!HoL_IsActive() || !IsTagBattle() || PlayerHasFollowerNPC())
        return;
    if (gHoLRun.players[1] == HOL_CHAR_NONE)
        return;
    CreateFollowerNPC(gHoLCharacters[gHoLRun.players[1]].objGfx, FNPC_ALL_LAND, NULL);
}
""",
        "partner follower guard",
    )
    replace_once(
        hol,
        """#if IS_FRLG
    // Elite Four runs start at the door of Lorelei's room, the rest in the Champion's room
    if (gHoLRun.runType == HOL_RUN_ELITE_FOUR)
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_LORELEIS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_LORELEIS_ROOM), 0, -1, -1);
    else
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), 0, -1, -1);
    WarpIntoMap();
#endif
""",
        """#if IS_FRLG
    // Elite Four runs start at the door of Lorelei's room, the rest in the Champion's room.
    // Daily Máximo: spawn on stage (DailyGold-style) — no door walk-in choreography.
    if (gHoLRun.runType == HOL_RUN_ELITE_FOUR)
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_LORELEIS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_LORELEIS_ROOM), 0, -1, -1);
    else if (DailyMax_IsActive())
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), WARP_ID_NONE, 6, 10);
    else
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), 0, -1, -1);
    WarpIntoMap();
#endif
""",
        "daily stage warp",
    )

    data = GAME / "src/data/hall_of_legends.h"
    # DEC-0.1: the rival is announced as STEVEN (not Máximo). Aura and Blasco stay in Spanish.
    replace_all_present(data, "MÁXIMO", "STEVEN", "máximo -> steven")
    replace_once(data, '.name = COMPOUND_STRING("MAY"),', '.name = COMPOUND_STRING("AURA"),', "name aura")
    replace_once(data, '.name = COMPOUND_STRING("WALLY"),', '.name = COMPOUND_STRING("BLASCO"),', "name blasco")
    replace_once(
        data,
        'HOL_TEXT("What has awoken in you because of your journey? I want you to hit me with it all! Now, bring it!",\n                 "You… I had no idea you\'d become so strong…",\n                 "When it comes down to it, I\'m still the strongest!",\n                 "I thought that you would eventually become the CHAMPION. My predictions usually come true."),',
        'HOL_TEXT("¿Qué ha despertado en ti con este viaje? ¡Quiero que me lo demuestres todo!\\p¡Pero no lo haré solo esta vez!",\n                 "Tú… no imaginaba que te hubieras hecho tan fuerte…",\n                 "Al final, ¡sigo siendo el más fuerte!",\n                 "Sabía que acabarías siendo CAMPEÓN. Mis predicciones casi nunca fallan."),',
        "máximo texts",
    )
    replace_once(
        data,
        'HOL_TEXT("How much stronger have you gotten? Let me check for you! Go!",\n                 "Yikes! You\'re better than I expected!",\n                 "Yahoo! I won! My POKéMON and I have been training really hard, too!",\n                 "I was worried that you might be struggling with your training. But I had absolutely nothing to be worried about!"),',
        'HOL_TEXT("¡He venido a ayudar a STEVEN! ¡Vamos a pelear juntos! ¡Allá voy!",\n                 "¡Uy! ¡Eres más fuerte de lo que pensaba!",\n                 "¡Bien! ¡Hemos ganado! ¡Mis POKéMON también entrenan duro!",\n                 "Me preocupaba que lo estuvieras pasando mal… ¡pero no hacía falta!"),',
        "aura texts",
    )
    replace_once(
        data,
        'HOL_TEXT("Losing to you that time made me stronger! But I\'m not going to lose anymore! Okay… Here I come!",\n                 "Wow! You are strong, after all!",\n                 "I… I won? I won! Thank you, everyone, for giving me courage!",\n                 "I couldn\'t beat you today, but one of these days, I\'ll catch up to you!"),',
        'HOL_TEXT("¡He venido corriendo para pelear junto a STEVEN! ¡Esta vez no pienso perder! ¡Allá voy!",\n                 "¡Guau! ¡Eres muy fuerte!",\n                 "¿Yo… he ganado? ¡He ganado! ¡Gracias a todos por darme valor!",\n                 "Hoy no he podido ganarte, ¡pero algún día te alcanzaré!"),',
        "blasco texts",
    )

    scripts = GAME / "data/scripts/hall_of_legends.inc"
    replace_once(
        scripts,
        """HallOfLegends_EventScript_Lost::
	special HoL_GetRoomType
	call_if_eq VAR_RESULT, HOL_ROOM_TYPE_VERSUS, HallOfLegends_EventScript_LostVersus
	call_if_ne VAR_RESULT, HOL_ROOM_TYPE_VERSUS, HallOfLegends_EventScript_LostEliteFour
	goto HallOfLegends_EventScript_BackToSelect
""",
        """HallOfLegends_EventScript_Lost::
	special HoL_GetRoomType
	call_if_eq VAR_RESULT, HOL_ROOM_TYPE_VERSUS, HallOfLegends_EventScript_LostVersus
	call_if_ne VAR_RESULT, HOL_ROOM_TYPE_VERSUS, HallOfLegends_EventScript_LostEliteFour
	setvar VAR_RESULT, FALSE
	special DailyMax_OnBattleEnd
	specialvar VAR_RESULT, DailyMax_IsDailyActive
	goto_if_eq VAR_RESULT, TRUE, HallOfLegends_EventScript_DailyMaxFinish
	goto HallOfLegends_EventScript_BackToSelect

HallOfLegends_EventScript_DailyMaxFinish::
	special DailyMax_FinishDaily
	releaseall
	end
""",
        "lost daily finish",
    )
    replace_once(
        scripts,
        """HallOfLegends_EventScript_WonVersus::
	setvar VAR_0x8004, HOL_RESULT_WON
	call HallOfLegends_EventScript_ShowResult
	goto HallOfLegends_EventScript_BackToSelect
""",
        """HallOfLegends_EventScript_WonVersus::
	setvar VAR_0x8004, HOL_RESULT_WON
	call HallOfLegends_EventScript_ShowResult
	setvar VAR_RESULT, TRUE
	special DailyMax_OnBattleEnd
	specialvar VAR_RESULT, DailyMax_IsDailyActive
	goto_if_eq VAR_RESULT, TRUE, HallOfLegends_EventScript_DailyMaxFinish
	goto HallOfLegends_EventScript_BackToSelect
""",
        "won versus daily",
    )
    replace_once(
        scripts,
        """	setvar VAR_0x8005, HOL_TEXT_INTRO
	call HallOfLegends_EventScript_TrainersSpeak
	special BattleSetup_StartHallOfLegendsBattle
""",
        """	setvar VAR_0x8005, HOL_TEXT_INTRO
	call HallOfLegends_EventScript_TrainersSpeak
	special DailyMax_OnBattleStart
	special BattleSetup_StartHallOfLegendsBattle
""",
        "battle start special",
    )

    champ = GAME / "data/maps/PokemonLeague_ChampionsRoom_Frlg/scripts.inc"
    replace_once(
        champ,
        """PokemonLeague_ChampionsRoom_EventScript_EnterRoom::
	lockall
	special HoL_SpawnPartnerFollower
	applymovement LOCALID_PLAYER, PokemonLeague_ChampionsRoom_Movement_PlayerEnter
	waitmovement 0
	call HallOfLegends_EventScript_PartnerStepBeside
	call PokemonLeague_ChampionsRoom_EventScript_BlockExits
	setvar VAR_TEMP_1, 1
	releaseall
	end
""",
        """PokemonLeague_ChampionsRoom_EventScript_EnterRoom::
	lockall
	specialvar VAR_RESULT, DailyMax_IsDailyActive
	goto_if_eq VAR_RESULT, TRUE, PokemonLeague_ChampionsRoom_EventScript_DailyMaxStart
	special HoL_SpawnPartnerFollower
	applymovement LOCALID_PLAYER, PokemonLeague_ChampionsRoom_Movement_PlayerEnter
	waitmovement 0
	call HallOfLegends_EventScript_PartnerStepBeside
	call PokemonLeague_ChampionsRoom_EventScript_BlockExits
	setvar VAR_TEMP_1, 1
	releaseall
	end

@ Daily Máximo: DailyGold-style — already on stage, jump straight into the fight.
PokemonLeague_ChampionsRoom_EventScript_DailyMaxStart::
	call PokemonLeague_ChampionsRoom_EventScript_BlockExits
	setvar VAR_TEMP_1, 1
	special DailyMax_OnBattleStart
	special BattleSetup_StartHallOfLegendsBattle
	goto_if_eq VAR_RESULT, FALSE, HallOfLegends_EventScript_Lost
	setflag FLAG_HOL_ROOM_CLEARED
	setvar VAR_0x8005, HOL_TEXT_POST
	call HallOfLegends_EventScript_TrainersSpeak
	goto HallOfLegends_EventScript_WonVersus
""",
        "immediate daily battle",
    )

    specials = GAME / "data/specials.inc"
    specials_text = specials.read_text(encoding="utf-8")
    if "DailyMax_OnBattleStart" not in specials_text:
        replace_once(
            specials,
            "\tdef_special HoL_ReturnToSelect",
            "\tdef_special HoL_ReturnToSelect\n"
            "\tdef_special DailyMax_OnBattleStart\n"
            "\tdef_special DailyMax_OnBattleEnd\n"
            "\tdef_special DailyMax_FinishDaily\n"
            "\tdef_special DailyMax_IsDailyActive",
            "specials",
        )
    else:
        print("  skip specials (already applied)")

    replace_once(
        GAME / "include/config/general.h",
        "#define EXPANSION_INTRO              TRUE    // If TRUE, a custom RHH intro will play after the vanilla copyright screen.",
        "#define EXPANSION_INTRO              FALSE   // Daily Máximo: skip \"Powered by\" splash.",
        "disable expansion intro",
    )
    replace_once(
        GAME / "include/config/battle.h",
        "#define B_FAST_HP_DRAIN                     TRUE       // If set to TRUE, HP bars will move faster.",
        "#define B_FAST_HP_DRAIN                     FALSE      // Daily Máximo: FireRed-speed HP bars (TRUE drains too fast).",
        "firered hp bar speed",
    )

    # Fase 0 / DEC-0.4: Gen3 battle rules. GEN_LATEST drives crit x2, physical/special
    # by type, Protect 50%, immediate faint switch, and spread damage (Surf 50% on
    # both foes; Earthquake hits the ally with no spread reduction).
    force_define(GAME / "include/config/general.h", "GEN_LATEST", "GEN_3", "gen3 mechanics")
    force_define(GAME / "include/config/species_enabled.h", "P_GEN_4_POKEMON", "FALSE", "disable gen4 species")
    # Gallade (Blasco provisional) is a Gen4 cross-evo of a Gen3 line. Turning this
    # off deletes its species data while the HoL party still references it.
    # Left TRUE until GAP-F2 swaps that slot. See docs/fase0-gen3-informe.md.
    force_define(GAME / "include/config/battle.h", "B_SHOW_CATEGORY_ICON", "FALSE", "hide category icon")

    intro = GAME / "src/intro_frlg.c"
    if intro.exists():
        text = intro.read_text(encoding="utf-8")
        if '#include "dailymaximo.h"' not in text:
            text = text.replace('#include "global.h"', '#include "global.h"\n#include "dailymaximo.h"', 1)
            intro.write_text(text, encoding="utf-8")
            print("  patched intro_frlg include")
        replace_once(
            intro,
            """static void CB2_WaitFadeBeforeSetUpIntro(void)
{
    if (!UpdatePaletteFade())
    {
#if EXPANSION_INTRO == TRUE
        SetMainCallback2(CB2_ExpansionIntro);
        CreateTask(Task_HandleExpansionIntro, 0);
#else
        SetMainCallback2(CB2_SetUpIntroFrlg);
#endif
    }
}
""",
            """static void CB2_WaitFadeBeforeSetUpIntro(void)
{
    if (!UpdatePaletteFade())
    {
        // Daily Máximo: skip "Powered by pokeemerald-expansion" and the Game Freak intro.
        if (DailyMax_GetSeed() != 0)
            SetMainCallback2(CB2_DailyMax_StartFromCopyright);
        else
            SetMainCallback2(CB2_InitTitleScreen);
    }
}
""",
            "skip intros after copyright",
        )

    # Daily Máximo: no HoL character-select quit (solo challenge).
    start_menu = GAME / "src/start_menu.c"
    replace_once(
        start_menu,
        '#include "hall_of_legends.h"',
        '#include "hall_of_legends.h"\n#include "dailymaximo.h"',
        "start_menu include",
    )
    replace_once(
        start_menu,
        """static void BuildHallOfLegendsStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    if (gHoLRun.runType == HOL_RUN_ELITE_FOUR)
        AddStartMenuAction(MENU_ACTION_BAG);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_QUIT_HOL);
    AddStartMenuAction(MENU_ACTION_EXIT);
}
""",
        """static void BuildHallOfLegendsStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    if (gHoLRun.runType == HOL_RUN_ELITE_FOUR)
        AddStartMenuAction(MENU_ACTION_BAG);
    AddStartMenuAction(MENU_ACTION_OPTION);
    // Daily Máximo skips HoL trainer select — no QUIT back to that screen.
    if (!DailyMax_IsActive())
        AddStartMenuAction(MENU_ACTION_QUIT_HOL);
    AddStartMenuAction(MENU_ACTION_EXIT);
}
""",
        "hide HoL quit for Daily Máximo",
    )

    # Battle intro uses the trainer record, not the overworld character name.
    party = GAME / "src/data/trainers_frlg.party"
    replace_once(
        party,
        "=== TRAINER_HOL_MAY ===\nName: MAY\n",
        "=== TRAINER_HOL_MAY ===\nName: AURA\n",
        "party name aura",
    )
    replace_once(
        party,
        "=== TRAINER_HOL_WALLY ===\nName: WALLY\n",
        "=== TRAINER_HOL_WALLY ===\nName: BLASCO\n",
        "party name blasco",
    )
    generated = GAME / "src/data/trainers_frlg.h"
    text = generated.read_text(encoding="utf-8")
    # Names sit on the following trainerName line; replace only those two records.
    def rename_trainer(src: str, trainer: str, new_name: str) -> str:
        needle = f"[DIFFICULTY_NORMAL][{trainer}] ="
        at = src.find(needle)
        if at < 0:
            raise SystemExit(f"patch failed: missing {trainer}")
        window = src[at:at + 500]
        old = '.trainerName = _("MAY"),' if trainer.endswith("MAY") else '.trainerName = _("WALLY"),'
        if new_name in window and old not in window:
            print(f"  skip trainer name {new_name} (already applied)")
            return src
        if old not in window:
            raise SystemExit(f"patch failed: {trainer} name not in record")
        return src[:at] + window.replace(old, f'.trainerName = _("{new_name}"),', 1) + src[at + 500:]

    renamed = rename_trainer(text, "TRAINER_HOL_MAY", "AURA")
    renamed = rename_trainer(renamed, "TRAINER_HOL_WALLY", "BLASCO")
    if renamed != text:
        generated.write_text(renamed, encoding="utf-8")
        print("  patched trainer battle names")

    messages = GAME / "src/battle_message.c"
    replace_once(
        messages,
        'static const u8 sText_TwoTrainersWantToBattle[] = _("You are challenged by {B_TRAINER1_NAME_WITH_CLASS} and {B_TRAINER2_NAME_WITH_CLASS}!\\p");',
        'static const u8 sText_TwoTrainersWantToBattle[] = _("¡{B_TRAINER1_NAME} y {B_TRAINER2_NAME}\\nquieren combatir!\\p");',
        "spanish two-trainer intro",
    )
    replace_once(
        messages,
        'static const u8 sText_TwoTrainersSentPkmn[] = _("{B_TRAINER1_NAME_WITH_CLASS} sent out {B_OPPONENT_MON1_NAME}!\\p{B_TRAINER2_NAME_WITH_CLASS} sent out {B_OPPONENT_MON2_NAME}!");',
        'static const u8 sText_TwoTrainersSentPkmn[] = _("¡{B_TRAINER1_NAME} envió a {B_OPPONENT_MON1_NAME}!\\p¡{B_TRAINER2_NAME} envió a {B_OPPONENT_MON2_NAME}!");',
        "spanish two-trainer send",
    )
    replace_once(
        messages,
        'static const u8 sText_Trainer2SentOutPkmn[] = _("{B_TRAINER2_NAME_WITH_CLASS} sent out {B_BUFF1}!");',
        'static const u8 sText_Trainer2SentOutPkmn[] = _("¡{B_TRAINER2_NAME} envió a {B_BUFF1}!");',
        "spanish trainer2 send",
    )
    replace_once(
        messages,
        'static const u8 sText_Trainer2WithdrewPkmn[] = _("{B_TRAINER2_NAME_WITH_CLASS} withdrew {B_BUFF1}!");',
        'static const u8 sText_Trainer2WithdrewPkmn[] = _("¡{B_TRAINER2_NAME} retiró a {B_BUFF1}!");',
        "spanish trainer2 withdraw",
    )
    replace_once(
        messages,
        'static const u8 sText_Trainer1SentOutPkmn2[] = _("¡{B_BUFF1} es el POKéMON enviado\\npor {B_TRAINER1_CLASS} {B_TRAINER1_NAME}!");',
        'static const u8 sText_Trainer1SentOutPkmn2[] = _("¡{B_TRAINER1_NAME} envió a {B_BUFF1}!");',
        "spanish trainer1 switch",
    )
    replace_once(
        messages,
        'static const u8 sText_Trainer1WithdrewPkmn[] = _("¡Retirada de {B_BUFF1} por \\n{B_TRAINER1_CLASS} {B_TRAINER1_NAME}!");',
        'static const u8 sText_Trainer1WithdrewPkmn[] = _("¡{B_TRAINER1_NAME} retiró a {B_BUFF1}!");',
        "spanish trainer1 withdraw",
    )

    print(">> Hooks done")


if __name__ == "__main__":
    main()
