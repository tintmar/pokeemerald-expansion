// Biomes du donjon : lignées évolutives (espèce de base) et types de dresseurs de chaque biome.
// Les Pokémon d'une lignée sont pris au stade adapté à la profondeur, avec les sets de isaac_sets.h.

// --- Grotte (tuiles de la Route Victoire) ------------------------------------------------------
// Roche, Sol, Combat, Poison, Dragon : les grottes classiques (Mont Sélénite, Grotte Granite,
// Chutes Météore, Route Victoire)
static const u16 sCaveLines[] =
{
    SPECIES_ZUBAT,      // Nosferapti, Nosferalto, Nostenfer
    SPECIES_GEODUDE,    // Racaillou, Gravalanch, Grolem
    SPECIES_ONIX,       // Onix, Steelix
    SPECIES_DIGLETT,    // Taupiqueur, Triopikeur
    SPECIES_MACHOP,     // Machoc, Machopeur, Mackogneur
    SPECIES_PARAS,      // Paras, Parasect
    SPECIES_CLEFFA,     // Mélo, Mélofée, Mélodelfe
    SPECIES_CUBONE,     // Osselait, Ossatueur
    SPECIES_RHYHORN,    // Rhinocorne, Rhinoféros
    SPECIES_LARVITAR,   // Embrylex, Ymphect, Tyranocif
    SPECIES_WHISMUR,    // Chuchmur, Ramboum, Brouhabam
    SPECIES_MAKUHITA,   // Makuhita, Hariyama
    SPECIES_ARON,       // Galekid, Galegon, Galeking
    SPECIES_MEDITITE,   // Méditikka, Charmina
    SPECIES_BAGON,      // Draby, Drackhaus, Drattak
};

static const u16 sCaveHikerLines[] = { SPECIES_GEODUDE, SPECIES_ONIX, SPECIES_DIGLETT, SPECIES_RHYHORN, SPECIES_ARON, SPECIES_ZUBAT };
static const u16 sCaveRuinManiacLines[] = { SPECIES_CUBONE, SPECIES_LARVITAR, SPECIES_ARON, SPECIES_ONIX, SPECIES_GEODUDE };
static const u16 sCaveBlackBeltLines[] = { SPECIES_MACHOP, SPECIES_MAKUHITA, SPECIES_MEDITITE };
static const u16 sCaveBattleGirlLines[] = { SPECIES_MEDITITE, SPECIES_MAKUHITA, SPECIES_MACHOP };
static const u16 sCavePokemaniacLines[] = { SPECIES_CUBONE, SPECIES_RHYHORN, SPECIES_CLEFFA, SPECIES_PARAS, SPECIES_WHISMUR };
static const u16 sCaveDragonTamerLines[] = { SPECIES_BAGON, SPECIES_LARVITAR, SPECIES_ONIX };

#define ISAAC_TRAINER(class, pic, gfx, gender, music, lines) \
    { TRAINER_CLASS_##class, TRAINER_PIC_##pic, OBJ_EVENT_GFX_##gfx, TRAINER_GENDER_##gender, TRAINER_ENCOUNTER_MUSIC_##music, lines, ARRAY_COUNT(lines) }

static const struct IsaacTrainerType sCaveTrainers[] =
{
    ISAAC_TRAINER(HIKER,        HIKER,        HIKER,      MALE,   HIKER,      sCaveHikerLines),
    ISAAC_TRAINER(RUIN_MANIAC,  RUIN_MANIAC,  MAN_5,      MALE,   HIKER,      sCaveRuinManiacLines),
    ISAAC_TRAINER(BLACK_BELT,   BLACK_BELT,   BLACK_BELT, MALE,   INTENSE,    sCaveBlackBeltLines),
    ISAAC_TRAINER(BATTLE_GIRL,  BATTLE_GIRL,  GIRL_3,     FEMALE, INTENSE,    sCaveBattleGirlLines),
    ISAAC_TRAINER(POKEMANIAC,   POKEMANIAC,   MANIAC,     MALE,   SUSPICIOUS, sCavePokemaniacLines),
    ISAAC_TRAINER(DRAGON_TAMER, DRAGON_TAMER, MAN_3,      MALE,   INTENSE,    sCaveDragonTamerLines),
};

#undef ISAAC_TRAINER

const struct IsaacBiome gIsaacBiomes[ISAAC_BIOME_COUNT] =
{
    [ISAAC_BIOME_CAVE] =
    {
        .lines = sCaveLines,
        .lineCount = ARRAY_COUNT(sCaveLines),
        .trainers = sCaveTrainers,
        .trainerCount = ARRAY_COUNT(sCaveTrainers),
    },
};
