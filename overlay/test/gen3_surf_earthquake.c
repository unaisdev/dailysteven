#include "global.h"
#include "test/battle.h"

// Daily Steven Fase 0 / DEC-0.4 / RI-01

DOUBLE_BATTLE_TEST("Gen3 Surf: hits both foes, not the ally")
{
    GIVEN {
        ASSUME(B_MULTIPLE_TARGETS_DMG < GEN_4);
        ASSUME(GetMoveTarget(MOVE_SURF) == TARGET_BOTH);
        PLAYER(SPECIES_LAPRAS) { Speed(10); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(200); HP(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_SURF);
            MOVE(playerRight, MOVE_SPLASH);
            MOVE(opponentLeft, MOVE_SPLASH);
            MOVE(opponentRight, MOVE_SPLASH);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SURF, playerLeft);
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight);
        NOT HP_BAR(playerRight);
    }
}

DOUBLE_BATTLE_TEST("Gen3 Surf: half damage with two foes versus one")
{
    s16 dmgTwo;
    s16 dmgOne;

    GIVEN {
        ASSUME(B_MULTIPLE_TARGETS_DMG < GEN_4);
        ASSUME(GetMoveTarget(MOVE_SURF) == TARGET_BOTH);
        PLAYER(SPECIES_LAPRAS) { Speed(10); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); MaxHP(500); HP(500); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(500); HP(1); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_SURF);
            MOVE(playerRight, MOVE_SPLASH);
            MOVE(opponentLeft, MOVE_SPLASH);
            MOVE(opponentRight, MOVE_SPLASH);
        }
        TURN {
            MOVE(playerLeft, MOVE_SURF);
            MOVE(playerRight, MOVE_SPLASH);
            MOVE(opponentLeft, MOVE_SPLASH);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SURF, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgTwo);
        HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SURF, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgOne);
    } THEN {
        EXPECT_MUL_EQ(dmgOne, Q_4_12(0.5), dmgTwo);
    }
}

// Los casos de TERREMOTO (aliado incluido, x1,0 a los tres objetivos) estan en gen3_mechanics.c.
