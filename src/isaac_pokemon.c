#include "global.h"
#include "battle.h"
#include "data.h"
#include "isaac_pokemon.h"
#include "party_menu.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "trainer_util.h"
#include "constants/abilities.h"
#include "constants/event_objects.h"
#include "constants/layouts.h"
#include "constants/songs.h"
#include "constants/weather.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/battle.h"

/*
 * Sets des Pokémon du donjon : pour chaque espèce (1 à 386), 3 paliers du plus faible au plus fort, chacun en
 * deux variantes A et B de même force mais de rôles différents (tirées à 50/50).
 * Les sets sont écrits dans tools/isaac/sets_gen*.py ; src/data/isaac_sets.h est généré par
 * tools/isaac/build_sets.py, qui vérifie aussi que les attaques sont apprenables.
 */

#include "data/isaac_sets.h"
#include "data/isaac_biomes.h"
#include "data/isaac_categories.h"

// NULL si l'espèce n'a pas de sets (au-delà de Deoxys)
const struct TrainerMon *IsaacGetSet(enum Species species, u32 tier, u32 variant)
{
    if (species == SPECIES_NONE || species >= ARRAY_COUNT(sIsaacSets) || sIsaacSets[species][0][0].species == SPECIES_NONE)
        return NULL;
    return &sIsaacSets[species][min(tier, ISAAC_SET_TIERS - 1)][variant % ISAAC_SET_VARIANTS];
}

// ISAAC_CATEGORY_NONE au-delà de Deoxys
enum IsaacCategory IsaacGetSpeciesCategory(enum Species species)
{
    if (species >= ARRAY_COUNT(sIsaacSpeciesCategories))
        return ISAAC_CATEGORY_NONE;
    return sIsaacSpeciesCategories[species];
}

// ---------------------------------------------------------------------------
// Lignées et évolutions
// ---------------------------------------------------------------------------

// Évolutions d'une espèce parmi les 386 premières (ni « clone » comme Munja, ni forme régionale) ; renvoie
// leur nombre
static u32 GetEvolutions(enum Species species, u16 *targets, u32 max)
{
    const struct Evolution *evolutions = GetSpeciesEvolutions(species);
    u32 i, j, count = 0;

    if (evolutions == NULL)
        return 0;
    for (i = 0; evolutions[i].method != EVOLUTIONS_END && count < max; i++)
    {
        enum Species target = SanitizeSpeciesId(evolutions[i].targetSpecies);

        if (target == SPECIES_NONE || target > SPECIES_DEOXYS
         || evolutions[i].method == EVO_NONE || evolutions[i].method == EVO_SPLIT_FROM_EVO)
            continue;
        for (j = 0; j < count && targets[j] != target; j++)
            ;
        if (j == count)
            targets[count++] = target;
    }
    return count;
}

// Stade final d'une espèce (Super Pierre Évolution), en tirant au hasard aux embranchements ; SPECIES_NONE si elle
// n'évolue pas
enum Species IsaacGetFinalEvolution(enum Species species, u32 roll)
{
    enum Species next = IsaacGetEvolution(species, roll), final = SPECIES_NONE;

    while (next != SPECIES_NONE)
    {
        final = next;
        roll = roll * 1103515245 + 12345;
        next = IsaacGetEvolution(final, roll >> 8);
    }
    return final;
}

// Objet spécial tiré au hasard (salle trésor, boutique, pacte du diable) : chaque entrée a la même chance ; les
// 6 boosts de stat partagent une seule entrée (puis la stat est tirée), pour ne pas être 6 fois plus courants
u16 IsaacPickSpecialItem(u32 roll)
{
    static const u16 sSpecialItems[] =
    {
        ITEM_EVOLUTION_STONE, ITEM_SUPER_EVOLUTION_STONE, ITEM_OMNIBOOST, ITEM_ABILITY_TRAINER, ITEM_BOOST_HP,
        ITEM_BRILLIANT_STONE,
    };
    u16 item = sSpecialItems[roll % ARRAY_COUNT(sSpecialItems)];

    if (item == ITEM_BOOST_HP)
        item = ITEM_BOOST_HP + (roll >> 8) % NUM_STATS;
    return item;
}

// Évolution suivante d'une espèce (choice départage Évoli, Têtarte...), SPECIES_NONE s'il n'y en a pas
enum Species IsaacGetEvolution(enum Species species, u32 choice)
{
    u16 targets[8];
    u32 count = GetEvolutions(species, targets, ARRAY_COUNT(targets));

    return count ? targets[choice % count] : SPECIES_NONE;
}

#define ISAAC_MAX_LINE_SPECIES  128  // espèces d'un biome entier (une trentaine de lignées et leurs évolutions)

// Toutes les espèces des lignées données (espèces de base et leurs évolutions) ; renvoie leur nombre
static u32 GetLinesSpecies(const u16 *lines, u32 lineCount, u16 *out, u32 max)
{
    u16 targets[8];
    u32 count = 0, i, j, k, n;

    for (i = 0; i < lineCount && count < max; i++)
        out[count++] = lines[i];
    for (i = 0; i < count; i++)  // la liste s'allonge pendant le parcours
    {
        n = GetEvolutions(out[i], targets, ARRAY_COUNT(targets));
        for (j = 0; j < n && count < max; j++)
        {
            for (k = 0; k < count && out[k] != targets[j]; k++)
                ;
            if (k == count)
                out[count++] = targets[j];
        }
    }
    return count;
}

static u32 GetSpeciesBST(enum Species species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->baseHP + info->baseAttack + info->baseDefense + info->baseSpeed + info->baseSpAttack + info->baseSpDefense;
}

// Espèces des lignées données de la catégorie voulue (1 à 4). Avec ISAAC_CATEGORIES_PER_BIOME, catégories
// recalculées sur ces lignées seules : leurs espèces triées par BST sont réparties en 4 quarts égaux (catégorie 1 : le
// quart le plus faible ; deux espèces de même BST ont la même catégorie). Sinon, catégories globales
// (IsaacGetSpeciesCategory). Renvoie le nombre d'espèces écrites dans out.
static u32 GetLinesSpeciesOfCategory(const u16 *lines, u32 lineCount, enum IsaacCategory category, u16 *out)
{
    u16 species[ISAAC_MAX_LINE_SPECIES], bst[ISAAC_MAX_LINE_SPECIES];
    u32 n = GetLinesSpecies(lines, lineCount, species, ARRAY_COUNT(species));
    u32 count = 0, i, j, first;
    u16 tmpSpecies, tmpBst;

    if (!ISAAC_CATEGORIES_PER_BIOME)
    {
        for (i = 0; i < n; i++)
            if (IsaacGetSpeciesCategory(species[i]) == category)
                out[count++] = species[i];
        return count;
    }

    for (i = 0; i < n; i++)
        bst[i] = GetSpeciesBST(species[i]);
    for (i = 1; i < n; i++)  // tri par insertion, du BST le plus faible au plus fort
    {
        for (j = i; j > 0 && bst[j - 1] > bst[j]; j--)
        {
            SWAP(bst[j - 1], bst[j], tmpBst);
            SWAP(species[j - 1], species[j], tmpSpecies);
        }
    }
    for (i = 0; i < n; i++)
    {
        for (first = i; first > 0 && bst[first - 1] == bst[i]; first--)
            ;
        if (ISAAC_CATEGORY_1 + first * 4 / n == category)
            out[count++] = species[i];
    }
    return count;
}

// Catégorie (1 à 4) d'une espèce au sens des lignées données (voir GetLinesSpeciesOfCategory), ISAAC_CATEGORY_NONE si
// elle n'en fait pas partie
enum IsaacCategory IsaacGetLinesCategory(const u16 *lines, u32 lineCount, enum Species species)
{
    u16 candidates[ISAAC_MAX_LINE_SPECIES];
    u32 category, n, i;

    for (category = ISAAC_CATEGORY_1; category <= ISAAC_CATEGORY_4; category++)
    {
        n = GetLinesSpeciesOfCategory(lines, lineCount, category, candidates);
        for (i = 0; i < n; i++)
            if (candidates[i] == species)
                return category;
    }
    return ISAAC_CATEGORY_NONE;
}

// Espèces des lignées données de la catégorie voulue, au sens de ces lignées (voir GetLinesSpeciesOfCategory), sans
// doublon avec celles déjà dans out ; renvoie le nouveau nombre d'espèces dans out
u32 IsaacAddLinesSpeciesOfCategory(const u16 *lines, u32 lineCount, enum IsaacCategory category, u16 *out, u32 count, u32 max)
{
    u16 species[ISAAC_MAX_LINE_SPECIES];
    u32 n = GetLinesSpeciesOfCategory(lines, lineCount, category, species);
    u32 i, j;

    for (i = 0; i < n && count < max; i++)
    {
        for (j = 0; j < count && out[j] != species[i]; j++)
            ;
        if (j == count)
            out[count++] = species[i];
    }
    return count;
}

// Une espèce des lignées données de la catégorie voulue, au sens de ces lignées (voir GetLinesSpeciesOfCategory),
// SPECIES_NONE s'il n'y en a pas
enum Species IsaacPickLineSpecies(const u16 *lines, u32 lineCount, enum IsaacCategory category, u32 roll)
{
    u16 candidates[ISAAC_MAX_LINE_SPECIES];
    u32 n = GetLinesSpeciesOfCategory(lines, lineCount, category, candidates);

    return n ? candidates[roll % n] : SPECIES_NONE;
}

// ---------------------------------------------------------------------------
// Omniboost
// ---------------------------------------------------------------------------

// Nombre d'Omniboosts reçus : rangé dans la stat de concours Sang-froid (pas de concours dans le jeu)
#define ISAAC_OMNIBOOST_DATA  MON_DATA_COOL
#define ISAAC_OMNIBOOST_MAX   10  // +100 % au plus

// Palier du set du Pokémon + 1 (0 : inconnu) : rangé dans la stat de concours Beauté ; variante (A, B) + 1 dans
// la stat Grâce
#define ISAAC_TIER_DATA       MON_DATA_BEAUTY
#define ISAAC_VARIANT_DATA    MON_DATA_CUTE

// Boosts de stat reçus (+25 %, un par stat) : un bit par stat (1 << STAT_*), dans la stat de concours Intelligence
#define ISAAC_STAT_BOOST_DATA  MON_DATA_SMART
#define ISAAC_STAT_BOOST_PCT   25
// Pierre brillante : stat boostée + 1 (0 : pas encore utilisée), dans la stat de concours Robustesse
#define ISAAC_BRILLIANT_DATA   MON_DATA_TOUGH
#define ISAAC_BRILLIANT_PCT    20

// Valeur d'une stat (STAT_*, PV compris) avec les bonus du donjon : Omniboosts (+10 % chacun), boost de cette stat,
// Pierre brillante
s32 IsaacApplyStatBoosts(struct Pokemon *mon, u32 stat, s32 value)
{
    value = value * (10 + GetMonData(mon, ISAAC_OMNIBOOST_DATA)) / 10;
    if (GetMonData(mon, ISAAC_STAT_BOOST_DATA) & (1 << stat))
        value = value * (100 + ISAAC_STAT_BOOST_PCT) / 100;
    if (GetMonData(mon, ISAAC_BRILLIANT_DATA) == stat + 1)
        value = value * (100 + ISAAC_BRILLIANT_PCT) / 100;
    return value;
}

// Boost de la stat (STAT_*) : FALSE si le Pokémon l'a déjà reçu ; les stats sont recalculées
bool8 IsaacAddStatBoost(struct Pokemon *mon, u32 stat)
{
    u32 boosts = GetMonData(mon, ISAAC_STAT_BOOST_DATA);

    if (boosts & (1 << stat))
        return FALSE;
    boosts |= 1 << stat;
    SetMonData(mon, ISAAC_STAT_BOOST_DATA, &boosts);
    CalculateMonStats(mon);
    return TRUE;
}

// Pierre brillante : le Pokémon devient chromatique et sa meilleure stat actuelle (PV compris) monte de 20 %. FALSE
// s'il en a déjà reçu une ; *stat = la stat boostée.
bool8 IsaacUseBrilliantStone(struct Pokemon *mon, u32 *stat)
{
    u32 best = STAT_HP, i, value = TRUE;

    if (GetMonData(mon, ISAAC_BRILLIANT_DATA) != 0)
        return FALSE;
    for (i = STAT_HP + 1; i < NUM_STATS; i++)
        if (GetMonData(mon, MON_DATA_MAX_HP + i) > GetMonData(mon, MON_DATA_MAX_HP + best))
            best = i;
    SetMonData(mon, MON_DATA_IS_SHINY, &value);
    value = best + 1;
    SetMonData(mon, ISAAC_BRILLIANT_DATA, &value);
    CalculateMonStats(mon);
    *stat = best;
    return TRUE;
}

// Un Omniboost de plus (FALSE s'il en a déjà le maximum) ; les stats sont recalculées
bool8 IsaacAddOmniboost(struct Pokemon *mon)
{
    u32 count = GetMonData(mon, ISAAC_OMNIBOOST_DATA);

    if (count >= ISAAC_OMNIBOOST_MAX)
        return FALSE;
    count++;
    SetMonData(mon, ISAAC_OMNIBOOST_DATA, &count);
    CalculateMonStats(mon);
    return TRUE;
}

// ---------------------------------------------------------------------------
// Pokémon du joueur et sauvages
// ---------------------------------------------------------------------------

// Pokémon (du joueur, ou sauvage) d'après le set de l'espèce (palier tier, variante variant), niveau
// ISAAC_MON_LEVEL
void IsaacCreateMonFromSet(struct Pokemon *mon, enum Species species, u32 tier, u32 variant, u32 seed)
{
    struct TrainerMon set = *IsaacGetSet(species, tier, variant);
    struct TrainerGenerator generator = {0};
    u32 value;

    set.lvl = ISAAC_MON_LEVEL;
    generator.otID = OTID_STRUCT_PLAYER_ID;
    generator.gender = gSaveBlock2Ptr->playerGender;
    StringCopy(generator.name, gSaveBlock2Ptr->playerName);
    generator.localRngState = LocalRandomSeed(seed);
    GenerateMonFromTrainerMon(mon, &set, &generator);
    // Le générateur est fait pour les dresseurs : amitié de base, Dynamax normal
    value = gSpeciesInfo[species].friendship;
    SetMonData(mon, MON_DATA_FRIENDSHIP, &value);
    value = 0;
    SetMonData(mon, MON_DATA_DYNAMAX_LEVEL, &value);
    value = tier + 1;
    SetMonData(mon, ISAAC_TIER_DATA, &value);
    value = variant % ISAAC_SET_VARIANTS + 1;
    SetMonData(mon, ISAAC_VARIANT_DATA, &value);
}

// Le set (palier et variante) dont le Pokémon a les attaques, FALSE si aucun
static bool8 FindMonSetByMoves(struct Pokemon *mon, u32 *tier, u32 *variant)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    const struct TrainerMon *set;
    u32 i;

    for (*tier = 0; *tier < ISAAC_SET_TIERS; (*tier)++)
    {
        for (*variant = 0; *variant < ISAAC_SET_VARIANTS; (*variant)++)
        {
            set = IsaacGetSet(species, *tier, *variant);
            if (set == NULL)
                return FALSE;
            for (i = 0; i < MAX_MON_MOVES && GetMonData(mon, MON_DATA_MOVE1 + i) == set->moves[i]; i++)
                ;
            if (i == MAX_MON_MOVES)
                return TRUE;
        }
    }
    return FALSE;
}

// Palier du set d'un Pokémon (0 à ISAAC_SET_TIERS - 1) : celui enregistré sur le Pokémon, sinon reconnu à ses
// attaques (0 si aucun set ne correspond)
u32 IsaacGetMonSetTier(struct Pokemon *mon)
{
    u32 stored = GetMonData(mon, ISAAC_TIER_DATA);
    u32 tier, variant;

    if (stored >= 1 && stored <= ISAAC_SET_TIERS)
        return stored - 1;
    return FindMonSetByMoves(mon, &tier, &variant) ? tier : 0;
}

// Variante du set d'un Pokémon (0 : A, 1 : B), comme IsaacGetMonSetTier
u32 IsaacGetMonSetVariant(struct Pokemon *mon)
{
    u32 stored = GetMonData(mon, ISAAC_VARIANT_DATA);
    u32 tier, variant;

    if (stored >= 1 && stored <= ISAAC_SET_VARIANTS)
        return stored - 1;
    return FindMonSetByMoves(mon, &tier, &variant) ? variant : 0;
}

// ---------------------------------------------------------------------------
// Pierre Évolution
// ---------------------------------------------------------------------------
// Le Pokémon évolué garde ses attaques, son objet, ses EVs et IVs (et son palier / sa variante de set).
// Exception : les Pokémon dont les attaques ne servent plus à rien une fois évolués (cocons, Magicarpe...,
// liste ci-dessous). Après l'animation d'évolution, ils proposent d'apprendre une à une les attaques du set
// de même palier et même variante de leur évolution, comme un Pokémon qui monte de niveau (choix de
// l'attaque à oublier). Voir IsaacTryLearningEvolutionMove, appelée par evolution_scene.c.
static const u16 sIsaacEvolutionLearnSpecies[] =
{
    SPECIES_CATERPIE,
    SPECIES_METAPOD,
    SPECIES_WEEDLE,
    SPECIES_KAKUNA,
    SPECIES_MAGIKARP,
    SPECIES_WURMPLE,
    SPECIES_SILCOON,
    SPECIES_CASCOON,
};

// Attaques à proposer pendant l'évolution en cours (MOVE_NONE : aucune)
static EWRAM_DATA u16 sIsaacEvolutionMoves[MAX_MON_MOVES] = {0};
static EWRAM_DATA u8 sIsaacEvolutionMoveIndex = 0;

// Avant la scène d'évolution de mon vers target par la Pierre Évolution
void IsaacPrepareEvolutionMoves(struct Pokemon *mon, enum Species target)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    const struct TrainerMon *set;
    u32 i;

    memset(sIsaacEvolutionMoves, 0, sizeof(sIsaacEvolutionMoves));
    for (i = 0; i < ARRAY_COUNT(sIsaacEvolutionLearnSpecies) && sIsaacEvolutionLearnSpecies[i] != species; i++)
        ;
    if (i == ARRAY_COUNT(sIsaacEvolutionLearnSpecies))
        return;
    set = IsaacGetSet(target, IsaacGetMonSetTier(mon), IsaacGetMonSetVariant(mon));
    if (set != NULL)
        memcpy(sIsaacEvolutionMoves, set->moves, sizeof(sIsaacEvolutionMoves));
}

// Pendant la scène d'évolution, à la place de MonTryLearningNewMoveEvolution : attaque suivante à proposer
// (dans gMoveToLearn), avec le résultat de GiveMoveToMon ; MOVE_NONE quand il n'y en a plus
u16 IsaacTryLearningEvolutionMove(struct Pokemon *mon, bool8 firstMove)
{
    if (firstMove)
        sIsaacEvolutionMoveIndex = 0;
    while (sIsaacEvolutionMoveIndex < MAX_MON_MOVES)
    {
        gMoveToLearn = sIsaacEvolutionMoves[sIsaacEvolutionMoveIndex++];
        if (gMoveToLearn != MOVE_NONE && !MonKnowsMove(mon, gMoveToLearn))
            return GiveMoveToMon(mon, gMoveToLearn);
    }
    memset(sIsaacEvolutionMoves, 0, sizeof(sIsaacEvolutionMoves));
    return MOVE_NONE;
}

// Ability Trainer : le Pokémon prend les attaques du palier suivant de son espèce, même variante (FALSE s'il a déjà
// le meilleur). Seul le moveset change : IVs, EVs, nature, talent et objet tenu restent les mêmes.
bool8 IsaacUpgradeSet(struct Pokemon *mon)
{
    const struct TrainerMon *set;
    u32 tier = IsaacGetMonSetTier(mon);
    u32 i, value;

    if (tier + 1 >= ISAAC_SET_TIERS)
        return FALSE;
    set = IsaacGetSet(GetMonData(mon, MON_DATA_SPECIES), tier + 1, IsaacGetMonSetVariant(mon));
    if (set == NULL)
        return FALSE;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        RemoveMonPPBonus(mon, i);
        SetMonMoveSlot(mon, set->moves[i], i);  // PP au complet (MOVE_NONE : place vide)
    }
    value = tier + 2;  // palier suivant + 1
    SetMonData(mon, ISAAC_TIER_DATA, &value);
    return TRUE;
}

// Choix du Montagnard du hub : le Pokémon proposé n° index (0 à ISAAC_STARTER_CHOICES - 1), une espèce
// différente de catégorie 1 pour chacun, sans type en commun entre eux, avec le set 2 (variante au hasard). Les
// espèces sont tirées à l'index 0.
// L'espèce a-t-elle un type en commun avec une des count espèces données ?
static bool32 SharesTypeWithStarters(u16 species, const u16 *starters, u32 count)
{
    u32 i, a, b;

    for (i = 0; i < count; i++)
        for (a = 0; a < 2; a++)
            for (b = 0; b < 2; b++)
                if (GetSpeciesType(species, a) == GetSpeciesType(starters[i], b))
                    return TRUE;
    return FALSE;
}

void IsaacCreateStarterChoice(struct Pokemon *mon, u32 index)
{
    static EWRAM_DATA u16 sStarterSpecies[ISAAC_STARTER_CHOICES] = {0};
    u16 candidates[SPECIES_DEOXYS];
    u32 count = 0, i, j, tmp;

    if (index >= ISAAC_STARTER_CHOICES)
        return;
    if (index == 0)
    {
        u32 chosen = 0;

        for (i = 1; i <= SPECIES_DEOXYS; i++)
            if (IsaacGetSpeciesCategory(i) == ISAAC_CATEGORY_1)
                candidates[count++] = i;
        // tirage dans un ordre au hasard, en gardant seulement les espèces sans type commun avec celles déjà prises
        for (i = 0; i < count && chosen < ISAAC_STARTER_CHOICES; i++)
        {
            j = i + Random() % (count - i);
            SWAP(candidates[i], candidates[j], tmp);
            if (!SharesTypeWithStarters(candidates[i], sStarterSpecies, chosen))
                sStarterSpecies[chosen++] = candidates[i];
        }
    }
    IsaacCreateMonFromSet(mon, sStarterSpecies[index], 1, Random() % ISAAC_SET_VARIANTS, Random32());  // set 2
}
