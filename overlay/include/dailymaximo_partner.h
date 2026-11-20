#ifndef GUARD_DAILYMAXIMO_PARTNER_H
#define GUARD_DAILYMAXIMO_PARTNER_H

#include <stdbool.h>
#include <stdint.h>

static inline bool DailyMax_SeedSelectsBlasco(uint32_t seed)
{
    // MurmurHash3's 32-bit finalizer mixes the full date hash before selection.
    seed ^= seed >> 16;
    seed *= UINT32_C(0x85ebca6b);
    seed ^= seed >> 13;
    seed *= UINT32_C(0xc2b2ae35);
    seed ^= seed >> 16;
    return (seed >> 31) != 0;
}

#endif
