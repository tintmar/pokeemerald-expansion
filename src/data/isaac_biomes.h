// Biomes du donjon : décor (tileset de la salle, musique, météo...), lignées évolutives (espèce de base),
// types de dresseurs et boss. Chaque biome a deux types principaux (l'Océan n'a que l'Eau) ; les lignées de type
// Normal sont réparties selon leur thème. Une descente traverse ISAAC_FLOOR_COUNT biomes tirés au hasard, un étage
// chacun. Les Pokémon d'une lignée sont pris au stade adapté (catégorie de BST), avec les sets de
// isaac_sets.h.
//
// Toutes les lignées du roster (tools/isaac/roster.py) sont réparties, chacune dans un seul biome. Les formes
// régionales sont des lignées à part, sauf celles « à poids partagé » (SHARED_FORMS de roster.py), tirées avec leur
// espèce d'origine. Tous les dresseurs du biome, boss compris, piochent leurs Pokémon au
// hasard dans toutes ses lignées (voir AddTrainerMon) : le type de dresseur ne donne que son apparence.

#define ISAAC_TRAINER(class, pic, gfx, gender, music) \
    { TRAINER_CLASS_##class, TRAINER_PIC_##pic, OBJ_EVENT_GFX_##gfx, TRAINER_GENDER_##gender, TRAINER_ENCOUNTER_MUSIC_##music }

// --- Grotte (Route Victoire) : Roche, Combat, et les habitants des grottes --------------------------------
static const u16 sCaveLines[] =
{
    SPECIES_GEODUDE, SPECIES_ONIX, SPECIES_MACHOP, SPECIES_MANKEY, SPECIES_MAKUHITA, SPECIES_MEDITITE,
    SPECIES_TYROGUE, SPECIES_BONSLY, SPECIES_NOSEPASS, SPECIES_SHUCKLE, SPECIES_LARVITAR, SPECIES_LUNATONE,
    SPECIES_SOLROCK, SPECIES_LILEEP, SPECIES_ANORITH, SPECIES_WHISMUR, SPECIES_DUNSPARCE, SPECIES_KANGASKHAN,
    SPECIES_RIOLU, SPECIES_TIMBURR, SPECIES_ROGGENROLA, SPECIES_ROCKRUFF, SPECIES_NACLI, SPECIES_FARFETCHD_GALAR,
    SPECIES_SNEASEL_HISUI, SPECIES_TAUROS_PALDEA_COMBAT,
};
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
static const u16 sDarkLines[] =
{
    SPECIES_GASTLY, SPECIES_MISDREAVUS, SPECIES_SHUPPET, SPECIES_DUSKULL, SPECIES_SABLEYE, SPECIES_MURKROW,
    SPECIES_POOCHYENA, SPECIES_NINCADA, SPECIES_ABSOL, SPECIES_EEVEE, SPECIES_RATTATA, SPECIES_KECLEON,
    SPECIES_DITTO, SPECIES_PORYGON, SPECIES_SMEARGLE, SPECIES_SPINDA, SPECIES_LITWICK, SPECIES_ZORUA,
    SPECIES_PAWNIARD, SPECIES_DRIFLOON, SPECIES_SINISTEA_PHONY, SPECIES_DEINO, SPECIES_MEOWTH_ALOLA, SPECIES_CORSOLA_GALAR,
    SPECIES_ZIGZAGOON_GALAR,
};
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
static const u16 sFireLines[] =
{
    SPECIES_CHARMANDER, SPECIES_VULPIX, SPECIES_GROWLITHE, SPECIES_PONYTA, SPECIES_MAGBY, SPECIES_SLUGMA,
    SPECIES_NUMEL, SPECIES_TORKOAL, SPECIES_HOUNDOUR, SPECIES_CYNDAQUIL, SPECIES_TORCHIC, SPECIES_PICHU,
    SPECIES_MAREEP, SPECIES_ELEKID, SPECIES_ELECTRIKE, SPECIES_PLUSLE, SPECIES_MINUN, SPECIES_VOLTORB,
    SPECIES_MAGNEMITE, SPECIES_SHINX, SPECIES_TYNAMO, SPECIES_CHARCADET, SPECIES_MAROWAK_ALOLA, SPECIES_TAUROS_PALDEA_BLAZE,
};
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
static const u16 sIceLines[] =
{
    SPECIES_SEEL, SPECIES_SHELLDER, SPECIES_SWINUB, SPECIES_SNORUNT, SPECIES_SPHEAL, SPECIES_SMOOCHUM,
    SPECIES_DELIBIRD, SPECIES_LAPRAS, SPECIES_ARON, SPECIES_BELDUM, SPECIES_MAWILE, SPECIES_TEDDIURSA,
    SPECIES_CASTFORM, SPECIES_STANTLER, SPECIES_MUNCHLAX, SPECIES_SNEASEL, SPECIES_HAPPINY, SPECIES_SNOVER,
    SPECIES_CUBCHOO, SPECIES_VANILLITE, SPECIES_BERGMITE, SPECIES_BRONZOR, SPECIES_KLINK, SPECIES_FRIGIBAX,
    SPECIES_AMAURA, SPECIES_VULPIX_ALOLA, SPECIES_SANDSHREW_ALOLA, SPECIES_MEOWTH_GALAR, SPECIES_MR_MIME_GALAR,
};
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
static const u16 sPoisonLines[] =
{
    SPECIES_EKANS, SPECIES_NIDORAN_F, SPECIES_NIDORAN_M, SPECIES_SPINARAK, SPECIES_GULPIN, SPECIES_WEEDLE,
    SPECIES_CATERPIE, SPECIES_WURMPLE, SPECIES_LEDYBA, SPECIES_YANMA, SPECIES_PINECO, SPECIES_SCYTHER,
    SPECIES_PINSIR, SPECIES_HERACROSS, SPECIES_KOFFING, SPECIES_GRIMER, SPECIES_SEVIPER, SPECIES_VENONAT,
    SPECIES_VOLBEAT, SPECIES_ILLUMISE, SPECIES_PARAS, SPECIES_SKORUPI, SPECIES_CROAGUNK, SPECIES_VENIPEDE,
    SPECIES_LARVESTA, SPECIES_QWILFISH_HISUI, SPECIES_WOOPER_PALDEA,
};
static const struct IsaacTrainerType sPoisonTrainers[] =
{
    ISAAC_TRAINER(BUG_MANIAC,  BUG_MANIAC,       MANIAC,      MALE,   SUSPICIOUS),
    ISAAC_TRAINER(BUG_CATCHER, BUG_CATCHER,      BUG_CATCHER, MALE,   MALE),
    ISAAC_TRAINER(NINJA_BOY,   NINJA_BOY,        NINJA_BOY,   MALE,   SUSPICIOUS),
    ISAAC_TRAINER(HEX_MANIAC,  HEX_MANIAC,       HEX_MANIAC,  FEMALE, SUSPICIOUS),
    ISAAC_TRAINER(PKMN_RANGER, POKEMON_RANGER_M, CAMPER,      MALE,   COOL),
};
static const struct IsaacTrainerType sPoisonBoss = ISAAC_TRAINER(LEADER, LEADER_KOGA_FRLG, KOGA, MALE, SUSPICIOUS);

// --- Océan : Eau, et rien d'autre (toutes les lignées qui ont le type Eau, sauf les Glace) ---------------
static const u16 sOceanLines[] =
{
    SPECIES_TENTACOOL, SPECIES_HORSEA, SPECIES_GOLDEEN, SPECIES_MAGIKARP, SPECIES_CHINCHOU, SPECIES_QWILFISH,
    SPECIES_REMORAID, SPECIES_MANTYKE, SPECIES_CARVANHA, SPECIES_WAILMER, SPECIES_CORSOLA, SPECIES_RELICANTH,
    SPECIES_LUVDISC, SPECIES_OMANYTE, SPECIES_KABUTO, SPECIES_STARYU, SPECIES_BARBOACH, SPECIES_CORPHISH,
    SPECIES_FEEBAS, SPECIES_CLAMPERL, SPECIES_KRABBY, SPECIES_AZURILL, SPECIES_MUDKIP, SPECIES_SQUIRTLE,
    SPECIES_LOTAD, SPECIES_SURSKIT, SPECIES_WINGULL, SPECIES_TOTODILE, SPECIES_PSYDUCK, SPECIES_POLIWAG,
    SPECIES_WOOPER, SPECIES_SLOWPOKE, SPECIES_TAUROS_PALDEA_AQUA,
};
// Que des nageurs dans l'océan (le joueur surfe)
static const struct IsaacTrainerType sOceanTrainers[] =
{
    ISAAC_TRAINER(SWIMMER_M, SWIMMER_M, SWIMMER_M, MALE,   SWIMMER),
    ISAAC_TRAINER(SWIMMER_F, SWIMMER_F, SWIMMER_F, FEMALE, SWIMMER),
};
// Boss : Ondine (image de combat de RFVF), en nageuse sur la carte
static const struct IsaacTrainerType sOceanBoss = ISAAC_TRAINER(LEADER, LEADER_MISTY_FRLG, SWIMMER_F, FEMALE, FEMALE);

// --- Désert (Route 111) : Sol, Psy (ruines du désert), et les bêtes des plaines arides ------------------
static const u16 sDesertLines[] =
{
    SPECIES_DIGLETT, SPECIES_CUBONE, SPECIES_PHANPY, SPECIES_SANDSHREW, SPECIES_TRAPINCH, SPECIES_BALTOY,
    SPECIES_CACNEA, SPECIES_ABRA, SPECIES_DROWZEE, SPECIES_NATU, SPECIES_WYNAUT, SPECIES_GIRAFARIG,
    SPECIES_UNOWN, SPECIES_SPOINK, SPECIES_CHINGLING, SPECIES_MEOWTH, SPECIES_ZANGOOSE, SPECIES_RHYHORN,
    SPECIES_LICKITUNG, SPECIES_HIPPOPOTAS, SPECIES_SANDILE, SPECIES_GIBLE, SPECIES_DRILBUR, SPECIES_SILICOBRA,
    SPECIES_SLOWPOKE_GALAR,
};

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
static const u16 sCliffLines[] =
{
    SPECIES_BAGON, SPECIES_DRATINI, SPECIES_SWABLU, SPECIES_PIDGEY, SPECIES_SPEAROW, SPECIES_TAILLOW,
    SPECIES_FARFETCHD, SPECIES_DODUO, SPECIES_HOOTHOOT, SPECIES_SKARMORY, SPECIES_ZUBAT, SPECIES_AERODACTYL,
    SPECIES_GLIGAR, SPECIES_MILTANK, SPECIES_TAUROS, SPECIES_SENTRET, SPECIES_ZIGZAGOON, SPECIES_STARLY,
    SPECIES_AXEW, SPECIES_NOIBAT, SPECIES_ROOKIDEE, SPECIES_JANGMO_O,
};

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
static const u16 sForestLines[] =
{
    SPECIES_BULBASAUR, SPECIES_ODDISH, SPECIES_BELLSPROUT, SPECIES_BUDEW, SPECIES_SHROOMISH, SPECIES_SEEDOT,
    SPECIES_TREECKO, SPECIES_CHIKORITA, SPECIES_HOPPIP, SPECIES_SUNKERN, SPECIES_EXEGGCUTE, SPECIES_TANGELA,
    SPECIES_TROPIUS, SPECIES_TOGEPI, SPECIES_CLEFFA, SPECIES_IGGLYBUFF, SPECIES_SNUBBULL, SPECIES_RALTS,
    SPECIES_MIME_JR, SPECIES_AIPOM, SPECIES_SKITTY, SPECIES_SLAKOTH, SPECIES_FLABEBE_RED, SPECIES_MORELULL,
    SPECIES_PHANTUMP, SPECIES_HATENNA, SPECIES_PONYTA_GALAR,
};

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
        .lines = s##prefix##Lines,                                        \
        .lineCount = ARRAY_COUNT(s##prefix##Lines),                       \
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
