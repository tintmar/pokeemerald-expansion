#include "global.h"
#include "data.h"
#include "isaac_pokemon.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "trainer_util.h"
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

// Une espèce des lignées données de la catégorie voulue, SPECIES_NONE s'il n'y en a pas
enum Species IsaacPickLineSpecies(const u16 *lines, u32 lineCount, enum IsaacCategory category, u32 roll)
{
    u16 species[64], candidates[64];
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

// Palier du set du Pokémon + 1 (0 : inconnu) : rangé dans la stat de concours Beauté
#define ISAAC_TIER_DATA       MON_DATA_BEAUTY

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

// Pokémon (du joueur, ou sauvage) d'après le set n° tier de l'espèce, niveau ISAAC_MON_LEVEL
void IsaacCreateMonFromSet(struct Pokemon *mon, enum Species species, u32 tier, u32 seed)
{
    struct TrainerMon set = *IsaacGetSet(species, tier);
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
}

// Palier du set d'un Pokémon (0 à ISAAC_SET_TIERS - 1) : celui enregistré sur le Pokémon, sinon reconnu à ses
// attaques (0 si aucun set ne correspond)
u32 IsaacGetMonSetTier(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u32 stored = GetMonData(mon, ISAAC_TIER_DATA);
    const struct TrainerMon *set;
    u32 tier, i;

    if (stored >= 1 && stored <= ISAAC_SET_TIERS)
        return stored - 1;
    for (tier = 0; tier < ISAAC_SET_TIERS; tier++)
    {
        set = IsaacGetSet(species, tier);
        if (set == NULL)
            return 0;
        for (i = 0; i < MAX_MON_MOVES && GetMonData(mon, MON_DATA_MOVE1 + i) == set->moves[i]; i++)
            ;
        if (i == MAX_MON_MOVES)
            return tier;
    }
    return 0;
}

// Remplace le Pokémon par le set n° tier de l'espèce donnée. Il garde son pourcentage de PV, son statut et ses
// Omniboosts.
static void ReplaceWithSet(struct Pokemon *mon, enum Species species, u32 tier)
{
    struct Pokemon replacement;
    u32 hp = GetMonData(mon, MON_DATA_HP);
    u32 maxHP = GetMonData(mon, MON_DATA_MAX_HP);
    u32 status = GetMonData(mon, MON_DATA_STATUS);
    u32 omniboosts = GetMonData(mon, ISAAC_OMNIBOOST_DATA);
    u32 newHP;

    IsaacCreateMonFromSet(&replacement, species, tier, Random32());
    SetMonData(&replacement, ISAAC_OMNIBOOST_DATA, &omniboosts);
    CalculateMonStats(&replacement);
    newHP = GetMonData(&replacement, MON_DATA_MAX_HP);
    if (maxHP != 0 && hp < maxHP)
        newHP = max(newHP * hp / maxHP, hp != 0 ? 1u : 0u);
    SetMonData(&replacement, MON_DATA_HP, &newHP);
    SetMonData(&replacement, MON_DATA_STATUS, &status);
    *mon = replacement;
}

// Après l'évolution par la Pierre Évolution : le Pokémon (déjà évolué) reçoit le set de même palier de sa
// nouvelle espèce
void IsaacApplyEvolvedSet(struct Pokemon *mon, u32 tier)
{
    ReplaceWithSet(mon, GetMonData(mon, MON_DATA_SPECIES), tier);
}

// Ability Trainer : le Pokémon passe au set suivant de son espèce (FALSE s'il a déjà le meilleur)
bool8 IsaacUpgradeSet(struct Pokemon *mon)
{
    u32 tier = IsaacGetMonSetTier(mon);

    if (tier + 1 >= ISAAC_SET_TIERS)
        return FALSE;
    ReplaceWithSet(mon, GetMonData(mon, MON_DATA_SPECIES), tier + 1);
    return TRUE;
}

// Choix du Montagnard du hub : le Pokémon proposé n° index (0 à ISAAC_STARTER_CHOICES - 1), une espèce
// différente de catégorie 1 pour chacun, avec un set au hasard (1 ou 2). Les espèces sont tirées à l'index 0.
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
    IsaacCreateMonFromSet(mon, sStarterSpecies[index], Random() % 2, Random32());
}
