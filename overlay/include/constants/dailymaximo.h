#ifndef GUARD_CONSTANTS_DAILYMAXIMO_H
#define GUARD_CONSTANTS_DAILYMAXIMO_H

enum {
    DAILYMAX_PHASE_EXPLORING,
    DAILYMAX_PHASE_BATTLE,
    DAILYMAX_PHASE_RESULT,
    DAILYMAX_PHASE_FINISHED,
};

enum {
    DAILYMAX_RESULT_NONE,
    DAILYMAX_RESULT_WON,
    DAILYMAX_RESULT_LOST,
};

// Reserved seed bit for optional challenge status (off in v1; see docs/status-al-entrar.md).
#define DAILYMAX_SEED_STATUS_BIT 0x100

// ROM DE PRUEBAS (scripts\build-wsl.ps1 -Debug). En la ROM normal vale 0 y todo lo de debajo NO se compila.
// El script de compilación lo pone a 1 solo en la copia de WSL, nunca en el repo.
#ifndef DAILYMAX_DEBUG
#define DAILYMAX_DEBUG 0
#endif

// Semillas de prueba (solo ROM de pruebas, con ?seed=NNN):
//   900001 = tu equipo es 1 Magikarp: pierdes enseguida (forzar DERROTA).
//   900002 = los rivales empiezan con 1 PS y dormidos: ganas con cualquier golpe (forzar VICTORIA).
#define DAILYMAX_DEBUG_SEED_LOSE 900001u
#define DAILYMAX_DEBUG_SEED_WIN  900002u   // compañero AURA
#define DAILYMAX_DEBUG_SEED_WIN2 900003u   // compañero BLASCO (misma victoria segura)

#endif // GUARD_CONSTANTS_DAILYMAXIMO_H
