#include "global.h"
#include "battle.h"
#include "battle_message.h"
#include "data.h"
#include "event_data.h"
#include "item.h"
#include "isaac_dungeon.h"
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
 * Sets des Pokémon du donjon : pour chaque espèce du roster, 4 paliers du plus faible au plus fort, chacun en
 * deux variantes A et B de même force mais de rôles différents (tirées à 50/50).
 * Les sets sont écrits dans tools/isaac/data/pokemon_sets_*.txt ; src/data/isaac_sets.h est généré par
 * tools/isaac/build_sets.py (via biome_sets.py), qui vérifie aussi que les attaques sont apprenables.
 */

#include "data/isaac_sets.h"
#include "data/isaac_biomes.h"
#include "data/isaac_categories.h"
#include "data/isaac_move_tiers.h"

// NULL si l'espèce n'a pas de sets (hors du roster)
const struct TrainerMon *IsaacGetSet(enum Species species, u32 tier, u32 variant)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES || sIsaacSetIndex[species] == 0)
        return NULL;
    return &sIsaacSets[sIsaacSetIndex[species] - 1][min(tier, ISAAC_SET_TIERS - 1)][variant % ISAAC_SET_VARIANTS];
}

// L'espèce a-t-elle des sets (isaac_sets.h) ? Celles qui n'en ont pas encore ne sont jamais tirées.
bool8 IsaacSpeciesHasSet(enum Species species)
{
    return species != SPECIES_NONE && species < NUM_SPECIES && sIsaacSetIndex[species] != 0;
}

// ISAAC_CATEGORY_NONE hors du roster
enum IsaacCategory IsaacGetSpeciesCategory(enum Species species)
{
    if (species >= ARRAY_COUNT(sIsaacSpeciesCategories))
        return ISAAC_CATEGORY_NONE;
    return sIsaacSpeciesCategories[species];
}

// Espèces du roster (data/isaac_categories.h), dans l'ordre du Pokédex national
u32 IsaacGetRosterCount(void)
{
    return ARRAY_COUNT(sIsaacRoster);
}

enum Species IsaacGetRosterSpecies(u32 index)
{
    return sIsaacRoster[index];
}

// ---------------------------------------------------------------------------
// Lignées et évolutions
// ---------------------------------------------------------------------------

// Forme régionale du roster (data/isaac_categories.h) ? *sharedWith : l'espèce d'origine si c'est une forme à poids
// partagé, SPECIES_NONE si c'est une lignée à part
static bool32 IsRegionalForm(enum Species species, u16 *sharedWith)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sIsaacRegionalForms); i++)
    {
        if (sIsaacRegionalForms[i][0] == species)
        {
            *sharedWith = sIsaacRegionalForms[i][1];
            return TRUE;
        }
    }
    return FALSE;
}

// Espèce d'origine d'une forme à poids partagé (Rattata pour Rattata d'Alola), l'espèce elle-même sinon
enum Species IsaacGetSharedFormBase(enum Species species)
{
    u16 base;

    return (IsRegionalForm(species, &base) && base != SPECIES_NONE) ? base : species;
}

// L'espèce ou une de ses formes à poids partagé, au hasard à parts égales
enum Species IsaacRollSharedForm(enum Species species, u32 roll)
{
    u16 choices[4];
    u32 count = 0, i;

    choices[count++] = species;
    for (i = 0; i < ARRAY_COUNT(sIsaacRegionalForms) && count < ARRAY_COUNT(choices); i++)
        if (sIsaacRegionalForms[i][1] == species)
            choices[count++] = sIsaacRegionalForms[i][0];
    return choices[roll % count];
}

// Évolutions d'une espèce dans le roster (ni « clone » comme Munja) ; renvoie leur nombre. Avec sameRegion, une
// espèce qui n'est pas une forme régionale ne passe pas à une forme régionale (Osselait -> Ossatueur seulement) : les
// formes régionales sont tirées à part (lignée à elles, ou poids partagé avec IsaacRollSharedForm).
static u32 GetEvolutions(enum Species species, u16 *targets, u32 max, bool32 sameRegion)
{
    u16 base;
    bool32 fromForm = IsRegionalForm(species, &base);
    const struct Evolution *evolutions = GetSpeciesEvolutions(species);
    u32 i, j, count = 0;

    if (evolutions == NULL)
        return 0;
    for (i = 0; evolutions[i].method != EVOLUTIONS_END && count < max; i++)
    {
        enum Species target = SanitizeSpeciesId(evolutions[i].targetSpecies);

        if (target == SPECIES_NONE || IsaacGetSpeciesCategory(target) == ISAAC_CATEGORY_NONE
         || evolutions[i].method == EVO_NONE || evolutions[i].method == EVO_SPLIT_FROM_EVO
         || (sameRegion && !fromForm && IsRegionalForm(target, &base)))
            continue;
        for (j = 0; j < count && targets[j] != target; j++)
            ;
        if (j == count)
            targets[count++] = target;
    }
    return count;
}

// Évolutions possibles d'une espèce (Pierre Évolution : un stade ; Super Pierre Évolution, toFinalStage : tous les
// stades finaux qu'elle peut atteindre, sans doublon) ; renvoie leur nombre
u32 IsaacGetEvolutionChoices(enum Species species, bool32 toFinalStage, u16 *out, u32 max)
{
    u16 targets[8], finals[ISAAC_MAX_EVOLUTION_CHOICES];
    u32 count = GetEvolutions(species, targets, ARRAY_COUNT(targets), FALSE), n = 0, i, j, k, finalCount;

    if (!toFinalStage)
    {
        for (i = 0; i < count && n < max; i++)
            out[n++] = targets[i];
        return n;
    }
    for (i = 0; i < count; i++)
    {
        finalCount = IsaacGetEvolutionChoices(targets[i], TRUE, finals, ARRAY_COUNT(finals));
        if (finalCount == 0)  // ce stade n'évolue plus : c'est un stade final
        {
            finals[0] = targets[i];
            finalCount = 1;
        }
        for (j = 0; j < finalCount; j++)
        {
            for (k = 0; k < n && out[k] != finals[j]; k++)
                ;
            if (k == n && n < max)
                out[n++] = finals[j];
        }
    }
    return n;
}

// Objet spécial tiré au hasard (salle trésor, salle secrète, boutique, boss, pacte du diable). Chaque entrée a un poids :
// SPECIAL_ITEM_WEIGHT, sauf l'Omniboost (OMNIBOOST_WEIGHT, deux fois plus rare) et les deux Pierres Évolution, plus
// rares en profondeur (sEvolutionStoneWeights : moitié à l'étage 4, quart à l'étage 5, absentes à partir de l'étage 6),
// et la Super CT, de plus en plus rare (sAbilityTrainerWeights), rare à partir de l'étage 4. Dans la boutique (shop),
// pas de Pierre Évolution après SHOP_EVOLUTION_STONE_MAX_FLOOR. La Pièce Porte-Bonheur est surtout au début
// (sLuckyCoinWeights, jamais si on l'a déjà), la Trousse de Secours surtout à la fin (sFirstAidKitWeights, retirée pour
// l'instant : FIRST_AID_KIT_ENABLED) ; la Pierre Prisme est rare (PRISM_STONE_WEIGHT), le Cœur Doré peu courant
// (GOLDEN_HEART_WEIGHT).
// Les 6 boosts de stat partagent une seule entrée (puis la stat est tirée), pour ne pas être 6 fois plus courants, un
// peu plus fréquente en profondeur (sStatBoostWeights) ; de même pour les Gemmes de Type (puis le type est tiré ;
// retirées pour l'instant : TYPE_GEM_WEIGHT à 0). Le Cœur Doré n'apparaît qu'à partir de GOLDEN_HEART_MIN_FLOOR.
// La Méga-Évolution (ITEM_MEGA_RING dans la liste) n'apparaît que si un Pokémon de l'équipe peut méga-évoluer avec
// une Méga-Gemme qu'on n'a pas encore : on reçoit l'une d'elles au hasard (et l'Anneau Méga avec, voir AddBagItem).
#define SPECIAL_ITEM_WEIGHT  4
#define OMNIBOOST_WEIGHT     2
#define TYPE_GEM_WEIGHT      0
#define GOLDEN_HEART_WEIGHT  2
#define PRISM_STONE_WEIGHT   1
#define Z_TM_WEIGHT          2
#define GOLDEN_HEART_MIN_FLOOR  4
#define FIRST_AID_KIT_ENABLED   FALSE
#define SHOP_EVOLUTION_STONE_MAX_FLOOR  3

#define ISAAC_MAX_MEGA_STONES  (PARTY_SIZE * 2)  // Dracaufeu et Mewtwo ont deux Méga-Évolutions

// Méga-Évolution qui va au build du Pokémon, parmi les siennes (Dracaufeu, Mewtwo : X ou Y) : celle qui favorise le
// plus l'Attaque pour un Pokémon physique (EVs d'Attaque au moins égaux à ceux d'Attaque Spéciale, voir son set),
// l'Attaque Spéciale sinon ; renvoie son indice dans formChanges
static u32 GetBuildMegaEvolution(struct Pokemon *mon, const struct FormChange *formChanges)
{
    bool32 physical = GetMonData(mon, MON_DATA_ATK_EV) >= GetMonData(mon, MON_DATA_SPATK_EV);
    bool32 found = FALSE;
    s32 score, bestScore = 0;
    u32 j, best = 0;
    const struct SpeciesInfo *mega;

    for (j = 0; formChanges[j].method != FORM_CHANGE_TERMINATOR; j++)
    {
        if (formChanges[j].method != FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM)
            continue;
        mega = &gSpeciesInfo[formChanges[j].targetSpecies];
        score = (s32)mega->baseAttack - (s32)mega->baseSpAttack;
        if (!physical)
            score = -score;
        if (!found || score > bestScore)
        {
            found = TRUE;
            best = j;
            bestScore = score;
        }
    }
    return best;
}

// Méga-Gemmes des Pokémon de l'équipe qu'on n'a pas encore (ni dans le sac, ni tenues) ; renvoie leur nombre. Pour
// un Pokémon qui en a plusieurs, seulement celle de sa Méga-Évolution qui va à son build (GetBuildMegaEvolution).
static u32 GetMissingPartyMegaStones(u16 *out)
{
    const struct FormChange *formChanges;
    u32 count = 0, i, j, k, build;
    u16 stone;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            continue;
        formChanges = GetSpeciesFormChanges(GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES));
        if (formChanges == NULL)
            continue;
        build = GetBuildMegaEvolution(&gParties[B_TRAINER_PLAYER][i], formChanges);
        for (j = 0; formChanges[j].method != FORM_CHANGE_TERMINATOR; j++)
        {
            if (formChanges[j].method != FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM || j != build
             || count == ISAAC_MAX_MEGA_STONES)
                continue;
            stone = formChanges[j].param1;
            if (CheckBagHasItem(stone, 1))
                continue;
            for (k = 0; k < gPartiesCount[B_TRAINER_PLAYER] && GetMonData(&gParties[B_TRAINER_PLAYER][k], MON_DATA_HELD_ITEM) != stone; k++)
                ;
            if (k < gPartiesCount[B_TRAINER_PLAYER])
                continue;
            for (k = 0; k < count && out[k] != stone; k++)
                ;
            if (k == count)
                out[count++] = stone;
        }
    }
    return count;
}

u16 IsaacPickSpecialItem(u32 roll, bool8 shop)
{
    static const u16 sSpecialItems[] =
    {
        ITEM_EVOLUTION_STONE, ITEM_SUPER_EVOLUTION_STONE, ITEM_OMNIBOOST, ITEM_ABILITY_TRAINER, ITEM_BOOST_HP,
        ITEM_BRILLIANT_STONE, ITEM_ISAAC_GEM_FIRST, ITEM_GOLDEN_HEART, ITEM_MEGA_RING, ITEM_LUCKY_COIN,
        ITEM_FIRST_AID_KIT, ITEM_PRISM_STONE, ITEM_Z_TM,
    };
    static const u8 sEvolutionStoneWeights[] = { 4, 4, 4, 2, 1, 0 };  // étages 1 à 6 (et au-delà)
    static const u8 sAbilityTrainerWeights[] = { 4, 3, 2, 1, 1, 1 };
    static const u8 sLuckyCoinWeights[] = { 6, 5, 4, 2, 1, 0 };
    static const u8 sFirstAidKitWeights[] = { 1, 1, 2, 3, 4, 5 };
    static const u8 sStatBoostWeights[] = { 4, 4, 4, 5, 6, 6 };
    u32 floor = min(max(VarGet(VAR_ISAAC_FLOOR), 1u), ARRAY_COUNT(sEvolutionStoneWeights));
    u32 weights[ARRAY_COUNT(sSpecialItems)], total = 0, pick, i;
    u16 item = sSpecialItems[0];
    u16 megaStones[ISAAC_MAX_MEGA_STONES];
    u32 megaStoneCount = GetMissingPartyMegaStones(megaStones);

    for (i = 0; i < ARRAY_COUNT(sSpecialItems); i++)
    {
        if (sSpecialItems[i] == ITEM_EVOLUTION_STONE || sSpecialItems[i] == ITEM_SUPER_EVOLUTION_STONE)
            weights[i] = (shop && floor > SHOP_EVOLUTION_STONE_MAX_FLOOR) ? 0 : sEvolutionStoneWeights[floor - 1];
        else if (sSpecialItems[i] == ITEM_BOOST_HP)
            weights[i] = sStatBoostWeights[floor - 1];
        else if (sSpecialItems[i] == ITEM_ABILITY_TRAINER)
            weights[i] = sAbilityTrainerWeights[floor - 1];
        else if (sSpecialItems[i] == ITEM_OMNIBOOST)
            weights[i] = OMNIBOOST_WEIGHT;
        else if (sSpecialItems[i] == ITEM_ISAAC_GEM_FIRST)
            weights[i] = TYPE_GEM_WEIGHT;
        else if (sSpecialItems[i] == ITEM_GOLDEN_HEART)
            weights[i] = VarGet(VAR_ISAAC_FLOOR) >= GOLDEN_HEART_MIN_FLOOR ? GOLDEN_HEART_WEIGHT : 0;
        else if (sSpecialItems[i] == ITEM_LUCKY_COIN)
            weights[i] = CheckBagHasItem(ITEM_LUCKY_COIN, 1) ? 0 : sLuckyCoinWeights[floor - 1];
        else if (sSpecialItems[i] == ITEM_FIRST_AID_KIT)
            weights[i] = FIRST_AID_KIT_ENABLED ? sFirstAidKitWeights[floor - 1] : 0;
        else if (sSpecialItems[i] == ITEM_PRISM_STONE)
            weights[i] = PRISM_STONE_WEIGHT;
        else if (sSpecialItems[i] == ITEM_Z_TM)
            weights[i] = Z_TM_WEIGHT;
        else if (sSpecialItems[i] == ITEM_MEGA_RING)
            weights[i] = megaStoneCount != 0 ? SPECIAL_ITEM_WEIGHT : 0;
        else
            weights[i] = SPECIAL_ITEM_WEIGHT;
        total += weights[i];
    }
    pick = roll % total;
    for (i = 0; i < ARRAY_COUNT(sSpecialItems); i++)
    {
        if (pick < weights[i])
        {
            item = sSpecialItems[i];
            break;
        }
        pick -= weights[i];
    }
    if (item == ITEM_BOOST_HP)
        item = ITEM_BOOST_HP + (roll >> 8) % NUM_STATS;
    else if (item == ITEM_ISAAC_GEM_FIRST)
        item = ITEM_ISAAC_GEM_FIRST + (roll >> 8) % (ITEM_ISAAC_GEM_LAST - ITEM_ISAAC_GEM_FIRST + 1);
    else if (item == ITEM_MEGA_RING)
        item = megaStones[(roll >> 8) % megaStoneCount];
    return item;
}

// Évolution suivante d'une espèce (choice départage Évoli, Têtarte...), SPECIES_NONE s'il n'y en a pas
enum Species IsaacGetEvolution(enum Species species, u32 choice)
{
    u16 targets[8];
    u32 count = GetEvolutions(species, targets, ARRAY_COUNT(targets), TRUE);

    return count ? targets[choice % count] : SPECIES_NONE;
}

#define ISAAC_MAX_LINE_SPECIES  160  // espèces d'un biome entier (56 avec biome_roster.py)

// Espèces des lignées données (les espèces d'un biome, évolutions comprises : isaac_biome_species.h), sans celles
// qui n'ont pas encore de sets (IsaacSpeciesHasSet) ; renvoie leur nombre
static u32 GetLinesSpecies(const u16 *lines, u32 lineCount, u16 *out, u32 max)
{
    u32 count = 0, i;

    for (i = 0; i < lineCount && count < max; i++)
        if (IsaacSpeciesHasSet(lines[i]))
            out[count++] = lines[i];
    return count;
}

static u32 GetSpeciesBST(enum Species species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->baseHP + info->baseAttack + info->baseDefense + info->baseSpeed + info->baseSpAttack + info->baseSpDefense;
}

u32 IsaacGetSpeciesBST(enum Species species)
{
    return GetSpeciesBST(species);
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
        if (ISAAC_CATEGORY_1 + first * (ISAAC_CATEGORY_LAST_REGULAR - ISAAC_CATEGORY_1 + 1) / n == category)
            out[count++] = species[i];
    }
    return count;
}

// Catégorie (1 à 4) d'une espèce au sens des lignées données (voir GetLinesSpeciesOfCategory), ISAAC_CATEGORY_NONE si
// elle n'en fait pas partie ; une forme à poids partagé a celle de son espèce d'origine
enum IsaacCategory IsaacGetLinesCategory(const u16 *lines, u32 lineCount, enum Species species)
{
    u16 candidates[ISAAC_MAX_LINE_SPECIES];
    u32 category, n, i;

    species = IsaacGetSharedFormBase(species);
    for (category = ISAAC_CATEGORY_1; category <= ISAAC_CATEGORY_LAST_REGULAR; category++)
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

// Une espèce des lignées données de la catégorie voulue, au sens de ces lignées (voir GetLinesSpeciesOfCategory), de
// BST au moins minBst s'il y en a (sinon sans plancher), ou une de ses formes à poids partagé ; SPECIES_NONE s'il n'y en
// a pas
enum Species IsaacPickLineSpecies(const u16 *lines, u32 lineCount, enum IsaacCategory category, u32 minBst, u32 roll)
{
    u16 candidates[ISAAC_MAX_LINE_SPECIES];
    u32 n = GetLinesSpeciesOfCategory(lines, lineCount, category, candidates);
    u32 kept = 0, i;

    for (i = 0; i < n; i++)
        if (GetSpeciesBST(candidates[i]) >= minBst)
            candidates[kept++] = candidates[i];
    if (kept != 0)
        n = kept;
    return n ? IsaacRollSharedForm(candidates[roll % n], (roll / n) >> 3) : SPECIES_NONE;
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

// Boosts de stat reçus (+25 %, un par stat) : un bit par stat (1 << STAT_*), dans la stat de concours Intelligence ;
// le bit ISAAC_BLOOD_PACT_BIT dit si le Pokémon a reçu un Pacte de Sang (PV max -ISAAC_BLOOD_PACT_HP_MALUS_PCT %)
#define ISAAC_STAT_BOOST_DATA  MON_DATA_SMART
#define ISAAC_STAT_BOOST_PCT   25
#define ISAAC_BLOOD_PACT_BIT   (1 << 7)
#define ISAAC_BLOOD_PACT_PCT           20  // Attaque, Attaque Spéciale et Vitesse
#define ISAAC_BLOOD_PACT_HP_MALUS_PCT  25
// Pierre brillante : stat boostée + 1 (0 : pas encore utilisée), dans la stat de concours Robustesse
#define ISAAC_BRILLIANT_DATA   MON_DATA_TOUGH
#define ISAAC_BRILLIANT_PCT    20

// Gemme de Type : type donné (enum Type), 0 si aucun, dans la stat de concours Lustre
#define ISAAC_GEM_TYPE_DATA    MON_DATA_SHEEN

// Cœur Doré en cours d'utilisation (combat contre le boss) : un Omniboost de plus pour toute l'équipe du joueur
#define FLAG_ISAAC_GOLDEN_HEART_ACTIVE  FLAG_UNUSED_0x024
// Cœur Doré utilisé depuis le sac, en attente du prochain combat contre le boss
#define FLAG_ISAAC_GOLDEN_HEART_READY   FLAG_UNUSED_0x025

static bool32 IsPlayerPartyMon(struct Pokemon *mon)
{
    return mon >= &gParties[B_TRAINER_PLAYER][0] && mon < &gParties[B_TRAINER_PLAYER][PARTY_SIZE];
}

// Valeur d'une stat (STAT_*, PV compris) avec les bonus du donjon : Omniboosts (+10 % chacun, un de plus avec le Cœur
// Doré), boost de cette stat, Pierre brillante
s32 IsaacApplyStatBoosts(struct Pokemon *mon, u32 stat, s32 value)
{
    u32 omniboosts = GetMonData(mon, ISAAC_OMNIBOOST_DATA);

    if (FlagGet(FLAG_ISAAC_GOLDEN_HEART_ACTIVE) && IsPlayerPartyMon(mon))
        omniboosts++;
    value = value * (10 + omniboosts) / 10;
    if (GetMonData(mon, ISAAC_STAT_BOOST_DATA) & (1 << stat))
        value = value * (100 + ISAAC_STAT_BOOST_PCT) / 100;
    if (GetMonData(mon, ISAAC_BRILLIANT_DATA) == stat + 1)
        value = value * (100 + ISAAC_BRILLIANT_PCT) / 100;
    if (GetMonData(mon, ISAAC_STAT_BOOST_DATA) & ISAAC_BLOOD_PACT_BIT)
    {
        if (stat == STAT_HP)
            value = max(value * (100 - ISAAC_BLOOD_PACT_HP_MALUS_PCT) / 100, 1);
        else if (stat == STAT_ATK || stat == STAT_SPATK || stat == STAT_SPEED)
            value = value * (100 + ISAAC_BLOOD_PACT_PCT) / 100;
    }
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

// Pierre brillante : le Pokémon devient chromatique et sa meilleure stat actuelle (hors PV) monte de 20 %. FALSE
// s'il en a déjà reçu une ; *stat = la stat boostée.
bool8 IsaacUseBrilliantStone(struct Pokemon *mon, u32 *stat)
{
    u32 best = STAT_ATK, i, value = TRUE;

    if (GetMonData(mon, ISAAC_BRILLIANT_DATA) != 0)
        return FALSE;
    for (i = STAT_ATK + 1; i < NUM_STATS; i++)
        if (GetMonData(mon, MON_DATA_MAX_HP + i) > GetMonData(mon, MON_DATA_MAX_HP + best))
            best = i;
    SetMonData(mon, MON_DATA_IS_SHINY, &value);
    value = best + 1;
    SetMonData(mon, ISAAC_BRILLIANT_DATA, &value);
    CalculateMonStats(mon);
    *stat = best;
    return TRUE;
}

// Le Pokémon a-t-il déjà reçu un boost (un seul par Pokémon) : boost de stat, Pierre brillante, Omniboost ou Pacte de
// Sang ?
bool8 IsaacHasBoost(struct Pokemon *mon)
{
    return GetMonData(mon, ISAAC_STAT_BOOST_DATA) != 0 || GetMonData(mon, ISAAC_BRILLIANT_DATA) != 0
        || GetMonData(mon, ISAAC_OMNIBOOST_DATA) != 0;
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

// Pacte de Sang : Attaque, Attaque Spéciale et Vitesse +ISAAC_BLOOD_PACT_PCT %, mais PV max
// -ISAAC_BLOOD_PACT_HP_MALUS_PCT %, pour toujours. FALSE si le Pokémon a déjà un boost (voir IsaacHasBoost).
bool8 IsaacUseBloodPact(struct Pokemon *mon)
{
    u32 boosts = ISAAC_BLOOD_PACT_BIT;

    if (IsaacHasBoost(mon))
        return FALSE;
    SetMonData(mon, ISAAC_STAT_BOOST_DATA, &boosts);
    CalculateMonStats(mon);
    return TRUE;
}

static void RecalculatePlayerPartyStats(void)
{
    u32 i;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
        CalculateMonStats(&gParties[B_TRAINER_PLAYER][i]);
}

// Cœur Doré utilisé depuis le sac : il agira au prochain combat contre le boss. FALSE si un autre attend déjà.
bool8 IsaacUseGoldenHeart(void)
{
    if (FlagGet(FLAG_ISAAC_GOLDEN_HEART_READY))
        return FALSE;
    FlagSet(FLAG_ISAAC_GOLDEN_HEART_READY);
    return TRUE;
}

// Nouvelle descente : un Cœur Doré utilisé mais pas encore déclenché est perdu
void IsaacGoldenHeartReset(void)
{
    FlagClear(FLAG_ISAAC_GOLDEN_HEART_READY);
}

// Début du combat contre le boss : le Cœur Doré utilisé donne un Omniboost de plus à toute l'équipe, jusqu'à la fin
// du combat (IsaacGoldenHeartEnd)
void IsaacGoldenHeartStart(void)
{
    if (!FlagGet(FLAG_ISAAC_GOLDEN_HEART_READY))
        return;
    FlagClear(FLAG_ISAAC_GOLDEN_HEART_READY);
    FlagSet(FLAG_ISAAC_GOLDEN_HEART_ACTIVE);
    RecalculatePlayerPartyStats();
}

// Fin de n'importe quel combat : l'effet du Cœur Doré s'arrête
void IsaacGoldenHeartEnd(void)
{
    if (!FlagGet(FLAG_ISAAC_GOLDEN_HEART_ACTIVE))
        return;
    FlagClear(FLAG_ISAAC_GOLDEN_HEART_ACTIVE);
    RecalculatePlayerPartyStats();
}

// ---------------------------------------------------------------------------
// Progression par les combats
// ---------------------------------------------------------------------------

// Pendant un combat contre un dresseur de salle ou le boss, chaque Pokémon de l'équipe compte ses tours actifs (sur le
// terrain à la fin du tour). Combat gagné : le Pokémon qui n'est pas à son stade final et en a le plus évolue
// (sProgressionEvolutionPct % de chances selon l'étage, toujours contre le boss ; s'il a plusieurs évolutions, l'une
// de celles qui collent le mieux au biome), et apprend en même temps une attaque de tier supérieur pour sa nouvelle
// espèce (PickUpgradeMove). Sinon, le Pokémon qui a le plus de tours actifs apprend une attaque de tier supérieur
// (sProgressionMovePct %, toujours contre le boss), dans l'écran de l'évolution mais sans l'animation. Sinon, rien.
// À égalité, le premier de l'équipe.
static const u8 sProgressionEvolutionPct[] = { 10, 12, 14, 16, 18, 20 };  // par étage
static const u8 sProgressionMovePct[]      = { 20, 22, 24, 26, 28, 30 };

enum
{
    PROGRESSION_NONE,
    PROGRESSION_ROOM,
    PROGRESSION_BOSS,
};

enum
{
    PROGRESSION_SCENE_NONE,
    PROGRESSION_SCENE_EVOLUTION,
    PROGRESSION_SCENE_MOVE,
};

static EWRAM_DATA u8 sIsaacActiveTurns[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sIsaacProgressionBattle = PROGRESSION_NONE;
static EWRAM_DATA u8 sIsaacProgressionScene = PROGRESSION_SCENE_NONE;  // scène d'évolution lancée par la progression

// Attaques à proposer pendant la scène d'évolution en cours (MOVE_NONE : aucune) : celles du set évolué
// (IsaacPrepareEvolutionMoves), puis l'attaque de tier supérieur de la progression
static EWRAM_DATA u16 sIsaacEvolutionMoves[MAX_MON_MOVES + 1] = {0};
static EWRAM_DATA u8 sIsaacEvolutionMoveIndex = 0;

// Début du combat contre un dresseur de salle ou le boss
void IsaacProgressionStart(bool8 boss)
{
    memset(sIsaacActiveTurns, 0, sizeof(sIsaacActiveTurns));
    sIsaacProgressionBattle = boss ? PROGRESSION_BOSS : PROGRESSION_ROOM;
}

// Fin d'un tour (et fin du combat) : chaque Pokémon du joueur sur le terrain compte un tour actif
void IsaacRecordActiveTurn(void)
{
    enum BattlerId battler;
    u32 slot;

    if (sIsaacProgressionBattle == PROGRESSION_NONE)
        return;
    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (!IsOnPlayerSide(battler))
            continue;
        slot = gBattlerPartyIndexes[battler];
        if (slot < PARTY_SIZE && sIsaacActiveTurns[slot] < 255)
            sIsaacActiveTurns[slot]++;
    }
}

// Nombre de types de l'espèce parmi les types principaux du biome courant
static u32 CountBiomeTypes(enum Species species)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[IsaacGetCurrentBiome()];
    enum Type type1 = GetSpeciesType(species, 0), type2 = GetSpeciesType(species, 1);
    u32 count = (type1 == biome->types[0] || type1 == biome->types[1]);

    if (type2 != type1 && (type2 == biome->types[0] || type2 == biome->types[1]))
        count++;
    return count;
}

// Parmi plusieurs évolutions, ne garde que celles qui collent le mieux au biome (le plus de types en commun avec
// lui) ; renvoie leur nombre
static u32 KeepBiomeEvolutions(u16 *choices, u32 count)
{
    u32 best = 0, n = 0, i;

    for (i = 0; i < count; i++)
        best = max(best, CountBiomeTypes(choices[i]));
    for (i = 0; i < count; i++)
        if (CountBiomeTypes(choices[i]) == best)
            choices[n++] = choices[i];
    return n;
}

// Tier d'une attaque pour la moyenne du Pokémon (sIsaacMoveTiers) : 0 pour une attaque situationnelle ou sans tier
// (ignorée)
static u32 GetMoveTierValue(u16 move)
{
    u32 tier = (move < MOVES_COUNT) ? sIsaacMoveTiers[move] : 0;

    return (tier == ISAAC_MOVE_TIER_SITUATIONAL) ? 0 : tier;
}

static bool8 CanSpeciesLearnMove(enum Species species, u16 move)
{
    const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(species);
    u32 i;

    if (CanLearnTeachableMove(species, move))
        return TRUE;
    for (i = 0; learnset[i].move != LEVEL_UP_MOVE_END; i++)
        if (learnset[i].move == move)
            return TRUE;
    return FALSE;
}

// L'attaque est-elle dans un des sets de l'espèce ?
static bool8 IsMoveInSpeciesSets(enum Species species, u16 move)
{
    const struct TrainerMon *set;
    u32 tier, variant, i;

    for (tier = 0; tier < ISAAC_SET_TIERS; tier++)
    {
        for (variant = 0; variant < ISAAC_SET_VARIANTS; variant++)
        {
            set = IsaacGetSet(species, tier, variant);
            if (set == NULL)
                return FALSE;
            for (i = 0; i < MAX_MON_MOVES; i++)
                if (set->moves[i] == move)
                    return TRUE;
        }
    }
    return FALSE;
}

// Attaque de tier supérieur pour le Pokémon, s'il était de l'espèce donnée (celle d'après l'évolution le cas échéant).
// Tier visé : 1 de plus que l'entier le plus proche de la moyenne des tiers de ses attaques (sans les situationnelles ;
// s'il n'a que des attaques situationnelles, le palier de son set plus 1), 4 au plus. Candidates, que l'espèce peut apprendre (niveau, CT, tuteur) et que le Pokémon ne
// connaît pas : les attaques offensives du tier visé dans la catégorie de son build (celle de la majorité de ses
// attaques offensives ; à égalité, sa meilleure stat d'attaque de base), de préférence d'un de ses types ou d'un type
// que ses attaques n'ont pas encore ; les attaques de statut du tier visé ou situationnelles qui sont dans les sets de
// l'espèce (de bonnes attaques pour elle). Tirée au hasard ; MOVE_NONE s'il n'y en a pas.
static u16 PickUpgradeMove(struct Pokemon *mon, enum Species species)
{
    u16 known[MAX_MON_MOVES], move, picked = MOVE_NONE;
    u32 setTier = IsaacGetMonSetTier(mon), sum = 0, count = 0, physical = 0, special = 0, i, tier, target, n, pass;
    enum Type types[2], gem = IsaacGetMonGemType(mon);
    bool8 wantsPhysical, ok;

    types[0] = GetSpeciesType(species, 0);
    types[1] = (gem != TYPE_NONE) ? gem : GetSpeciesType(species, 1);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        known[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
        tier = GetMoveTierValue(known[i]);
        if (tier != 0)
        {
            sum += tier;
            count++;
        }
        if (known[i] != MOVE_NONE && GetMoveCategory(known[i]) == DAMAGE_CATEGORY_PHYSICAL)
            physical++;
        else if (known[i] != MOVE_NONE && GetMoveCategory(known[i]) == DAMAGE_CATEGORY_SPECIAL)
            special++;
    }
    target = (count == 0) ? setTier + 2 : (2 * sum + count) / (2 * count) + 1;
    target = min(target, 4u);
    if (physical != special)
        wantsPhysical = physical > special;
    else
        wantsPhysical = gSpeciesInfo[species].baseAttack >= gSpeciesInfo[species].baseSpAttack;

    for (pass = 0, n = 0; pass < 2 && n == 0; pass++)  // d'abord les types utiles, puis tous
    {
        for (move = 1; move < MOVES_COUNT; move++)
        {
            tier = sIsaacMoveTiers[move];
            if (tier == 0)
                continue;
            for (i = 0; i < MAX_MON_MOVES && known[i] != move; i++)
                ;
            if (i != MAX_MON_MOVES)
                continue;
            if (GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS)
            {
                ok = (tier == target || tier == ISAAC_MOVE_TIER_SITUATIONAL) && IsMoveInSpeciesSets(species, move);
            }
            else
            {
                ok = tier == target && (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL) == wantsPhysical;
                if (ok && pass == 0 && GetMoveType(move) != types[0] && GetMoveType(move) != types[1])
                {
                    for (i = 0; i < MAX_MON_MOVES; i++)  // un type que ses attaques offensives n'ont pas
                        if (known[i] != MOVE_NONE && GetMoveCategory(known[i]) != DAMAGE_CATEGORY_STATUS
                         && GetMoveType(known[i]) == GetMoveType(move))
                            ok = FALSE;
                }
            }
            if (ok && CanSpeciesLearnMove(species, move) && Random() % ++n == 0)
                picked = move;
        }
    }
    return picked;
}

// Après le combat (une seule fois) : évolution ou attaque de tier supérieur (voir plus haut). Renvoie l'espèce de la
// scène d'évolution à lancer (celle du Pokémon pour un simple apprentissage) et sa place dans l'équipe, SPECIES_NONE
// s'il ne se passe rien.
enum Species IsaacPickBattleProgression(bool8 won, u32 *outSlot)
{
    u16 choices[ISAAC_MAX_EVOLUTION_CHOICES], move;
    u32 i, evolver = PARTY_SIZE, active = PARTY_SIZE, count, floor;
    bool8 boss = (sIsaacProgressionBattle == PROGRESSION_BOSS);
    struct Pokemon *mon;
    enum Species species;

    if (sIsaacProgressionBattle == PROGRESSION_NONE)
        return SPECIES_NONE;
    IsaacRecordActiveTurn();  // le dernier tour, qui finit le combat sans passer par la fin de tour
    sIsaacProgressionBattle = PROGRESSION_NONE;
    if (!won)
        return SPECIES_NONE;
    floor = min(max(VarGet(VAR_ISAAC_FLOOR), 1u), ARRAY_COUNT(sProgressionEvolutionPct)) - 1;
    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
    {
        mon = &gParties[B_TRAINER_PLAYER][i];
        if (sIsaacActiveTurns[i] == 0 || GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        if (active == PARTY_SIZE || sIsaacActiveTurns[i] > sIsaacActiveTurns[active])
            active = i;
        if ((evolver == PARTY_SIZE || sIsaacActiveTurns[i] > sIsaacActiveTurns[evolver])
         && IsaacGetEvolutionChoices(GetMonData(mon, MON_DATA_SPECIES), FALSE, choices, ARRAY_COUNT(choices)) != 0)
            evolver = i;
    }
    if (evolver != PARTY_SIZE && (boss || Random() % 100 < sProgressionEvolutionPct[floor]))
    {
        mon = &gParties[B_TRAINER_PLAYER][evolver];
        count = IsaacGetEvolutionChoices(GetMonData(mon, MON_DATA_SPECIES), FALSE, choices, ARRAY_COUNT(choices));
        count = KeepBiomeEvolutions(choices, count);
        species = choices[Random() % count];
        IsaacPrepareEvolutionMoves(mon, species);
        sIsaacEvolutionMoves[MAX_MON_MOVES] = PickUpgradeMove(mon, species);
        sIsaacProgressionScene = PROGRESSION_SCENE_EVOLUTION;
        *outSlot = evolver;
        return species;
    }
    if (active != PARTY_SIZE && (boss || Random() % 100 < sProgressionMovePct[floor]))
    {
        mon = &gParties[B_TRAINER_PLAYER][active];
        species = GetMonData(mon, MON_DATA_SPECIES);
        move = PickUpgradeMove(mon, species);
        if (move != MOVE_NONE)
        {
            memset(sIsaacEvolutionMoves, 0, sizeof(sIsaacEvolutionMoves));
            sIsaacEvolutionMoves[MAX_MON_MOVES] = move;
            sIsaacProgressionScene = PROGRESSION_SCENE_MOVE;
            *outSlot = active;
            return species;
        }
    }
    return SPECIES_NONE;
}

// Début de la scène d'évolution : texte d'annonce, qui explique la progression après le combat. *learnOnly : la scène
// ne fait qu'apprendre une attaque (pas d'animation d'évolution).
const u8 *IsaacGetEvolutionIntroText(bool8 *learnOnly)
{
    u32 scene = sIsaacProgressionScene;

    sIsaacProgressionScene = PROGRESSION_SCENE_NONE;
    *learnOnly = (scene == PROGRESSION_SCENE_MOVE);
    if (scene == PROGRESSION_SCENE_EVOLUTION)
        return COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\p"
                               "To reward its efforts…\pWhat? {STR_VAR_1} is evolving!");
    if (scene == PROGRESSION_SCENE_MOVE)
        return COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\p"
                               "It gained new experience!");
    return gText_PkmnIsEvolving;
}

// ---------------------------------------------------------------------------
// Gemmes de Type
// ---------------------------------------------------------------------------

// Type donné par une Gemme de Type au Pokémon (TYPE_NONE s'il n'en a pas reçu)
enum Type IsaacGetMonGemType(struct Pokemon *mon)
{
    return GetMonData(mon, ISAAC_GEM_TYPE_DATA);
}

// Types du Pokémon avec sa Gemme de Type : elle remplace le second type (s'ajoute à un type simple). Si son espèce a
// déjà ce type (après une évolution), il redevient simple.
void IsaacGetMonTypes(struct Pokemon *mon, enum Type *types)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Type gem = IsaacGetMonGemType(mon);

    types[0] = GetSpeciesType(species, 0);
    types[1] = GetSpeciesType(species, 1);
    if (gem != TYPE_NONE)
        types[1] = gem;
}

// Gemme de Type : FALSE si le Pokémon a déjà ce type
bool8 IsaacUseTypeGem(struct Pokemon *mon, enum Type type)
{
    enum Type types[2];
    u32 value = type;

    IsaacGetMonTypes(mon, types);
    if (types[0] == type || types[1] == type)
        return FALSE;
    SetMonData(mon, ISAAC_GEM_TYPE_DATA, &value);
    return TRUE;
}

// En combat, après que le jeu a remis les types de l'espèce (entrée en combat, Méga-Évolution...) : le second type
// redevient celui de la Gemme de Type
void IsaacApplyBattlerGemType(enum BattlerId battler)
{
    struct Pokemon *mon = GetBattlerMon(battler);
    enum Type gem;

    if (mon == NULL || !IsPlayerPartyMon(mon))
        return;
    gem = IsaacGetMonGemType(mon);
    if (gem != TYPE_NONE)
        gBattleMons[battler].types[1] = gem;
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

// Avant la scène d'évolution de mon vers target (Pierre Évolution, progression par les combats)
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
        memcpy(sIsaacEvolutionMoves, set->moves, sizeof(set->moves));
}

// Pendant la scène d'évolution, à la place de MonTryLearningNewMoveEvolution : attaque suivante à proposer
// (dans gMoveToLearn), avec le résultat de GiveMoveToMon ; MOVE_NONE quand il n'y en a plus
u16 IsaacTryLearningEvolutionMove(struct Pokemon *mon, bool8 firstMove)
{
    if (firstMove)
        sIsaacEvolutionMoveIndex = 0;
    while (sIsaacEvolutionMoveIndex < ARRAY_COUNT(sIsaacEvolutionMoves))
    {
        gMoveToLearn = sIsaacEvolutionMoves[sIsaacEvolutionMoveIndex++];
        if (gMoveToLearn != MOVE_NONE && !MonKnowsMove(mon, gMoveToLearn))
            return GiveMoveToMon(mon, gMoveToLearn);
    }
    memset(sIsaacEvolutionMoves, 0, sizeof(sIsaacEvolutionMoves));
    return MOVE_NONE;
}

// Le Pokémon prend les attaques du set (palier tier, variante variant) de son espèce. Seul le moveset change : IVs,
// EVs, nature, talent et objet tenu restent les mêmes. FALSE si l'espèce n'a pas ce set.
static bool8 SetMovesFromSet(struct Pokemon *mon, u32 tier, u32 variant)
{
    const struct TrainerMon *set = IsaacGetSet(GetMonData(mon, MON_DATA_SPECIES), tier, variant);
    u32 i, value;

    if (set == NULL)
        return FALSE;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        RemoveMonPPBonus(mon, i);
        SetMonMoveSlot(mon, set->moves[i], i);  // PP au complet (MOVE_NONE : place vide)
    }
    value = tier + 1;
    SetMonData(mon, ISAAC_TIER_DATA, &value);
    value = variant + 1;
    SetMonData(mon, ISAAC_VARIANT_DATA, &value);
    return TRUE;
}

// Super CT (ITEM_ABILITY_TRAINER) : le Pokémon prend les attaques du palier suivant de son espèce, même variante (FALSE s'il a déjà
// le meilleur)
bool8 IsaacUpgradeSet(struct Pokemon *mon)
{
    u32 tier = IsaacGetMonSetTier(mon);

    if (tier + 1 >= ISAAC_SET_TIERS)
        return FALSE;
    return SetMovesFromSet(mon, tier + 1, IsaacGetMonSetVariant(mon));
}

// Dé Maudit : le Pokémon prend les attaques d'un set tiré au hasard, un palier plus bas, le même ou un plus haut (dans
// les limites), en variante A ou B. Renvoie le palier gagné (-1, 0 ou 1) ; FALSE dans *ok si l'espèce n'a pas le set.
s32 IsaacRollCursedDie(struct Pokemon *mon, bool8 *ok)
{
    u32 roll = Random(), tier = IsaacGetMonSetTier(mon), newTier;

    newTier = tier + roll % 3;  // tier - 1, tier ou tier + 1, décalé d'un
    newTier = (newTier == 0) ? 0 : min(newTier - 1, (u32)ISAAC_SET_TIERS - 1);
    *ok = SetMovesFromSet(mon, newTier, (roll >> 8) % ISAAC_SET_VARIANTS);
    return (s32)newTier - (s32)tier;
}

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

// L'espèce a-t-elle un type super efficace contre un des types du biome ?
static bool32 CountersBiome(u16 species, const struct IsaacBiome *biome)
{
    u32 a, b;

    for (a = 0; a < 2; a++)
        for (b = 0; b < 2; b++)
            if (GetTypeModifier(GetSpeciesType(species, a), biome->types[b]) > UQ_4_12(1.0))
                return TRUE;
    return FALSE;
}

// Tirage des choix du Montagnard : il ne dépend que de la graine de la prochaine descente (VAR_ISAAC_BIOME_ORDER,
// tirée avant l'écran de choix par IsaacBufferFirstBiome), pour que les choix restent les mêmes si on lui reparle
static u32 StarterRandom(u32 *state)
{
    *state = *state * 1103515245 + 12345;
    return *state >> 16;
}

// Choix du Montagnard du hub : le Pokémon proposé n° index (0 à ISAAC_STARTER_CHOICES - 1). Les
// ISAAC_STARTER_CHOICES / 2 premiers sont de catégorie 1, les autres de catégorie 2 (espèces avec des sets),
// sans type en commun entre eux, de préférence d'un type super efficace contre le premier biome de la descente
// (CountersBiome), avec le set 2 (variante au hasard). Les espèces sont tirées à l'index 0. Mêmes choix tant que
// la descente n'a pas commencé (StarterRandom).
void IsaacCreateStarterChoice(struct Pokemon *mon, u32 index)
{
    static EWRAM_DATA u16 sStarterSpecies[ISAAC_STARTER_CHOICES] = {0};
    u16 candidates[ARRAY_COUNT(sIsaacRoster)];
    u32 count = 0, i, j, tmp, state, seed, pass;

    if (index >= ISAAC_STARTER_CHOICES)
        return;
    state = VarGet(VAR_ISAAC_BIOME_ORDER) * 2654435761u ^ 0x57A47E25;
    if (index == 0)
    {
        const struct IsaacBiome *biome = &gIsaacBiomes[IsaacGetFirstBiome()];
        u16 picked[ISAAC_STARTER_CHOICES];
        u32 chosen = 0, perCategory[2] = {0};

        for (i = 0; i < ARRAY_COUNT(sIsaacRoster); i++)
            if (IsaacGetSpeciesCategory(sIsaacRoster[i]) <= ISAAC_CATEGORY_2 && IsaacSpeciesHasSet(sIsaacRoster[i]))
                candidates[count++] = sIsaacRoster[i];
        for (i = 0; i < count; i++)  // ordre au hasard
        {
            j = i + StarterRandom(&state) % (count - i);
            SWAP(candidates[i], candidates[j], tmp);
        }
        // d'abord les espèces qui contrent le premier biome, puis les autres, jusqu'à ISAAC_STARTER_CHOICES / 2 par
        // catégorie, en gardant seulement les espèces sans type commun avec celles déjà prises
        for (pass = 0; pass < 2; pass++)
        {
            for (i = 0; i < count && chosen < ISAAC_STARTER_CHOICES; i++)
            {
                u32 cat;

                if (candidates[i] == SPECIES_NONE)  // déjà pris
                    continue;
                cat = IsaacGetSpeciesCategory(candidates[i]) - ISAAC_CATEGORY_1;
                if (perCategory[cat] >= ISAAC_STARTER_CHOICES / 2
                 || (pass == 0 && !CountersBiome(candidates[i], biome))
                 || SharesTypeWithStarters(candidates[i], picked, chosen))
                    continue;
                picked[chosen++] = candidates[i];
                perCategory[cat]++;
                candidates[i] = SPECIES_NONE;
            }
        }
        // catégorie 1 d'abord, puis catégorie 2
        for (i = 0, j = 0; i < 2; i++)
            for (tmp = 0; tmp < chosen; tmp++)
                if (IsaacGetSpeciesCategory(picked[tmp]) == ISAAC_CATEGORY_1 + i)
                    sStarterSpecies[j++] = picked[tmp];
    }
    state ^= (index + 1) * 0x9E3779B9;
    seed = StarterRandom(&state) << 16;
    seed |= StarterRandom(&state);
    IsaacCreateMonFromSet(mon, sStarterSpecies[index], 1, StarterRandom(&state) % ISAAC_SET_VARIANTS, seed);  // set 2
}
