#include "global.h"
#include "test/battle.h"

// Daily Steven, ronda 6A: pruebas del comportamiento REAL de la ROM en mecÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¡nicas de 3.ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Âª generaciÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â³n.
// Se ejecutan con el sistema de pruebas de la expansion (make check TESTS='Gen3*', ver scripts/run-gen3-tests.sh).
// Los mensajes del juego estÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¡n en castellano, asÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â­ que estas pruebas NO usan MESSAGE(): comprueban animaciones, barras de
// vida, estados y estadÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â­sticas. La fÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â³rmula de daÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â±o puede diferir en ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â±1-2 PS del cartucho original (aceptado).

// DaÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â±o base de la fÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â³rmula de 3.ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Âª gen para un PokÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â©mon de nivel 100, con la tirada aleatoria al mÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¡ximo (100 %).
static s32 BaseDmg(u32 power, u32 atk, u32 def)
{
    return ((2 * 100 / 5 + 2) * power * atk / def) / 50 + 2;
}

#define MAX_ROLL WITH_RNG(RNG_DAMAGE_MODIFIER, 0)

DOUBLE_BATTLE_TEST("Gen3 multiples: Terremoto conserva dano con dos objetivos vivos")
{
    s16 dmgTwo, dmgOne;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); Attack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); Defense(100); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(999); HP(1); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_EARTHQUAKE, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
        TURN { MOVE(playerLeft, MOVE_EARTHQUAKE, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgTwo);
        HP_BAR(playerRight);
        HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgOne);
        HP_BAR(playerRight);
    } THEN {
        EXPECT_MUL_EQ(dmgOne, Q_4_12(1.0), dmgTwo);
        EXPECT_GE(dmgOne, BaseDmg(100, 200, 100) - 4);
        EXPECT_LE(dmgOne, BaseDmg(100, 200, 100) + 4);
    }
}


DOUBLE_BATTLE_TEST("Gen3 multiples: ataques a ambos rivales con dos y uno vivos")
{
    s16 dmgTwo, dmgOne;
    u32 move;
    PARAMETRIZE { move = MOVE_ACID; }
    PARAMETRIZE { move = MOVE_SURF; }
    PARAMETRIZE { move = MOVE_BUBBLE; }
    PARAMETRIZE { move = MOVE_ROCK_SLIDE; }
    PARAMETRIZE { move = MOVE_POWDER_SNOW; }
    PARAMETRIZE { move = MOVE_ERUPTION; }
    PARAMETRIZE { move = MOVE_WATER_SPOUT; }
    PARAMETRIZE { move = MOVE_HEAT_WAVE; }
    PARAMETRIZE { move = MOVE_ICY_WIND; }
    PARAMETRIZE { move = MOVE_BLIZZARD; }
    PARAMETRIZE { move = MOVE_HYPER_VOICE; }
    PARAMETRIZE { move = MOVE_MUDDY_WATER; }
    PARAMETRIZE { move = MOVE_RAZOR_LEAF; }
    PARAMETRIZE { move = MOVE_AIR_CUTTER; }
    PARAMETRIZE { move = MOVE_TWISTER; }
    PARAMETRIZE { move = MOVE_SWIFT; }
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); SpDefense(100); Ability(ABILITY_SHIELD_DUST); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(999); HP(1); }
    } WHEN {
        TURN { MOVE(playerLeft, move, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
        TURN { MOVE(playerLeft, move, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgTwo);
        HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgOne);
    } THEN {
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
        EXPECT_MUL_EQ(dmgOne, Q_4_12(0.5), dmgTwo);
    }
}


DOUBLE_BATTLE_TEST("Gen3 multiples: Follow Me redirects single target moves used by opponents to user")
{
    struct BattlePokemon *moveUser = NULL;
    struct BattlePokemon *partner = NULL;
    PARAMETRIZE { moveUser = opponentLeft; partner = opponentRight; }
    PARAMETRIZE { moveUser = opponentRight; partner = opponentLeft; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SCRATCH, target: moveUser);
               MOVE(playerRight, MOVE_SCRATCH, target: partner);
               MOVE(moveUser, MOVE_FOLLOW_ME);
               MOVE(partner, MOVE_SCRATCH, target: playerLeft); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FOLLOW_ME, moveUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerLeft);
        HP_BAR(moveUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerRight);
        HP_BAR(moveUser);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, partner);
        HP_BAR(playerLeft);
    }
}


DOUBLE_BATTLE_TEST("Gen3 multiples: Helping Hand boosts the power of attacking moves by 50%", s16 damage)
{
    bool32 useHelpingHand;

    PARAMETRIZE { useHelpingHand = FALSE; }
    PARAMETRIZE { useHelpingHand = TRUE;  }

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        if (useHelpingHand)
            TURN { MOVE(playerRight, MOVE_HELPING_HAND, target: playerLeft); MOVE(playerLeft, MOVE_SCRATCH, target: opponentLeft); }
        else
            TURN { MOVE(playerLeft, MOVE_SCRATCH, target: opponentLeft); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.5), results[1].damage);
    }
}


DOUBLE_BATTLE_TEST("Gen3 multiples: Estado afecta a ambos rivales")
{
    u32 move, stat;
    PARAMETRIZE { move = MOVE_GROWL; stat = STAT_ATK; }
    PARAMETRIZE { move = MOVE_LEER; stat = STAT_DEF; }
    PARAMETRIZE { move = MOVE_TAIL_WHIP; stat = STAT_DEF; }
    PARAMETRIZE { move = MOVE_STRING_SHOT; stat = STAT_SPEED; }
    PARAMETRIZE { move = MOVE_SWEET_SCENT; stat = STAT_EVASION; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, move); }
    } THEN {
        EXPECT_EQ(opponentLeft->statStages[stat], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponentRight->statStages[stat], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(playerRight->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Gen3 multiples: Swords Dance afecta al usuario")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SWORDS_DANCE); }
    } THEN {
        EXPECT_EQ(playerLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        EXPECT_EQ(playerRight->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponentLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponentRight->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Gen3 multiples: Haze afecta a todo el campo")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SWORDS_DANCE); MOVE(playerRight, MOVE_SWORDS_DANCE); MOVE(opponentLeft, MOVE_SWORDS_DANCE); MOVE(opponentRight, MOVE_SWORDS_DANCE); }
        TURN { MOVE(playerLeft, MOVE_HAZE); }
    } THEN {
        EXPECT_EQ(playerLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponentLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(playerRight->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        EXPECT_EQ(opponentRight->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Gen3 multiples: Lightning Rod redirige sin inmunidad ni aumento especial")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_LIGHTNING_ROD); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_THUNDER_SHOCK, target: opponentLeft); }
    } SCENE {
        HP_BAR(opponentRight);
        NOT HP_BAR(opponentLeft);
    } THEN {
        EXPECT_EQ(opponentRight->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Gen3 multiples: Thrash elige un unico rival")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_THRASH); }
    } THEN {
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
        EXPECT((opponentLeft->hp < opponentLeft->maxHP) != (opponentRight->hp < opponentRight->maxHP));
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EARTHQUAKE ausencias 0")
{
    u32 move = MOVE_EARTHQUAKE, power = 100, missing = 0;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EARTHQUAKE ausencias 1")
{
    u32 move = MOVE_EARTHQUAKE, power = 100, missing = 1;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EARTHQUAKE ausencias 2")
{
    u32 move = MOVE_EARTHQUAKE, power = 100, missing = 2;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EARTHQUAKE ausencias 3")
{
    u32 move = MOVE_EARTHQUAKE, power = 100, missing = 3;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_MAGNITUDE ausencias 0")
{
    u32 move = MOVE_MAGNITUDE, power = 70, missing = 0;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_MAGNITUDE ausencias 1")
{
    u32 move = MOVE_MAGNITUDE, power = 70, missing = 1;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_MAGNITUDE ausencias 2")
{
    u32 move = MOVE_MAGNITUDE, power = 70, missing = 2;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_MAGNITUDE ausencias 3")
{
    u32 move = MOVE_MAGNITUDE, power = 70, missing = 3;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EXPLOSION ausencias 0")
{
    u32 move = MOVE_EXPLOSION, power = 250, missing = 0;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EXPLOSION ausencias 1")
{
    u32 move = MOVE_EXPLOSION, power = 250, missing = 1;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EXPLOSION ausencias 2")
{
    u32 move = MOVE_EXPLOSION, power = 250, missing = 2;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_EXPLOSION ausencias 3")
{
    u32 move = MOVE_EXPLOSION, power = 250, missing = 3;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_SELF_DESTRUCT ausencias 0")
{
    u32 move = MOVE_SELF_DESTRUCT, power = 200, missing = 0;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_SELF_DESTRUCT ausencias 1")
{
    u32 move = MOVE_SELF_DESTRUCT, power = 200, missing = 1;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_SELF_DESTRUCT ausencias 2")
{
    u32 move = MOVE_SELF_DESTRUCT, power = 200, missing = 2;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MOVE_SELF_DESTRUCT ausencias 3")
{
    u32 move = MOVE_SELF_DESTRUCT, power = 200, missing = 3;
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(missing == 1 || missing == 3 ? 1 : 999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(200); MaxHP(999); HP(missing >= 2 ? 1 : 999); }
    } WHEN {
        if (missing == 1 || missing == 3)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        if (missing >= 2)
            TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_MAGNITUDE, 50)); }
    } SCENE {
        if (missing == 1 || missing == 3) HP_BAR(playerRight);
        if (missing >= 2) HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &damage);
    } THEN {
        s32 expected = BaseDmg(power, 100, move == MOVE_EXPLOSION || move == MOVE_SELF_DESTRUCT ? 100 : 200);
        EXPECT_GE(damage, expected * 85 / 100 - 2);
        EXPECT_LE(damage, expected + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Viento Cortante carga y reduce solo con dos rivales", s16 damage)
{
    bool32 removeFoe;
    PARAMETRIZE { removeFoe = FALSE; }
    PARAMETRIZE { removeFoe = TRUE; }
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(10); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); Defense(200); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); MaxHP(999); HP(1); }
    } WHEN {
        if (removeFoe) TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); }
        TURN { MOVE(playerLeft, MOVE_RAZOR_WIND, MAX_ROLL); }
        TURN { SKIP_TURN(playerLeft); }
    } SCENE {
        if (removeFoe) HP_BAR(opponentRight);
        HP_BAR(opponentLeft, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[1].damage, Q_4_12(0.5), results[0].damage);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: curacion de equipo incluye seis plazas y respeta Insonorizar")
{
    u32 move;
    PARAMETRIZE { move = MOVE_HEAL_BELL; }
    PARAMETRIZE { move = MOVE_AROMATHERAPY; }
    GIVEN {
        PLAYER(SPECIES_MEW) { Status1(STATUS1_POISON); }
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); Ability(ABILITY_SOUNDPROOF); }
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); }
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); Ability(ABILITY_SOUNDPROOF); }
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); }
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); }
        OPPONENT(SPECIES_WYNAUT) { Status1(STATUS1_POISON); }
    } WHEN {
        TURN { MOVE(playerLeft, move); }
    } THEN {
        for (u32 slot = 0; slot < 6; slot++) {
            u32 expected = move == MOVE_HEAL_BELL && (slot == 1 || slot == 3) ? STATUS1_POISON : STATUS1_NONE;
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_STATUS), expected);
        }
        EXPECT_EQ(opponentLeft->status1, STATUS1_POISON);
        EXPECT_EQ(opponentRight->status1, STATUS1_POISON);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Canto Mortal ignora Proteccion y respeta Insonorizar")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_SOUNDPROOF); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_PERISH_SONG); MOVE(playerRight, MOVE_PROTECT); MOVE(opponentLeft, MOVE_PROTECT); }
        TURN {} TURN {} TURN {}
    } THEN {
        EXPECT_EQ(playerLeft->hp, 0);
        EXPECT_EQ(playerRight->hp, 0);
        EXPECT_EQ(opponentLeft->hp, 0);
        EXPECT_GT(opponentRight->hp, 0);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: clima afecta a ambos bandos")
{
    u32 move, weather;
    PARAMETRIZE { move = MOVE_RAIN_DANCE; weather = B_WEATHER_RAIN; }
    PARAMETRIZE { move = MOVE_SUNNY_DAY; weather = B_WEATHER_SUN; }
    PARAMETRIZE { move = MOVE_SANDSTORM; weather = B_WEATHER_SANDSTORM; }
    PARAMETRIZE { move = MOVE_HAIL; weather = B_WEATHER_HAIL; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, move); }
    } THEN {
        EXPECT(gBattleWeather & weather);
        if (move == MOVE_HAIL || move == MOVE_SANDSTORM) {
            EXPECT_LT(playerLeft->hp, playerLeft->maxHP);
            EXPECT_LT(playerRight->hp, playerRight->maxHP);
            EXPECT_LT(opponentLeft->hp, opponentLeft->maxHP);
            EXPECT_LT(opponentRight->hp, opponentRight->maxHP);
        }
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: deportes reducen ambos bandos y cesan al retirar usuario")
{
    u32 sport, attack;
    s16 before[2], during[2], after[2];
    PARAMETRIZE { sport = MOVE_MUD_SPORT; attack = MOVE_SHOCK_WAVE; }
    PARAMETRIZE { sport = MOVE_WATER_SPORT; attack = MOVE_EMBER; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(999); HP(999); Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(999); HP(999); Speed(50); Ability(ABILITY_SHIELD_DUST); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(999); HP(999); Speed(20); Ability(ABILITY_SHIELD_DUST); }
        OPPONENT(SPECIES_WYNAUT) { MaxHP(999); HP(999); Speed(10); }
    } WHEN {
        TURN { MOVE(playerRight, attack, target: opponentLeft, MAX_ROLL); MOVE(opponentLeft, attack, target: playerRight, MAX_ROLL); }
        TURN { MOVE(playerLeft, sport); }
        TURN { MOVE(playerRight, attack, target: opponentLeft, MAX_ROLL); MOVE(opponentLeft, attack, target: playerRight, MAX_ROLL); }
        TURN { SWITCH(playerLeft, 2); }
        TURN { MOVE(playerRight, attack, target: opponentLeft, MAX_ROLL); MOVE(opponentLeft, attack, target: playerRight, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft, captureDamage: &before[0]); HP_BAR(playerRight, captureDamage: &before[1]);
        HP_BAR(opponentLeft, captureDamage: &during[0]); HP_BAR(playerRight, captureDamage: &during[1]);
        HP_BAR(opponentLeft, captureDamage: &after[0]); HP_BAR(playerRight, captureDamage: &after[1]);
    } THEN {
        for (u32 side = 0; side < 2; side++) {
            EXPECT_MUL_EQ(before[side], Q_4_12(0.5), during[side]);
            EXPECT_EQ(before[side], after[side]);
        }
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: Proteccion e inmunidad conservan reduccion de dos rivales", s16 damage)
{
    u32 mode;
    PARAMETRIZE { mode = 0; }
    PARAMETRIZE { mode = 1; }
    PARAMETRIZE { mode = 2; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Ability(mode == 2 ? ABILITY_WATER_ABSORB : ABILITY_SHADOW_TAG); MaxHP(999); HP(999); }
    } WHEN {
        if (mode == 1)
            TURN { MOVE(playerLeft, MOVE_SURF, MAX_ROLL); MOVE(opponentRight, MOVE_PROTECT); }
        else
            TURN { MOVE(playerLeft, MOVE_SURF, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft, captureDamage: &results[i].damage);
    } THEN {
        if (mode != 0) EXPECT_EQ(opponentRight->hp, opponentRight->maxHP);
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
        EXPECT_EQ(results[0].damage, results[2].damage);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Terremoto con Proteccion o Levitacion conserva dano", s16 damage)
{
    u32 mode;
    PARAMETRIZE { mode = 0; }
    PARAMETRIZE { mode = 1; }
    PARAMETRIZE { mode = 2; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET) { Ability(mode == 2 ? ABILITY_LEVITATE : ABILITY_SHADOW_TAG); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        if (mode == 1)
            TURN { MOVE(playerLeft, MOVE_EARTHQUAKE, MAX_ROLL); MOVE(playerRight, MOVE_PROTECT); }
        else
            TURN { MOVE(playerLeft, MOVE_EARTHQUAKE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft, captureDamage: &results[i].damage);
    } THEN {
        if (mode != 0) EXPECT_EQ(playerRight->hp, playerRight->maxHP);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
        EXPECT_EQ(results[0].damage, results[2].damage);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: cuatro movimientos aleatorios pueden elegir ambos rivales")
{
    u32 move, choice;
    PARAMETRIZE { move = MOVE_THRASH; choice = 0; }
    PARAMETRIZE { move = MOVE_THRASH; choice = 1; }

    PARAMETRIZE { move = MOVE_PETAL_DANCE; choice = 0; }
    PARAMETRIZE { move = MOVE_PETAL_DANCE; choice = 1; }

    PARAMETRIZE { move = MOVE_OUTRAGE; choice = 0; }
    PARAMETRIZE { move = MOVE_OUTRAGE; choice = 1; }

    PARAMETRIZE { move = MOVE_UPROAR; choice = 0; }
    PARAMETRIZE { move = MOVE_UPROAR; choice = 1; }


    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_RANDOM_TARGET, choice)); }
    } THEN {
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
        EXPECT_LT((choice ? opponentRight : opponentLeft)->hp, (choice ? opponentRight : opponentLeft)->maxHP);
        EXPECT_EQ((choice ? opponentLeft : opponentRight)->hp, (choice ? opponentLeft : opponentRight)->maxHP);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: prioridad de Senuelo sobre Pararrayos y ataques multiples")
{
    u32 move;
    PARAMETRIZE { move = MOVE_THUNDER_SHOCK; }
    PARAMETRIZE { move = MOVE_SURF; }
    PARAMETRIZE { move = MOVE_GROWL; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_LIGHTNING_ROD); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_FOLLOW_ME); MOVE(playerLeft, move, target: opponentRight); }
    } THEN {
        if (move == MOVE_THUNDER_SHOCK) {
            EXPECT_LT(opponentLeft->hp, opponentLeft->maxHP);
            EXPECT_EQ(opponentRight->hp, opponentRight->maxHP);
        } else if (move == MOVE_SURF) {
            EXPECT_LT(opponentLeft->hp, opponentLeft->maxHP);
            EXPECT_LT(opponentRight->hp, opponentRight->maxHP);
        } else {
            EXPECT_EQ(opponentLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
            EXPECT_EQ(opponentRight->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        }
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Pararrayos del aliado no atrae ataques propios en Esmeralda")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_LIGHTNING_ROD); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_THUNDER_SHOCK, target: opponentLeft); }
    } THEN {
        EXPECT_LT(opponentLeft->hp, opponentLeft->maxHP);
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Onda Trueno es individual y Pararrayos la redirige")
{
    bool32 redirect;
    PARAMETRIZE { redirect = FALSE; }
    PARAMETRIZE { redirect = TRUE; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT) { Ability(redirect ? ABILITY_LIGHTNING_ROD : ABILITY_SHADOW_TAG); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_THUNDER_WAVE, target: opponentLeft); }
    } THEN {
        EXPECT_EQ(opponentLeft->status1, redirect ? STATUS1_NONE : STATUS1_PARALYSIS);
        EXPECT_EQ(opponentRight->status1, redirect ? STATUS1_PARALYSIS : STATUS1_NONE);
        EXPECT_EQ(playerRight->status1, STATUS1_NONE);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Danza Caos confunde a los otros tres")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TEETER_DANCE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, opponentLeft);
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, playerRight);
        ANIMATION(ANIM_TYPE_STATUS, B_ANIM_STATUS_CONFUSION, opponentRight);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: duracion de los cuatro climas")
{
    u32 move;
    PARAMETRIZE { move = MOVE_RAIN_DANCE; }
    PARAMETRIZE { move = MOVE_SUNNY_DAY; }
    PARAMETRIZE { move = MOVE_SANDSTORM; }
    PARAMETRIZE { move = MOVE_HAIL; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, move); }
        TURN {} TURN {} TURN {} TURN {}
    } THEN {
        EXPECT_EQ(gBattleWeather, B_WEATHER_NONE);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: lluvia y sol modifican ataques de ambos bandos", s16 left, s16 right)
{
    u32 weather;
    PARAMETRIZE { weather = MOVE_SPLASH; }
    PARAMETRIZE { weather = MOVE_RAIN_DANCE; }
    PARAMETRIZE { weather = MOVE_SUNNY_DAY; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); }
    } WHEN {
        TURN { MOVE(playerLeft, weather); }
        TURN { MOVE(playerRight, MOVE_WATER_GUN, target: opponentLeft, MAX_ROLL); MOVE(opponentLeft, MOVE_WATER_GUN, target: playerRight, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft, captureDamage: &results[i].left);
        HP_BAR(playerRight, captureDamage: &results[i].right);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].left, Q_4_12(1.5), results[1].left);
        EXPECT_MUL_EQ(results[0].right, Q_4_12(1.5), results[1].right);
        EXPECT_MUL_EQ(results[0].left, Q_4_12(0.5), results[2].left);
        EXPECT_MUL_EQ(results[0].right, Q_4_12(0.5), results[2].right);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Canto Mortal con usuario Insonorizar")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Ability(ABILITY_SOUNDPROOF); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_PERISH_SONG); }
        TURN {} TURN {} TURN {}
    } THEN {
        EXPECT_GT(playerLeft->hp, 0);
        EXPECT_EQ(playerRight->hp, 0);
        EXPECT_EQ(opponentLeft->hp, 0);
        EXPECT_EQ(opponentRight->hp, 0);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Refuerzo falla sin aliado")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: playerRight); }
        TURN { MOVE(playerLeft, MOVE_HELPING_HAND); }
    } SCENE {
        HP_BAR(playerRight);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_HELPING_HAND, playerLeft);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Senuelo cesa cuando su usuario cae")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(90); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(10); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(5); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_FOLLOW_ME); MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentRight); MOVE(playerRight, MOVE_SCRATCH, target: opponentRight); }
    } THEN {
        EXPECT_EQ(opponentLeft->hp, 0);
        EXPECT_LT(opponentRight->hp, opponentRight->maxHP);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: ACID conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ACID, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(40, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(40, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: SURF conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SURF, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(95, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(95, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: BLIZZARD conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_BLIZZARD, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(120, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(120, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: RAZOR_LEAF conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_RAZOR_LEAF, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(55, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(55, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: SWIFT conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SWIFT, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(60, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(60, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: BUBBLE conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_BUBBLE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(20, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(20, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: ROCK_SLIDE conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ROCK_SLIDE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(75, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(75, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: POWDER_SNOW conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_POWDER_SNOW, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(40, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(40, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: ICY_WIND conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ICY_WIND, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(55, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(55, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: TWISTER conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TWISTER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(40, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(40, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: HEAT_WAVE conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_HEAT_WAVE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(100, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(100, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: ERUPTION conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ERUPTION, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(150, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(150, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: HYPER_VOICE conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_HYPER_VOICE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(90, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(90, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: AIR_CUTTER conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_AIR_CUTTER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(55, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(55, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: WATER_SPOUT conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_WATER_SPOUT, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(150, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(150, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: MUDDY_WATER conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_MUDDY_WATER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(95, 200, 100) / 2) - 2);
        EXPECT_LE(damage, (BaseDmg(95, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: RAZOR_WIND conserva reduccion si cae el primer rival")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(50); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_RAZOR_WIND, MAX_ROLL); }
        TURN { SKIP_TURN(playerLeft); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, (BaseDmg(80, 200, 100) / 2) * 85 / 100 - 2);
        EXPECT_LE(damage, (BaseDmg(80, 200, 100) / 2) + 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Danza Caos respeta Ritmo Propio y Proteccion")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_OWN_TEMPO); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_TEETER_DANCE); MOVE(opponentRight, MOVE_PROTECT); }
    } THEN {
        EXPECT_GT((u32)opponentLeft->volatiles.confusionTimer, 0);
        EXPECT_EQ((u32)playerRight->volatiles.confusionTimer, 0);
        EXPECT_EQ((u32)opponentRight->volatiles.confusionTimer, 0);
        EXPECT_EQ((u32)playerLeft->volatiles.confusionTimer, 0);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Explosion y Autodestruccion respetan Humedad")
{
    u32 move;
    PARAMETRIZE { move = MOVE_EXPLOSION; }
    PARAMETRIZE { move = MOVE_SELF_DESTRUCT; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_DAMP); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, move); }
    } THEN {
        EXPECT_EQ(playerLeft->hp, playerLeft->maxHP);
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
        EXPECT_EQ(opponentLeft->hp, opponentLeft->maxHP);
        EXPECT_EQ(opponentRight->hp, opponentRight->maxHP);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 1 a 50")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_POUND), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_KARATE_CHOP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DOUBLE_SLAP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_COMET_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MEGA_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PAY_DAY), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FIRE_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ICE_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_THUNDER_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SCRATCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_VISE_GRIP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_GUILLOTINE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_RAZOR_WIND), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_SWORDS_DANCE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_CUT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_GUST), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WING_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WHIRLWIND), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FLY), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BIND), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SLAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_VINE_WHIP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_STOMP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DOUBLE_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MEGA_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_JUMP_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ROLLING_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SAND_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HEADBUTT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HORN_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FURY_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HORN_DRILL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TACKLE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BODY_SLAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WRAP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TAKE_DOWN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_THRASH), TARGET_RANDOM);
        EXPECT_EQ(GetMoveTarget(MOVE_DOUBLE_EDGE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TAIL_WHIP), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_POISON_STING), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TWINEEDLE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PIN_MISSILE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_LEER), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_BITE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_GROWL), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_ROAR), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SING), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SUPERSONIC), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SONIC_BOOM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DISABLE), TARGET_SELECTED);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 51 a 100")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_ACID), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_EMBER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FLAMETHROWER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MIST), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_WATER_GUN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HYDRO_PUMP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SURF), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_ICE_BEAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BLIZZARD), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_PSYBEAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BUBBLE_BEAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_AURORA_BEAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HYPER_BEAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PECK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DRILL_PECK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SUBMISSION), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_LOW_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_COUNTER), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_SEISMIC_TOSS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_STRENGTH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ABSORB), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MEGA_DRAIN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_LEECH_SEED), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_GROWTH), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_RAZOR_LEAF), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_SOLAR_BEAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_POISON_POWDER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_STUN_SPORE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SLEEP_POWDER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PETAL_DANCE), TARGET_RANDOM);
        EXPECT_EQ(GetMoveTarget(MOVE_STRING_SHOT), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_DRAGON_RAGE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FIRE_SPIN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_THUNDER_SHOCK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_THUNDERBOLT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_THUNDER_WAVE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_THUNDER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ROCK_THROW), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_EARTHQUAKE), TARGET_FOES_AND_ALLY);
        EXPECT_EQ(GetMoveTarget(MOVE_FISSURE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DIG), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TOXIC), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CONFUSION), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PSYCHIC), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HYPNOSIS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MEDITATE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_AGILITY), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_QUICK_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_RAGE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TELEPORT), TARGET_USER);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 101 a 150")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_NIGHT_SHADE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MIMIC), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SCREECH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DOUBLE_TEAM), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_RECOVER), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_HARDEN), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_MINIMIZE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_SMOKESCREEN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CONFUSE_RAY), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WITHDRAW), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_DEFENSE_CURL), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_BARRIER), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_LIGHT_SCREEN), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_HAZE), TARGET_FIELD);
        EXPECT_EQ(GetMoveTarget(MOVE_REFLECT), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_FOCUS_ENERGY), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_BIDE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_METRONOME), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_MIRROR_MOVE), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_SELF_DESTRUCT), TARGET_FOES_AND_ALLY);
        EXPECT_EQ(GetMoveTarget(MOVE_EGG_BOMB), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_LICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SMOG), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SLUDGE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BONE_CLUB), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FIRE_BLAST), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WATERFALL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CLAMP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SWIFT), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_SKULL_BASH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SPIKE_CANNON), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CONSTRICT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_AMNESIA), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_KINESIS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SOFT_BOILED), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_HIGH_JUMP_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_GLARE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DREAM_EATER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_POISON_GAS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BARRAGE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_LEECH_LIFE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_LOVELY_KISS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SKY_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TRANSFORM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BUBBLE), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_DIZZY_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SPORE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FLASH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PSYWAVE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SPLASH), TARGET_USER);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 151 a 200")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_ACID_ARMOR), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_CRABHAMMER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_EXPLOSION), TARGET_FOES_AND_ALLY);
        EXPECT_EQ(GetMoveTarget(MOVE_FURY_SWIPES), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BONEMERANG), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_REST), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_ROCK_SLIDE), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_HYPER_FANG), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SHARPEN), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_CONVERSION), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_TRI_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SUPER_FANG), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SLASH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SUBSTITUTE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_STRUGGLE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SKETCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TRIPLE_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_THIEF), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SPIDER_WEB), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MIND_READER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_NIGHTMARE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FLAME_WHEEL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SNORE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CURSE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FLAIL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CONVERSION_2), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_AEROBLAST), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_COTTON_SPORE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_REVERSAL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SPITE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_POWDER_SNOW), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_PROTECT), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_MACH_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SCARY_FACE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FEINT_ATTACK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SWEET_KISS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BELLY_DRUM), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_SLUDGE_BOMB), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MUD_SLAP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_OCTAZOOKA), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SPIKES), TARGET_OPPONENTS_FIELD);
        EXPECT_EQ(GetMoveTarget(MOVE_ZAP_CANNON), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FORESIGHT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DESTINY_BOND), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_PERISH_SONG), TARGET_ALL_BATTLERS);
        EXPECT_EQ(GetMoveTarget(MOVE_ICY_WIND), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_DETECT), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_BONE_RUSH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_LOCK_ON), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_OUTRAGE), TARGET_RANDOM);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 201 a 250")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_SANDSTORM), TARGET_FIELD);
        EXPECT_EQ(GetMoveTarget(MOVE_GIGA_DRAIN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ENDURE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_CHARM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ROLLOUT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FALSE_SWIPE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SWAGGER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MILK_DRINK), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_SPARK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FURY_CUTTER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_STEEL_WING), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MEAN_LOOK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ATTRACT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SLEEP_TALK), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_HEAL_BELL), TARGET_USER_AND_ALLY);
        EXPECT_EQ(GetMoveTarget(MOVE_RETURN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PRESENT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FRUSTRATION), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SAFEGUARD), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_PAIN_SPLIT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SACRED_FIRE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MAGNITUDE), TARGET_FOES_AND_ALLY);
        EXPECT_EQ(GetMoveTarget(MOVE_DYNAMIC_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MEGAHORN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DRAGON_BREATH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BATON_PASS), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_ENCORE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PURSUIT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_RAPID_SPIN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SWEET_SCENT), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_IRON_TAIL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_METAL_CLAW), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_VITAL_THROW), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MORNING_SUN), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_SYNTHESIS), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_MOONLIGHT), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_HIDDEN_POWER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CROSS_CHOP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TWISTER), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_RAIN_DANCE), TARGET_FIELD);
        EXPECT_EQ(GetMoveTarget(MOVE_SUNNY_DAY), TARGET_FIELD);
        EXPECT_EQ(GetMoveTarget(MOVE_CRUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MIRROR_COAT), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_PSYCH_UP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_EXTREME_SPEED), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ANCIENT_POWER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SHADOW_BALL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FUTURE_SIGHT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ROCK_SMASH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WHIRLPOOL), TARGET_SELECTED);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 251 a 300")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_BEAT_UP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FAKE_OUT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_UPROAR), TARGET_RANDOM);
        EXPECT_EQ(GetMoveTarget(MOVE_STOCKPILE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_SPIT_UP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SWALLOW), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_HEAT_WAVE), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_HAIL), TARGET_FIELD);
        EXPECT_EQ(GetMoveTarget(MOVE_TORMENT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FLATTER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WILL_O_WISP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MEMENTO), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FACADE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FOCUS_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SMELLING_SALTS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FOLLOW_ME), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_NATURE_POWER), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_CHARGE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_TAUNT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HELPING_HAND), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_TRICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ROLE_PLAY), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WISH), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_ASSIST), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_INGRAIN), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_SUPERPOWER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MAGIC_COAT), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_RECYCLE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_REVENGE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BRICK_BREAK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_YAWN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_KNOCK_OFF), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ENDEAVOR), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ERUPTION), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_SKILL_SWAP), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_IMPRISON), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_REFRESH), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_GRUDGE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_SNATCH), TARGET_DEPENDS);
        EXPECT_EQ(GetMoveTarget(MOVE_SECRET_POWER), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DIVE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ARM_THRUST), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CAMOUFLAGE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_TAIL_GLOW), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_LUSTER_PURGE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MIST_BALL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FEATHER_DANCE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TEETER_DANCE), TARGET_FOES_AND_ALLY);
        EXPECT_EQ(GetMoveTarget(MOVE_BLAZE_KICK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MUD_SPORT), TARGET_FIELD);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 301 a 350")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_ICE_BALL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_NEEDLE_ARM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SLACK_OFF), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_HYPER_VOICE), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_POISON_FANG), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_CRUSH_CLAW), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BLAST_BURN), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HYDRO_CANNON), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_METEOR_MASH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ASTONISH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WEATHER_BALL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_AROMATHERAPY), TARGET_USER_AND_ALLY);
        EXPECT_EQ(GetMoveTarget(MOVE_FAKE_TEARS), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_AIR_CUTTER), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_OVERHEAT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ODOR_SLEUTH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ROCK_TOMB), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SILVER_WIND), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_METAL_SOUND), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_GRASS_WHISTLE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_TICKLE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_COSMIC_POWER), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_WATER_SPOUT), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_SIGNAL_BEAM), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SHADOW_PUNCH), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_EXTRASENSORY), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SKY_UPPERCUT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SAND_TOMB), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_SHEER_COLD), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MUDDY_WATER), TARGET_BOTH);
        EXPECT_EQ(GetMoveTarget(MOVE_BULLET_SEED), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_AERIAL_ACE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_ICICLE_SPEAR), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_IRON_DEFENSE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_BLOCK), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_HOWL), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_DRAGON_CLAW), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_FRENZY_PLANT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_BULK_UP), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_BOUNCE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MUD_SHOT), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_POISON_TAIL), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_COVET), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_VOLT_TACKLE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_MAGICAL_LEAF), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WATER_SPORT), TARGET_FIELD);
        EXPECT_EQ(GetMoveTarget(MOVE_CALM_MIND), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_LEAF_BLADE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DRAGON_DANCE), TARGET_USER);
        EXPECT_EQ(GetMoveTarget(MOVE_ROCK_BLAST), TARGET_SELECTED);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: tabla compilada de entradas 351 a 354")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(GetMoveTarget(MOVE_SHOCK_WAVE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_WATER_PULSE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_DOOM_DESIRE), TARGET_SELECTED);
        EXPECT_EQ(GetMoveTarget(MOVE_PSYCHO_BOOST), TARGET_SELECTED);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: estado multiple respeta Proteccion")
{
    u32 move, stat;
    PARAMETRIZE { move = MOVE_GROWL; stat = STAT_ATK; }
    PARAMETRIZE { move = MOVE_LEER; stat = STAT_DEF; }
    PARAMETRIZE { move = MOVE_TAIL_WHIP; stat = STAT_DEF; }
    PARAMETRIZE { move = MOVE_STRING_SHOT; stat = STAT_SPEED; }
    PARAMETRIZE { move = MOVE_SWEET_SCENT; stat = STAT_EVASION; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, move); MOVE(opponentRight, MOVE_PROTECT); }
    } THEN {
        EXPECT_EQ(opponentLeft->statStages[stat], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponentRight->statStages[stat], DEFAULT_STAT_STAGE);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Insonorizar bloquea Grunido y Vozarron solo en su portador")
{
    u32 move;
    PARAMETRIZE { move = MOVE_GROWL; }
    PARAMETRIZE { move = MOVE_HYPER_VOICE; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT) { Ability(ABILITY_SOUNDPROOF); }
    } WHEN {
        TURN { MOVE(playerLeft, move); }
    } THEN {
        EXPECT_EQ(opponentRight->hp, opponentRight->maxHP);
        EXPECT_EQ(opponentRight->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
        if (move == MOVE_GROWL)
            EXPECT_EQ(opponentLeft->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        else
            EXPECT_LT(opponentLeft->hp, opponentLeft->maxHP);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: aleatorios alcanzan al unico rival restante")
{
    u32 move;
    PARAMETRIZE { move = MOVE_THRASH; }
    PARAMETRIZE { move = MOVE_PETAL_DANCE; }
    PARAMETRIZE { move = MOVE_OUTRAGE; }
    PARAMETRIZE { move = MOVE_UPROAR; }
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_DRAGON_RAGE, target: opponentLeft); }
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_RANDOM_TARGET, 0)); }
    } THEN {
        EXPECT_EQ(opponentLeft->hp, 0);
        EXPECT_LT(opponentRight->hp, opponentRight->maxHP);
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: Puas afecta al campo rival y sus dos entradas")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WYNAUT) { MaxHP(400); HP(400); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPIKES); }
        TURN { SWITCH(opponentLeft, 2); SWITCH(opponentRight, 3); }
    } THEN {
        EXPECT_EQ(opponentLeft->hp, 350);
        EXPECT_EQ(opponentRight->hp, 350);
        EXPECT_EQ(playerRight->hp, playerRight->maxHP);
        EXPECT_EQ(playerLeft->hp, playerLeft->maxHP);
    }
}

DOUBLE_BATTLE_TEST("Gen3 auditoria: ACID tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_ACID, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(40, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(40, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: SURF tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_SURF, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(95, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(95, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: BLIZZARD tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_BLIZZARD, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(120, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(120, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: RAZOR_LEAF tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_RAZOR_LEAF, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(55, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(55, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: SWIFT tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_SWIFT, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(60, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(60, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: BUBBLE tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_BUBBLE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(20, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(20, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: ROCK_SLIDE tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_ROCK_SLIDE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(75, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(75, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: POWDER_SNOW tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_POWDER_SNOW, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(40, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(40, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: ICY_WIND tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_ICY_WIND, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(55, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(55, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: TWISTER tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_TWISTER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(40, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(40, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: HEAT_WAVE tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_HEAT_WAVE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(100, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(100, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: ERUPTION tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_ERUPTION, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(150, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(150, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: HYPER_VOICE tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_HYPER_VOICE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(90, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(90, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: AIR_CUTTER tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_AIR_CUTTER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(55, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(55, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: WATER_SPOUT tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_WATER_SPOUT, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(150, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(150, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: MUDDY_WATER tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); MOVE(playerLeft, MOVE_MUDDY_WATER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(95, 200, 100) - 2);
        EXPECT_LE(damage, BaseDmg(95, 200, 100) + 2);
    }
}
DOUBLE_BATTLE_TEST("Gen3 auditoria: RAZOR_WIND tras caer un rival antes en el mismo turno")
{
    s16 damage;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(50); Attack(200); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(20); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(10); Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_RAZOR_WIND); }
        TURN { MOVE(playerRight, MOVE_DRAGON_RAGE, target: opponentLeft); SKIP_TURN(playerLeft); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight, captureDamage: &damage);
    } THEN {
        EXPECT_GE(damage, BaseDmg(80, 200, 100) * 85 / 100 - 2);
        EXPECT_LE(damage, BaseDmg(80, 200, 100) + 2);
    }
}
