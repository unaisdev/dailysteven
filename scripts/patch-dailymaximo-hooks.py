#!/usr/bin/env python3
"""Patch Hall of Legends sources for Daily Máximo after the HoL patch is applied."""
from __future__ import annotations

import pathlib
import sys

GAME = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "pokeemerald-expansion")
ROOT = pathlib.Path(__file__).resolve().parents[1]
TEXTS_H = ROOT / "overlay" / "include" / "dailymax_texts.h"


def dm_name(key: str) -> str:
    """Lee DAILYMAX_NAME_<key> de overlay/include/dailymax_texts.h (única fuente de los nombres)."""
    import re

    m = re.search(rf'#define\s+DAILYMAX_NAME_{key}\s+"([^"]+)"', TEXTS_H.read_text(encoding="utf-8"))
    if m is None:
        raise SystemExit(f"patch failed: DAILYMAX_NAME_{key} no está en {TEXTS_H}")
    return m.group(1)


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
    // Daily: start at the door of Bruno's room (entrance scene, data/scripts/dailymax_bruno.inc).
    if (gHoLRun.runType == HOL_RUN_ELITE_FOUR)
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_LORELEIS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_LORELEIS_ROOM), 0, -1, -1);
    else if (DailyMax_IsActive())
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_BRUNOS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_BRUNOS_ROOM), 0, -1, -1);
    else
        SetWarpDestination(MAP_GROUP(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), MAP_NUM(MAP_POKEMON_LEAGUE_CHAMPIONS_ROOM), 0, -1, -1);
    WarpIntoMap();
#endif
""",
        "daily bruno warp",
    )

    data = GAME / "src/data/hall_of_legends.h"
    # Nombres y frases salen de overlay/include/dailymax_texts.h (único sitio a editar).
    replace_once(
        data,
        "// Hall of Legends characters.",
        '#include "dailymax_texts.h"\n\n// Hall of Legends characters.',
        "texts include",
    )
    import re

    def set_character(label: str, name_macro: str, intro: str, defeat: str, victory: str, post: str) -> None:
        text = data.read_text(encoding="utf-8")
        pat = re.compile(
            r'(\[' + label + r'\] =\s*\{\s*\.name = COMPOUND_STRING\()[^)]*(\),\s*)HOL_TEXT\(.*?\),(\s*\.trainerId)',
            re.S,
        )
        block = (
            "HOL_TEXT(" + intro + ",\n"
            "                 " + defeat + ",\n"
            "                 " + victory + ",\n"
            "                 " + post + "),"
        )
        new, n = pat.subn(lambda m: m.group(1) + name_macro + m.group(2) + block + m.group(3), text, count=1)
        if n != 1:
            raise SystemExit(f"patch failed: {data}: personaje {label} no encontrado")
        if new != text:
            data.write_text(new, encoding="utf-8")
            print(f"  patched {label} name+texts")
        else:
            print(f"  skip {label} (already applied)")

    # intro/post de HoL ya no se usan (la escena la lleva dailymax_bruno.inc); se rellenan para que compile.
    set_character("HOL_CHAR_STEVEN", "DAILYMAX_NAME_BOSS", "DAILYMAX_TEXT_BOSS_TALK_1",
                  "DAILYMAX_TEXT_BOSS_DEFEAT", "DAILYMAX_TEXT_BOSS_VICTORY", "DAILYMAX_TEXT_BOSS_TALK_2")
    set_character("HOL_CHAR_MAY", "DAILYMAX_NAME_AURA", "DAILYMAX_TEXT_AURA_ENTER",
                  "DAILYMAX_TEXT_AURA_DEFEAT", "DAILYMAX_TEXT_AURA_VICTORY", "DAILYMAX_TEXT_AURA_BESIDE")
    set_character("HOL_CHAR_WALLY", "DAILYMAX_NAME_BLASCO", "DAILYMAX_TEXT_BLASCO_ENTER",
                  "DAILYMAX_TEXT_BLASCO_DEFEAT", "DAILYMAX_TEXT_BLASCO_VICTORY", "DAILYMAX_TEXT_BLASCO_BESIDE")

    specials = GAME / "data/specials.inc"
    specials_text = specials.read_text(encoding="utf-8")
    if "DailyMax_OnBattleStart" not in specials_text:
        # OJO: se insertan ANTES de HoL_ReturnToSelect. Esa línea termina en ", waitstate=1";
        # si se ponen detrás, el waitstate pasa a la última especial nueva y el juego se queda
        # parado para siempre en la primera "specialvar" (bug que bloqueaba el juego en la ronda 1).
        replace_once(
            specials,
            "\tdef_special HoL_ReturnToSelect",
            "\tdef_special DailyMax_OnBattleStart\n"
            "\tdef_special DailyMax_OnBattleEnd\n"
            "\tdef_special DailyMax_FinishDaily\n"
            "\tdef_special DailyMax_IsDailyActive\n"
            "\tdef_special DailyMax_BufferSpeech\n"
            "\tdef_special DailyMax_BufferResult\n"
            "\tdef_special HoL_ReturnToSelect",
            "specials",
        )
    else:
        print("  skip specials (already applied)")
    replace_once(
        GAME / "data/event_scripts.s",
        '.include "data/scripts/hall_of_legends.inc"',
        '.include "data/scripts/hall_of_legends.inc"\n\t.include "data/scripts/dailymax_bruno.inc"',
        "include bruno scene script",
    )

    # Sala de Bruno: la entrada y el hablar con el jefe pasan a la escena diaria.
    bruno = GAME / "data/maps/PokemonLeague_BrunosRoom_Frlg/scripts.inc"
    replace_once(
        bruno,
        "PokemonLeague_BrunosRoom_EventScript_EnterRoom::\n\tlockall\n",
        "PokemonLeague_BrunosRoom_EventScript_EnterRoom::\n\tlockall\n"
        "\tspecialvar VAR_RESULT, DailyMax_IsDailyActive\n"
        "\tgoto_if_eq VAR_RESULT, TRUE, DailyMax_BrunosRoom_EventScript_Enter\n",
        "bruno enter room",
    )
    replace_once(
        bruno,
        "PokemonLeague_BrunosRoom_EventScript_Trainer::\n",
        "PokemonLeague_BrunosRoom_EventScript_Trainer::\n"
        "\tspecialvar VAR_RESULT, DailyMax_IsDailyActive\n"
        "\tgoto_if_eq VAR_RESULT, TRUE, DailyMax_BrunosRoom_EventScript_Talk\n",
        "bruno talk",
    )

    # HoL_SetupRoom: en el modo diario solo el jefe se ve; el compañero y el "escaparate" se ocultan.
    replace_once(
        hol,
        "        SetObjEventTemplateCoords(LOCALID_HOL_MON_1 + i, layout->monX[i], layout->monY[i]);\n    }\n}\n",
        "        SetObjEventTemplateCoords(LOCALID_HOL_MON_1 + i, layout->monX[i], layout->monY[i]);\n    }\n"
        "    DailyMax_SetupRoom();\n}\n",
        "daily room setup hook",
    )
    replace_once(
        GAME / "src/overworld.c",
        "void SetObjEventTemplateMovementType(u8 localId, u8 movementType)\n",
        "void SetObjEventTemplateFlagId(u8 localId, u16 flagId)\n"
        "{\n"
        "    struct ObjectEventTemplate *objectEventTemplate = GetObjectEventTemplate(localId);\n"
        "    if (objectEventTemplate)\n"
        "        objectEventTemplate->flagId = flagId;\n"
        "}\n\n"
        "void SetObjEventTemplateMovementType(u8 localId, u8 movementType)\n",
        "template flag setter",
    )
    replace_once(
        GAME / "include/overworld.h",
        "void SetObjEventTemplateCoords(u8 localId, s16 x, s16 y);\n",
        "void SetObjEventTemplateCoords(u8 localId, s16 x, s16 y);\nvoid SetObjEventTemplateFlagId(u8 localId, u16 flagId);\n",
        "template flag setter decl",
    )

    # Con un solo jugador (players[1] == HOL_CHAR_NONE == 0xFF) estas funciones leían fuera de la tabla.
    replace_once(
        hol,
        """    if (IsTagBattle() && gHoLCharacters[gHoLRun.players[1]].backPic == TRAINER_PIC_NONE)
        return TRAINER_PIC_HOL_BLANK;
    return gHoLCharacters[gHoLRun.players[player]].backPic;""",
        """    if (IsTagBattle() && gHoLRun.players[1] != HOL_CHAR_NONE && gHoLCharacters[gHoLRun.players[1]].backPic == TRAINER_PIC_NONE)
        return TRAINER_PIC_HOL_BLANK;
    if (gHoLRun.players[player] == HOL_CHAR_NONE)
        player = 0;
    return gHoLCharacters[gHoLRun.players[player]].backPic;""",
        "guard back pic",
    )
    replace_once(
        hol,
        """u16 HoL_GetPartnerFrontPic(void)
{
    return gHoLCharacters[gHoLRun.players[1]].frontPic;""",
        """u16 HoL_GetPartnerFrontPic(void)
{
    if (gHoLRun.players[1] == HOL_CHAR_NONE)
        return TRAINER_PIC_HOL_BLANK;
    return gHoLCharacters[gHoLRun.players[1]].frontPic;""",
        "guard partner front pic",
    )
    replace_once(
        hol,
        """const u8 *HoL_GetPartnerName(void)
{
    return gHoLCharacters[gHoLRun.players[1]].name;""",
        """const u8 *HoL_GetPartnerName(void)
{
    if (gHoLRun.players[1] == HOL_CHAR_NONE)
        return gText_EmptyString2;
    return gHoLCharacters[gHoLRun.players[1]].name;""",
        "guard partner name",
    )
    replace_once(
        hol,
        """    if (IsTagBattle())
    {
        StringAppend(gStringVar1, COMPOUND_STRING(" and "));""",
        """    if (IsTagBattle() && gHoLRun.players[1] != HOL_CHAR_NONE)
    {
        StringAppend(gStringVar1, COMPOUND_STRING(" and "));""",
        "guard result text",
    )

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

    # El combate usa el nombre del registro del entrenador (.party), no el del personaje del mapa.
    party = GAME / "src/data/trainers_frlg.party"
    ptext = party.read_text(encoding="utf-8")
    for trainer, name in (
        ("TRAINER_HOL_STEVEN", dm_name("BOSS")),
        ("TRAINER_HOL_MAY", dm_name("AURA")),
        ("TRAINER_HOL_WALLY", dm_name("BLASCO")),
    ):
        ptext, n = re.subn(rf"(=== {trainer} ===\nName: )[^\n]*", lambda m, nm=name: m.group(1) + nm, ptext, count=1)
        if n != 1:
            raise SystemExit(f"patch failed: {trainer} no está en {party}")
    party.write_text(ptext, encoding="utf-8")
    print("  patched trainer names in .party")

    # trainers_frlg.h se genera desde el .party al compilar; si ya existe de una compilación
    # anterior, se actualiza también (si no existe no hace falta: make lo crea ya con los nombres).
    generated = GAME / "src/data/trainers_frlg.h"
    if generated.exists():
        gtext = generated.read_text(encoding="utf-8")
        for trainer, name in (
            ("TRAINER_HOL_STEVEN", dm_name("BOSS")),
            ("TRAINER_HOL_MAY", dm_name("AURA")),
            ("TRAINER_HOL_WALLY", dm_name("BLASCO")),
        ):
            at = gtext.find(f"[DIFFICULTY_NORMAL][{trainer}] =")
            if at < 0:
                continue
            window = gtext[at:at + 500]
            window = re.sub(r'\.trainerName = _\("[^"]*"\)', lambda m, nm=name: f'.trainerName = _("{nm}")', window, count=1)
            gtext = gtext[:at] + window + gtext[at + 500:]
        generated.write_text(gtext, encoding="utf-8")
        print("  patched trainers_frlg.h names")
    else:
        print("  skip trainers_frlg.h (se genera al compilar)")

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
