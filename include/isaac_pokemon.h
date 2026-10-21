#ifndef GUARD_ISAAC_POKEMON_H
#define GUARD_ISAAC_POKEMON_H

#include "data.h"

#define ISAAC_MON_LEVEL  50  // niveau de tous les Pokémon du jeu
#define ISAAC_SET_TIERS  3  // sets par Pokémon, du plus faible (0) au plus fort (ISAAC_SET_TIERS - 1)

// Catégorie d'un Pokémon selon son BST (tools/isaac/build_categories.py) : quartiles des non légendaires,
// légendaires à part
enum IsaacCategory
{
    ISAAC_CATEGORY_NONE,
    ISAAC_CATEGORY_1,                // BST <= 310 (Chenipan, Rattata, Magicarpe...)
    ISAAC_CATEGORY_2,                // BST <= 405 (Herbizarre, Pikachu, Machopeur...)
    ISAAC_CATEGORY_3,                // BST <= 479 (Raichu, Golbat, Galegon...)
    ISAAC_CATEGORY_4,                // au-delà (Florizarre, Tyranocif, Drattak...)
    ISAAC_CATEGORY_LEGENDARY_SMALL,  // légendaires de BST < 620 (Électhor, Suicune, Mew...)
    ISAAC_CATEGORY_LEGENDARY_BIG,    // légendaires de BST >= 620 (Mewtwo, Lugia, Rayquaza...)
};

enum IsaacBiomeId
{
    ISAAC_BIOME_CAVE,
    ISAAC_BIOME_COUNT,
};

// Type de dresseur d'un biome et lignées (espèce de base) qu'il utilise
struct IsaacTrainerType
{
    u8 trainerClass;
    enum TrainerPicID trainerPic;
    u16 objEventGfx;
    u8 gender;          // TRAINER_GENDER_MALE / FEMALE
    u8 encounterMusic;  // TRAINER_ENCOUNTER_MUSIC_*
    const u16 *lines;
    u8 lineCount;
};

struct IsaacBiome
{
    const u16 *lines;  // lignées évolutives du biome (espèce de base)
    u8 lineCount;
    const struct IsaacTrainerType *trainers;
    u8 trainerCount;
};

extern const struct IsaacBiome gIsaacBiomes[ISAAC_BIOME_COUNT];

const struct TrainerMon *IsaacGetSet(enum Species species, u32 tier);
enum IsaacCategory IsaacGetSpeciesCategory(enum Species species);

#endif // GUARD_ISAAC_POKEMON_H
