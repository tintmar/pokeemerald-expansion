#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_message.h"
#include "battle_scripts.h"
#include "battle_z_move.h"
#include "data.h"
#include "event_data.h"
#include "item.h"
#include "isaac_dungeon.h"
#include "isaac_pokemon.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
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
    if (species > SPECIES_NONE && species < NUM_SPECIES && sIsaacSetIndex[species] == 0 && gSpeciesInfo[species].isMegaEvolution)
        species = GET_BASE_SPECIES_ID(species);  // Méga-Évolution définitive (Cristal Méga) : les sets de sa forme de base
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
// rares en profondeur (sEvolutionStoneWeights : moitié à l'étage 4, quart à l'étage 5, absentes à partir de l'étage 6 ;
// retirées pour l'instant : EVOLUTION_STONE_ENABLED, SUPER_EVOLUTION_STONE_ENABLED),
// et la Super CT, de plus en plus rare (sAbilityTrainerWeights), rare à partir de l'étage 4. Dans la boutique (shop),
// pas de Pierre Évolution après SHOP_EVOLUTION_STONE_MAX_FLOOR. La Pièce Porte-Bonheur est surtout au début
// (sLuckyCoinWeights, jamais si on l'a déjà), la Trousse de Secours surtout à la fin (sFirstAidKitWeights, retirée pour
// l'instant : FIRST_AID_KIT_ENABLED) ; la Pierre Prisme un peu moins courante (PRISM_STONE_WEIGHT), le Cœur Doré peu courant
// (GOLDEN_HEART_WEIGHT). La Master Ball est peu courante (MASTER_BALL_WEIGHT). Le Cristal Méga n'apparaît que si un
// Pokémon de l'équipe a sa Méga-Gemme (tenue ou dans le sac).
// Les 6 boosts de stat partagent une seule entrée (puis la stat est tirée), pour ne pas être 6 fois plus courants, un
// peu plus fréquente en profondeur (sStatBoostWeights) ; de même pour les Gemmes de Type (puis le type est tiré ;
// retirées pour l'instant : TYPE_GEM_WEIGHT à 0). Le Cœur Doré n'apparaît qu'à partir de GOLDEN_HEART_MIN_FLOOR.
// La Méga-Évolution (ITEM_MEGA_RING dans la liste) n'apparaît que si un Pokémon de l'équipe peut méga-évoluer avec
// une Méga-Gemme qu'on n'a pas encore : on reçoit l'une d'elles au hasard (et l'Anneau Méga avec, voir AddBagItem).
#define SPECIAL_ITEM_WEIGHT  4
#define OMNIBOOST_WEIGHT     2
#define TYPE_GEM_WEIGHT      0
#define GOLDEN_HEART_WEIGHT  2
#define PRISM_STONE_WEIGHT   3
#define Z_TM_WEIGHT          2
#define MASTER_BALL_WEIGHT   2
#define GOLDEN_HEART_MIN_FLOOR  4
#define FIRST_AID_KIT_ENABLED   FALSE
#define EVOLUTION_STONE_ENABLED        FALSE
#define SUPER_EVOLUTION_STONE_ENABLED  FALSE
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

static bool8 PartyHasPermanentMegaEvolution(void);

u16 IsaacPickSpecialItem(u32 roll, bool8 shop)
{
    static const u16 sSpecialItems[] =
    {
        ITEM_EVOLUTION_STONE, ITEM_SUPER_EVOLUTION_STONE, ITEM_OMNIBOOST, ITEM_ABILITY_TRAINER, ITEM_BOOST_HP,
        ITEM_BRILLIANT_STONE, ITEM_ISAAC_GEM_FIRST, ITEM_GOLDEN_HEART, ITEM_MEGA_RING, ITEM_LUCKY_COIN,
        ITEM_FIRST_AID_KIT, ITEM_PRISM_STONE, ITEM_Z_TM, ITEM_MASTER_BALL, ITEM_GLASS_CROWN, ITEM_TEAMWORK_TOTEM,
        ITEM_MEGA_CRYSTAL,
    };
    static const u8 sEvolutionStoneWeights[] = { 4, 4, 4, 2, 1, 0 };  // étages 1 à 6 (et au-delà)
    static const u8 sAbilityTrainerWeights[] = { 4, 3, 2, 1, 1, 1 };
    static const u8 sLuckyCoinWeights[] = { 4, 3, 3, 2, 1, 0 };
    static const u8 sFirstAidKitWeights[] = { 1, 1, 2, 3, 4, 5 };
    static const u8 sStatBoostWeights[] = { 4, 4, 4, 5, 6, 6 };
    u32 floor = min(max(VarGet(VAR_ISAAC_FLOOR), 1u), ARRAY_COUNT(sEvolutionStoneWeights));
    u32 weights[ARRAY_COUNT(sSpecialItems)], total = 0, pick, i;
    u16 item = sSpecialItems[0];
    u16 megaStones[ISAAC_MAX_MEGA_STONES];
    u32 megaStoneCount = GetMissingPartyMegaStones(megaStones);

    for (i = 0; i < ARRAY_COUNT(sSpecialItems); i++)
    {
        if (sSpecialItems[i] == ITEM_EVOLUTION_STONE && !EVOLUTION_STONE_ENABLED)
            weights[i] = 0;
        else if (sSpecialItems[i] == ITEM_SUPER_EVOLUTION_STONE && !SUPER_EVOLUTION_STONE_ENABLED)
            weights[i] = 0;
        else if (sSpecialItems[i] == ITEM_EVOLUTION_STONE || sSpecialItems[i] == ITEM_SUPER_EVOLUTION_STONE)
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
        else if (sSpecialItems[i] == ITEM_MASTER_BALL)
            weights[i] = MASTER_BALL_WEIGHT;
        else if (sSpecialItems[i] == ITEM_MEGA_CRYSTAL)
            weights[i] = PartyHasPermanentMegaEvolution() ? SPECIAL_ITEM_WEIGHT : 0;
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
#define ISAAC_GLASS_CROWN_BIT  (1 << 6)  // Couronne de Verre (toutes les stats +ISAAC_GLASS_CROWN_PCT %)
#define ISAAC_GLASS_CROWN_PCT  20
#define ISAAC_BLOOD_PACT_PCT           20  // Attaque, Attaque Spéciale et Vitesse
#define ISAAC_BLOOD_PACT_HP_MALUS_PCT  25
// Pierre brillante : stat boostée + 1 (0 : pas encore utilisée), dans la stat de concours Robustesse
#define ISAAC_BRILLIANT_DATA   MON_DATA_TOUGH
#define ISAAC_BRILLIANT_PCT    20
// Totem d'Entraide : valeur ISAAC_TEAMWORK_TOTEM dans la même stat (jamais une stat + 1), +ISAAC_TEAMWORK_TOTEM_PCT % à
// la meilleure stat (hors PV) à chaque entrée en combat en relais
#define ISAAC_TEAMWORK_TOTEM      0x80
#define ISAAC_TEAMWORK_TOTEM_PCT  30

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
    if (GetMonData(mon, ISAAC_STAT_BOOST_DATA) & ISAAC_GLASS_CROWN_BIT)
        value = value * (100 + ISAAC_GLASS_CROWN_PCT) / 100;
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

// L'objet donne-t-il le boost d'un Pokémon (un seul par Pokémon, voir IsaacHasBoost) ?
bool8 IsaacIsBoostItem(u16 item)
{
    return (item >= ITEM_BOOST_HP && item < ITEM_BOOST_HP + NUM_STATS) || item == ITEM_OMNIBOOST
        || item == ITEM_BRILLIANT_STONE || item == ITEM_BLOOD_PACT || item == ITEM_GLASS_CROWN
        || item == ITEM_TEAMWORK_TOTEM;
}

// Menu de l'équipe, objet de boost : le Pokémon peut-il le recevoir (pas un Œuf, pas déjà de boost ; la Couronne de
// Verre, pas sur un Pokémon K.O.) ?
bool8 IsaacCanReceiveBoost(struct Pokemon *mon, u16 item)
{
    if (GetMonData(mon, MON_DATA_IS_EGG) || IsaacHasBoost(mon))
        return FALSE;
    return item != ITEM_GLASS_CROWN || GetMonData(mon, MON_DATA_HP) != 0;
}

// Nom du boost du Pokémon pour l'écran de résumé (emplacement BOOST, à la place des rubans), NULL s'il n'en a pas
const u8 *IsaacGetBoostName(struct Pokemon *mon)
{
    static const u8 *const sStatBoostNames[NUM_STATS] =
    {
        [STAT_HP]    = COMPOUND_STRING("HP +25%"),
        [STAT_ATK]   = COMPOUND_STRING("ATTACK +25%"),
        [STAT_DEF]   = COMPOUND_STRING("DEFENSE +25%"),
        [STAT_SPEED] = COMPOUND_STRING("SPEED +25%"),
        [STAT_SPATK] = COMPOUND_STRING("SP. ATK +25%"),
        [STAT_SPDEF] = COMPOUND_STRING("SP. DEF +25%"),
    };
    u32 boosts = GetMonData(mon, ISAAC_STAT_BOOST_DATA), stat;

    if (boosts & ISAAC_GLASS_CROWN_BIT)
        return COMPOUND_STRING("GLASS CROWN");
    if (boosts & ISAAC_BLOOD_PACT_BIT)
        return COMPOUND_STRING("BLOOD PACT");
    for (stat = 0; stat < NUM_STATS; stat++)
        if (boosts & (1 << stat))
            return sStatBoostNames[stat];
    if (GetMonData(mon, ISAAC_BRILLIANT_DATA) == ISAAC_TEAMWORK_TOTEM)
        return COMPOUND_STRING("TEAMWORK TOTEM");
    if (GetMonData(mon, ISAAC_BRILLIANT_DATA) != 0)
        return COMPOUND_STRING("BRILLIANT");
    if (GetMonData(mon, ISAAC_OMNIBOOST_DATA) != 0)
        return COMPOUND_STRING("OMNIBOOST");
    return NULL;
}

// Le Pokémon a-t-il déjà reçu un boost (un seul par Pokémon) : boost de stat, Pierre brillante, Omniboost, Pacte de
// Sang, Couronne de Verre ou Totem d'Entraide ?
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

// Couronne de Verre : toutes les stats +ISAAC_GLASS_CROWN_PCT %, pour toujours. FALSE si le Pokémon a déjà un boost.
bool8 IsaacUseGlassCrown(struct Pokemon *mon)
{
    u32 boosts = ISAAC_GLASS_CROWN_BIT;

    if (IsaacHasBoost(mon))
        return FALSE;
    SetMonData(mon, ISAAC_STAT_BOOST_DATA, &boosts);
    CalculateMonStats(mon);
    return TRUE;
}

// Pokémon couronné tombé K.O. : il ne peut plus être relevé (il sera relâché à la fin du combat)
bool8 IsaacIsFallenCrown(struct Pokemon *mon)
{
    return (GetMonData(mon, ISAAC_STAT_BOOST_DATA) & ISAAC_GLASS_CROWN_BIT) && GetMonData(mon, MON_DATA_HP) == 0
        && !GetMonData(mon, MON_DATA_IS_EGG);
}

// Fin de combat : chaque Pokémon couronné K.O. est relâché (son objet tenu revient au sac), et laisse sa Master Ball.
// Le message et la Master Ball viennent ensuite sur le terrain (VAR_ISAAC_FALLEN_CROWN : espèce du dernier relâché, et
// nombre de relâchés à partir du bit ISAAC_FALLEN_CROWN_COUNT_SHIFT, VAR_ISAAC_FALLEN_CROWN_PENDING ; voir
// IsaacTakeFallenCrown)
#define ISAAC_FALLEN_CROWN_COUNT_SHIFT  12

void IsaacReleaseFallenCrowns(void)
{
    u32 i, count = VarGet(VAR_ISAAC_FALLEN_CROWN) >> ISAAC_FALLEN_CROWN_COUNT_SHIFT;
    u16 species = VarGet(VAR_ISAAC_FALLEN_CROWN) & ((1 << ISAAC_FALLEN_CROWN_COUNT_SHIFT) - 1);

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        u16 heldItem;

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || !IsaacIsFallenCrown(mon))
            continue;
        species = GetMonData(mon, MON_DATA_SPECIES);
        heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
        if (heldItem != ITEM_NONE)
            AddBagItem(heldItem, 1);
        ZeroMonData(mon);
        count++;
    }
    if (count == 0)
        return;
    CompactPartySlots();
    CalculatePlayerPartyCount();
    VarSet(VAR_ISAAC_FALLEN_CROWN, species | (min(count, 7u) << ISAAC_FALLEN_CROWN_COUNT_SHIFT));
    VarSet(VAR_ISAAC_FALLEN_CROWN_PENDING, 1);
}

// Sur le terrain, après le combat : espèce du Pokémon couronné relâché dans gStringVar1 et nombre de Master Balls
// dans VAR_0x8005 ; la variable est remise à zéro
void IsaacTakeFallenCrown(void)
{
    u32 value = VarGet(VAR_ISAAC_FALLEN_CROWN);

    StringCopy(gStringVar1, GetSpeciesName(value & ((1 << ISAAC_FALLEN_CROWN_COUNT_SHIFT) - 1)));
    gSpecialVar_0x8005 = max(value >> ISAAC_FALLEN_CROWN_COUNT_SHIFT, 1u);
    VarSet(VAR_ISAAC_FALLEN_CROWN, 0);
}

// Totem d'Entraide : FALSE si le Pokémon a déjà un boost
bool8 IsaacUseTeamworkTotem(struct Pokemon *mon)
{
    u32 value = ISAAC_TEAMWORK_TOTEM;

    if (IsaacHasBoost(mon))
        return FALSE;
    SetMonData(mon, ISAAC_BRILLIANT_DATA, &value);
    return TRUE;
}

// Combattants dont le Totem d'Entraide est actif (un bit par combattant) : il faut le réappliquer quand le jeu recalcule
// leurs stats (Méga-Évolution, changement de forme...)
static EWRAM_DATA u8 sIsaacTotemActiveBattlers = 0;
// Combattants dont le message du Totem (animation de hausse de stat et texte) est à afficher, et leur stat boostée
static EWRAM_DATA u8 sIsaacTotemMessageBattlers = 0;
static EWRAM_DATA u8 sIsaacTotemStat[MAX_BATTLERS_COUNT] = {0};
// Pokémon de l'équipe (un bit par place) dont le Totem a déjà servi dans ce combat : seulement à la première entrée
static EWRAM_DATA u8 sIsaacTotemUsedSlots = 0;

// +ISAAC_TEAMWORK_TOTEM_PCT % à la meilleure stat de combat (hors PV) du combattant ; renvoie la stat (STAT_*)
static u32 ApplyTotemBoost(enum BattlerId battler)
{
    // dans l'ordre de STAT_ATK à STAT_SPDEF
    u16 *stats[] = { &gBattleMons[battler].attack, &gBattleMons[battler].defense, &gBattleMons[battler].speed,
                     &gBattleMons[battler].spAttack, &gBattleMons[battler].spDefense };
    u32 best = 0, i;

    for (i = 1; i < ARRAY_COUNT(stats); i++)
        if (*stats[i] > *stats[best])
            best = i;
    *stats[best] = *stats[best] * (100 + ISAAC_TEAMWORK_TOTEM_PCT) / 100;
    return STAT_ATK + best;
}

// Entrée en combat en relais (pas au début du combat) : avec le Totem d'Entraide, sa meilleure stat (hors PV) prend
// +ISAAC_TEAMWORK_TOTEM_PCT % jusqu'à ce qu'il sorte, seulement à sa première entrée en relais du combat
void IsaacApplySwitchInTotem(enum BattlerId battler)
{
    struct Pokemon *mon = GetBattlerMon(battler);

    sIsaacTotemActiveBattlers &= ~(1 << battler);
    sIsaacTotemMessageBattlers &= ~(1 << battler);
    if (mon == NULL || !IsPlayerPartyMon(mon) || GetMonData(mon, ISAAC_BRILLIANT_DATA) != ISAAC_TEAMWORK_TOTEM
     || (sIsaacTotemUsedSlots & (1 << gBattlerPartyIndexes[battler])))
        return;
    sIsaacTotemUsedSlots |= 1 << gBattlerPartyIndexes[battler];
    sIsaacTotemActiveBattlers |= 1 << battler;
    sIsaacTotemMessageBattlers |= 1 << battler;
    sIsaacTotemStat[battler] = ApplyTotemBoost(battler);
}

// Effets d'entrée en combat (battle_switch_in.c) : animation de hausse de la stat boostée par le Totem d'Entraide et
// message, une fois par entrée en relais. TRUE si le script est lancé.
bool8 IsaacTryTeamworkTotemMessage(enum BattlerId battler)
{
    if (!(sIsaacTotemMessageBattlers & (1 << battler)))
        return FALSE;
    sIsaacTotemMessageBattlers &= ~(1 << battler);
    gBattleScripting.battler = battler;
    gBattleScripting.animArg1 = STAT_ANIM_PLUS1 + sIsaacTotemStat[battler];
    gBattleScripting.animArg2 = 0;
    PREPARE_STAT_BUFFER(gBattleTextBuff1, sIsaacTotemStat[battler]);
    BattleScriptCall(BattleScript_IsaacTeamworkTotem);
    return TRUE;
}

// Début du combat : le Pokémon en tête n'entre pas en relais, pas de Totem actif ; aucun Totem n'a encore servi
void IsaacClearSwitchInTotem(enum BattlerId battler)
{
    sIsaacTotemUsedSlots = 0;
    sIsaacTotemActiveBattlers &= ~(1 << battler);
    sIsaacTotemMessageBattlers &= ~(1 << battler);
}

// Stats de combat recalculées depuis le Pokémon (RecalcBattlerStats) : le Totem actif s'applique de nouveau
void IsaacReapplySwitchInTotem(enum BattlerId battler)
{
    if (sIsaacTotemActiveBattlers & (1 << battler))
        sIsaacTotemStat[battler] = ApplyTotemBoost(battler);
}

// Méga-Évolution que le Pokémon peut faire pour toujours : sa Méga-Gemme, tenue par lui ou dans le sac (*held : tenue),
// et l'espèce Méga ; FALSE s'il n'en a pas
static bool8 FindPermanentMegaEvolution(struct Pokemon *mon, u16 *stone, enum Species *mega, bool8 *held)
{
    const struct FormChange *formChanges = GetSpeciesFormChanges(GetMonData(mon, MON_DATA_SPECIES));
    u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
    u32 i, pass;

    if (formChanges == NULL || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    for (pass = 0; pass < 2; pass++)  // d'abord la Méga-Gemme qu'il tient, puis celles du sac
    {
        for (i = 0; formChanges[i].method != FORM_CHANGE_TERMINATOR; i++)
        {
            if (formChanges[i].method != FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM)
                continue;
            if (pass == 0 ? heldItem != formChanges[i].param1 : !CheckBagHasItem(formChanges[i].param1, 1))
                continue;
            *stone = formChanges[i].param1;
            *mega = formChanges[i].targetSpecies;
            *held = (pass == 0);
            return TRUE;
        }
    }
    return FALSE;
}

// Un Pokémon de l'équipe peut-il méga-évoluer pour toujours (Cristal Méga) ?
static bool8 PartyHasPermanentMegaEvolution(void)
{
    u16 stone;
    enum Species mega;
    bool8 held;
    u32 i;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
        if (FindPermanentMegaEvolution(&gParties[B_TRAINER_PLAYER][i], &stone, &mega, &held))
            return TRUE;
    return FALSE;
}

// Cristal Méga : le Pokémon prend sa forme Méga pour toujours, sa Méga-Gemme est consommée. FALSE s'il ne peut pas.
bool8 IsaacUseMegaCrystal(struct Pokemon *mon)
{
    u16 stone, none = ITEM_NONE;
    enum Species mega;
    bool8 held;

    if (!FindPermanentMegaEvolution(mon, &stone, &mega, &held))
        return FALSE;
    if (held)
        SetMonData(mon, MON_DATA_HELD_ITEM, &none);
    else
        RemoveBagItem(stone, 1);
    SetMonData(mon, MON_DATA_SPECIES, &mega);
    CalculateMonStats(mon);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(mega), FLAG_SET_SEEN);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(mega), FLAG_SET_CAUGHT);
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
// (environ PROGRESSION_EVOLUTION_PCT % de chances, voir l'anti-malchance plus bas, toujours contre le boss ; s'il a plusieurs évolutions, l'une
// de celles qui collent le mieux au biome), et apprend en même temps une attaque de tier supérieur pour sa nouvelle
// espèce (PickUpgradeMove). Sinon, avec environ PROGRESSION_MOVE_PCT % de chances (PROGRESSION_MOVE_NO_EVOLUTION_PCT %
// si personne ne peut évoluer) (toujours contre le boss), le Pokémon qui a le
// plus de tours actifs apprend une attaque de tier supérieur, dans l'écran de l'évolution mais sans l'animation ; si la
// moyenne de ses attaques est déjà de tier 4, il gagne à la place un cran d'EVs (IncreaseMonEvs), avec un simple
// message dans la même scène. Sinon, avec environ PROGRESSION_EVS_PCT % de chances, ce même Pokémon gagne un cran d'EVs, ou, si
// ses EVs sont au maximum, que son stade est final, que ses attaques sont de tier moyen 4 et qu'il n'a pas déjà de boost
// (IsaacHasBoost), un boost de stat au hasard. Sinon, rien. À égalité, le premier de l'équipe.
#define PROGRESSION_EVOLUTION_PCT  25  // à tous les étages
#define PROGRESSION_MOVE_PCT       25
#define PROGRESSION_EVS_PCT        20  // après l'échec des deux autres tirages (EVs ou boost)

// Anti-malchance (distribution pseudo-aléatoire, comme les coups critiques de Dota) : la chance d'un tirage est de
// n x pas (en pour mille) au n-ième essai depuis le dernier succès, avec un pas choisi pour que le taux moyen reste
// celui de PROGRESSION_*_PCT. Les séries d'échecs sont bien plus courtes : à 25 %, 8,5 % au premier essai, 25,5 % au
// troisième, 51 % au sixième, succès garanti au douzième ; à 20 %, garanti au dix-huitième. Les échecs sont comptés
// par tirage (évolution, attaque, EVs) dans VAR_ISAAC_PROGRESSION_PITY (4 bits chacun), remis à zéro à chaque
// descente. Seuls les combats contre les dresseurs de salle comptent : jamais celui du boss, aux effets garantis.
enum
{
    PITY_EVOLUTION,
    PITY_MOVE,
    PITY_EVS,
};

#define PITY_BITS  4
#define PITY_MAX   ((1 << PITY_BITS) - 1)

static const u16 sPityStepPermille[] =
{
    [PITY_EVOLUTION] = 85,  // ~ 25 %
    [PITY_MOVE]      = 85,  // ~ 25 %
    [PITY_EVS]       = 56,  // ~ 20 %
};

static u32 GetPityFails(u32 kind)
{
    return (VarGet(VAR_ISAAC_PROGRESSION_PITY) >> (kind * PITY_BITS)) & PITY_MAX;
}

// Personne ne peut évoluer : l'attaque prend la place de l'évolution, environ PROGRESSION_MOVE_NO_EVOLUTION_PCT %
#define PROGRESSION_MOVE_NO_EVOLUTION_PCT  40
#define PITY_MOVE_NO_EVOLUTION_STEP        202  // ~ 40 %

// Tirage du type kind avec un pas (pour mille) donné, sans le compter (voir RecordPity)
static bool8 RollPityStep(u32 kind, u32 step)
{
    return Random() % 1000 < step * (GetPityFails(kind) + 1);
}

static bool8 RollPity(u32 kind)
{
    return RollPityStep(kind, sPityStepPermille[kind]);
}

// Résultat du tirage : un succès remet le compteur d'échecs à zéro. Un tirage n'a lieu (et ne compte) que si le
// Pokémon a quelque chose à gagner : personne qui puisse évoluer, ou attaques et EVs déjà au maximum, ne fait pas
// monter le compteur.
static void RecordPity(u32 kind, bool8 success)
{
    u32 pity = VarGet(VAR_ISAAC_PROGRESSION_PITY), fails = success ? 0 : min(GetPityFails(kind) + 1, (u32)PITY_MAX);

    pity &= ~(PITY_MAX << (kind * PITY_BITS));
    VarSet(VAR_ISAAC_PROGRESSION_PITY, pity | (fails << (kind * PITY_BITS)));
}

void IsaacResetProgressionPity(void)
{
    VarSet(VAR_ISAAC_PROGRESSION_PITY, 0);
}

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
    PROGRESSION_SCENE_EVS,
    PROGRESSION_SCENE_BOOST,
};

static EWRAM_DATA u8 sIsaacActiveTurns[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sIsaacProgressionBattle = PROGRESSION_NONE;
static EWRAM_DATA u8 sIsaacBoostedStat = 0;  // stat du boost de la scène PROGRESSION_SCENE_BOOST
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

// Un cran d'EVs : chaque stat du dernier set de l'espèce (même variante), qui est le maximum, gagne un quart de son EV
// maximal, sans le dépasser. FALSE si le Pokémon a déjà tous ses EVs au maximum de ce set.
static bool8 IncreaseMonEvs(struct Pokemon *mon)
{
    // ordre des EVs d'un set (TRAINER_PARTY_EVS) : PV, Att, Déf, AttSp, DéfSp, Vit
    static const u8 sEvData[] = { MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV };
    const struct TrainerMon *set = IsaacGetSet(GetMonData(mon, MON_DATA_SPECIES), ISAAC_SET_TIERS - 1, IsaacGetMonSetVariant(mon));
    bool8 changed = FALSE;
    u32 i, current, max, value;

    if (set == NULL || set->ev == NULL)
        return FALSE;
    for (i = 0; i < ARRAY_COUNT(sEvData); i++)
    {
        max = set->ev[i];
        current = GetMonData(mon, sEvData[i]);
        if (current >= max)
            continue;
        value = min(max, current + max / 4);
        SetMonData(mon, sEvData[i], &value);
        changed = TRUE;
    }
    if (changed)
        CalculateMonStats(mon);
    return changed;
}

// Le Pokémon peut-il encore gagner un cran d'EVs (voir IncreaseMonEvs) ?
static bool8 CanIncreaseMonEvs(struct Pokemon *mon)
{
    static const u8 sEvData[] = { MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV };
    const struct TrainerMon *set = IsaacGetSet(GetMonData(mon, MON_DATA_SPECIES), ISAAC_SET_TIERS - 1, IsaacGetMonSetVariant(mon));
    u32 i;

    if (set == NULL || set->ev == NULL)
        return FALSE;
    for (i = 0; i < ARRAY_COUNT(sEvData); i++)
        if (GetMonData(mon, sEvData[i]) < set->ev[i])
            return TRUE;
    return FALSE;
}

// Tier d'une attaque pour la moyenne du Pokémon (sIsaacMoveTiers) : 0 pour une attaque situationnelle ou sans tier
// (ignorée) ; une attaque Z (CT Z) compte comme une attaque de tier 4
static u32 GetMoveTierValue(u16 move)
{
    u32 tier = (move < MOVES_COUNT) ? sIsaacMoveTiers[move] : 0;

    if (IsZMove(move))
        return 4;

    return (tier == ISAAC_MOVE_TIER_SITUATIONAL) ? 0 : tier;
}

// Moyenne arrondie des tiers de ses attaques (sans les situationnelles), 0 s'il n'en a aucune
static u32 GetMonAverageMoveTier(struct Pokemon *mon)
{
    u32 sum = 0, count = 0, i, tier;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        tier = GetMoveTierValue(GetMonData(mon, MON_DATA_MOVE1 + i));
        if (tier != 0)
        {
            sum += tier;
            count++;
        }
    }
    return count == 0 ? 0 : (2 * sum + count) / (2 * count);
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
// s'il n'a que des attaques situationnelles, le palier de son set plus 1), 4 au plus. Coup de chance : une attaque d'un
// tier de plus encore (UPGRADE_JACKPOT_RARITY fois plus rare). Candidates, que l'espèce peut apprendre (niveau, CT,
// tuteur) et que le Pokémon ne connaît pas, jamais une attaque ISAAC_MOVE_ROLE_COMBO (recharge, tour de charge,
// Mitra-Poing...) :
//  - les attaques offensives dans la catégorie de son orientation (IsMonPhysicallyOriented : sa plus haute stat
//    d'attaque réelle), ou dans les deux s'il est mixte (stats à moins de ISAAC_MIXED_GAP_PCT % l'une de l'autre) ;
//  - les attaques de statut du tier visé ou situationnelles qui sont dans les sets de l'espèce (de bonnes attaques pour
//    elle), plus les boosts de tier 4 de son build même hors de ses sets (Danse Lames, Machination...), sans seconde
//    attaque d'un rôle qu'il a déjà (deux soins, deux boosts...). Un Pokémon offensif (au moins UPGRADE_OFFENSIVE_MOVES
//    attaques offensives) ne prend qu'un boost de son build ou un soin (pas Repos).
// Chaque candidate reçoit un score d'intérêt (ScoreOffensiveUpgrade, ScoreStatusUpgrade), et le tirage se fait avec un
// poids égal au carré du score : les attaques qui font vraiment plaisir sortent souvent, les autres restent possibles.
// Le poids des attaques de coverage est divisé par le nombre de candidates de leur type (x 2 / (n + 1)), pour qu'un type
// riche en attaques (Combat, Normal...) n'écrase pas les autres ; pas celui des attaques de son type, qu'on veut voir
// souvent.
// Une attaque offensive sans intérêt (un type qu'il a déjà, sans gain de tier) n'est proposée que s'il n'y a rien
// d'autre. La puissance brute n'entre jamais en compte : seuls comptent le tier et l'utilité. MOVE_NONE s'il n'y a
// aucune candidate.
#define UPGRADE_OFFENSIVE_MOVES   2
#define UPGRADE_JACKPOT_RARITY    4   // une attaque d'un tier au-dessus du tier visé : 4 fois moins de chances
#define UPGRADE_MAX_COVERAGE      10  // plafond du score de coverage
#define UPGRADE_WEAK_STAB_TIER    2   // sa meilleure attaque de son type est de ce tier ou moins : la remplacer presse
#define UPGRADE_THREAT_TYPES      4   // types des biomes de cet étage et du suivant
#define UPGRADE_MAX_CANDIDATES    96

// Ce que le Pokémon a déjà, pour juger les candidates
struct UpgradeContext
{
    u16 known[MAX_MON_MOVES];
    enum Species species;
    enum Type types[2];
    uq4_12_t bestEffect[NUMBER_OF_MON_TYPES];  // meilleure efficacité de ses attaques offensives contre chaque type
    u8 typeTier[NUMBER_OF_MON_TYPES];          // meilleur tier de ses attaques offensives de chaque type (0 : aucune)
    enum Type threats[UPGRADE_THREAT_TYPES];   // types qu'il va affronter (biomes de cet étage et du suivant)
    u8 threatCount;
    u8 offensiveCount;
    u8 stabTier;                               // meilleur tier de ses attaques de son type (0 : aucune)
    u32 knownRoles;
    u32 buildSetup;
    bool8 wantsPhysical;
    bool8 mixed;                               // ses deux stats d'attaque de base sont proches
    bool8 bulky;                               // PV et défenses de base bien au-dessus de son attaque
};

// Rôle d'une attaque connue (lue sur le Pokémon, donc pas forcément valide) : 0 hors de la table.
static u32 GetKnownMoveRole(u16 move)
{
    return (move < MOVES_COUNT) ? sIsaacMoveRoles[move] : 0;
}

static bool8 IsCountedOffensiveMove(u16 move)
{
    return move != MOVE_NONE && move < MOVES_COUNT && GetMoveCategory(move) != DAMAGE_CATEGORY_STATUS
        && !(GetKnownMoveRole(move) & ISAAC_MOVE_ROLE_COMBO);
}

static bool8 IsRealType(enum Type type)
{
    return type != TYPE_NONE && type != TYPE_MYSTERY && type != TYPE_STELLAR;
}

static void AddThreatType(struct UpgradeContext *ctx, enum Type type)
{
    u32 i;

    for (i = 0; i < ctx->threatCount; i++)
        if (ctx->threats[i] == type)
            return;
    if (ctx->threatCount < UPGRADE_THREAT_TYPES)
        ctx->threats[ctx->threatCount++] = type;
}

// Orientation offensive du Pokémon (s'il était de l'espèce donnée, après une évolution le cas échéant) d'après ses
// vraies stats d'Attaque et d'Attaque Spéciale (nature, EVs et boosts compris ; pour une évolution, mises à l'échelle
// des stats de base de la nouvelle espèce) : physique ou spécial selon la plus haute ; si elles sont à moins de
// ISAAC_MIXED_GAP_PCT % l'une de l'autre, mixte (*mixed), et la catégorie de la majorité de ses attaques offensives (à égalité, la plus
// haute). Renvoie TRUE pour physique.
#define ISAAC_MIXED_GAP_PCT  15

static bool8 IsMonPhysicallyOriented(struct Pokemon *mon, enum Species species, bool8 *mixed)
{
    enum Species current = GetMonData(mon, MON_DATA_SPECIES);
    u32 atk = GetMonData(mon, MON_DATA_ATK), spa = GetMonData(mon, MON_DATA_SPATK), physical = 0, special = 0, i;
    u16 move;

    if (species != current && gSpeciesInfo[current].baseAttack != 0 && gSpeciesInfo[current].baseSpAttack != 0)
    {
        atk = atk * gSpeciesInfo[species].baseAttack / gSpeciesInfo[current].baseAttack;
        spa = spa * gSpeciesInfo[species].baseSpAttack / gSpeciesInfo[current].baseSpAttack;
    }
    *mixed = (100 * min(atk, spa) >= (100 - ISAAC_MIXED_GAP_PCT) * max(atk, spa));
    if (!*mixed)
        return atk > spa;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move == MOVE_NONE || move >= MOVES_COUNT)
            continue;
        if (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL)
            physical++;
        else if (GetMoveCategory(move) == DAMAGE_CATEGORY_SPECIAL)
            special++;
    }
    if (physical != special)
        return physical > special;
    return atk >= spa;
}

// Remplit ctx ; renvoie le tier visé
static u32 InitUpgradeContext(struct UpgradeContext *ctx, struct Pokemon *mon, enum Species species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    u32 sum = 0, count = 0, physical = 0, special = 0, i, tier, target;
    enum Type gem = IsaacGetMonGemType(mon), type, def;
    uq4_12_t effect;

    memset(ctx, 0, sizeof(*ctx));
    ctx->species = species;
    ctx->types[0] = GetSpeciesType(species, 0);
    ctx->types[1] = (gem != TYPE_NONE) ? gem : GetSpeciesType(species, 1);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        ctx->known[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
        tier = GetMoveTierValue(ctx->known[i]);
        if (tier != 0)
        {
            sum += tier;
            count++;
        }
        if (ctx->known[i] == MOVE_NONE)
            continue;
        if (GetMoveCategory(ctx->known[i]) == DAMAGE_CATEGORY_STATUS)
        {
            ctx->knownRoles |= GetKnownMoveRole(ctx->known[i]);
            continue;
        }
        if (GetMoveCategory(ctx->known[i]) == DAMAGE_CATEGORY_PHYSICAL)
            physical++;
        else
            special++;
        if (!IsCountedOffensiveMove(ctx->known[i]))
            continue;
        ctx->offensiveCount++;
        type = GetMoveType(ctx->known[i]);
        ctx->typeTier[type] = max(ctx->typeTier[type], tier);
        if (type == ctx->types[0] || type == ctx->types[1])
            ctx->stabTier = max(ctx->stabTier, tier);
        for (def = 0; def < NUMBER_OF_MON_TYPES; def++)
        {
            if (!IsRealType(def))
                continue;
            effect = GetTypeModifier(type, def);
            if (effect > ctx->bestEffect[def])
                ctx->bestEffect[def] = effect;
        }
    }
    target = (count == 0) ? IsaacGetMonSetTier(mon) + 2 : (2 * sum + count) / (2 * count) + 1;
    target = min(target, 4u);

    ctx->wantsPhysical = IsMonPhysicallyOriented(mon, species, &ctx->mixed);
    ctx->bulky = (2 * (info->baseHP + info->baseDefense + info->baseSpDefense) >= 5 * max(info->baseAttack, info->baseSpAttack));
    ctx->buildSetup = ctx->wantsPhysical ? ISAAC_MOVE_ROLE_SETUP_PHYSICAL : ISAAC_MOVE_ROLE_SETUP_SPECIAL;
    if (ctx->mixed)
        ctx->buildSetup |= ISAAC_MOVE_ROLE_SETUP_PHYSICAL | ISAAC_MOVE_ROLE_SETUP_SPECIAL;
    if (ctx->knownRoles & (ISAAC_MOVE_ROLE_SETUP_PHYSICAL | ISAAC_MOVE_ROLE_SETUP_SPECIAL))
        ctx->knownRoles |= ISAAC_MOVE_ROLE_SETUP_PHYSICAL | ISAAC_MOVE_ROLE_SETUP_SPECIAL;  // un seul boost

    for (i = 0; i < 2; i++)
    {
        AddThreatType(ctx, gIsaacBiomes[IsaacGetCurrentBiome()].types[i]);
        AddThreatType(ctx, gIsaacBiomes[IsaacGetNextBiome()].types[i]);
    }
    return target;
}

// Score d'une attaque offensive de tier tier, 0 si elle n'apporte rien (un type qu'il a déjà, sans gain de tier) :
//  - attaque de son type (STAB) : très forte s'il n'en a aucune, forte pour le second type d'un double type, sinon
//    selon le gain de tier sur la meilleure de ce type, plus encore si sa meilleure attaque de son type est faible ;
//  - coverage réel, d'après la table des types : chaque type qu'aucune de ses attaques ne touche en super efficace et
//    qu'elle touche en super efficace, et surtout chaque type qui résiste à toutes ses attaques et qu'elle touche au
//    moins normalement (plafonné à UPGRADE_MAX_COVERAGE) ;
//  - les types des biomes de cet étage et du suivant qu'elle touche en super efficace, surtout s'il n'a rien contre eux ;
//  - un petit bonus si elle est dans les sets de l'espèce (une attaque qui lui va bien).
static u32 ScoreOffensiveUpgrade(const struct UpgradeContext *ctx, u16 move, u32 tier)
{
    enum Type type = GetMoveType(move), def;
    u32 score = 2, coverage = 0, i;
    uq4_12_t effect;

    if (ctx->typeTier[type] >= tier)
        return 0;
    if (type == ctx->types[0] || type == ctx->types[1])
    {
        if (ctx->stabTier == 0)
            score += 12;  // enfin une attaque de son type
        else if (ctx->typeTier[type] == 0)
            score += 6;   // son second type
        else
            score += 7 * (tier - ctx->typeTier[type]);  // nette amélioration de son attaque de ce type
        if (ctx->stabTier != 0 && ctx->stabTier <= UPGRADE_WEAK_STAB_TIER)
            score += 4;
    }
    else if (ctx->typeTier[type] != 0)
    {
        score += 2 * (tier - ctx->typeTier[type]);  // meilleure version d'une attaque de coverage
    }

    if (ctx->offensiveCount == 0)
    {
        coverage = 4;
    }
    else
    {
        for (def = 0; def < NUMBER_OF_MON_TYPES; def++)
        {
            if (!IsRealType(def))
                continue;
            effect = GetTypeModifier(type, def);
            if (effect >= UQ_4_12(2.0) && ctx->bestEffect[def] < UQ_4_12(2.0))
                coverage += 2;
            if (effect >= UQ_4_12(1.0) && ctx->bestEffect[def] < UQ_4_12(1.0))
                coverage += 3;  // un type qui résistait à tout
        }
    }
    score += min(coverage, (u32)UPGRADE_MAX_COVERAGE);

    for (i = 0; i < ctx->threatCount; i++)
        if (GetTypeModifier(type, ctx->threats[i]) >= UQ_4_12(2.0))
            score += (ctx->bestEffect[ctx->threats[i]] >= UQ_4_12(2.0)) ? 1 : 3;

    if (IsMoveInSpeciesSets(ctx->species, move))
        score += 2;
    return score;
}

// Score d'une attaque de statut de tier tier (ISAAC_MOVE_TIER_SITUATIONAL pour une situationnelle), 0 si elle n'est pas
// candidate :
// boost de son build (bien plus fort s'il a des attaques pour en profiter), soin (plus fort s'il est bulky), statut
// infligé, boost de défense (bulky), le reste à peine ; bonus pour une attaque de tier 4
static u32 ScoreStatusUpgrade(const struct UpgradeContext *ctx, u16 move, u32 tier)
{
    u32 role = sIsaacMoveRoles[move], score = 2;
    bool8 offensive = (ctx->offensiveCount >= UPGRADE_OFFENSIVE_MOVES);

    if ((role & ISAAC_MOVE_ROLE_COMBO) || (role & ctx->knownRoles))
        return 0;
    if (move == MOVE_CURSE && (ctx->types[0] == TYPE_GHOST || ctx->types[1] == TYPE_GHOST))
        return 0;  // pour un Spectre, ce n'est plus un boost
    if (!IsMoveInSpeciesSets(ctx->species, move) && !((role & ctx->buildSetup) && tier == 4))
        return 0;
    if (offensive && (!(role & (ctx->buildSetup | ISAAC_MOVE_ROLE_RECOVERY)) || move == MOVE_REST))
        return 0;
    if (tier == 4)
        score += 3;
    if (role & ctx->buildSetup)
        score += (ctx->offensiveCount >= UPGRADE_OFFENSIVE_MOVES) ? 10 : 3;
    else if (role & (ISAAC_MOVE_ROLE_SETUP_PHYSICAL | ISAAC_MOVE_ROLE_SETUP_SPECIAL))
        return 0;  // le boost de l'autre build
    else if (role & ISAAC_MOVE_ROLE_RECOVERY)
        score += ctx->bulky ? 9 : 6;
    else if (role & ISAAC_MOVE_ROLE_STATUS)
        score += 5;
    else if (role & ISAAC_MOVE_ROLE_DEF_BOOST)
        score += ctx->bulky ? 5 : 2;
    else
        score += 1;
    return score;
}

static u16 PickUpgradeMove(struct Pokemon *mon, enum Species species)
{
    struct UpgradeContext ctx;
    u16 moves[UPGRADE_MAX_CANDIDATES], move, fallback = MOVE_NONE;
    u32 weights[UPGRADE_MAX_CANDIDATES], count = 0, fallbackTotal = 0, total = 0, roll, tier, rawTier, score, i;
    u8 typeCount[NUMBER_OF_MON_TYPES] = {0};
    u32 target = InitUpgradeContext(&ctx, mon, species);
    enum Type gem = IsaacGetMonGemType(mon);
    bool8 jackpot, physical;

    for (move = 1; move < MOVES_COUNT && count < UPGRADE_MAX_CANDIDATES; move++)
    {
        tier = rawTier = sIsaacMoveTiers[move];
        if (tier == 0 || (sIsaacMoveRoles[move] & ISAAC_MOVE_ROLE_COMBO))
            continue;
        for (i = 0; i < MAX_MON_MOVES && ctx.known[i] != move; i++)
            ;
        if (i != MAX_MON_MOVES)
            continue;
        if (GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS && tier == ISAAC_MOVE_TIER_SITUATIONAL)
            tier = target;
        jackpot = (tier == target + 1);
        if (tier != target && !jackpot)
            continue;
        if (GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS)
        {
            score = ScoreStatusUpgrade(&ctx, move, rawTier);
            if (score == 0)
                continue;
        }
        else
        {
            physical = (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL);
            if (physical != ctx.wantsPhysical && !ctx.mixed)
                continue;
            score = ScoreOffensiveUpgrade(&ctx, move, tier);
        }
        // le type donné par la Pierre Prisme n'est presque jamais dans les attaques de l'espèce : elles sont permises
        if (!CanSpeciesLearnMove(species, move)
         && (gem == TYPE_NONE || GetMoveType(move) != gem || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS))
            continue;
        if (score == 0)  // sans intérêt : seulement s'il n'y a rien d'autre
        {
            if (!jackpot && Random() % ++fallbackTotal == 0)
                fallback = move;
            continue;
        }
        moves[count] = move;
        weights[count++] = score * score * (jackpot ? 1 : UPGRADE_JACKPOT_RARITY);
        if (GetMoveCategory(move) != DAMAGE_CATEGORY_STATUS)
            typeCount[GetMoveType(move)]++;
    }
    if (count == 0)
        return fallback;
    // un type de coverage qui a beaucoup d'attaques candidates ne doit pas écraser les autres : poids x 2 / (n + 1)
    for (i = 0; i < count; i++)
    {
        enum Type type = GetMoveType(moves[i]);

        if (GetMoveCategory(moves[i]) != DAMAGE_CATEGORY_STATUS && type != ctx.types[0] && type != ctx.types[1])
            weights[i] = weights[i] * 2 / (typeCount[type] + 1);
        total += weights[i];
    }
    if (total == 0)
        return moves[0];
    roll = Random32() % total;
    for (i = 0; i + 1 < count && roll >= weights[i]; i++)
        roll -= weights[i];
    return moves[i];
}

// Pierre Prisme : attaque du nouveau type à proposer au Pokémon, toujours (même si son espèce n'en apprend aucune :
// son nouveau type lui en ouvre). Une attaque offensive d'un tier au-dessus de ses attaques (moyenne arrondie + 1, 4 au
// plus ; sans attaque à tier, le palier de son set + 2), jamais une attaque ISAAC_MOVE_ROLE_COMBO ni une qu'il connaît,
// cherchée dans cet ordre jusqu'à en trouver :
//  1. que son espèce peut apprendre, dans la catégorie de son orientation (IsMonPhysicallyOriented ; les deux s'il
//     est mixte) ;
//  2. n'importe laquelle de cette catégorie ;
//  3. n'importe laquelle, physique ou spéciale ;
//  4. du tier le plus proche (1, puis 2, puis 3 d'écart).
// Rarement (PRISM_SITUATIONAL_WEIGHT contre PRISM_OFFENSIVE_WEIGHT par candidate), une attaque situationnelle du type
// à la place ; et seulement elles si le type n'a rien d'autre. Tirée au hasard.
#define PRISM_OFFENSIVE_WEIGHT    8
#define PRISM_SITUATIONAL_WEIGHT  1

enum
{
    PRISM_PASS_LEARNABLE,
    PRISM_PASS_BUILD,
    PRISM_PASS_ANY_CATEGORY,
    PRISM_PASS_NEAREST_TIER,
    PRISM_PASS_SITUATIONAL,
};

static bool8 IsPrismMoveUsable(struct Pokemon *mon, u16 move, enum Type type)
{
    return sIsaacMoveTiers[move] != 0 && GetMoveType(move) == type && !(sIsaacMoveRoles[move] & ISAAC_MOVE_ROLE_COMBO)
        && !MonKnowsMove(mon, move);
}

u16 IsaacPickPrismMove(struct Pokemon *mon, enum Type type)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u32 tier = GetMonAverageMoveTier(mon), total, offensive, weight, pass, gap, moveTier;
    u16 move, picked;
    bool8 mixed, wantsPhysical = IsMonPhysicallyOriented(mon, species, &mixed);

    tier = min((tier == 0) ? IsaacGetMonSetTier(mon) + 2 : tier + 1, 4u);

    for (pass = PRISM_PASS_LEARNABLE; pass <= PRISM_PASS_SITUATIONAL; pass++)
    {
        for (gap = (pass == PRISM_PASS_NEAREST_TIER) ? 1 : 0; gap <= ((pass == PRISM_PASS_NEAREST_TIER) ? 3 : 0); gap++)
        {
            picked = MOVE_NONE;
            total = offensive = 0;
            for (move = 1; move < MOVES_COUNT; move++)
            {
                if (!IsPrismMoveUsable(mon, move, type))
                    continue;
                moveTier = sIsaacMoveTiers[move];
                if (moveTier == ISAAC_MOVE_TIER_SITUATIONAL)
                {
                    weight = PRISM_SITUATIONAL_WEIGHT;  // rarement, à la place d'une attaque offensive
                }
                else
                {
                    if (pass == PRISM_PASS_SITUATIONAL || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS
                     || (moveTier != tier + gap && moveTier + gap != tier))
                        continue;
                    if (pass <= PRISM_PASS_BUILD && !mixed && (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL) != wantsPhysical)
                        continue;
                    if (pass == PRISM_PASS_LEARNABLE && !CanSpeciesLearnMove(species, move))
                        continue;
                    weight = PRISM_OFFENSIVE_WEIGHT;
                    offensive++;
                }
                total += weight;
                if (Random() % total < weight)
                    picked = move;
            }
            if (offensive != 0 || (pass == PRISM_PASS_SITUATIONAL && picked != MOVE_NONE))
                return picked;
        }
    }
    return MOVE_NONE;
}

// Un cran d'EVs pour le Pokémon n° slot, avec la scène de message (FALSE si ses EVs sont déjà au maximum)
static bool8 TryProgressionEvs(struct Pokemon *mon, u32 slot, u32 *outSlot)
{
    if (!IncreaseMonEvs(mon))
        return FALSE;
    memset(sIsaacEvolutionMoves, 0, sizeof(sIsaacEvolutionMoves));
    sIsaacProgressionScene = PROGRESSION_SCENE_EVS;
    *outSlot = slot;
    return TRUE;
}

// Après le combat (une seule fois) : évolution ou attaque de tier supérieur (voir plus haut). Renvoie l'espèce de la
// scène d'évolution à lancer (celle du Pokémon pour un simple apprentissage) et sa place dans l'équipe, SPECIES_NONE
// s'il ne se passe rien.
enum Species IsaacPickBattleProgression(bool8 won, u32 *outSlot)
{
    u16 choices[ISAAC_MAX_EVOLUTION_CHOICES], move;
    u32 i, evolver = PARTY_SIZE, active = PARTY_SIZE, count;
    bool8 boss = (sIsaacProgressionBattle == PROGRESSION_BOSS), rolled = FALSE, maxedMoves, canEvs, canBoost;
    struct Pokemon *mon;
    enum Species species;

    if (sIsaacProgressionBattle == PROGRESSION_NONE)
        return SPECIES_NONE;
    IsaacRecordActiveTurn();  // le dernier tour, qui finit le combat sans passer par la fin de tour
    sIsaacProgressionBattle = PROGRESSION_NONE;
    if (!won)
        return SPECIES_NONE;
    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
    {
        mon = &gParties[B_TRAINER_PLAYER][i];
        if (sIsaacActiveTurns[i] == 0 || GetMonData(mon, MON_DATA_IS_EGG) || IsaacIsFallenCrown(mon))
            continue;  // un Pokémon couronné K.O. va être relâché
        if (active == PARTY_SIZE || sIsaacActiveTurns[i] > sIsaacActiveTurns[active])
            active = i;
        if ((evolver == PARTY_SIZE || sIsaacActiveTurns[i] > sIsaacActiveTurns[evolver])
         && IsaacGetEvolutionChoices(GetMonData(mon, MON_DATA_SPECIES), FALSE, choices, ARRAY_COUNT(choices)) != 0)
            evolver = i;
    }
    if (evolver != PARTY_SIZE && !boss)
    {
        rolled = RollPity(PITY_EVOLUTION);
        RecordPity(PITY_EVOLUTION, rolled);
    }
    if (evolver != PARTY_SIZE && (boss || rolled))
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
    if (active == PARTY_SIZE)
        return SPECIES_NONE;
    mon = &gParties[B_TRAINER_PLAYER][active];
    species = GetMonData(mon, MON_DATA_SPECIES);
    maxedMoves = (GetMonAverageMoveTier(mon) >= ISAAC_SET_TIERS);  // ses attaques n'ont plus rien à gagner
    canEvs = CanIncreaseMonEvs(mon);

    // Attaque de tier supérieur, ou des EVs à la place si ses attaques sont au maximum ; tirage seulement s'il y a
    // quelque chose à gagner
    move = maxedMoves ? MOVE_NONE : PickUpgradeMove(mon, species);
    if (maxedMoves ? canEvs : (move != MOVE_NONE))
    {
        // personne ne peut évoluer (pas de tirage d'évolution) : l'attaque est plus probable
        rolled = boss || RollPityStep(PITY_MOVE, (evolver == PARTY_SIZE) ? PITY_MOVE_NO_EVOLUTION_STEP : sPityStepPermille[PITY_MOVE]);
        if (!boss)
            RecordPity(PITY_MOVE, rolled);
        if (rolled && maxedMoves)
        {
            TryProgressionEvs(mon, active, outSlot);
            return species;
        }
        if (rolled)
        {
            memset(sIsaacEvolutionMoves, 0, sizeof(sIsaacEvolutionMoves));
            sIsaacEvolutionMoves[MAX_MON_MOVES] = move;
            sIsaacProgressionScene = PROGRESSION_SCENE_MOVE;
            *outSlot = active;
            return species;
        }
    }

    // Un cran d'EVs, ou, s'ils sont au maximum, un boost de stat au hasard si le Pokémon est à son stade final, que ses
    // attaques sont de tier moyen 4 et qu'il n'a pas déjà de boost ; tirage seulement si l'un des deux est possible.
    // Contre le boss (rare : l'attaque n'a rien donné), simple tirage, hors anti-malchance.
    canBoost = !canEvs && maxedMoves && !IsaacHasBoost(mon)
            && IsaacGetEvolutionChoices(species, FALSE, choices, ARRAY_COUNT(choices)) == 0;
    if (canEvs || canBoost)
    {
        rolled = boss ? (Random() % 100 < PROGRESSION_EVS_PCT) : RollPity(PITY_EVS);
        if (!boss)
            RecordPity(PITY_EVS, rolled);
        if (rolled && canEvs)
        {
            TryProgressionEvs(mon, active, outSlot);
            return species;
        }
        if (rolled)
        {
            sIsaacBoostedStat = Random() % NUM_STATS;
            IsaacAddStatBoost(mon, sIsaacBoostedStat);
            memset(sIsaacEvolutionMoves, 0, sizeof(sIsaacEvolutionMoves));
            sIsaacProgressionScene = PROGRESSION_SCENE_BOOST;
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
    *learnOnly = (scene == PROGRESSION_SCENE_MOVE || scene == PROGRESSION_SCENE_EVS || scene == PROGRESSION_SCENE_BOOST);
    if (scene == PROGRESSION_SCENE_EVOLUTION)
        return COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\p"
                               "To reward its efforts…\pWhat? {STR_VAR_1} is evolving!");
    if (scene == PROGRESSION_SCENE_MOVE)
        return COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\p"
                               "It gained new experience!");
    if (scene == PROGRESSION_SCENE_BOOST)
    {
        static const u8 *const sBoostTexts[NUM_STATS] =
        {
            [STAT_HP]    = COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\pIts HP rose for good!"),
            [STAT_ATK]   = COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\pIts Attack rose for good!"),
            [STAT_DEF]   = COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\pIts Defense rose for good!"),
            [STAT_SPEED] = COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\pIts Speed rose for good!"),
            [STAT_SPATK] = COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\pIts Sp. Atk rose for good!"),
            [STAT_SPDEF] = COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\pIts Sp. Def rose for good!"),
        };

        return sBoostTexts[sIsaacBoostedStat % NUM_STATS];
    }
    if (scene == PROGRESSION_SCENE_EVS)
        return COMPOUND_STRING("{STR_VAR_1} fought the hardest\nin this battle!\p"
                               "It feels stronger!");
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

// Testeur du hub (pour les tests) : l'équipe est remplacée par six grands légendaires (catégorie 9) différents tirés
// au hasard, avec leur meilleur set
void IsaacGiveLegendaryTestTeam(void)
{
    u16 candidates[64];
    u32 count = 0, i, j, tmp;

    for (i = 0; i < IsaacGetRosterCount() && count < ARRAY_COUNT(candidates); i++)
    {
        enum Species species = IsaacGetRosterSpecies(i);

        if (IsaacGetSpeciesCategory(species) == ISAAC_CATEGORY_LEGENDARY_BIG && IsaacSpeciesHasSet(species))
            candidates[count++] = species;
    }
    for (i = 0; i < count; i++)  // ordre au hasard
    {
        j = i + Random() % (count - i);
        SWAP(candidates[i], candidates[j], tmp);
    }
    ZeroPlayerPartyMons();
    for (i = 0; i < PARTY_SIZE && i < count; i++)
        IsaacCreateMonFromSet(&gParties[B_TRAINER_PLAYER][i], candidates[i], ISAAC_SET_TIERS - 1, Random() % ISAAC_SET_VARIANTS, Random32());
    CalculatePlayerPartyCount();
    FlagSet(FLAG_SYS_POKEMON_GET);
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
