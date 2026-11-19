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

DOUBLE_BATTLE_TEST("Gen3 Earthquake: hits the ally and both foes")
{
    GIVEN {
        ASSUME(B_MULTIPLE_TARGETS_DMG < GEN_4);
        ASSUME(GetMoveTarget(MOVE_EARTHQUAKE) == TARGET_FOES_AND_ALLY);
        PLAYER(SPECIES_RHYDON) { Speed(10); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(400); HP(400); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_EARTHQUAKE);
            MOVE(playerRight, MOVE_SPLASH);
            MOVE(opponentLeft, MOVE_SPLASH);
            MOVE(opponentRight, MOVE_SPLASH);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, playerLeft);
        HP_BAR(playerRight);
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight);
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
        EXPECT_MUL_EQ(dmgTwo, Q_4_12(0.5), dmgOne);
    }
}

DOUBLE_BATTLE_TEST("Gen3 Earthquake: full damage with three targets, half with two")
{
    s16 dmgThree;
    s16 dmgTwo;

    GIVEN {
        ASSUME(B_MULTIPLE_TARGETS_DMG < GEN_4);
        ASSUME(GetMoveTarget(MOVE_EARTHQUAKE) == TARGET_FOES_AND_ALLY);
        PLAYER(SPECIES_RHYDON) { Speed(10); }
        PLAYER(SPECIES_MAGIKARP) { Speed(5); MaxHP(20); HP(20); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); MaxHP(600); HP(600); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(600); HP(600); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_EARTHQUAKE);
            MOVE(playerRight, MOVE_SPLASH);
            MOVE(opponentLeft, MOVE_SPLASH);
            MOVE(opponentRight, MOVE_SPLASH);
        }
        TURN {
            MOVE(playerLeft, MOVE_EARTHQUAKE);
            MOVE(opponentLeft, MOVE_SPLASH);
            MOVE(opponentRight, MOVE_SPLASH);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, playerLeft);
        HP_BAR(playerRight);
        HP_BAR(opponentLeft, captureDamage: &dmgThree);
        HP_BAR(opponentRight);

        ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, playerLeft);
        NOT HP_BAR(playerRight);
        HP_BAR(opponentLeft, captureDamage: &dmgTwo);
        HP_BAR(opponentRight);
    } THEN {
        EXPECT_MUL_EQ(dmgTwo, Q_4_12(0.5), dmgThree);
    }
}
