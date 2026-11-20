#!/usr/bin/env python3
"""Patch Hall of Legends sources for Daily Steven after the HoL patch is applied."""
from __future__ import annotations

import pathlib
import re
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
    print(">> Patching Daily Steven hooks")

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
                  "DAILYMAX_TEXT_AURA_DEFEAT", "DAILYMAX_TEXT_AURA_VICTORY", "DAILYMAX_TEXT_AURA_AFTER_1")
    set_character("HOL_CHAR_WALLY", "DAILYMAX_NAME_BLASCO", "DAILYMAX_TEXT_BLASCO_ENTER",
                  "DAILYMAX_TEXT_BLASCO_DEFEAT", "DAILYMAX_TEXT_BLASCO_VICTORY", "DAILYMAX_TEXT_BLASCO_AFTER_1")

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
        "#define EXPANSION_INTRO              FALSE   // Daily Steven: skip \"Powered by\" splash.",
        "disable expansion intro",
    )
    replace_once(
        GAME / "include/config/battle.h",
        "#define B_FAST_HP_DRAIN                     TRUE       // If set to TRUE, HP bars will move faster.",
        "#define B_FAST_HP_DRAIN                     FALSE      // Daily Steven: FireRed-speed HP bars (TRUE drains too fast).",
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
        // Daily Steven: skip "Powered by pokeemerald-expansion" and the Game Freak intro.
        if (DailyMax_GetSeed() != 0)
            SetMainCallback2(CB2_DailyMax_StartFromCopyright);
        else
            SetMainCallback2(CB2_InitTitleScreen);
    }
}
""",
            "skip intros after copyright",
        )

    # Daily Steven: no HoL character-select quit (solo challenge).
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
    // Daily Steven skips HoL trainer select — no QUIT back to that screen.
    if (!DailyMax_IsActive())
        AddStartMenuAction(MENU_ACTION_QUIT_HOL);
    AddStartMenuAction(MENU_ACTION_EXIT);
}
""",
        "hide HoL quit for Daily Steven",
    )

    # ---- Ronda 3: equipos rivales provisionales (un único archivo), HORTE, 3.ª gen estricta -----------------
    import re

    rivals_src = (ROOT / "overlay" / "data" / "dailymax_rivals.party").read_text(encoding="utf-8")
    rival_blocks = dict(re.findall(r"(=== TRAINER_HOL_\w+ ===\n.*?)(?=\n=== |\Z)", rivals_src, re.S) and
                        [(m.group(1), m.group(0)) for m in re.finditer(r"=== (TRAINER_HOL_\w+) ===\n.*?(?=\n=== |\Z)", rivals_src, re.S)])
    party_file = GAME / "src/data/trainers_frlg.party"
    ptxt = party_file.read_text(encoding="utf-8")
    for trainer in ("TRAINER_HOL_STEVEN", "TRAINER_HOL_MAY", "TRAINER_HOL_WALLY"):
        block = rival_blocks.get(trainer)
        if block is None:
            raise SystemExit(f"patch failed: {trainer} falta en overlay/data/dailymax_rivals.party")
        block = block.rstrip("\n") + "\n"
        ptxt, n = re.subn(rf"=== {trainer} ===\n.*?(?=\n=== |\Z)", lambda m, b=block: b, ptxt, count=1, flags=re.S)
        if n != 1:
            raise SystemExit(f"patch failed: {trainer} no está en {party_file}")
    party_file.write_text(ptxt, encoding="utf-8")
    print("  patched rival teams from overlay/data/dailymax_rivals.party")

    # Los 3 Pokémon de cada rival son ahora los slots 0, 1 y 2 de su equipo.
    hdata = GAME / "src/data/hall_of_legends.h"
    htxt = hdata.read_text(encoding="utf-8")
    for label in ("HOL_CHAR_STEVEN", "HOL_CHAR_MAY", "HOL_CHAR_WALLY"):
        htxt, n = re.subn(r"(\[" + label + r"\] =.*?\.iconic = )\{[^}]*\}", lambda m: m.group(1) + "{0, 1, 2}", htxt, count=1, flags=re.S)
        if n != 1:
            raise SystemExit(f"patch failed: iconic de {label}")
    # HORTE: nombre, sprite del mapa (Spenser) y frontal/espalda de combate (ver TRAINER_PIC_RED más abajo).
    htxt, n = re.subn(
        r'(\[HOL_CHAR_RED\] =\s*\{\s*\.name = COMPOUND_STRING\()[^)]*(\))', lambda m: m.group(1) + "DAILYMAX_NAME_PLAYER" + m.group(2), htxt, count=1)
    if n != 1:
        raise SystemExit("patch failed: nombre de HOL_CHAR_RED")
    htxt, n = re.subn(r"(\[HOL_CHAR_RED\] =.*?\.objGfx = )OBJ_EVENT_GFX_\w+", lambda m: m.group(1) + "OBJ_EVENT_GFX_SPENSER", htxt, count=1, flags=re.S)
    if n != 1:
        raise SystemExit("patch failed: objGfx de HOL_CHAR_RED")
    hdata.write_text(htxt, encoding="utf-8")
    print("  patched iconic slots and HORTE (name, overworld sprite)")

    trainers_gfx = GAME / "src/data/graphics/trainers.h"
    replace_once(
        trainers_gfx,
        "#define TRAINER_FRONT_PIC(picFile, paletteFile, ...)",
        "// Daily Steven: la espalda de HORTE reutiliza el retrato frontal de Spenser (igual que en Daily Gold).\n"
        'const u8 gTrainerBackPic_DailyMaxHorte[] = INCGFX_U8("graphics/trainers/front_pics/palace_maven_spenser.png", ".4bpp");\n'
        'const u16 gTrainerBackPicPalette_DailyMaxHorte[] = INCGFX_U16("graphics/trainers/front_pics/palace_maven_spenser.png", ".gbapal");\n\n'
        "#define TRAINER_FRONT_PIC(picFile, paletteFile, ...)",
        "horte back pic data",
    )
    replace_once(
        trainers_gfx,
        """    [TRAINER_PIC_RED] =
    {
        .frontPic = TRAINER_FRONT_PIC(gTrainerFrontPic_Red, gTrainerPalette_Red),
        .backPic = TRAINER_BACK_PIC(5, gTrainerBackPic_Red, gTrainerBackPicPalette_Red, sBackAnims_Kanto),
    },""",
        """    [TRAINER_PIC_RED] = // Daily Steven: este es HORTE (Spenser), no RED
    {
        .frontPic = TRAINER_FRONT_PIC(gTrainerFrontPic_PalaceMavenSpenser, gTrainerPalette_PalaceMavenSpenser),
        .backPic = TRAINER_BACK_PIC(5, gTrainerBackPic_DailyMaxHorte, gTrainerBackPicPalette_DailyMaxHorte, sBackAnims_None),
    },""",
        "horte pics",
    )

    # STEVEN: 6 mons en dailymax_rivals.party → slots según compañero (Aura 0-2 / Blasco 3-5).
    # También engancha DailyMax_DebugOnOpponents (ROM -Debug).
    _steven_create = """        CreateCharacterMons(&gParties[B_TRAINER_OPPONENT_A][0], a,
            (DailyMax_IsActive() && a == HOL_CHAR_STEVEN) ? DailyMax_GetStevenIconicSlots() : gHoLCharacters[a].iconic,
            3, FALSE);
        CreateCharacterMons(&gParties[B_TRAINER_OPPONENT_B][0], b, gHoLCharacters[b].iconic, 3, FALSE);
    }
    DailyMax_DebugOnOpponents();
    return TRUE;"""
    replace_once(
        hol,
        """        CreateCharacterMons(&gParties[B_TRAINER_OPPONENT_A][0], a, gHoLCharacters[a].iconic, 3, FALSE);
        CreateCharacterMons(&gParties[B_TRAINER_OPPONENT_B][0], b, gHoLCharacters[b].iconic, 3, FALSE);
    }
    return TRUE;""",
        _steven_create,
        "steven matchup slots + debug opponents",
    )
    replace_once(
        hol,
        """        CreateCharacterMons(&gParties[B_TRAINER_OPPONENT_A][0], a, gHoLCharacters[a].iconic, 3, FALSE);
        CreateCharacterMons(&gParties[B_TRAINER_OPPONENT_B][0], b, gHoLCharacters[b].iconic, 3, FALSE);
    }
    DailyMax_DebugOnOpponents();
    return TRUE;""",
        _steven_create,
        "steven matchup slots (after debug hook)",
    )

    # La hoja de datos de Esmeralda resalta la naturaleza (y el nivel) en rojo; P_SUMMARY_SCREEN_NATURE_COLORS
    # solo afecta a la página de estadísticas. Se iguala al color normal del texto.
    replace_once(
        GAME / "src/pokemon_summary_screen.c",
        'static const u8 sMemoNatureTextColor[] = _("{COLOR LIGHT_RED}{SHADOW GREEN}");',
        'static const u8 sMemoNatureTextColor[] = _("{COLOR WHITE}{SHADOW DARK_GRAY}"); // Daily Steven: sin resaltar la naturaleza',
        "memo nature color",
    )

    # La ventana de habilidad ("SALAMENCE's / INTIMIDACIÓN") añade el posesivo inglés "'s" desde el código C.
    # En castellano se muestra solo el nombre del Pokémon.
    replace_once(
        GAME / "src/battle_interface.c",
        '    lastChar = gStringVar1[totalChar - 1];\n'
        '    StringAppend(gStringVar1, COMPOUND_STRING("\'"));\n'
        '    if (lastChar != CHAR_S && lastChar != CHAR_s)\n'
        '        StringAppend(gStringVar1, COMPOUND_STRING("s"));\n',
        '    lastChar = gStringVar1[totalChar - 1];  // Daily Steven: sin el posesivo inglés "\'s"\n'
        '    (void)lastChar;\n',
        "ability popup apostrophe",
    )

    # Aviso del movimiento del compañero (solo en combates con compañero de la IA; no sale en Daily Steven, pero se traduce).
    replace_once(
        GAME / "src/battle_controller_player.c",
        'COMPOUND_STRING("Partner will use:\\n")',
        'COMPOUND_STRING("El compañero usará:\\n")',
        "partner will use",
    )

    # ---- Mecánicas y ayudas: 3.ª generación estricta ----------------------------------------------------
    cfg = GAME / "include/config"
    for fname, name, value in (
        ("species_enabled.h", "P_GEN_4_CROSS_EVOS", "FALSE"),       # nada de evoluciones de 4.ª gen (Gallade...)
        ("battle.h", "B_RUN_TRAINER_BATTLE", "FALSE"),              # no se puede huir de un combate de entrenador
        ("battle.h", "B_AFFECTION_MECHANICS", "FALSE"),             # sin afecto (6.ª gen)
        ("battle.h", "B_SHOW_EFFECTIVENESS", "SHOW_EFFECTIVENESS_NEVER"),  # sin indicador de eficacia
        ("battle.h", "B_SHOW_TARGETS", "FALSE"),                    # sin resaltar objetivos
        ("battle.h", "B_SHOW_MOVE_DESCRIPTION", "FALSE"),           # sin "MOVE INFO" (L)
        ("battle.h", "B_LAST_USED_BALL", "FALSE"),                  # sin "última Poké Ball"
        ("battle.h", "B_TRAINERS_KNOCK_OFF_ITEMS", "FALSE"),        # los entrenadores no roban objetos (3.ª gen)
        ("summary_screen.h", "P_SUMMARY_SCREEN_MOVE_RELEARNER", "FALSE"),  # no reaprender movimientos
        ("summary_screen.h", "P_SUMMARY_SCREEN_NATURE_COLORS", "FALSE"),   # sin colores de naturaleza
        ("summary_screen.h", "P_SUMMARY_SCREEN_RENAME", "FALSE"),          # sin cambiar motes
        ("contest.h", "C_HIDE_CONTEST_DATA", "TRUE"),             # sin concursos (no existen en Rojo Fuego)
        ("item.h", "I_BERRY_PRICE", "GEN_3"),
        ("item.h", "I_EXP_SHARE_ITEM", "GEN_3"),
        ("pokemon.h", "P_TM_LITERACY", "GEN_3"),
        ("pokemon.h", "P_SHOW_TERA_TYPE", "GEN_3"),
    ):
        force_define(cfg / fname, name, value, f"gen3/{name}")

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
        'static const u8 sText_TwoTrainersWantToBattle[] = _("¡{B_TRAINER1_NAME_WITH_CLASS} y {B_TRAINER2_NAME_WITH_CLASS} quieren combatir!\\p");',
        "spanish two-trainer intro",
    )
    replace_once(
        messages,
        'static const u8 sText_TwoTrainersSentPkmn[] = _("{B_TRAINER1_NAME_WITH_CLASS} sent out {B_OPPONENT_MON1_NAME}!\\p{B_TRAINER2_NAME_WITH_CLASS} sent out {B_OPPONENT_MON2_NAME}!");',
        'static const u8 sText_TwoTrainersSentPkmn[] = _("¡{B_TRAINER1_NAME_WITH_CLASS} envió a {B_OPPONENT_MON1_NAME}!\\p¡{B_TRAINER2_NAME_WITH_CLASS} envió a {B_OPPONENT_MON2_NAME}!");',
        "spanish two-trainer send",
    )
    replace_once(
        messages,
        'static const u8 sText_Trainer2SentOutPkmn[] = _("{B_TRAINER2_NAME_WITH_CLASS} sent out {B_BUFF1}!");',
        'static const u8 sText_Trainer2SentOutPkmn[] = _("¡{B_TRAINER2_NAME_WITH_CLASS} envió a {B_BUFF1}!");',
        "spanish trainer2 send",
    )
    replace_once(
        messages,
        'static const u8 sText_Trainer2WithdrewPkmn[] = _("{B_TRAINER2_NAME_WITH_CLASS} withdrew {B_BUFF1}!");',
        'static const u8 sText_Trainer2WithdrewPkmn[] = _("¡{B_TRAINER2_NAME_WITH_CLASS} retiró a {B_BUFF1}!");',
        "spanish trainer2 withdraw",
    )
    # Estas dos se fijan por nombre de símbolo (valen tanto si el texto actual es el inglés original como si es
    # el de FireRed): así no dependen de lo que haya dejado la traducción.
    def set_string(symbol: str, inner: str) -> None:
        text = messages.read_text(encoding="utf-8")
        pat = re.compile(r"(static const u8 " + symbol + r"\[\] = _\()(?:\s*\"(?:\\.|[^\"\\])*\")+(\);)", re.S)
        new, n = pat.subn(lambda m: m.group(1) + '"' + inner + '"' + m.group(2), text, count=1)
        if n != 1:
            raise SystemExit(f"patch failed: {messages}: no existe {symbol}")
        if new != text:
            messages.write_text(new, encoding="utf-8")
            print(f"  patched {symbol}")
        else:
            print(f"  skip {symbol} (already applied)")

    set_string("sText_Trainer1SentOutPkmn2", "¡{B_TRAINER1_NAME_WITH_CLASS} envió a {B_BUFF1}!")
    set_string("sText_Trainer1WithdrewPkmn", "¡{B_TRAINER1_NAME_WITH_CLASS} retiró a {B_BUFF1}!")

    # --- Clases de entrenador de los rivales (STEVEN: como GOLD en Daily Gold; AURA/BLASCO: ASPIRANTE) ---
    # La clase + nombre se arma en dailymaximo.c (DailyMax_GetNameWithClass); aquí solo se engancha.
    text = messages.read_text(encoding="utf-8")
    if "DailyMax_GetNameWithClass" not in text:
        text = text.replace(
            '#include "constants/weather.h"\n',
            '#include "constants/weather.h"\n#include "dailymaximo.h"\n', 1)
        text, n = re.subn(
            r"(static const u8 \*BattleStringGetOpponentClassByTrainerId\(u16 trainerId\)\n\{\n    const u8 \*toCpy;\n)",
            r"\1\n    if (DailyMax_GetClassName(trainerId) != NULL)\n        return DailyMax_GetClassName(trainerId);\n", text, count=1)
        if n != 1:
            raise SystemExit("patch failed: BattleStringGetOpponentClassByTrainerId")
        for case, expr in (
            ("B_TXT_TRAINER1_NAME_WITH_CLASS", "TRAINER_BATTLE_PARAM.opponentA"),
            ("B_TXT_TRAINER2_NAME_WITH_CLASS", "TRAINER_BATTLE_PARAM.opponentB"),
        ):
            text, n = re.subn(
                r"(            case " + case + r":[^\n]*\n)",
                lambda m, e=expr: m.group(1) + "                toCpy = DailyMax_GetNameWithClass(" + e + ");\n                if (toCpy != NULL)\n                    break;\n",
                text, count=1)
            if n != 1:
                raise SystemExit(f"patch failed: {case}")
        atk = (
            "                if (GetBattlerPosition(gBattlerAttacker) == B_POSITION_OPPONENT_LEFT || GetBattlerPosition(gBattlerAttacker) == B_POSITION_OPPONENT_RIGHT)\n"
            "                {\n"
            "                    toCpy = DailyMax_GetNameWithClass((GetBattlerPosition(gBattlerAttacker) == B_POSITION_OPPONENT_RIGHT\n"
            "                        && (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) && !BATTLE_TWO_VS_ONE_OPPONENT)\n"
            "                        ? TRAINER_BATTLE_PARAM.opponentB : TRAINER_BATTLE_PARAM.opponentA);\n"
            "                    if (toCpy != NULL)\n                        break;\n"
            "                }\n")
        text, n = re.subn(
            r"(            case B_TXT_ATK_TRAINER_NAME_WITH_CLASS:[^\n]*\n)",
            lambda m: m.group(1) + atk, text, count=1)
        if n != 1:
            raise SystemExit("patch failed: B_TXT_ATK_TRAINER_NAME_WITH_CLASS")
        messages.write_text(text, encoding="utf-8")
        print("  patched clases de entrenador (STEVEN / ASPIRANTE) en battle_message.c")
    else:
        print("  skip clases de entrenador (already applied)")

    # --- Glifo «Lv» -> «Nv.»: el dibujo está en overlay/graphics/fonts (tools/make_es_graphics.py); aquí se ensancha ---
    # Mantener sincronizado con FONT_LV_WIDTHS de tools/make_es_graphics.py.
    fonts_c = GAME / "src/fonts.c"
    ftext = fonts_c.read_text(encoding="utf-8")
    for table, width in (("gFontSmallLatinGlyphWidths", 9), ("gFontNormalLatinGlyphWidths", 14)):
        m = re.search(r"(" + table + r"\[\] = \{)(.*?)(\n\};)", ftext, re.S)
        if m is None:
            raise SystemExit(f"patch failed: {fonts_c}: no existe {table}")
        nums = re.findall(r"\d+", m.group(2))
        if len(nums) != 512:
            raise SystemExit(f"patch failed: {table} tiene {len(nums)} anchos (se esperaban 512)")
        nums[0x34] = str(width)
        nums[0x105] = str(width)
        body = "\n" + "\n".join("    " + ", ".join(f"{int(n):2d}" for n in nums[i:i + 16]) + "," for i in range(0, 512, 16))
        ftext = ftext[:m.start(2)] + body + ftext[m.end(2):]
    fonts_c.write_text(ftext, encoding="utf-8")
    print("  patched anchos del glifo Nv. en fonts.c")

    # --- Orden castellano en los textos con nombre + «enemigo/salvaje»: «SKARMORY enemigo» (no «enemigoSKARMORY») ---
    text = messages.read_text(encoding="utf-8")
    old_nick = (
        "            if (!IsOnPlayerSide(src[srcID + 1]))\n"
        "            {\n"
        "                if (src[srcID] == B_BUFF_MON_NICK_WITH_PREFIX_LOWER)\n"
        "                {\n"
        "                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)\n"
        "                        StringAppend(dst, sText_FoePkmnPrefixLower);\n"
        "                    else\n"
        "                        StringAppend(dst, sText_WildPkmnPrefixLower);\n"
        "                }\n"
        "                else\n"
        "                {\n"
        "                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)\n"
        "                        StringAppend(dst, sText_FoePkmnPrefix);\n"
        "                    else\n"
        "                        StringAppend(dst, sText_WildPkmnPrefix);\n"
        "                }\n"
        "            }\n"
        "            GetMonData(&GetBattlerParty(src[srcID + 1])[src[srcID + 2]], MON_DATA_NICKNAME, nickname);\n"
        "            StringGet_Nickname(nickname);\n"
        "            StringAppend(dst, nickname);\n"
    )
    new_nick = (
        "            GetMonData(&GetBattlerParty(src[srcID + 1])[src[srcID + 2]], MON_DATA_NICKNAME, nickname);\n"
        "            StringGet_Nickname(nickname);\n"
        "            StringAppend(dst, nickname);\n"
        "            if (!IsOnPlayerSide(src[srcID + 1]))\n"
        "            {\n"
        "                if (src[srcID] == B_BUFF_MON_NICK_WITH_PREFIX_LOWER)\n"
        "                {\n"
        "                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)\n"
        "                        StringAppend(dst, sText_FoePkmnPrefixLower);\n"
        "                    else\n"
        "                        StringAppend(dst, sText_WildPkmnPrefixLower);\n"
        "                }\n"
        "                else\n"
        "                {\n"
        "                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)\n"
        "                        StringAppend(dst, sText_FoePkmnPrefix);\n"
        "                    else\n"
        "                        StringAppend(dst, sText_WildPkmnPrefix);\n"
        "                }\n"
        "            }\n"
    )
    if old_nick in text:
        messages.write_text(text.replace(old_nick, new_nick, 1), encoding="utf-8")
        print("  patched orden nombre + enemigo en buffers")
    elif new_nick in text:
        print("  skip orden nombre + enemigo (already applied)")
    else:
        raise SystemExit("patch failed: B_BUFF_MON_NICK_WITH_PREFIX no tiene el aspecto esperado")

    print(">> Hooks done")


if __name__ == "__main__":
    main()
