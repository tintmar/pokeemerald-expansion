#include "global.h"
#include "data.h"
#include "isaac_pokemon.h"
#include "constants/abilities.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/battle.h"

/*
 * Sets des Pokémon du donjon : 3 par espèce (1 à 386), du plus faible au plus fort.
 * Les sets sont écrits dans tools/isaac/sets_gen*.py ; src/data/isaac_sets.h est généré par
 * tools/isaac/build_sets.py, qui vérifie aussi que les attaques sont apprenables.
 */

#include "data/isaac_sets.h"
#include "data/isaac_biomes.h"
#include "data/isaac_categories.h"

// NULL si l'espèce n'a pas de sets (au-delà de Deoxys)
const struct TrainerMon *IsaacGetSet(enum Species species, u32 tier)
{
    if (species == SPECIES_NONE || species >= ARRAY_COUNT(sIsaacSets) || sIsaacSets[species][0].species == SPECIES_NONE)
        return NULL;
    return &sIsaacSets[species][min(tier, ISAAC_SET_TIERS - 1)];
}

// ISAAC_CATEGORY_NONE au-delà de Deoxys
enum IsaacCategory IsaacGetSpeciesCategory(enum Species species)
{
    if (species >= ARRAY_COUNT(sIsaacSpeciesCategories))
        return ISAAC_CATEGORY_NONE;
    return sIsaacSpeciesCategories[species];
}
