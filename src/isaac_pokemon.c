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

// Espèces des lignées données de la catégorie voulue (sans doublon avec celles déjà dans out) ; renvoie le
// nouveau nombre d'espèces dans out
u32 IsaacAddLinesSpeciesOfCategory(const u16 *lines, u32 lineCount, enum IsaacCategory category, u16 *out, u32 count, u32 max)
{
    u16 species[ISAAC_MAX_LINE_SPECIES];
    u32 n = GetLinesSpecies(lines, lineCount, species, ARRAY_COUNT(species));
    u32 i, j;

    for (i = 0; i < n && count < max; i++)
    {
        if (IsaacGetSpeciesCategory(species[i]) != category)
            continue;
        for (j = 0; j < count && out[j] != species[i]; j++)
            ;
        if (j == count)
            out[count++] = species[i];
    }
    return count;
}

// Une espèce des lignées données de la catégorie voulue, SPECIES_NONE s'il n'y en a pas
enum Species IsaacPickLineSpecies(const u16 *lines, u32 lineCount, enum IsaacCategory category, u32 roll)
{
    u16 species[ISAAC_MAX_LINE_SPECIES], candidates[ISAAC_MAX_LINE_SPECIES];
    u32 count = GetLinesSpecies(lines, lineCount, species, ARRAY_COUNT(species));
    u32 n = 0, i;

    for (i = 0; i < count; i++)
        if (IsaacGetSpeciesCategory(species[i]) == category)
            candidates[n++] = species[i];
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

// Stat (PV compris) avec les Omniboosts du Pokémon : +10 % chacun
s32 IsaacApplyOmniboost(struct Pokemon *mon, s32 stat)
{
    return stat * (10 + GetMonData(mon, ISAAC_OMNIBOOST_DATA)) / 10;
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

// Remplace le Pokémon par un set de l'espèce donnée. Il garde son pourcentage de PV, son statut et ses
// Omniboosts.
static void ReplaceWithSet(struct Pokemon *mon, enum Species species, u32 tier, u32 variant)
{
    struct Pokemon replacement;
    u32 hp = GetMonData(mon, MON_DATA_HP);
    u32 maxHP = GetMonData(mon, MON_DATA_MAX_HP);
    u32 status = GetMonData(mon, MON_DATA_STATUS);
    u32 omniboosts = GetMonData(mon, ISAAC_OMNIBOOST_DATA);
    u32 newHP;

    IsaacCreateMonFromSet(&replacement, species, tier, variant, Random32());
    SetMonData(&replacement, ISAAC_OMNIBOOST_DATA, &omniboosts);
    CalculateMonStats(&replacement);
    newHP = GetMonData(&replacement, MON_DATA_MAX_HP);
    if (maxHP != 0 && hp < maxHP)
        newHP = max(newHP * hp / maxHP, hp != 0 ? 1u : 0u);
    SetMonData(&replacement, MON_DATA_HP, &newHP);
    SetMonData(&replacement, MON_DATA_STATUS, &status);
    *mon = replacement;
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

// Ability Trainer : le Pokémon passe au palier suivant de son espèce, même variante (FALSE s'il a déjà le
// meilleur). Il garde l'objet qu'il tient (souvent acheté ou donné par le joueur), pas celui du set.
bool8 IsaacUpgradeSet(struct Pokemon *mon)
{
    u32 tier = IsaacGetMonSetTier(mon);
    u32 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);

    if (tier + 1 >= ISAAC_SET_TIERS)
        return FALSE;
    ReplaceWithSet(mon, GetMonData(mon, MON_DATA_SPECIES), tier + 1, IsaacGetMonSetVariant(mon));
    SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
    return TRUE;
}

// Choix du Montagnard du hub : le Pokémon proposé n° index (0 à ISAAC_STARTER_CHOICES - 1), une espèce
// différente de catégorie 1 pour chacun, avec le set 2 (variante au hasard). Les espèces sont tirées à l'index 0.
void IsaacCreateStarterChoice(struct Pokemon *mon, u32 index)
{
    static EWRAM_DATA u16 sStarterSpecies[ISAAC_STARTER_CHOICES] = {0};
    u16 candidates[SPECIES_DEOXYS];
    u32 count = 0, i, j, tmp;

    if (index >= ISAAC_STARTER_CHOICES)
        return;
    if (index == 0)
    {
        for (i = 1; i <= SPECIES_DEOXYS; i++)
            if (IsaacGetSpeciesCategory(i) == ISAAC_CATEGORY_1)
                candidates[count++] = i;
        for (i = 0; i < ISAAC_STARTER_CHOICES; i++)
        {
            j = i + Random() % (count - i);
            SWAP(candidates[i], candidates[j], tmp);
            sStarterSpecies[i] = candidates[i];
        }
    }
    IsaacCreateMonFromSet(mon, sStarterSpecies[index], 1, Random() % ISAAC_SET_VARIANTS, Random32());  // set 2
}
