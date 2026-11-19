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

// Practice-only probe seeds (?seed=): fixed teams to verify Gen3 Surf / Earthquake.
#define DAILYMAX_SEED_TEST_SURF      900001u
#define DAILYMAX_SEED_TEST_EARTHQUAKE 900002u

#endif // GUARD_CONSTANTS_DAILYMAXIMO_H
