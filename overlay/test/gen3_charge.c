#include "global.h"
#include "test/battle.h"

DOUBLE_BATTLE_TEST("Gen3 CARGA en dobles potencia solo el siguiente turno y no sube Defensa Especial")
{
    enum Move nextMove;
    s16 normal, afterCharge, expired;

    PARAMETRIZE { nextMove = MOVE_THUNDERBOLT; }
    PARAMETRIZE { nextMove = MOVE_SPLASH; }

    GIVEN {
        ASSUME(B_UPDATED_MOVE_DATA < GEN_5);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_LANTURN);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_BATTLE_ARMOR); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(playerRight, MOVE_THUNDERBOLT, target: opponentLeft, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
        TURN { MOVE(playerRight, MOVE_CHARGE); }
        if (nextMove == MOVE_THUNDERBOLT) {
            TURN { MOVE(playerRight, MOVE_THUNDERBOLT, target: opponentLeft, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
        } else {
            TURN { MOVE(playerRight, MOVE_SPLASH); }
        }
        TURN { MOVE(playerRight, MOVE_THUNDERBOLT, target: opponentLeft, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        HP_BAR(opponentLeft, captureDamage: &normal);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CHARGE, playerRight);
        if (nextMove == MOVE_THUNDERBOLT)
            HP_BAR(opponentLeft, captureDamage: &afterCharge);
        HP_BAR(opponentLeft, captureDamage: &expired);
    } THEN {
        if (nextMove == MOVE_THUNDERBOLT)
            EXPECT_MUL_EQ(normal, Q_4_12(2.0), afterCharge);
        EXPECT_EQ(normal, expired);
        EXPECT_EQ(playerRight->statStages[STAT_SPDEF], DEFAULT_STAT_STAGE);
    }
}
