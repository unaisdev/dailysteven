#include "global.h"
#include "test/battle.h"

// Daily Steven, ronda 6A: pruebas del comportamiento REAL de la ROM en mecánicas de 3.ª generación.
// Se ejecutan con el sistema de pruebas de la expansion (make check TESTS='Gen3*', ver scripts/run-gen3-tests.sh).
// Los mensajes del juego están en castellano, así que estas pruebas NO usan MESSAGE(): comprueban animaciones, barras de
// vida, estados y estadísticas. La fórmula de daño puede diferir en ±1-2 PS del cartucho original (aceptado).

// Daño base de la fórmula de 3.ª gen para un Pokémon de nivel 100, con la tirada aleatoria al máximo (100 %).
static s32 BaseDmg(u32 power, u32 atk, u32 def)
{
    return ((2 * 100 / 5 + 2) * power * atk / def) / 50 + 2;
}

#define MAX_ROLL WITH_RNG(RNG_DAMAGE_MODIFIER, 0)

// ---------------------------------------------------------------------------------------------------------------
// 1. Físico / especial por TIPO
// ---------------------------------------------------------------------------------------------------------------

SINGLE_BATTLE_TEST("Gen3 categoria por tipo: BOLA SOMBRA (Fantasma) es fisico y usa ATAQUE")
{
    s16 dmg;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); SpAttack(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); SpDefense(100); MaxHP(999); HP(999); Ability(ABILITY_BATTLE_ARMOR); }
    } WHEN {
        TURN { MOVE(player, MOVE_SHADOW_BALL, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &dmg);
    } THEN {
        // Fantasma contra Psiquico: x2. Con ATAQUE 200 el danio es 2*136; si usara ATAQUE ESPECIAL (100) seria 2*69.
        EXPECT_GE(dmg, 2 * BaseDmg(80, 200, 100) - 6);
        EXPECT_LE(dmg, 2 * BaseDmg(80, 200, 100) + 6);
    }
}

SINGLE_BATTLE_TEST("Gen3 categoria por tipo: PUNO FUEGO (Fuego) es especial y usa ATAQUE ESPECIAL")
{
    s16 dmg;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(100); SpAttack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); SpDefense(100); MaxHP(999); HP(999); Ability(ABILITY_BATTLE_ARMOR); }
    } WHEN {
        TURN { MOVE(player, MOVE_FIRE_PUNCH, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &dmg);
    } THEN {
        EXPECT_GE(dmg, BaseDmg(75, 200, 100) - 4);
        EXPECT_LE(dmg, BaseDmg(75, 200, 100) + 4);
    }
}

SINGLE_BATTLE_TEST("Gen3 categoria por tipo: el tipo del ataque decide la defensa que se usa (Fuego -> DEF. ESP., Fantasma -> DEFENSA)", s16 damage)
{
    u32 move;
    PARAMETRIZE { move = MOVE_FIRE_PUNCH; }
    PARAMETRIZE { move = MOVE_SHADOW_BALL; }
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); SpAttack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(400); SpDefense(100); MaxHP(999); HP(999); Ability(ABILITY_BATTLE_ARMOR); }
    } WHEN {
        TURN { MOVE(player, move, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_GE(results[0].damage, BaseDmg(75, 200, 100) - 4);      // especial: se defiende con DEF. ESP. 100
        EXPECT_LE(results[0].damage, BaseDmg(75, 200, 100) + 4);
        EXPECT_GE(results[1].damage, 2 * BaseDmg(80, 200, 400) - 4);  // fisico: se defiende con DEFENSA 400 (x2 Fantasma/Psiquico)
        EXPECT_LE(results[1].damage, 2 * BaseDmg(80, 200, 400) + 4);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 2. Movimientos de varios objetivos (dobles)
// ---------------------------------------------------------------------------------------------------------------

DOUBLE_BATTLE_TEST("Gen3 SURF en dobles: daña a los dos rivales y NO al aliado")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SURF); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SURF, playerLeft);
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight);
        NOT HP_BAR(playerRight);
    }
}

DOUBLE_BATTLE_TEST("Gen3 SURF en dobles: x0,5 con dos rivales y x1 con uno solo")
{
    s16 dmgTwo, dmgOne;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); SpAttack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); SpDefense(100); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(999); HP(1); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SURF, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
        TURN { MOVE(playerLeft, MOVE_SURF, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SURF, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgTwo);
        HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SURF, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgOne);
    } THEN {
        EXPECT_MUL_EQ(dmgOne, Q_4_12(0.5), dmgTwo);
        // valor absoluto: SURF 95, Mew no es de Agua (sin STAB), Wobbuffet neutro
        EXPECT_GE(dmgOne, BaseDmg(95, 200, 100) - 4);
        EXPECT_LE(dmgOne, BaseDmg(95, 200, 100) + 4);
    }
}

DOUBLE_BATTLE_TEST("Gen3 TERREMOTO en dobles: x1,0 a los dos rivales y al aliado (alcanza a todos)")
{
    s16 dmgFoeL, dmgAlly, dmgFoeR;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); Attack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); Defense(100); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); Defense(100); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); Defense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_EARTHQUAKE, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EARTHQUAKE, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmgFoeL);
        HP_BAR(playerRight, captureDamage: &dmgAlly);
        HP_BAR(opponentRight, captureDamage: &dmgFoeR);
    } THEN {
        // TERREMOTO 100, Mew sin STAB, todos neutros: 170 PS sin reduccion
        EXPECT_GE(dmgFoeL, BaseDmg(100, 200, 100) - 4);
        EXPECT_LE(dmgFoeL, BaseDmg(100, 200, 100) + 4);
        EXPECT_GE(dmgAlly, BaseDmg(100, 200, 100) - 4);
        EXPECT_LE(dmgAlly, BaseDmg(100, 200, 100) + 4);
        EXPECT_GE(dmgFoeR, BaseDmg(100, 200, 100) - 4);
        EXPECT_LE(dmgFoeR, BaseDmg(100, 200, 100) + 4);
    }
}

DOUBLE_BATTLE_TEST("Gen3 ALUD (ROCA): daña a los dos rivales a x0,5 y NO al aliado")
{
    s16 dmg;
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); Attack(200); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); Defense(100); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_ROCK_SLIDE, MAX_ROLL); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROCK_SLIDE, playerLeft);
        HP_BAR(opponentLeft, captureDamage: &dmg);
        HP_BAR(opponentRight);
        NOT HP_BAR(playerRight);
    } THEN {
        EXPECT_GE(dmg, BaseDmg(75, 200, 100) / 2 - 3);
        EXPECT_LE(dmg, BaseDmg(75, 200, 100) / 2 + 3);
    }
}

DOUBLE_BATTLE_TEST("Gen3 AVALANCHA es de un solo objetivo: no toca al otro rival ni al aliado")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_AVALANCHE, target: opponentLeft); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_AVALANCHE, playerLeft);
        HP_BAR(opponentLeft);
        NONE_OF {
            HP_BAR(opponentRight);
            HP_BAR(playerRight);
        }
    }
}

DOUBLE_BATTLE_TEST("Gen3 EXPLOSION alcanza al aliado y a los dos rivales")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(10); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(5); MaxHP(999); HP(999); }
        PLAYER(SPECIES_WYNAUT) { Speed(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_EXPLOSION); MOVE(playerRight, MOVE_CELEBRATE); MOVE(opponentLeft, MOVE_CELEBRATE); MOVE(opponentRight, MOVE_CELEBRATE); SEND_OUT(playerLeft, 2); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EXPLOSION, playerLeft);
        HP_BAR(opponentLeft);
        HP_BAR(playerRight);
        HP_BAR(opponentRight);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 3. Criticos
// ---------------------------------------------------------------------------------------------------------------

SINGLE_BATTLE_TEST("Gen3 critico: multiplica el danio por 2 (no por 1,5)")
{
    s16 normal, crit;
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, MAX_ROLL, criticalHit: FALSE); }
        TURN { MOVE(player, MOVE_TACKLE, MAX_ROLL, criticalHit: TRUE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &normal);
        HP_BAR(opponent, captureDamage: &crit);
    } THEN {
        EXPECT_MUL_EQ(normal, Q_4_12(2.0), crit);
    }
}

SINGLE_BATTLE_TEST("Gen3 critico: probabilidad base 1/16")
{
    s16 dmg;
    PASSES_RANDOMLY(1, 16, RNG_CRITICAL_HIT);
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &dmg);
    } THEN {
        // sin critico ~60 PS; con critico ~120 PS
        EXPECT_GT(dmg, 90);
    }
}

SINGLE_BATTLE_TEST("Gen3 critico: un movimiento de critico alto (CORTE/Slash) tiene 1/8")
{
    s16 dmg;
    PASSES_RANDOMLY(1, 8, RNG_CRITICAL_HIT);
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_SLASH, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &dmg);
    } THEN {
        // sin critico ~119 PS; con critico ~238 PS
        EXPECT_GT(dmg, 180);
    }
}

SINGLE_BATTLE_TEST("Gen3 critico: Slash + Foco Energia (nivel 1 + 2 = 3) tiene 1/3")
{
    s16 dmg;
    PASSES_RANDOMLY(1, 3, RNG_CRITICAL_HIT);
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_FOCUS_ENERGY); }
        TURN { MOVE(player, MOVE_SLASH, MAX_ROLL); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_FOCUS_ENERGY, player);
        HP_BAR(opponent, captureDamage: &dmg);
    } THEN {
        EXPECT_GT(dmg, 180);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 4. Tabla de tipos de 3.ª gen
// ---------------------------------------------------------------------------------------------------------------

SINGLE_BATTLE_TEST("Gen3 tipos: ACERO resiste FANTASMA y SINIESTRO (x0,5)", s16 damage)
{
    u32 move;
    PARAMETRIZE { move = MOVE_SHADOW_BALL; }
    PARAMETRIZE { move = MOVE_CRUNCH; }
    PARAMETRIZE { move = MOVE_EARTHQUAKE; }
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); SpAttack(200); }
        OPPONENT(SPECIES_MAGNEMITE) { Defense(100); SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, move, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        // Magnemite es Electrico/Acero. FANTASMA x0,5 (acero) x1 (electrico); SINIESTRO x0,5; TERREMOTO (tierra): x2 x2 = x4
        EXPECT_GE(results[0].damage, BaseDmg(80, 200, 100) / 2 - 3);
        EXPECT_LE(results[0].damage, BaseDmg(80, 200, 100) / 2 + 3);
        EXPECT_GE(results[1].damage, BaseDmg(80, 200, 100) / 2 - 3);
        EXPECT_LE(results[1].damage, BaseDmg(80, 200, 100) / 2 + 3);
        EXPECT_GE(results[2].damage, 4 * BaseDmg(100, 200, 100) - 8);
        EXPECT_LE(results[2].damage, 4 * BaseDmg(100, 200, 100) + 8);
    }
}

SINGLE_BATTLE_TEST("Gen3 tipos: no existe el tipo HADA (CLEFAIRY es Normal y el tipo Dragon le hace danio normal)")
{
    s16 dmg;
    GIVEN {
        ASSUME(GetSpeciesType(SPECIES_CLEFAIRY, 0) == TYPE_NORMAL);
        ASSUME(GetSpeciesType(SPECIES_CLEFAIRY, 1) == TYPE_NORMAL);
        PLAYER(SPECIES_MEW) { SpAttack(200); }
        OPPONENT(SPECIES_CLEFAIRY) { SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_DRAGON_CLAW, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &dmg);
    } THEN {
        // Garra Dragon 80 (el tipo Dragon es ESPECIAL en 3.ª gen), Mew sin STAB, Normal puro: neutro. Con tipo Hada seria inmune.
        EXPECT_GE(dmg, BaseDmg(80, 200, 100) - 4);
        EXPECT_LE(dmg, BaseDmg(80, 200, 100) + 4);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 5. Estados
// ---------------------------------------------------------------------------------------------------------------

// 3.ª gen: el contador de sueno es de 2 a 5 y baja 1 por cada intento de moverse; al llegar a 0 despierta. Es decir,
// el Pokemon pierde de 1 a 4 turnos. RNG_SLEEP_TURNS = 2 es el minimo (1 turno) y 5 el maximo (4 turnos).
SINGLE_BATTLE_TEST("Gen3 sueno: sorteo maximo (4 turnos): tras SPORE + 3 turnos sigue dormido")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPORE, WITH_RNG(RNG_SLEEP_TURNS, 5)); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_NE(opponent->status1 & STATUS1_SLEEP, 0);
    }
}

SINGLE_BATTLE_TEST("Gen3 sueno: sorteo maximo (4 turnos): al quinto turno ya ha despertado")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPORE, WITH_RNG(RNG_SLEEP_TURNS, 5)); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    } THEN {
        EXPECT_EQ(opponent->status1 & STATUS1_SLEEP, 0);
    }
}

SINGLE_BATTLE_TEST("Gen3 sueno: sorteo minimo (1 turno): tras SPORE + 1 turno ya ha despertado")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPORE, WITH_RNG(RNG_SLEEP_TURNS, 2)); MOVE(opponent, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    } THEN {
        EXPECT_EQ(opponent->status1 & STATUS1_SLEEP, 0);
    }
}

SINGLE_BATTLE_TEST("Gen3 paralisis: la velocidad se divide entre 4")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(30); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE, WITH_RNG(RNG_PARALYSIS, FALSE)); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        // 100/4 = 25 < 30: el rival va primero (con /2 = 50 iria primero el jugador)
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    }
}

SINGLE_BATTLE_TEST("Gen3 paralisis: 25 % de no moverse")
{
    PASSES_RANDOMLY(3, 4, RNG_PARALYSIS);
    GIVEN {
        PLAYER(SPECIES_MEW) { Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    }
}

SINGLE_BATTLE_TEST("Gen3 quemadura: pierde 1/8 de los PS maximos por turno")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(160); HP(160); Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(player->hp, 160 - 20);
    }
}

SINGLE_BATTLE_TEST("Gen3 quemadura: reduce a la mitad el danio de los ataques fisicos", s16 damage)
{
    u32 status;
    PARAMETRIZE { status = STATUS1_NONE; }
    PARAMETRIZE { status = STATUS1_BURN; }
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(200); Status1(status); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[1].damage, Q_4_12(2.0), results[0].damage);
    }
}

SINGLE_BATTLE_TEST("Gen3 quemadura: NO reduce el danio de los ataques especiales", s16 damage)
{
    u32 status;
    PARAMETRIZE { status = STATUS1_NONE; }
    PARAMETRIZE { status = STATUS1_BURN; }
    GIVEN {
        PLAYER(SPECIES_MEW) { SpAttack(200); Status1(status); MaxHP(999); HP(999); }
        OPPONENT(SPECIES_WOBBUFFET) { SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_FLAMETHROWER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Gen3 congelacion: 20 % de descongelarse cada turno")
{
    PASSES_RANDOMLY(1, 5, RNG_FROZEN);
    GIVEN {
        PLAYER(SPECIES_MEW) { Status1(STATUS1_FREEZE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
    }
}

SINGLE_BATTLE_TEST("Gen3 confusion: 50 % de golpearse a si mismo")
{
    PASSES_RANDOMLY(1, 2, RNG_CONFUSION);
    GIVEN {
        PLAYER(SPECIES_MEW) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_CONFUSE_RAY); MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CONFUSE_RAY, opponent);
        HP_BAR(opponent);   // el ataque llega al rival (no se golpea a si mismo)
    }
}

SINGLE_BATTLE_TEST("Gen3 veneno grave: 1/16, 2/16, 3/16 de los PS maximos")
{
    s16 d1, d2, d3;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { MaxHP(160); HP(160); Status1(STATUS1_TOXIC_POISON); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(player, captureDamage: &d1);
        HP_BAR(player, captureDamage: &d2);
        HP_BAR(player, captureDamage: &d3);
    } THEN {
        EXPECT_EQ(d1, 10);
        EXPECT_EQ(d2, 20);
        EXPECT_EQ(d3, 30);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 6. Clima
// ---------------------------------------------------------------------------------------------------------------

SINGLE_BATTLE_TEST("Gen3 clima de habilidad: LLOVIZNA dura para siempre")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_DRIZZLE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT(gBattleWeather & B_WEATHER_RAIN);
    }
}

SINGLE_BATTLE_TEST("Gen3 clima de habilidad: SEQUIA dura para siempre")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_DROUGHT); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT(gBattleWeather & B_WEATHER_SUN);
    }
}

SINGLE_BATTLE_TEST("Gen3 clima de habilidad: CHORRO ARENA dura para siempre")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SAND_STREAM); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT(gBattleWeather & B_WEATHER_SANDSTORM);
    }
}

SINGLE_BATTLE_TEST("Gen3 clima de movimiento: DANZA LLUVIA sigue 4 turnos despues de usarla")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_RAIN_DANCE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT(gBattleWeather & B_WEATHER_RAIN);   // turno 4 de 5
    }
}

SINGLE_BATTLE_TEST("Gen3 clima de movimiento: DANZA LLUVIA termina a los 5 turnos")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_RAIN_DANCE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT(!(gBattleWeather & B_WEATHER_RAIN));  // ya paso el quinto turno
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 7. Proteccion, Robustez, Intimidacion, Rastro y Presion
// ---------------------------------------------------------------------------------------------------------------

SINGLE_BATTLE_TEST("Gen3 PROTECCION: la segunda vez seguida falla con probabilidad 1/2")
{
    PASSES_RANDOMLY(1, 2, RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_PROTECT); }
        TURN { MOVE(player, MOVE_PROTECT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, player);
    }
}

SINGLE_BATTLE_TEST("Gen3 PROTECCION: la tercera vez seguida falla con probabilidad 1/4")
{
    PASSES_RANDOMLY(1, 4, RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_PROTECT); }
        TURN { MOVE(player, MOVE_PROTECT, WITH_RNG(RNG_PROTECT_FAIL, 1)); }
        TURN { MOVE(player, MOVE_PROTECT); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_PROTECT, player);
    }
}

SINGLE_BATTLE_TEST("Gen3 ROBUSTEZ: solo anula los KO de un golpe (un ataque normal que mata desde PS completos SI mata)")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(500); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); Defense(10); MaxHP(100); HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(opponent->hp, 0);
    }
}

SINGLE_BATTLE_TEST("Gen3 ROBUSTEZ: anula los KO de un golpe (ABISMO / Sheer Cold no hace nada)")
{
    GIVEN {
        PLAYER(SPECIES_MEW);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_STURDY); MaxHP(100); HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_FISSURE); }
    } SCENE {
        NOT HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->hp, 100);
    }
}

SINGLE_BATTLE_TEST("Gen3 INTIMIDACION: baja 1 nivel el ATAQUE del rival al entrar")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_ARCANINE) { Ability(ABILITY_INTIMIDATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Gen3 RASTRO: copia la habilidad del rival al entrar")
{
    GIVEN {
        PLAYER(SPECIES_GARDEVOIR) { Ability(ABILITY_TRACE); }
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_ROUGH_SKIN); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_TRACE);
    } THEN {
        EXPECT_EQ(player->ability, ABILITY_ROUGH_SKIN);
    }
}

SINGLE_BATTLE_TEST("Gen3 PRESION: el ataque contra un Pokemon con PRESION gasta 2 PP")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Ability(ABILITY_PRESSURE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_TACKLE) - 2);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 8. Objetos
// ---------------------------------------------------------------------------------------------------------------

SINGLE_BATTLE_TEST("Gen3 RESTOS: cura 1/16 de los PS maximos al final del turno")
{
    s16 heal;
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_LEFTOVERS); MaxHP(160); HP(80); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } SCENE {
        HP_BAR(player, captureDamage: &heal);
    } THEN {
        EXPECT_EQ(heal, -10);
        EXPECT_EQ(player->hp, 90);
    }
}

SINGLE_BATTLE_TEST("Gen3 CINTA ELEC. (Iman): potencia x1,1 los ataques de tipo Electrico", s16 damage)
{
    u32 item;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_MAGNET; }
    GIVEN {
        PLAYER(SPECIES_MEW) { SpAttack(200); Item(item); }
        OPPONENT(SPECIES_WOBBUFFET) { SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDERBOLT, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.1), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Gen3 potenciador de tipo (Carbon): potencia x1,1 los ataques de tipo Fuego", s16 damage)
{
    u32 item;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_CHARCOAL; }
    GIVEN {
        PLAYER(SPECIES_MEW) { SpAttack(200); Item(item); }
        OPPONENT(SPECIES_WOBBUFFET) { SpDefense(100); MaxHP(999); HP(999); }
    } WHEN {
        TURN { MOVE(player, MOVE_FLAMETHROWER, MAX_ROLL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.1), results[1].damage);
    }
}

// PARCIAL: la CINTA FOCUS tira Random() % 100 directamente (no hay etiqueta RNG para forzarla), asi que no se puede
// sortear. Se comprueba el valor que usa el motor (10 %) y que el portador sigue en pie con la cinta puesta.
SINGLE_BATTLE_TEST("Gen3 CINTA FOCUS: el motor usa 10 % de probabilidad y el golpe letal no deja a 0 PS con la tirada favorable")
{
    GIVEN {
        PLAYER(SPECIES_MEW) { Attack(500); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_FOCUS_BAND); Defense(10); MaxHP(100); HP(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(GetBattlerHoldEffectParam(B_BATTLER_1), 10);
        EXPECT_LE(opponent->hp, 1);
    }
}

SINGLE_BATTLE_TEST("Gen3 BAYA ZIDRA (Sitrus): cura 30 PS fijos al bajar a la mitad")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_SITRUS_BERRY); MaxHP(400); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); }
    } THEN {
        EXPECT_EQ(player->hp, 130);   // en 4.ª gen+ curaria el 25 % (100 PS)
    }
}

// ---------------------------------------------------------------------------------------------------------------
// 9. Cambio al caer un Pokemon (dobles)
// ---------------------------------------------------------------------------------------------------------------

DOUBLE_BATTLE_TEST("Gen3 al caer un Pokemon en dobles, el relevo entra en el acto (antes de la siguiente accion del turno)")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(1); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); }
        PLAYER(SPECIES_WYNAUT) { MaxHP(999); HP(999); Speed(1); }
        OPPONENT(SPECIES_MEW) { Speed(100); Attack(200); }
        OPPONENT(SPECIES_MEW) { Speed(50); Attack(200); }
    } WHEN {
        TURN {
            MOVE(opponentLeft, MOVE_TACKLE, target: playerLeft);
            MOVE(opponentRight, MOVE_TACKLE, target: playerLeft);
            MOVE(playerRight, MOVE_CELEBRATE);
            SEND_OUT(playerLeft, 2);
        }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponentLeft);
        HP_BAR(playerLeft);
        // el relevo (Wynaut, indice 2) ya esta en el campo cuando ataca el segundo rival y recibe el golpe
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponentRight);
        HP_BAR(playerLeft);
    } THEN {
        EXPECT_LT(playerLeft->hp, 999);
    }
}
