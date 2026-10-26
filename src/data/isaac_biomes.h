// Biomes du donjon : décor (tileset de la salle, musique, météo...), espèces, types de dresseurs et boss. Chaque
// biome a deux types principaux (l'Océan n'a que l'Eau). Une descente traverse ISAAC_FLOOR_COUNT biomes tirés au
// hasard, un étage chacun.
//
// Les espèces de chaque biome (toutes, évolutions comprises) sont dans isaac_biome_species.h, généré d'après
// tools/isaac/biome_roster.py : chaque espèce du roster est dans un seul biome. Tous les dresseurs du biome, boss
// compris, piochent leurs Pokémon au hasard dans toutes ses espèces de la catégorie voulue (voir AddTrainerMon) : le
// type de dresseur ne donne que son apparence. Ils ont les sets de isaac_sets.h.

#include "isaac_biome_species.h"

#define ISAAC_TRAINER(class, pic, gfx, gender, music) \
    { TRAINER_CLASS_##class, TRAINER_PIC_##pic, OBJ_EVENT_GFX_##gfx, TRAINER_GENDER_##gender, TRAINER_ENCOUNTER_MUSIC_##music }

// --- Grotte (Route Victoire) : Roche, Combat, et les habitants des grottes --------------------------------
static const struct IsaacTrainerType sCaveTrainers[] =
{
    ISAAC_TRAINER(HIKER,        HIKER,        HIKER,      MALE,   HIKER),
    ISAAC_TRAINER(RUIN_MANIAC,  RUIN_MANIAC,  HIKER,      MALE,   HIKER),
    ISAAC_TRAINER(BLACK_BELT,   BLACK_BELT,   BLACK_BELT, MALE,   INTENSE),
    ISAAC_TRAINER(BATTLE_GIRL,  BATTLE_GIRL,  GIRL_3,     FEMALE, INTENSE),
    ISAAC_TRAINER(POKEMANIAC,   POKEMANIAC,   MANIAC,     MALE,   SUSPICIOUS),
};
static const struct IsaacTrainerType sCaveBoss = ISAAC_TRAINER(LEADER, LEADER_BROCK_FRLG, BROCK, MALE, INTENSE);

// --- Grotte obscure (Route Victoire dans le noir) : Spectre, Ténèbres, et les Pokémon de la nuit --------
static const struct IsaacTrainerType sDarkTrainers[] =
{
    ISAAC_TRAINER(HEX_MANIAC, HEX_MANIAC,    HEX_MANIAC,     FEMALE, SUSPICIOUS),
    ISAAC_TRAINER(PSYCHIC,    PSYCHIC_M,     PSYCHIC_M,      MALE,   INTENSE),
    ISAAC_TRAINER(PSYCHIC,    PSYCHIC_F,     LASS,           FEMALE, INTENSE),
    ISAAC_TRAINER(TEAM_MAGMA, MAGMA_GRUNT_M, MAGMA_MEMBER_M, MALE,   MAGMA),
    ISAAC_TRAINER(TEAM_MAGMA, MAGMA_GRUNT_F, MAGMA_MEMBER_F, FEMALE, MAGMA),
    ISAAC_TRAINER(NINJA_BOY,  NINJA_BOY,     NINJA_BOY,      MALE,   SUSPICIOUS),
};
static const struct IsaacTrainerType sDarkBoss = ISAAC_TRAINER(ELITE_FOUR, ELITE_FOUR_PHOEBE, PHOEBE, FEMALE, ELITE_FOUR);

// --- Grotte de feu (Chemin Ardent) : Feu, Électrik -------------------------------------------------------
static const struct IsaacTrainerType sFireTrainers[] =
{
    ISAAC_TRAINER(KINDLER,    KINDLER,       MAN_5,          MALE,   HIKER),
    ISAAC_TRAINER(HIKER,      HIKER,         HIKER,          MALE,   HIKER),
    ISAAC_TRAINER(CAMPER,     CAMPER,        CAMPER,         MALE,   MALE),
    ISAAC_TRAINER(PICNICKER,  PICNICKER,     PICNICKER,      FEMALE, GIRL),
    ISAAC_TRAINER(GUITARIST,  GUITARIST,     MAN_5,          MALE,   INTENSE),
    ISAAC_TRAINER(TEAM_MAGMA, MAGMA_GRUNT_M, MAGMA_MEMBER_M, MALE,   MAGMA),
};
static const struct IsaacTrainerType sFireBoss = ISAAC_TRAINER(LEADER, LEADER_FLANNERY, FLANNERY, FEMALE, FEMALE);

// --- Grotte gelée (Grotte Tréfonds) : Glace, Acier, et les bêtes des neiges --------------------------------
static const struct IsaacTrainerType sIceTrainers[] =
{
    ISAAC_TRAINER(COOLTRAINER, COOLTRAINER_M, MAN_3,    MALE,   COOL),
    ISAAC_TRAINER(COOLTRAINER, COOLTRAINER_F, WOMAN_5,  FEMALE, COOL),
    ISAAC_TRAINER(EXPERT,      EXPERT_M,      EXPERT_M, MALE,   INTENSE),
    ISAAC_TRAINER(EXPERT,      EXPERT_F,      EXPERT_F, FEMALE, INTENSE),
    ISAAC_TRAINER(HIKER,       HIKER,         HIKER,    MALE,   HIKER),
};
static const struct IsaacTrainerType sIceBoss = ISAAC_TRAINER(ELITE_FOUR, ELITE_FOUR_GLACIA, GLACIA, FEMALE, ELITE_FOUR);

// --- Grotte poison (Tunnel Mérazon, moussu) : Poison, Insecte ---------------------------------------------
static const struct IsaacTrainerType sPoisonTrainers[] =
{
    ISAAC_TRAINER(BUG_MANIAC,  BUG_MANIAC,       MANIAC,      MALE,   SUSPICIOUS),
    ISAAC_TRAINER(BUG_CATCHER, BUG_CATCHER,      BUG_CATCHER, MALE,   MALE),
    ISAAC_TRAINER(NINJA_BOY,   NINJA_BOY,        NINJA_BOY,   MALE,   SUSPICIOUS),
    ISAAC_TRAINER(HEX_MANIAC,  HEX_MANIAC,       HEX_MANIAC,  FEMALE, SUSPICIOUS),
    ISAAC_TRAINER(PKMN_RANGER, POKEMON_RANGER_M, CAMPER,      MALE,   COOL),
};
static const struct IsaacTrainerType sPoisonBoss = ISAAC_TRAINER(LEADER, LEADER_KOGA_FRLG, KOGA, MALE, SUSPICIOUS);

// --- Océan : Eau ------------------------------------------------------------------------------------------
// Que des nageurs dans l'océan (le joueur surfe)
static const struct IsaacTrainerType sOceanTrainers[] =
{
    ISAAC_TRAINER(SWIMMER_M, SWIMMER_M, SWIMMER_M, MALE,   SWIMMER),
    ISAAC_TRAINER(SWIMMER_F, SWIMMER_F, SWIMMER_F, FEMALE, SWIMMER),
};
// Boss : Ondine (image de combat de RFVF), en nageuse sur la carte
static const struct IsaacTrainerType sOceanBoss = ISAAC_TRAINER(LEADER, LEADER_MISTY_FRLG, SWIMMER_F, FEMALE, FEMALE);

// --- Désert (Route 111) : Sol, Psy (ruines du désert), et les bêtes des plaines arides ------------------

static const struct IsaacTrainerType sDesertTrainers[] =
{
    ISAAC_TRAINER(RUIN_MANIAC, RUIN_MANIAC, HIKER,     MALE,   HIKER),
    ISAAC_TRAINER(PSYCHIC,     PSYCHIC_M,   PSYCHIC_M, MALE,   INTENSE),
    ISAAC_TRAINER(PSYCHIC,     PSYCHIC_F,   LASS,      FEMALE, INTENSE),
    ISAAC_TRAINER(CAMPER,      CAMPER,      CAMPER,    MALE,   MALE),
    ISAAC_TRAINER(PICNICKER,   PICNICKER,   PICNICKER, FEMALE, GIRL),
};
static const struct IsaacTrainerType sDesertBoss = ISAAC_TRAINER(LEADER, LEADER_GIOVANNI_FRLG, GIOVANNI, MALE, INTENSE);

// --- Falaise : Vol, Dragon, et les Pokémon des hauts plateaux ----------------------------------------------

static const struct IsaacTrainerType sCliffTrainers[] =
{
    ISAAC_TRAINER(BIRD_KEEPER,  BIRD_KEEPER,   MAN_5,   MALE,   COOL),
    ISAAC_TRAINER(DRAGON_TAMER, DRAGON_TAMER,  MAN_3,   MALE,   INTENSE),
    ISAAC_TRAINER(COOLTRAINER,  COOLTRAINER_M, MAN_3,   MALE,   COOL),
    ISAAC_TRAINER(COOLTRAINER,  COOLTRAINER_F, WOMAN_5, FEMALE, COOL),
    ISAAC_TRAINER(HIKER,        HIKER,         HIKER,   MALE,   HIKER),
};
static const struct IsaacTrainerType sCliffBoss = ISAAC_TRAINER(LEADER, LEADER_WINONA, WINONA, FEMALE, FEMALE);

// --- Forêt enchantée (Bois Clémentines) : Plante, Fée, et les Pokémon des bois -----------------------------

static const struct IsaacTrainerType sForestTrainers[] =
{
    ISAAC_TRAINER(AROMA_LADY,  AROMA_LADY,       WOMAN_2,   FEMALE, FEMALE),
    ISAAC_TRAINER(LASS,        LASS,             LASS,      FEMALE, GIRL),
    ISAAC_TRAINER(BEAUTY,      BEAUTY,           BEAUTY,    FEMALE, FEMALE),
    ISAAC_TRAINER(PKMN_RANGER, POKEMON_RANGER_M, CAMPER,    MALE,   COOL),
    ISAAC_TRAINER(PKMN_RANGER, POKEMON_RANGER_F, PICNICKER, FEMALE, COOL),
};
static const struct IsaacTrainerType sForestBoss = ISAAC_TRAINER(LEADER, LEADER_ERIKA_FRLG, ERIKA, FEMALE, FEMALE);

#undef ISAAC_TRAINER

#define ISAAC_BIOME(prefix, type1, type2, layout, mus, wea, isDark, isOcean, hasTrees, isCliff, name) \
    {                                                                     \
        .lines = s##prefix##Species,                                      \
        .lineCount = ARRAY_COUNT(s##prefix##Species),                     \
        .trainers = s##prefix##Trainers,                                  \
        .trainerCount = ARRAY_COUNT(s##prefix##Trainers),                 \
        .boss = &s##prefix##Boss,                                         \
        .bossName = COMPOUND_STRING(name),                                \
        .layoutId = LAYOUT_##layout,                                      \
        .music = MUS_##mus,                                               \
        .weather = WEATHER_##wea,                                         \
        .dark = isDark,                                                   \
        .ocean = isOcean,                                                 \
        .trees = hasTrees,                                                \
        .cliff = isCliff,                                                 \
        .types = { TYPE_##type1, TYPE_##type2 },                          \
    }

const struct IsaacBiome gIsaacBiomes[ISAAC_BIOME_COUNT] =
{
    [ISAAC_BIOME_CAVE]   = ISAAC_BIOME(Cave,   ROCK,     FIGHTING, ISAAC_ROOM,        VICTORY_ROAD,       NONE,           FALSE, FALSE, FALSE, FALSE, "BROCK"),
    [ISAAC_BIOME_DARK]   = ISAAC_BIOME(Dark,   GHOST,    DARK,     ISAAC_ROOM,        MT_PYRE,            FOG_DIAGONAL,   FALSE, FALSE, FALSE, FALSE, "PHOEBE"),
    [ISAAC_BIOME_FIRE]   = ISAAC_BIOME(Fire,   FIRE,     ELECTRIC, ISAAC_ROOM_FIRE,   MT_CHIMNEY,         NONE,           FALSE, FALSE, FALSE, FALSE, "FLANNERY"),
    [ISAAC_BIOME_ICE]    = ISAAC_BIOME(Ice,    ICE,      STEEL,    ISAAC_ROOM_ICE,    SEALED_CHAMBER,     NONE,           FALSE, FALSE, FALSE, FALSE, "GLACIA"),
    [ISAAC_BIOME_POISON] = ISAAC_BIOME(Poison, POISON,   BUG,      ISAAC_ROOM_POISON, RG_VIRIDIAN_FOREST, FOG_HORIZONTAL, FALSE, FALSE, FALSE, FALSE, "KOGA"),
    [ISAAC_BIOME_OCEAN]  = ISAAC_BIOME(Ocean,  WATER,    WATER,    ISAAC_ROOM_OCEAN,  SURF,               NONE,           FALSE, TRUE,  FALSE, FALSE, "MISTY"),
    [ISAAC_BIOME_DESERT] = ISAAC_BIOME(Desert, GROUND,   PSYCHIC,  ISAAC_ROOM_DESERT, DESERT,             SANDSTORM,      FALSE, FALSE, FALSE, FALSE, "GIOVANNI"),
    [ISAAC_BIOME_CLIFF]  = ISAAC_BIOME(Cliff,  FLYING,   DRAGON,   ISAAC_ROOM_CLIFF,  ROUTE119,           NONE,           FALSE, FALSE, FALSE, TRUE,  "WINONA"),
    [ISAAC_BIOME_FOREST] = ISAAC_BIOME(Forest, GRASS,    FAIRY,    ISAAC_ROOM_FOREST, PETALBURG_WOODS,    NONE,           FALSE, FALSE, TRUE,  FALSE, "ERIKA"),
};

#undef ISAAC_BIOME
