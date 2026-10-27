#ifndef GUARD_ISAAC_POKEMON_H
#define GUARD_ISAAC_POKEMON_H

#include "data.h"

struct Pokemon;

#define ISAAC_MON_LEVEL  50  // niveau de tous les Pokémon du jeu
#define ISAAC_SET_TIERS     4  // paliers de sets par Pokémon, du plus faible (0) au plus fort (ISAAC_SET_TIERS - 1)
#define ISAAC_SET_VARIANTS  2  // variantes de chaque palier (A, B) : même force, rôles différents
#define ISAAC_STARTER_CHOICES  6  // Pokémon proposés par le Montagnard du hub (écran de l'Usine de Combat)
#define ISAAC_STARTER_PICKS    2  // Pokémon à choisir parmi eux
#define ISAAC_MOVE_TIER_SITUATIONAL  5  // tier des attaques situationnelles (src/data/isaac_move_tiers.h)
// Rôle des attaques de statut (sIsaacMoveRoles, src/data/isaac_move_tiers.h)
#define ISAAC_MOVE_ROLE_SETUP_PHYSICAL  (1 << 0)  // boost d'Attaque (ou de Vitesse)
#define ISAAC_MOVE_ROLE_SETUP_SPECIAL   (1 << 1)  // boost d'Attaque Spéciale (ou de Vitesse)
#define ISAAC_MOVE_ROLE_RECOVERY        (1 << 2)
#define ISAAC_MOVE_ROLE_PROTECTION      (1 << 3)
#define ISAAC_MOVE_ROLE_STATUS          (1 << 4)  // statut infligé (poison, brûlure, sommeil...)
#define ISAAC_MOVE_ROLE_COMBO           (1 << 5)  // inutile seule (Blabla Dodo, Dévorêve...) ou perd un tour (Giga Impact)
#define ISAAC_MOVE_ROLE_DEF_BOOST       (1 << 6)
#define ISAAC_MOVE_ROLE_OTHER           (1 << 7)
#define ISAAC_MAX_EVOLUTION_CHOICES  8  // évolutions au choix d'une Pierre Évolution (Évoli : 5)

// Catégorie d'un Pokémon (tools/isaac/build_categories.py, d'après tools/isaac/biome_roster.py) : septiles de BST des
// non légendaires du roster, légendaires à part (IsaacGetSpeciesCategory).
// ISAAC_CATEGORIES_PER_BIOME : si TRUE, les dresseurs, boss et Pokémon de la salle de capture utilisent à la place des
// catégories 1 à 7 recalculées sur les seules espèces du biome (septiles de BST du biome, voir IsaacPickLineSpecies),
// où les bornes ci-dessous ne valent plus. Si FALSE, les catégories sont les mêmes partout.
#define ISAAC_CATEGORIES_PER_BIOME  FALSE
enum IsaacCategory
{
    ISAAC_CATEGORY_NONE,
    ISAAC_CATEGORY_1,                // BST <= 288 (Chenipan, Rattata, Magicarpe...)
    ISAAC_CATEGORY_2,                // BST <= 325 (Bulbizarre, Pikachu, Machoc...)
    ISAAC_CATEGORY_3,                // BST <= 395 (Papilusion, Racaillou, Onix...)
    ISAAC_CATEGORY_4,                // BST <= 440 (Herbizarre, Machopeur, Draco...)
    ISAAC_CATEGORY_5,                // BST <= 481 (Arbok, Golbat, Roucarnage...)
    ISAAC_CATEGORY_6,                // BST <= 515 (Ectoplasma, Mackogneur, Steelix...)
    ISAAC_CATEGORY_7,                // au-delà (Florizarre, Tyranocif, Dracolosse...)
    ISAAC_CATEGORY_LEGENDARY_SMALL,  // catégorie 8 : légendaires de BST <= 600 (Électhor, Suicune, Mew...)
    ISAAC_CATEGORY_LEGENDARY_BIG,    // catégorie 9 : légendaires de BST > 600 (Mewtwo, Lugia, Rayquaza...)
};
#define ISAAC_CATEGORY_LAST_REGULAR  ISAAC_CATEGORY_7

// Puissance des Z-moves appris par la CT Z, lancés comme des attaques normales (voir CalcMoveBasePower)
#define ISAAC_Z_MOVE_POWER  160

enum IsaacBiomeId
{
    ISAAC_BIOME_CAVE,        // Route Victoire
    ISAAC_BIOME_DARK,        // Route Victoire dans le noir
    ISAAC_BIOME_FIRE,        // Chemin Ardent
    // ISAAC_BIOME_MYSTIC (Chutes Météore) : mis de côté, décor à retravailler (voir isaac_biomes.h)
    ISAAC_BIOME_ICE,         // Grotte Tréfonds gelée
    ISAAC_BIOME_POISON,      // Tunnel Mérazon
    ISAAC_BIOME_OCEAN,       // océan de Hoenn (le joueur surfe)
    ISAAC_BIOME_DESERT,      // désert de la Route 111
    ISAAC_BIOME_CLIFF,       // sommet de falaise (la salle surplombe la plaine)
    ISAAC_BIOME_FOREST,      // forêt enchantée (Bois Clémentines)
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
};

struct IsaacBiome
{
    const u16 *lines;  // espèces du biome, évolutions comprises (isaac_biome_species.h)
    u8 lineCount;
    const struct IsaacTrainerType *trainers;
    u8 trainerCount;
    const struct IsaacTrainerType *boss;  // champion de la salle du boss
    const u8 *bossName;
    u16 layoutId;     // disposition de la salle (même map, tileset du décor)
    u16 music;
    u8 weather;
    bool8 dark;       // dans le noir, comme la Grotte Granite sans Flash
    bool8 ocean;      // le joueur surfe sur toute la salle
    bool8 trees;      // murs en arbres de 2 x 2 (demi-arbres TILE_TREE_*) au lieu des rôles de falaise
    bool8 cliff;      // la salle est le haut d'une falaise : paroi sud sur deux rangées, couloirs en ponts
    u8 types[2];      // les deux types principaux du biome (deux fois le même pour l'océan)
};

extern const struct IsaacBiome gIsaacBiomes[ISAAC_BIOME_COUNT];

const struct TrainerMon *IsaacGetSet(enum Species species, u32 tier, u32 variant);
bool8 IsaacSpeciesHasSet(enum Species species);
enum IsaacCategory IsaacGetSpeciesCategory(enum Species species);
u32 IsaacGetSpeciesBST(enum Species species);
u32 IsaacGetRosterCount(void);
enum Species IsaacGetSharedFormBase(enum Species species);
enum Species IsaacRollSharedForm(enum Species species, u32 roll);
enum Species IsaacGetRosterSpecies(u32 index);
enum Species IsaacGetEvolution(enum Species species, u32 choice);
enum Species IsaacPickLineSpecies(const u16 *lines, u32 lineCount, enum IsaacCategory category, u32 minBst, u32 roll);
enum IsaacCategory IsaacGetLinesCategory(const u16 *lines, u32 lineCount, enum Species species);
u32 IsaacAddLinesSpeciesOfCategory(const u16 *lines, u32 lineCount, enum IsaacCategory category, u16 *out, u32 count, u32 max);
void IsaacGiveLegendaryTestTeam(void);
void IsaacResetProgressionPity(void);
void IsaacCreateMonFromSet(struct Pokemon *mon, enum Species species, u32 tier, u32 variant, u32 seed);
u32 IsaacGetMonSetTier(struct Pokemon *mon);
u32 IsaacGetMonSetVariant(struct Pokemon *mon);
void IsaacPrepareEvolutionMoves(struct Pokemon *mon, enum Species target);
u16 IsaacTryLearningEvolutionMove(struct Pokemon *mon, bool8 firstMove);
void IsaacCreateStarterChoice(struct Pokemon *mon, u32 index);
s32 IsaacApplyStatBoosts(struct Pokemon *mon, u32 stat, s32 value);
bool8 IsaacAddStatBoost(struct Pokemon *mon, u32 stat);
bool8 IsaacUseBrilliantStone(struct Pokemon *mon, u32 *stat);
u32 IsaacGetEvolutionChoices(enum Species species, bool32 toFinalStage, u16 *out, u32 max);
u16 IsaacPickSpecialItem(u32 roll, bool8 shop);
bool8 IsaacAddOmniboost(struct Pokemon *mon);
bool8 IsaacHasBoost(struct Pokemon *mon);
bool8 IsaacUpgradeSet(struct Pokemon *mon);
bool8 IsaacUseBloodPact(struct Pokemon *mon);
s32 IsaacRollCursedDie(struct Pokemon *mon, bool8 *ok);
bool8 IsaacUseGoldenHeart(void);
void IsaacGoldenHeartReset(void);
void IsaacGoldenHeartStart(void);
void IsaacGoldenHeartEnd(void);
void IsaacProgressionStart(bool8 boss);
void IsaacRecordActiveTurn(void);
enum Species IsaacPickBattleProgression(bool8 won, u32 *outSlot);
const u8 *IsaacGetEvolutionIntroText(bool8 *learnOnly);
enum Type IsaacGetMonGemType(struct Pokemon *mon);
void IsaacGetMonTypes(struct Pokemon *mon, enum Type *types);
bool8 IsaacUseTypeGem(struct Pokemon *mon, enum Type type);
void IsaacApplyBattlerGemType(enum BattlerId battler);

#endif // GUARD_ISAAC_POKEMON_H
