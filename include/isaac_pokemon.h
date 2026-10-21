#ifndef GUARD_ISAAC_POKEMON_H
#define GUARD_ISAAC_POKEMON_H

#include "data.h"

#define ISAAC_SET_TIERS  3  // sets par Pokémon, du plus faible (0) au plus fort (ISAAC_SET_TIERS - 1)

const struct TrainerMon *IsaacGetSet(enum Species species, u32 tier);

#endif // GUARD_ISAAC_POKEMON_H
