// Biomes du donjon : décor (tileset de la salle, musique, météo...), lignées évolutives (espèce de base),
// types de dresseurs et boss. Une descente traverse les ISAAC_BIOME_COUNT biomes, un étage chacun, dans un
// ordre tiré au hasard. Les Pokémon d'une lignée sont pris au stade adapté (catégorie de BST), avec les sets de
// isaac_sets.h.
//
// Les 181 lignées évolutives non légendaires des 386 premiers Pokémon sont toutes réparties, chacune dans un
// seul biome (une trentaine par biome). Tous les dresseurs du biome, boss compris, piochent leurs Pokémon au
// hasard dans toutes ses lignées (voir AddTrainerMon) : le type de dresseur ne donne que son apparence.
// Grotte mystique (Chutes Météore, boss STEVEN) : mise de côté, le décor ne convenait pas.

#define ISAAC_TRAINER(class, pic, gfx, gender, music) \
    { TRAINER_CLASS_##class, TRAINER_PIC_##pic, OBJ_EVENT_GFX_##gfx, TRAINER_GENDER_##gender, TRAINER_ENCOUNTER_MUSIC_##music }

// --- Grotte (Route Victoire) : Roche, Sol, Combat, Acier, Dragon, et les habitants des grottes -----------
static const u16 sCaveLines[] =
{
    SPECIES_GEODUDE, SPECIES_ONIX, SPECIES_DIGLETT, SPECIES_MACHOP, SPECIES_MANKEY, SPECIES_CUBONE,
    SPECIES_RHYHORN, SPECIES_LARVITAR, SPECIES_WHISMUR, SPECIES_MAKUHITA, SPECIES_ARON, SPECIES_MEDITITE,
    SPECIES_BAGON, SPECIES_SUDOWOODO, SPECIES_NOSEPASS, SPECIES_PHANPY, SPECIES_TYROGUE, SPECIES_GLIGAR,
    SPECIES_SNORLAX, SPECIES_KANGASKHAN, SPECIES_DUNSPARCE, SPECIES_AERODACTYL, SPECIES_SHUCKLE, SPECIES_PARAS,
    SPECIES_CLEFFA, SPECIES_ZUBAT, SPECIES_BELDUM, SPECIES_LUNATONE, SPECIES_SOLROCK, SPECIES_BALTOY,
};
static const struct IsaacTrainerType sCaveTrainers[] =
{
    ISAAC_TRAINER(HIKER,        HIKER,        HIKER,      MALE,   HIKER),
    ISAAC_TRAINER(RUIN_MANIAC,  RUIN_MANIAC,  HIKER,      MALE,   HIKER),
    ISAAC_TRAINER(BLACK_BELT,   BLACK_BELT,   BLACK_BELT, MALE,   INTENSE),
    ISAAC_TRAINER(BATTLE_GIRL,  BATTLE_GIRL,  GIRL_3,     FEMALE, INTENSE),
    ISAAC_TRAINER(POKEMANIAC,   POKEMANIAC,   MANIAC,     MALE,   SUSPICIOUS),
    ISAAC_TRAINER(DRAGON_TAMER, DRAGON_TAMER, MAN_3,      MALE,   INTENSE),
};
static const struct IsaacTrainerType sCaveBoss = ISAAC_TRAINER(LEADER, LEADER_BROCK_FRLG, BROCK, MALE, INTENSE);

// --- Grotte obscure (Route Victoire dans le noir) : Spectre, Ténèbres, Psy, et les Pokémon de la nuit ----
static const u16 sDarkLines[] =
{
    SPECIES_GASTLY, SPECIES_MISDREAVUS, SPECIES_SHUPPET, SPECIES_DUSKULL, SPECIES_SABLEYE, SPECIES_MURKROW,
    SPECIES_POOCHYENA, SPECIES_HOOTHOOT, SPECIES_NINCADA, SPECIES_RATTATA, SPECIES_EEVEE, SPECIES_ABRA,
    SPECIES_DROWZEE, SPECIES_NATU, SPECIES_RALTS, SPECIES_WYNAUT, SPECIES_GIRAFARIG, SPECIES_UNOWN,
    SPECIES_KECLEON, SPECIES_MEOWTH, SPECIES_SPINDA, SPECIES_IGGLYBUFF, SPECIES_MAWILE, SPECIES_DITTO,
    SPECIES_PORYGON, SPECIES_SNUBBULL, SPECIES_SMEARGLE, SPECIES_VOLBEAT, SPECIES_ILLUMISE, SPECIES_VENONAT,
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

// --- Grotte de feu (Chemin Ardent) : Feu, Électrik, désert et plaines brûlantes --------------------------
static const u16 sFireLines[] =
{
    SPECIES_CHARMANDER, SPECIES_VULPIX, SPECIES_GROWLITHE, SPECIES_PONYTA, SPECIES_MAGBY, SPECIES_SLUGMA,
    SPECIES_NUMEL, SPECIES_TORKOAL, SPECIES_HOUNDOUR, SPECIES_CYNDAQUIL, SPECIES_TORCHIC, SPECIES_KOFFING,
    SPECIES_GRIMER, SPECIES_SPOINK, SPECIES_PICHU, SPECIES_MAREEP, SPECIES_ELEKID, SPECIES_ELECTRIKE,
    SPECIES_PLUSLE, SPECIES_MINUN, SPECIES_VOLTORB, SPECIES_MAGNEMITE, SPECIES_TRAPINCH, SPECIES_CACNEA,
    SPECIES_SANDSHREW, SPECIES_TAUROS, SPECIES_DODUO, SPECIES_ZANGOOSE, SPECIES_SEVIPER, SPECIES_MILTANK,
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

// --- Grotte gelée (Grotte Tréfonds) : Glace, Eau froide, et la montagne enneigée (oiseaux, bêtes à fourrure) 
static const u16 sIceLines[] =
{
    SPECIES_SEEL, SPECIES_SHELLDER, SPECIES_SWINUB, SPECIES_SNORUNT, SPECIES_SPHEAL, SPECIES_SMOOCHUM,
    SPECIES_SNEASEL, SPECIES_DELIBIRD, SPECIES_LAPRAS, SPECIES_PSYDUCK, SPECIES_POLIWAG, SPECIES_WOOPER,
    SPECIES_SLOWPOKE, SPECIES_STANTLER, SPECIES_SWABLU, SPECIES_DRATINI, SPECIES_CASTFORM, SPECIES_TEDDIURSA,
    SPECIES_MR_MIME, SPECIES_SKARMORY, SPECIES_ABSOL, SPECIES_CHIMECHO, SPECIES_PIDGEY, SPECIES_SPEAROW,
    SPECIES_TAILLOW, SPECIES_FARFETCHD, SPECIES_CHANSEY, SPECIES_LICKITUNG, SPECIES_SENTRET, SPECIES_ZIGZAGOON,
};
static const struct IsaacTrainerType sIceTrainers[] =
{
    ISAAC_TRAINER(COOLTRAINER, COOLTRAINER_M, MAN_3,    MALE,   COOL),
    ISAAC_TRAINER(COOLTRAINER, COOLTRAINER_F, WOMAN_5,  FEMALE, COOL),
    ISAAC_TRAINER(EXPERT,      EXPERT_M,      EXPERT_M, MALE,   INTENSE),
    ISAAC_TRAINER(EXPERT,      EXPERT_F,      EXPERT_F, FEMALE, INTENSE),
    ISAAC_TRAINER(HIKER,       HIKER,         HIKER,    MALE,   HIKER),
    ISAAC_TRAINER(BIRD_KEEPER, BIRD_KEEPER,   MAN_5,    MALE,   COOL),
};
static const struct IsaacTrainerType sIceBoss = ISAAC_TRAINER(ELITE_FOUR, ELITE_FOUR_GLACIA, GLACIA, FEMALE, ELITE_FOUR);

// --- Grotte poison (Tunnel Mérazon, moussu) : Poison, Plante, Insecte, et les Pokémon des bois -----------
static const u16 sPoisonLines[] =
{
    SPECIES_BULBASAUR, SPECIES_ODDISH, SPECIES_BELLSPROUT, SPECIES_EKANS, SPECIES_NIDORAN_F, SPECIES_NIDORAN_M,
    SPECIES_SPINARAK, SPECIES_GULPIN, SPECIES_ROSELIA, SPECIES_SHROOMISH, SPECIES_SEEDOT, SPECIES_TREECKO,
    SPECIES_CHIKORITA, SPECIES_WEEDLE, SPECIES_CATERPIE, SPECIES_WURMPLE, SPECIES_LEDYBA, SPECIES_YANMA,
    SPECIES_PINECO, SPECIES_SCYTHER, SPECIES_PINSIR, SPECIES_HERACROSS, SPECIES_HOPPIP, SPECIES_SUNKERN,
    SPECIES_EXEGGCUTE, SPECIES_TANGELA, SPECIES_TROPIUS, SPECIES_TOGEPI, SPECIES_AIPOM, SPECIES_SKITTY,
    SPECIES_SLAKOTH,
};
static const struct IsaacTrainerType sPoisonTrainers[] =
{
    ISAAC_TRAINER(AROMA_LADY,  AROMA_LADY,       WOMAN_2,     FEMALE, FEMALE),
    ISAAC_TRAINER(BUG_MANIAC,  BUG_MANIAC,       MANIAC,      MALE,   SUSPICIOUS),
    ISAAC_TRAINER(BUG_CATCHER, BUG_CATCHER,      BUG_CATCHER, MALE,   MALE),
    ISAAC_TRAINER(NINJA_BOY,   NINJA_BOY,        NINJA_BOY,   MALE,   SUSPICIOUS),
    ISAAC_TRAINER(PKMN_RANGER, POKEMON_RANGER_M, CAMPER,      MALE,   COOL),
    ISAAC_TRAINER(PKMN_RANGER, POKEMON_RANGER_F, PICNICKER,   FEMALE, COOL),
};
static const struct IsaacTrainerType sPoisonBoss = ISAAC_TRAINER(LEADER, LEADER_ERIKA_FRLG, ERIKA, FEMALE, FEMALE);

// --- Grotte sous-marine (fonds marins d'Émeraude) : Eau, fossiles marins ---------------------------------
static const u16 sOceanLines[] =
{
    SPECIES_TENTACOOL, SPECIES_HORSEA, SPECIES_GOLDEEN, SPECIES_MAGIKARP, SPECIES_CHINCHOU, SPECIES_QWILFISH,
    SPECIES_REMORAID, SPECIES_MANTINE, SPECIES_CARVANHA, SPECIES_WAILMER, SPECIES_CORSOLA, SPECIES_RELICANTH,
    SPECIES_LUVDISC, SPECIES_OMANYTE, SPECIES_KABUTO, SPECIES_ANORITH, SPECIES_LILEEP, SPECIES_STARYU,
    SPECIES_BARBOACH, SPECIES_CORPHISH, SPECIES_FEEBAS, SPECIES_CLAMPERL, SPECIES_KRABBY, SPECIES_AZURILL,
    SPECIES_MUDKIP, SPECIES_SQUIRTLE, SPECIES_LOTAD, SPECIES_SURSKIT, SPECIES_WINGULL, SPECIES_TOTODILE,
};
// Que des nageurs dans l'océan (le joueur surfe)
static const struct IsaacTrainerType sOceanTrainers[] =
{
    ISAAC_TRAINER(SWIMMER_M, SWIMMER_M, SWIMMER_M, MALE,   SWIMMER),
    ISAAC_TRAINER(SWIMMER_F, SWIMMER_F, SWIMMER_F, FEMALE, SWIMMER),
};
// Boss : Ondine (image de combat de RFVF), en nageuse sur la carte
static const struct IsaacTrainerType sOceanBoss = ISAAC_TRAINER(LEADER, LEADER_MISTY_FRLG, SWIMMER_F, FEMALE, FEMALE);

#undef ISAAC_TRAINER

#define ISAAC_BIOME(prefix, layout, mus, wea, isDark, isOcean, name) \
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
        .ocean = isOcean,                                            \
    }

const struct IsaacBiome gIsaacBiomes[ISAAC_BIOME_COUNT] =
{
    [ISAAC_BIOME_CAVE]       = ISAAC_BIOME(Cave,       ISAAC_ROOM,            VICTORY_ROAD,    NONE,               FALSE, FALSE, "BROCK"),
    [ISAAC_BIOME_DARK]       = ISAAC_BIOME(Dark,       ISAAC_ROOM,            MT_PYRE,         FOG_DIAGONAL,       FALSE, FALSE, "PHOEBE"),
    [ISAAC_BIOME_FIRE]       = ISAAC_BIOME(Fire,       ISAAC_ROOM_FIRE,       MT_CHIMNEY,      NONE,               FALSE, FALSE, "FLANNERY"),
    [ISAAC_BIOME_ICE]        = ISAAC_BIOME(Ice,        ISAAC_ROOM_ICE,        SEALED_CHAMBER,  NONE,               FALSE, FALSE, "GLACIA"),
    [ISAAC_BIOME_POISON]     = ISAAC_BIOME(Poison,     ISAAC_ROOM_POISON,     PETALBURG_WOODS, FOG_HORIZONTAL,     FALSE, FALSE, "ERIKA"),
    [ISAAC_BIOME_OCEAN]      = ISAAC_BIOME(Ocean,      ISAAC_ROOM_OCEAN,      SURF,            NONE,               FALSE, TRUE,  "MISTY"),
};

#undef ISAAC_BIOME
