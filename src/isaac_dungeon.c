#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_camera.h"
#include "fieldmap.h"
#include "data.h"
#include "international_string_util.h"
#include "item.h"
#include "isaac_dungeon.h"
#include "isaac_pokemon.h"
#include "item_icon.h"
#include "menu.h"
#include "money.h"
#include "move.h"
#include "pokemon_icon.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script_pokemon_util.h"
#include "script.h"
#include "shop.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/battle_ai.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/isaac_dungeon.h"
#include "constants/items.h"
#include "constants/map_event_ids.h"
#include "constants/opponents.h"
#include "constants/trainer_types.h"
#include "constants/trainers.h"
#include "constants/characters.h"
#include "constants/map_types.h"
#include "constants/maps.h"
#include "constants/species.h"
#include "constants/songs.h"
#include "constants/vars.h"

/*
 * Donjon façon Binding of Isaac (map IsaacRoom, tuiles de la Route Victoire : General + Cave).
 *
 * Un étage est une grille de ISAAC_GRID_W x ISAAC_GRID_H cellules, une salle 1 x 1 par cellule occupée.
 * Toutes les salles utilisent la même map IsaacRoom, redessinée à chaque chargement (IsaacPaintRoom) :
 * intérieur 13 x 7, une porte au milieu de chaque côté qui donne sur une salle voisine, roche sinon.
 *
 *   1. la CROISSANCE (comme Isaac) : départ au centre de la grille, file d'attente. Pour chaque cellule
 *      retirée de la file, on essaie ses 4 voisines ; une voisine est refusée si elle est déjà occupée,
 *      si elle touche déjà plus d'une salle (ça donne des couloirs plutôt que des blocs), si l'étage a
 *      déjà assez de salles, ou une fois sur deux au hasard. Sinon elle devient une salle et entre dans
 *      la file. Le nombre de salles visé grandit avec la profondeur (GetTargetRoomCount) ;
 *   2. les SALLES SPÉCIALES sur les culs-de-sac (salles à un seul voisin, hors départ) : le boss sur le
 *      plus éloigné du départ, puis le trésor, la salle de capture (une par étage), et la boutique (à
 *      partir de ISAAC_SHOP_MIN_FLOOR).
 *      S'il manque des culs-de-sac ou des salles, on recommence avec une autre graine ;
 *   3. la SALLE SECRÈTE (à partir de ISAAC_SECRET_MIN_FLOOR) : la case vide qui touche le plus de salles
 *      (au moins 2, jamais le boss). Les portes qui y mènent sont fermées par de la roche tant que le passage
 *      n'est pas ouvert (FLAG_ISAAC_SECRET_OPEN) : avec Éclate-Roc en parlant à l'une de ces
 *      roches (IsaacGetWallInteractScript).
 *
 * BIOMES : une descente traverse ISAAC_FLOOR_COUNT des ISAAC_BIOME_COUNT biomes (data/isaac_biomes.h), un étage
 * chacun, tirés au début de la descente (VAR_ISAAC_BIOME_ORDER) ; après le boss du dernier, la salle de la victoire.
 * La map IsaacRoom prend le décor du biome de l'étage quand elle est chargée (IsaacApplyBiomeToMapHeader) :
 * tileset (tous ont la même numérotation), musique, météo, obscurité, plongée.
 *
 * Tout ne dépend que de VAR_ISAAC_SEED et VAR_ISAAC_FLOOR : l'étage est régénéré à l'identique à chaque
 * chargement (retour de combat, de menu...). VAR_ISAAC_ROOM est la cellule où se trouve le joueur,
 * VAR_ISAAC_VISITED_0 à 4 les salles déjà visitées (un bit par cellule).
 *
 * DRESSEURS : une salle normale a ISAAC_TRAINER_PCT % de chances d'avoir un dresseur du biome, au centre. Il
 * regarde la porte par laquelle le joueur arrive : le moteur le fait réagir, s'approcher et lancer le combat
 * dès l'arrivée. Vaincu, il retourne au centre et la salle est « nettoyée » (FLAG_ISAAC_CLEARED_FIRST + cellule).
 * Ses Pokémon (nombre, catégorie, set selon l'étage : sRoomTrainerParties) sont tirés au hasard dans toutes
 * les lignées du biome, sans doublon (AddTrainerMon). Vaincu, il donne ISAAC_TRAINER_MONEY_PER_FLOOR x
 * la profondeur, et un objet de son butin (IsaacPickTrainerLoot) : à partir de l'étage 2, l'un des dresseurs de
 * l'étage donne une Master Ball à la place, à coup sûr si on les bat tous.
 *
 * SALLE DE CAPTURE : des Pokémon sauvages différents du biome se promènent (3 aux étages 1-2, 4 aux étages
 * 3-4, 5 aux étages 5-6), de catégorie de plus en plus haute avec la profondeur (RollCaptureCategory), au set 2
 * jusqu'à l'étage 3 puis au set 3. Leur parler lance le combat, après quoi ils disparaissent. SALLE TRÉSOR : un objet spécial au sol (deux salles
 * trésor à partir de ISAAC_TWO_TREASURES_MIN_FLOOR).
 *
 * MINI-CARTE (bouton R dans une salle) : comme Isaac, on voit les salles visitées et celles qui les
 * touchent (pas la salle secrète tant qu'on n'y est pas entré), avec l'icône des salles spéciales.
 */

#define ISAAC_MAX_ROOMS        20
#define ISAAC_SHOP_MIN_FLOOR   2
#define ISAAC_SECRET_MIN_FLOOR 3
// Salles secrètes (et donc l'entraîneur d'Éclate-Roc, qui ne vient que s'il y en a une) : mises de côté pour l'instant
#define ISAAC_SECRET_ROOMS     FALSE
#define ISAAC_TWO_TREASURES_MIN_FLOOR 4  // deux salles trésor à partir de cet étage
#define ISAAC_MAX_TRIES        200  // graines essayées pour un étage valide

#define ISAAC_TRAINER_PCT    60  // % de salles normales avec un dresseur
#define ISAAC_TRAINER_MONEY_PER_FLOOR  1200  // argent gagné contre un dresseur (ou le boss) : x profondeur
#define ISAAC_BIOME          IsaacGetCurrentBiome()
#define ISAAC_FLOOR_COUNT    6  // étages d'une descente (un biome chacun, tous différents), puis la salle de la victoire
#define WIN_ROOM_X  5  // arrivée dans la salle de la victoire (IsaacWin)
#define WIN_ROOM_Y  6

// Salles nettoyées (dresseur vaincu) : un drapeau par cellule, dans des drapeaux inutilisés du jeu
#define FLAG_ISAAC_CLEARED_FIRST  FLAG_UNUSED_0x264
STATIC_ASSERT(FLAG_ISAAC_CLEARED_FIRST + ISAAC_GRID_W * ISAAC_GRID_H - 1 <= FLAG_UNUSED_0x2BB, IsaacClearedFlagsAreUnused);
// Master Ball déjà donnée par un dresseur de l'étage (juste après les drapeaux des salles)
#define FLAG_ISAAC_MASTER_BALL_DROPPED  (FLAG_ISAAC_CLEARED_FIRST + ISAAC_GRID_W * ISAAC_GRID_H)
STATIC_ASSERT(FLAG_ISAAC_MASTER_BALL_DROPPED <= FLAG_UNUSED_0x2BB, IsaacMasterBallFlagIsUnused);
#define ISAAC_MASTER_BALL_MIN_FLOOR  2  // à partir de cet étage, un dresseur de l'étage donne une Master Ball
// Pokémon de la salle de capture déjà combattus (un drapeau par emplacement, juste après)
#define CAPTURE_MAX_MONS                5
#define FLAG_ISAAC_CAPTURE_DONE_FIRST   (FLAG_ISAAC_MASTER_BALL_DROPPED + 1)
STATIC_ASSERT(FLAG_ISAAC_CAPTURE_DONE_FIRST + CAPTURE_MAX_MONS - 1 <= FLAG_UNUSED_0x2BB, IsaacCaptureFlagsAreUnused);

// Kecleon (boutique) déjà rencontré : pour toute la partie, il ne fait sa blague qu'une fois
#define FLAG_ISAAC_KECLEON_MET  FLAG_UNUSED_0x023
// Articles de la boutique de l'étage déjà vendus, chacun en un seul exemplaire (voir SHOP_SLOT_*), dans des drapeaux
// inutilisés du jeu
#define FLAG_ISAAC_SHOP_SOLD_FIRST  FLAG_UNUSED_0x030
#define SHOP_LIMITED_SLOTS  13
STATIC_ASSERT(FLAG_ISAAC_SHOP_SOLD_FIRST + SHOP_LIMITED_SLOTS - 1 <= FLAG_UNUSED_0x047, IsaacShopFlagsAreUnused);

// Passage vers la salle secrète de l'étage ouvert (sinon ses portes sont fermées par de la roche)
#define FLAG_ISAAC_SECRET_OPEN  FLAG_UNUSED_0x020
// Récompense de la salle secrète à dresseur ramassée (elle apparaît au sol une fois le dresseur vaincu)
#define FLAG_ISAAC_SECRET_LOOT_TAKEN  FLAG_UNUSED_0x026
// Personne dans l'équipe ne connaissait Éclate-Roc au début de l'étage : l'entraîneur d'Éclate-Roc peut y apparaître
#define FLAG_ISAAC_ROCK_SMASH_TEACHER  FLAG_UNUSED_0x021
// Ordre des biomes de la prochaine descente déjà tiré (par le Montagnard du hub, qui annonce le premier biome)
#define FLAG_ISAAC_RUN_PREPARED  FLAG_UNUSED_0x022

#define ISAAC_START_X  (ISAAC_GRID_W / 2)
#define ISAAC_START_Y  (ISAAC_GRID_H / 2)

// Salle IsaacRoom : murs de 1 case à gauche et à droite, 2 en haut et en bas
#define ROOM_MIN_X  1
#define ROOM_MAX_X  (ISAAC_ROOM_W - 2)
#define ROOM_MIN_Y  2
#define ROOM_MAX_Y  (ISAAC_ROOM_H - 3)
#define ROOM_MID_X  (ISAAC_ROOM_W / 2)
#define ROOM_MID_Y  ((ROOM_MIN_Y + ROOM_MAX_Y) / 2)

// Metatiles : tous les tilesets du donjon (un par décor de biome, tools/isaac/build_tileset.py) ont la même
// numérotation, dans l'ordre de ROLES
#define TILE_FLOOR       0x200
#define TILE_ROCK_1      0x201
#define TILE_ROCK_2      0x202
#define TILE_CLIFF_TL    0x203
#define TILE_CLIFF_T     0x204
#define TILE_CLIFF_TR    0x205
#define TILE_CLIFF_L     0x206
#define TILE_CLIFF_C     0x207
#define TILE_CLIFF_R     0x208
#define TILE_CLIFF_BL    0x209
#define TILE_CLIFF_B     0x20A
#define TILE_CLIFF_BR    0x20B
#define TILE_CLIFF_IN_SE 0x20C  // coins rentrants : sol en diagonale
#define TILE_CLIFF_IN_SW 0x20D
#define TILE_CLIFF_IN_NE 0x20E
#define TILE_CLIFF_IN_NW 0x20F
#define TILE_SANDBAR     0x211  // banc de sable d'une case dans l'eau (océan)
#define TILE_DECO_1      0x212  // décorations des coins de salle (rochers, glace...), voir PaintCornerDecorations
#define TILE_DECO_2      0x213
// Demi-arbres des biomes en forêt (arbres de 2 x 2), dans l'ordre : haut, bas, bas avec le tronc ; + 1 à droite
#define TILE_TREE_TOP    0x214
#define TILE_TREE_BOTTOM 0x216
#define TILE_TREE_TRUNK  0x218
// Falaise : seconde rangée de la paroi sud (et ses bouts), dessus des couloirs (nord-sud, est-ouest)
#define TILE_CLIFF_T2     0x21A
#define TILE_CLIFF_IN_NE2 0x21B
#define TILE_CLIFF_IN_NW2 0x21C
#define TILE_BRIDGE_V     0x21D
#define TILE_BRIDGE_H     0x21E

// Paillassons devant les portes des salles spéciales (tileset IsaacCave, tools/isaac/build_tileset.py) :
// motif (MAT_BOSS...), orientation (MAT_HORIZONTAL...), case du paillasson (ligne par ligne)
#define TILE_MAT_FIRST   0x240
#define MAT_TILE(mat, orientation, i)  (TILE_MAT_FIRST + ((mat) * MAT_ORIENTATION_COUNT + (orientation)) * 6 + (i))

#define FLOOR_ELEVATION  3

// Zone peinte : la map et sa marge (MAP_OFFSET cases tout autour, voir InitMapLayoutData)
#define PAINT_MIN  (-MAP_OFFSET)
#define PAINT_W    (ISAAC_ROOM_W + MAP_OFFSET_W)
#define PAINT_H    (ISAAC_ROOM_H + MAP_OFFSET_H)

enum
{
    ROOM_NONE,
    ROOM_NORMAL,
    ROOM_START,
    ROOM_BOSS,
    ROOM_TREASURE,
    ROOM_SHOP,
    ROOM_SECRET,
    ROOM_CAPTURE,
};

enum
{
    MAT_BOSS,
    MAT_TREASURE,
    MAT_SHOP,
    MAT_CAPTURE,
    MAT_NONE,
};

enum
{
    MAT_HORIZONTAL,  // portes nord et sud : 3 x 2, tête à l'endroit
    MAT_WEST,        // porte ouest : 2 x 3, tête tournée vers la porte
    MAT_EAST,        // porte est : 2 x 3, tête tournée vers la porte
    MAT_ORIENTATION_COUNT,
};

enum
{
    ISAAC_DIR_NORTH,
    ISAAC_DIR_EAST,
    ISAAC_DIR_SOUTH,
    ISAAC_DIR_WEST,
    ISAAC_DIR_COUNT,
};

enum
{
    EXIT_NONE,
    EXIT_DOOR,
    EXIT_NEXT_FLOOR,
    EXIT_HUB_ENTRANCE,
};

enum
{
    CELL_WALL,
    CELL_FLOOR,
    CELL_HIDDEN,  // porte de la salle secrète fermée : roche infranchissable
};

static const s8 sDirDx[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = 0, [ISAAC_DIR_EAST] = 1, [ISAAC_DIR_SOUTH] = 0, [ISAAC_DIR_WEST] = -1 };
static const s8 sDirDy[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = -1, [ISAAC_DIR_EAST] = 0, [ISAAC_DIR_SOUTH] = 1, [ISAAC_DIR_WEST] = 0 };
static const u8 sDirToFacing[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = DIR_NORTH, [ISAAC_DIR_EAST] = DIR_EAST, [ISAAC_DIR_SOUTH] = DIR_SOUTH, [ISAAC_DIR_WEST] = DIR_WEST };

// Case de chaque porte, sur le bord de la map (y passer fait changer de salle)
static const s8 sDoorX[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = ROOM_MID_X, [ISAAC_DIR_EAST] = ISAAC_ROOM_W - 1, [ISAAC_DIR_SOUTH] = ROOM_MID_X, [ISAAC_DIR_WEST] = 0 };
static const s8 sDoorY[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = 0, [ISAAC_DIR_EAST] = ROOM_MID_Y, [ISAAC_DIR_SOUTH] = ISAAC_ROOM_H - 1, [ISAAC_DIR_WEST] = ROOM_MID_Y };

// Entrée du donjon dans le hub (porte de la grotte)
#define HUB_ENTRANCE_X  18
#define HUB_ENTRANCE_Y  5

struct IsaacFloor
{
    u8 room[ISAAC_GRID_H][ISAAC_GRID_W];
    u8 roomCount;
};

// Mini-carte : une salle = un pavé, MINIMAP_GAP pixels entre deux. Grands pavés quand toutes les salles connues
// tiennent dans la fenêtre, petits sinon. Les salles spéciales ont l'icône animée d'un Pokémon par-dessus.
#define MINIMAP_BIG_ROOM_W    22
#define MINIMAP_BIG_ROOM_H    16
#define MINIMAP_SMALL_ROOM_W  16
#define MINIMAP_SMALL_ROOM_H  12
#define MINIMAP_GAP           2
#define MINIMAP_ICON_RISE     4   // les icônes ont du vide en haut : on les remonte un peu
#define MINIMAP_TITLE_H       16
#define MINIMAP_WIDTH         24  // fenêtre (tuiles de 8 pixels), sous les tuiles de la boîte de dialogue
#define MINIMAP_HEIGHT        16
#define MINIMAP_MAX_ICONS     4   // icônes de Pokémon : boss, trésor, boutique, secrète
#define MINIMAP_BALL_TAG      0x7A00  // icône de Poké Ball de la salle de capture (tuiles et palette)
#define MINIMAP_ITEM_ICON_OFFSET  ((32 - 24) / 2)

#define MINIMAP_COLOR_BG       TEXT_COLOR_WHITE
#define MINIMAP_COLOR_SEEN     TEXT_COLOR_LIGHT_GRAY
#define MINIMAP_COLOR_VISITED  TEXT_COLOR_DARK_GRAY
#define MINIMAP_COLOR_OUTLINE  TEXT_COLOR_BLUE  // contour de la salle courante

#define MINIMAP_AREA_W  (MINIMAP_WIDTH * 8)
#define MINIMAP_AREA_H  (MINIMAP_HEIGHT * 8 - MINIMAP_TITLE_H)

STATIC_ASSERT(ISAAC_GRID_W * (MINIMAP_SMALL_ROOM_W + MINIMAP_GAP) - MINIMAP_GAP <= MINIMAP_AREA_W, MinimapFitsInWindowWidth);
STATIC_ASSERT(ISAAC_GRID_H * (MINIMAP_SMALL_ROOM_H + MINIMAP_GAP) - MINIMAP_GAP <= MINIMAP_AREA_H, MinimapFitsInWindowHeight);
STATIC_ASSERT(8 + MINIMAP_WIDTH * MINIMAP_HEIGHT <= 0x194, MinimapWindowBelowTextBoxTiles);

static const struct WindowTemplate sMinimapWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = (DISPLAY_TILE_WIDTH - MINIMAP_WIDTH) / 2,
    .tilemapTop = 1,
    .width = MINIMAP_WIDTH,
    .height = MINIMAP_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 8,
};

static const u16 sVisitedVars[] =
{
    VAR_ISAAC_VISITED_0,
    VAR_ISAAC_VISITED_1,
    VAR_ISAAC_VISITED_2,
    VAR_ISAAC_VISITED_3,
    VAR_ISAAC_VISITED_4,
};

STATIC_ASSERT(ARRAY_COUNT(sVisitedVars) * 16 >= ISAAC_GRID_W * ISAAC_GRID_H, EnoughVisitedVars);

extern const u8 Isaac_EventScript_Exit[];
extern const u8 Hub_EventScript_NeedPokemon[];
extern const u8 IsaacRoom_EventScript_SecretWall[];
extern const u8 IsaacRoom_EventScript_Trainer[];
extern const u8 IsaacRoom_EventScript_OptionalTrainer[];
extern const u8 Isaac_EventScript_Minimap[];

static EWRAM_DATA u32 sSeed = 0;
static EWRAM_DATA struct IsaacFloor sFloor = {0};
static EWRAM_DATA u8 sCell[PAINT_H][PAINT_W] = {0};
static EWRAM_DATA bool8 sIsBigRock[PAINT_H][PAINT_W] = {0};
static EWRAM_DATA u8 sPendingExit = EXIT_NONE;
static EWRAM_DATA u8 sPendingDoor = 0;
static EWRAM_DATA u8 sArrivalFacing = DIR_NONE;

static u16 IsaacRandom(void)
{
    sSeed = sSeed * 1103515245 + 12345;
    return sSeed >> 16;
}

static bool8 IsInRoomMap(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ISAAC_ROOM)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_ISAAC_ROOM);
}

static bool8 IsInHub(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_HUB)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_HUB);
}

// ---------------------------------------------------------------------------
// Biomes
// ---------------------------------------------------------------------------

// Biome de l'étage n° floor (1 à ISAAC_FLOOR_COUNT) : les ISAAC_FLOOR_COUNT premiers biomes d'un mélange tiré à
// partir de VAR_ISAAC_BIOME_ORDER
static u32 GetBiomeOfFloor(u32 floor)
{
    u8 order[ISAAC_BIOME_COUNT];
    u32 state = VarGet(VAR_ISAAC_BIOME_ORDER) * 2654435761u + 1;
    u32 i, j, tmp;

    for (i = 0; i < ISAAC_BIOME_COUNT; i++)
        order[i] = i;
    for (i = ISAAC_BIOME_COUNT - 1; i > 0; i--)
    {
        state = state * 1103515245 + 12345;
        j = (state >> 16) % (i + 1);
        SWAP(order[i], order[j], tmp);
    }
    floor = min(max(floor, 1u), (u32)ISAAC_FLOOR_COUNT);
    return order[floor - 1];
}

u32 IsaacGetCurrentBiome(void)
{
    return GetBiomeOfFloor(VarGet(VAR_ISAAC_FLOOR));
}

// Type de la salle du donjon selon le biome : route marine (le joueur surfe) ou souterrain. Sert
// aussi à GetMapTypeByGroupAndId, qui lit sinon le type d'origine de la map
enum MapType IsaacGetRoomMapType(void)
{
    return gIsaacBiomes[IsaacGetCurrentBiome()].ocean ? MAP_TYPE_OCEAN_ROUTE : MAP_TYPE_UNDERGROUND;
}

// Chargement de l'en-tête d'une map : si c'est la salle du donjon, décor du biome de l'étage (le tileset vient
// de *layoutId). FALSE pour les autres maps.
bool8 IsaacApplyBiomeToMapHeader(struct MapHeader *header, u16 *layoutId)
{
    const struct IsaacBiome *biome;

    if (!IsInRoomMap())
        return FALSE;
    biome = &gIsaacBiomes[IsaacGetCurrentBiome()];
    *layoutId = biome->layoutId;
    header->cave = biome->dark;
    header->mapType = IsaacGetRoomMapType();
    header->weather = biome->weather;
    return TRUE;
}

// Musique de la salle du donjon
u16 IsaacGetBiomeMusic(void)
{
    return gIsaacBiomes[IsaacGetCurrentBiome()].music;
}

// ---------------------------------------------------------------------------
// Plan de l'étage
// ---------------------------------------------------------------------------

static bool8 CellInGrid(s32 x, s32 y)
{
    return x >= 0 && y >= 0 && x < ISAAC_GRID_W && y < ISAAC_GRID_H;
}

static u8 GetRoom(s32 x, s32 y)
{
    if (!CellInGrid(x, y))
        return ROOM_NONE;
    return sFloor.room[y][x];
}

static u32 CountNeighborRooms(s32 x, s32 y)
{
    u32 dir, count = 0;

    for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
        if (GetRoom(x + sDirDx[dir], y + sDirDy[dir]) != ROOM_NONE)
            count++;
    return count;
}

// 7 à 9 salles au 1er étage (ISAAC_FIRST_FLOOR_EXTRA_ROOMS de plus), 9 à 11 au 2e, 11 à 13 au 3e, 14 à 16 au 4e,
// 16 à 18 au 5e, 19 à 21 au 6e... jusqu'à ISAAC_MAX_ROOMS
#define ISAAC_FIRST_FLOOR_EXTRA_ROOMS  1

static u32 GetTargetRoomCount(u32 floor)
{
    u32 count = 4 + floor * 5 / 2 + IsaacRandom() % 3;

    if (floor == 1)
        count += ISAAC_FIRST_FLOOR_EXTRA_ROOMS;

    return min(count, ISAAC_MAX_ROOMS);
}

static void GrowFloor(u32 target)
{
    u8 queueX[ISAAC_GRID_W * ISAAC_GRID_H];
    u8 queueY[ISAAC_GRID_W * ISAAC_GRID_H];
    u32 head = 0, tail = 0, dir;
    s32 x, y, nx, ny;

    memset(&sFloor, 0, sizeof(sFloor));
    sFloor.room[ISAAC_START_Y][ISAAC_START_X] = ROOM_START;
    sFloor.roomCount = 1;
    queueX[tail] = ISAAC_START_X;
    queueY[tail++] = ISAAC_START_Y;

    while (head < tail)
    {
        x = queueX[head];
        y = queueY[head++];
        for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
        {
            nx = x + sDirDx[dir];
            ny = y + sDirDy[dir];
            if (!CellInGrid(nx, ny) || sFloor.room[ny][nx] != ROOM_NONE)
                continue;
            if (sFloor.roomCount >= target || CountNeighborRooms(nx, ny) > 1 || (IsaacRandom() & 1))
                continue;
            sFloor.room[ny][nx] = ROOM_NORMAL;
            sFloor.roomCount++;
            queueX[tail] = nx;
            queueY[tail++] = ny;
        }
    }
}

// Distance (en salles) depuis le départ, 0xFF si inaccessible
static void ComputeDistances(u8 dist[ISAAC_GRID_H][ISAAC_GRID_W])
{
    u8 queueX[ISAAC_GRID_W * ISAAC_GRID_H];
    u8 queueY[ISAAC_GRID_W * ISAAC_GRID_H];
    u32 head = 0, tail = 0, dir;
    s32 x, y, nx, ny;

    memset(dist, 0xFF, ISAAC_GRID_W * ISAAC_GRID_H);
    dist[ISAAC_START_Y][ISAAC_START_X] = 0;
    queueX[tail] = ISAAC_START_X;
    queueY[tail++] = ISAAC_START_Y;
    while (head < tail)
    {
        x = queueX[head];
        y = queueY[head++];
        for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
        {
            nx = x + sDirDx[dir];
            ny = y + sDirDy[dir];
            if (GetRoom(nx, ny) == ROOM_NONE || dist[ny][nx] != 0xFF)
                continue;
            dist[ny][nx] = dist[y][x] + 1;
            queueX[tail] = nx;
            queueY[tail++] = ny;
        }
    }
}

// Culs-de-sac (hors départ), du plus éloigné au plus proche du départ. Renvoie leur nombre.
static u32 FindDeadEnds(u8 *outX, u8 *outY)
{
    u8 dist[ISAAC_GRID_H][ISAAC_GRID_W];
    u32 count = 0, i, j;
    s32 x, y;
    u8 tmp;

    ComputeDistances(dist);
    for (y = 0; y < ISAAC_GRID_H; y++)
    {
        for (x = 0; x < ISAAC_GRID_W; x++)
        {
            if (sFloor.room[y][x] != ROOM_NORMAL || CountNeighborRooms(x, y) != 1)
                continue;
            outX[count] = x;
            outY[count++] = y;
        }
    }
    for (i = 1; i < count; i++)
    {
        for (j = i; j > 0 && dist[outY[j]][outX[j]] > dist[outY[j - 1]][outX[j - 1]]; j--)
        {
            SWAP(outX[j], outX[j - 1], tmp);
            SWAP(outY[j], outY[j - 1], tmp);
        }
    }
    return count;
}

static void PlaceSecretRoom(void)
{
    u8 candX[ISAAC_GRID_W * ISAAC_GRID_H];
    u8 candY[ISAAC_GRID_W * ISAAC_GRID_H];
    u32 best = 2, count = 0, neighbors, dir;
    s32 x, y;
    bool8 nextToBoss;

    for (y = 0; y < ISAAC_GRID_H; y++)
    {
        for (x = 0; x < ISAAC_GRID_W; x++)
        {
            if (sFloor.room[y][x] != ROOM_NONE)
                continue;
            nextToBoss = FALSE;
            for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
                if (GetRoom(x + sDirDx[dir], y + sDirDy[dir]) == ROOM_BOSS)
                    nextToBoss = TRUE;
            neighbors = CountNeighborRooms(x, y);
            if (nextToBoss || neighbors < best)
                continue;
            if (neighbors > best)
            {
                best = neighbors;
                count = 0;
            }
            candX[count] = x;
            candY[count++] = y;
        }
    }
    if (count != 0)
    {
        u32 i = IsaacRandom() % count;
        sFloor.room[candY[i]][candX[i]] = ROOM_SECRET;
    }
}

// Salle spéciale sur un cul-de-sac pas encore pris, au hasard (le premier, le plus éloigné, est au boss)
static void PlaceOnFreeDeadEnd(const u8 *deadX, const u8 *deadY, u32 deadCount, u8 room)
{
    u32 i;

    do
        i = 1 + IsaacRandom() % (deadCount - 1);
    while (sFloor.room[deadY[i]][deadX[i]] != ROOM_NORMAL);
    sFloor.room[deadY[i]][deadX[i]] = room;
}

static bool8 TryBuildFloor(u32 floor)
{
    u8 deadX[ISAAC_GRID_W * ISAAC_GRID_H];
    u8 deadY[ISAAC_GRID_W * ISAAC_GRID_H];
    u32 target = GetTargetRoomCount(floor);
    u32 needed = (floor >= ISAAC_SHOP_MIN_FLOOR) ? 4 : 3;  // boss, trésor, capture (+ boutique, + 2e trésor)
    u32 deadCount;

    if (floor >= ISAAC_TWO_TREASURES_MIN_FLOOR)
        needed++;
    GrowFloor(target);
    if (sFloor.roomCount < target)
        return FALSE;
    deadCount = FindDeadEnds(deadX, deadY);
    if (deadCount < needed)
        return FALSE;

    // Boss sur le cul-de-sac le plus éloigné, les autres spéciales au hasard parmi le reste
    sFloor.room[deadY[0]][deadX[0]] = ROOM_BOSS;
    PlaceOnFreeDeadEnd(deadX, deadY, deadCount, ROOM_TREASURE);
    if (floor >= ISAAC_TWO_TREASURES_MIN_FLOOR)
        PlaceOnFreeDeadEnd(deadX, deadY, deadCount, ROOM_TREASURE);
    PlaceOnFreeDeadEnd(deadX, deadY, deadCount, ROOM_CAPTURE);
    if (floor >= ISAAC_SHOP_MIN_FLOOR)
        PlaceOnFreeDeadEnd(deadX, deadY, deadCount, ROOM_SHOP);
    if (ISAAC_SECRET_ROOMS && floor >= ISAAC_SECRET_MIN_FLOOR)
        PlaceSecretRoom();
    return TRUE;
}

// Repli si aucune graine ne marche (ne devrait pas arriver) : départ, une salle, boss, trésor, capture
static void BuildFallbackFloor(void)
{
    memset(&sFloor, 0, sizeof(sFloor));
    sFloor.room[ISAAC_START_Y][ISAAC_START_X] = ROOM_START;
    sFloor.room[ISAAC_START_Y][ISAAC_START_X + 1] = ROOM_NORMAL;
    sFloor.room[ISAAC_START_Y][ISAAC_START_X + 2] = ROOM_BOSS;
    sFloor.room[ISAAC_START_Y][ISAAC_START_X - 1] = ROOM_TREASURE;
    sFloor.room[ISAAC_START_Y - 1][ISAAC_START_X] = ROOM_CAPTURE;
    sFloor.roomCount = 5;
}

static void BuildFloorPlan(void)
{
    u32 floor = VarGet(VAR_ISAAC_FLOOR);
    u32 try;

    for (try = 0; try < ISAAC_MAX_TRIES; try++)
    {
        sSeed = VarGet(VAR_ISAAC_SEED) + try * 0x9E3779B9;
        if (TryBuildFloor(floor))
            return;
    }
    BuildFallbackFloor();
}

static void GetCurrentCell(s32 *x, s32 *y)
{
    u32 room = VarGet(VAR_ISAAC_ROOM);

    *x = room % ISAAC_GRID_W;
    *y = room / ISAAC_GRID_W;
}

static void SetCurrentCell(s32 x, s32 y)
{
    VarSet(VAR_ISAAC_ROOM, y * ISAAC_GRID_W + x);
}

static bool8 IsVisited(s32 x, s32 y)
{
    u32 cell = y * ISAAC_GRID_W + x;

    if (!CellInGrid(x, y))
        return FALSE;
    return (VarGet(sVisitedVars[cell / 16]) >> (cell % 16)) & 1;
}

static void SetVisited(s32 x, s32 y)
{
    u32 cell = y * ISAAC_GRID_W + x;

    VarSet(sVisitedVars[cell / 16], VarGet(sVisitedVars[cell / 16]) | (1 << (cell % 16)));
}

static bool8 IsCleared(s32 x, s32 y)
{
    return FlagGet(FLAG_ISAAC_CLEARED_FIRST + y * ISAAC_GRID_W + x);
}

static void ClearClearedRooms(void)
{
    u32 i;

    for (i = 0; i < ISAAC_GRID_W * ISAAC_GRID_H; i++)
        FlagClear(FLAG_ISAAC_CLEARED_FIRST + i);
}

static void ClearVisited(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sVisitedVars); i++)
        VarSet(sVisitedVars[i], 0);
}

// ---------------------------------------------------------------------------
// Dessin de la salle
// ---------------------------------------------------------------------------

static bool8 InPaintArea(s32 x, s32 y)
{
    return x >= PAINT_MIN && y >= PAINT_MIN && x < PAINT_MIN + PAINT_W && y < PAINT_MIN + PAINT_H;
}

// Hors de la zone peinte = mur. Les portes cachées comptent comme du mur pour l'habillage.
static bool8 IsWallAt(s32 x, s32 y)
{
    return !InPaintArea(x, y) || sCell[y - PAINT_MIN][x - PAINT_MIN] != CELL_FLOOR;
}

static void FillRect(s32 x0, s32 y0, s32 x1, s32 y1, u8 cell)
{
    s32 x, y;

    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            if (InPaintArea(x, y))
                sCell[y - PAINT_MIN][x - PAINT_MIN] = cell;
}

// Porte : couloir de 1 case, de l'intérieur jusqu'au bout de la marge (il s'enfonce dans le noir).
// Une porte cachée ne couvre que les cases de mur de la map.
static void CarveDoor(u32 dir, u8 cell)
{
    s32 far = (cell == CELL_HIDDEN) ? 0 : MAP_OFFSET;

    switch (dir)
    {
    case ISAAC_DIR_NORTH:
        FillRect(ROOM_MID_X, -far, ROOM_MID_X, ROOM_MIN_Y - 1, cell);
        break;
    case ISAAC_DIR_SOUTH:
        FillRect(ROOM_MID_X, ROOM_MAX_Y + 1, ROOM_MID_X, ISAAC_ROOM_H - 1 + far, cell);
        break;
    case ISAAC_DIR_WEST:
        FillRect(-far, ROOM_MID_Y, ROOM_MIN_X - 1, ROOM_MID_Y, cell);
        break;
    case ISAAC_DIR_EAST:
        FillRect(ROOM_MAX_X + 1, ROOM_MID_Y, ISAAC_ROOM_W - 1 + far, ROOM_MID_Y, cell);
        break;
    }
}

// Salle du boss (un cul-de-sac) : le passage vers l'étage suivant s'ouvre dans le mur d'en face de sa porte
static u32 GetBossExitDir(s32 x, s32 y)
{
    u32 dir;

    for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
        if (GetRoom(x + sDirDx[dir], y + sDirDy[dir]) != ROOM_NONE)
            return (dir + 2) % ISAAC_DIR_COUNT;
    return ISAAC_DIR_SOUTH;
}

static void CarveRoom(void)
{
    s32 x, y;
    u32 dir;
    u8 here, there;

    GetCurrentCell(&x, &y);
    here = GetRoom(x, y);
    memset(sCell, CELL_WALL, sizeof(sCell));
    FillRect(ROOM_MIN_X, ROOM_MIN_Y, ROOM_MAX_X, ROOM_MAX_Y, CELL_FLOOR);
    for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
    {
        there = GetRoom(x + sDirDx[dir], y + sDirDy[dir]);
        if (there == ROOM_NONE)
            continue;
        if (there == ROOM_SECRET && here != ROOM_SECRET && !FlagGet(FLAG_ISAAC_SECRET_OPEN))
            CarveDoor(dir, CELL_HIDDEN);
        else
            CarveDoor(dir, CELL_FLOOR);
    }
    if (here == ROOM_BOSS && IsCleared(x, y))  // boss vaincu : passage vers l'étage suivant
        CarveDoor(GetBossExitDir(x, y), CELL_FLOOR);
}

static void SetTile(s32 x, s32 y, u16 entry)
{
    MapGridSetMetatileEntryAt(x + MAP_OFFSET, y + MAP_OFFSET, entry);
}

static bool8 IsBigRockAt(s32 x, s32 y)
{
    if (!InPaintArea(x, y))
        return TRUE;
    return sIsBigRock[y - PAINT_MIN][x - PAINT_MIN];
}

// Case de sol d'un couloir (hors du rectangle de la salle)
static bool8 IsCorridorCell(s32 x, s32 y)
{
    return !IsWallAt(x, y) && (x < ROOM_MIN_X || x > ROOM_MAX_X || y < ROOM_MIN_Y || y > ROOM_MAX_Y);
}

// Une case de mur est un « gros rocher » si elle fait partie d'un carré de mur 3 x 3
static void ComputeBigRocks(void)
{
    s32 x, y, dx, dy;
    bool8 full;

    memset(sIsBigRock, 0, sizeof(sIsBigRock));
    for (y = PAINT_MIN - 2; y < PAINT_MIN + PAINT_H; y++)
    {
        for (x = PAINT_MIN - 2; x < PAINT_MIN + PAINT_W; x++)
        {
            full = TRUE;
            for (dy = 0; dy < 3 && full; dy++)
                for (dx = 0; dx < 3 && full; dx++)
                    full = IsWallAt(x + dx, y + dy);
            if (!full)
                continue;
            for (dy = 0; dy < 3; dy++)
                for (dx = 0; dx < 3; dx++)
                    if (InPaintArea(x + dx, y + dy))
                        sIsBigRock[y + dy - PAINT_MIN][x + dx - PAINT_MIN] = TRUE;
        }
    }
}

static u16 GetCliffRoleTile(s32 x, s32 y)
{
    bool8 up, down, left, right;

    up    = IsBigRockAt(x, y - 1);
    down  = IsBigRockAt(x, y + 1);
    left  = IsBigRockAt(x - 1, y);
    right = IsBigRockAt(x + 1, y);

    if (!up)
        return !left ? TILE_CLIFF_TL : (!right ? TILE_CLIFF_TR : TILE_CLIFF_T);
    if (!down)
        return !left ? TILE_CLIFF_BL : (!right ? TILE_CLIFF_BR : TILE_CLIFF_B);
    if (!left)
        return TILE_CLIFF_L;
    if (!right)
        return TILE_CLIFF_R;
    if (!IsBigRockAt(x + 1, y + 1))
        return TILE_CLIFF_IN_SE;
    if (!IsBigRockAt(x - 1, y + 1))
        return TILE_CLIFF_IN_SW;
    if (!IsBigRockAt(x + 1, y - 1))
        return TILE_CLIFF_IN_NE;
    if (!IsBigRockAt(x - 1, y - 1))
        return TILE_CLIFF_IN_NW;
    return TILE_CLIFF_C;
}

// Nombre de cases jusqu'au sol le plus proche dans la direction (dx, dy), ou TREE_FAR s'il est trop loin
#define TREE_FAR  99

static s32 DistanceToFloor(s32 x, s32 y, s32 dx, s32 dy)
{
    s32 d;

    for (d = 1; d < 9; d++)
        if (!IsWallAt(x + dx * d, y + dy * d))
            return d;
    return TREE_FAR;
}

// Case de mur d'une forêt : quel demi-arbre, pour que les arbres de 2 x 2 soient entiers contre le sol le plus proche
// (le tronc juste au-dessus du sol, le côté de l'arbre contre le sol à gauche ou à droite). Sans sol dans la ligne ou
// la colonne, les arbres sont calés sur le mur de la salle le plus proche (la salle fait un nombre impair de cases :
// les deux moitiés se rejoignent au milieu par un arbre en partie caché).
static u16 GetTreeTile(s32 x, s32 y)
{
    s32 down = DistanceToFloor(x, y, 0, 1), up = DistanceToFloor(x, y, 0, -1);
    s32 right = DistanceToFloor(x, y, 1, 0), left = DistanceToFloor(x, y, -1, 0);
    u16 tile;
    bool32 rightHalf;

    if (down == TREE_FAR && up == TREE_FAR)
    {
        if (y <= ROOM_MID_Y)
            down = ROOM_MIN_Y - y;
        else
            up = y - ROOM_MAX_Y;
    }
    if (down <= up)
        tile = (down == 1) ? TILE_TREE_TRUNK : ((down & 1) ? TILE_TREE_BOTTOM : TILE_TREE_TOP);
    else
        tile = (up & 1) ? TILE_TREE_TOP : TILE_TREE_BOTTOM;

    if (right == TREE_FAR && left == TREE_FAR)
    {
        if (x <= ROOM_MID_X)
            right = ROOM_MIN_X - x;
        else
            left = x - ROOM_MAX_X;
    }
    if (right <= left)
        rightHalf = right & 1;
    else
        rightHalf = !(left & 1);
    return tile + rightHalf;
}

// Falaise : la salle est le dessus d'un plateau dont les bords sont sur deux niveaux : deux rangées de paroi au sud,
// deux flancs à l'ouest et à l'est (ROOM_MIN_X - 2 à ROOM_MAX_X + 2), la crête au nord. Renvoie 0 hors de ces bords.
static u16 GetCliffPlatformTile(s32 x, s32 y)
{
    if (x < ROOM_MIN_X - 2 || x > ROOM_MAX_X + 2 || y < ROOM_MIN_Y - 1 || y > ROOM_MAX_Y + 2)
        return 0;
    if (y == ROOM_MIN_Y - 1)
        return (x == ROOM_MIN_X - 2) ? TILE_CLIFF_IN_SE : ((x == ROOM_MAX_X + 2) ? TILE_CLIFF_IN_SW : TILE_CLIFF_B);
    if (y <= ROOM_MAX_Y)
        return (x < ROOM_MIN_X) ? TILE_CLIFF_R : ((x > ROOM_MAX_X) ? TILE_CLIFF_L : 0);
    if (y == ROOM_MAX_Y + 1)
        return (x == ROOM_MIN_X - 2) ? TILE_CLIFF_IN_NE : ((x == ROOM_MAX_X + 2) ? TILE_CLIFF_IN_NW : TILE_CLIFF_T);
    return (x == ROOM_MIN_X - 2) ? TILE_CLIFF_IN_NE2 : ((x == ROOM_MAX_X + 2) ? TILE_CLIFF_IN_NW2 : TILE_CLIFF_T2);
}

// Falaise : les couloirs sont des crêtes au-dessus du vide. Couloir est-ouest : on marche sur sa crête, sa paroi (deux
// rangées) est dessous, le vide au-dessus ; il traverse à plat les deux flancs du plateau. Couloir nord-sud : une bande
// de dessus de montagne bordée de ses deux flancs ; au nord, ils se raccordent à la crête de la salle, au sud, la paroi
// de la salle s'arrête de chaque côté. Renvoie 0 hors des abords des couloirs.
static bool8 IsVerticalCorridorCell(s32 x, s32 y)
{
    return IsCorridorCell(x, y) && x >= ROOM_MIN_X && x <= ROOM_MAX_X;
}

static bool8 IsHorizontalCorridorCell(s32 x, s32 y)
{
    return IsCorridorCell(x, y) && (x < ROOM_MIN_X || x > ROOM_MAX_X);
}

static u16 GetCliffCorridorTile(s32 x, s32 y)
{
    s32 d;
    bool32 left;

    if (IsVerticalCorridorCell(x, y))
        return TILE_BRIDGE_V;
    if (IsHorizontalCorridorCell(x, y))
        return (x >= ROOM_MIN_X - 2 && x <= ROOM_MAX_X + 2) ? TILE_FLOOR : TILE_BRIDGE_H;
    if (IsVerticalCorridorCell(x + 1, y) || IsVerticalCorridorCell(x - 1, y))  // flancs du couloir nord-sud
    {
        left = IsVerticalCorridorCell(x + 1, y);
        if (y == ROOM_MIN_Y - 1)
            return left ? TILE_CLIFF_BL : TILE_CLIFF_BR;
        if (y == ROOM_MAX_Y + 1)
            return left ? TILE_CLIFF_TR : TILE_CLIFF_TL;
        if (y == ROOM_MAX_Y + 2)
            return left ? TILE_CLIFF_IN_NW2 : TILE_CLIFF_IN_NE2;
        return left ? TILE_CLIFF_R : TILE_CLIFF_L;
    }
    if (x >= ROOM_MIN_X - 2 && x <= ROOM_MAX_X + 2)  // contre le plateau : ses flancs
        return 0;
    if (IsHorizontalCorridorCell(x, y + 1))  // au-dessus du couloir est-ouest
        return TILE_CLIFF_C;
    for (d = 1; d <= 2; d++)  // sous le couloir est-ouest : sa paroi
        if (IsHorizontalCorridorCell(x, y - d))
            return (d == 1) ? TILE_CLIFF_T : TILE_CLIFF_T2;
    return 0;
}

static u8 GetDoorMat(u8 room)
{
    switch (room)
    {
    case ROOM_BOSS:
        return MAT_BOSS;
    case ROOM_TREASURE:
        return MAT_TREASURE;
    case ROOM_SHOP:
        return MAT_SHOP;
    case ROOM_CAPTURE:
        return MAT_CAPTURE;
    default:  // pas la salle secrète : elle doit le rester
        return MAT_NONE;
    }
}

// Paillasson devant chaque porte qui mène à une salle spéciale, avec le symbole de la mini-carte
static void PaintDoorMats(s32 cellX, s32 cellY)
{
    // Coin haut gauche du paillasson et taille (en cases), par porte
    static const s8 sMatX[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = ROOM_MID_X - 1, [ISAAC_DIR_SOUTH] = ROOM_MID_X - 1,
                                               [ISAAC_DIR_WEST] = ROOM_MIN_X, [ISAAC_DIR_EAST] = ROOM_MAX_X - 1 };
    static const s8 sMatY[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = ROOM_MIN_Y, [ISAAC_DIR_SOUTH] = ROOM_MAX_Y - 1,
                                               [ISAAC_DIR_WEST] = ROOM_MID_Y - 1, [ISAAC_DIR_EAST] = ROOM_MID_Y - 1 };
    static const u8 sMatOrientation[ISAAC_DIR_COUNT] = { [ISAAC_DIR_NORTH] = MAT_HORIZONTAL, [ISAAC_DIR_SOUTH] = MAT_HORIZONTAL,
                                                         [ISAAC_DIR_WEST] = MAT_WEST, [ISAAC_DIR_EAST] = MAT_EAST };
    u32 dir, i, w;
    u8 mat;

    for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
    {
        mat = GetDoorMat(GetRoom(cellX + sDirDx[dir], cellY + sDirDy[dir]));
        if (mat == MAT_NONE)
            continue;
        w = (sMatOrientation[dir] == MAT_HORIZONTAL) ? 3 : 2;
        for (i = 0; i < 6; i++)
            SetTile(sMatX[dir] + i % w, sMatY[dir] + i / w,
                    MAT_TILE(mat, sMatOrientation[dir], i) | PACK_ELEVATION(FLOOR_ELEVATION));
    }
}

// Décorations des coins de la salle, semi-aléatoires (toujours les mêmes pour une salle) : dans chaque coin, rien
// (DECO_EMPTY_PCT %) ou une des deux décorations du décor, parfois doublée d'une seconde le long du mur
// (DECO_PAIR_PCT %). Elles sont infranchissables ; elles ne vont que sur du sol, loin des portes et du centre.
#define DECO_EMPTY_PCT  30
#define DECO_PAIR_PCT   40

static u32 RoomRandom(s32 x, s32 y, u32 salt);

static void PaintCornerDecorations(s32 cellX, s32 cellY)
{
    static const s8 sCornerX[] = { ROOM_MIN_X, ROOM_MAX_X, ROOM_MIN_X, ROOM_MAX_X };
    static const s8 sCornerY[] = { ROOM_MIN_Y, ROOM_MIN_Y, ROOM_MAX_Y, ROOM_MAX_Y };
    static const s8 sAlongWallDx[] = { 1, -1, 1, -1 };
    u32 corner, roll, i, count;
    s32 x, y;

    for (corner = 0; corner < ARRAY_COUNT(sCornerX); corner++)
    {
        roll = RoomRandom(cellX, cellY, 0xDEC0 + corner);
        if (roll % 100 < DECO_EMPTY_PCT)
            continue;
        count = ((roll >> 8) % 100 < DECO_PAIR_PCT) ? 2 : 1;
        for (i = 0; i < count; i++)
        {
            x = sCornerX[corner] + i * sAlongWallDx[corner];
            y = sCornerY[corner];
            if (IsWallAt(x, y))
                continue;
            SetTile(x, y, (((roll >> (16 + i)) & 1) ? TILE_DECO_2 : TILE_DECO_1)
                          | MAPGRID_IMPASSABLE | PACK_ELEVATION(FLOOR_ELEVATION));
        }
    }
}

static void PaintRoom(void)
{
    s32 x, y, cellX, cellY;
    u16 tile;

    ComputeBigRocks();
    for (y = PAINT_MIN; y < PAINT_MIN + PAINT_H; y++)
    {
        for (x = PAINT_MIN; x < PAINT_MIN + PAINT_W; x++)
        {
            if (gIsaacBiomes[ISAAC_BIOME].trees && IsWallAt(x, y))
                tile = GetTreeTile(x, y);
            else if (gIsaacBiomes[ISAAC_BIOME].cliff && GetCliffCorridorTile(x, y) != 0)
                tile = GetCliffCorridorTile(x, y);
            else if (gIsaacBiomes[ISAAC_BIOME].cliff && GetCliffPlatformTile(x, y) != 0)
                tile = GetCliffPlatformTile(x, y);
            else if (gIsaacBiomes[ISAAC_BIOME].cliff && IsWallAt(x, y))
                tile = TILE_CLIFF_C;
            else if (IsBigRockAt(x, y))
                tile = GetCliffRoleTile(x, y);
            else if (IsWallAt(x, y))  // bout de mur trop fin pour une falaise : rocher isolé
                tile = (IsaacRandom() & 1) ? TILE_ROCK_1 : TILE_ROCK_2;
            else
                tile = TILE_FLOOR;

            if (sCell[y - PAINT_MIN][x - PAINT_MIN] != CELL_FLOOR)
                SetTile(x, y, tile | MAPGRID_IMPASSABLE);
            else
                SetTile(x, y, tile | PACK_ELEVATION(FLOOR_ELEVATION));
        }
    }

    GetCurrentCell(&cellX, &cellY);
    PaintCornerDecorations(cellX, cellY);
    PaintDoorMats(cellX, cellY);
    // Océan : Kecleon (boutique) se tient sur un banc de sable, on lui parle sans quitter l'eau
    if (gIsaacBiomes[ISAAC_BIOME].ocean && GetRoom(cellX, cellY) == ROOM_SHOP)
        SetTile(ROOM_MID_X, ROOM_MID_Y, TILE_SANDBAR | MAPGRID_IMPASSABLE | PACK_ELEVATION(FLOOR_ELEVATION));
}

// ---------------------------------------------------------------------------
// Dresseurs
// ---------------------------------------------------------------------------

static const u8 *const sMaleTrainerNames[] = {
    COMPOUND_STRING("ROCKY"), COMPOUND_STRING("HUGO"), COMPOUND_STRING("MARCO"), COMPOUND_STRING("LÉON"),
    COMPOUND_STRING("PAUL"), COMPOUND_STRING("THÉO"), COMPOUND_STRING("BASILE"), COMPOUND_STRING("YVES"),
};
static const u8 *const sFemaleTrainerNames[] = {
    COMPOUND_STRING("LINA"), COMPOUND_STRING("EMMA"), COMPOUND_STRING("CLARA"), COMPOUND_STRING("JADE"),
    COMPOUND_STRING("ROSE"), COMPOUND_STRING("NINA"), COMPOUND_STRING("MAËL"), COMPOUND_STRING("IRIS"),
};

// Après sa défaite, le dresseur retourne au centre de la salle (voir IsaacPrepareTrainerReturn)
EWRAM_DATA u8 gIsaacTrainerReturnMovement[ISAAC_ROOM_W + 2] = {0};

static EWRAM_DATA struct Trainer sRoomTrainer = {0};
static EWRAM_DATA struct TrainerMon sRoomTrainerMons[PARTY_SIZE] = {0};

// Équipe des dresseurs de salle selon l'étage : maxCount Pokémon (maxCountPct % des salles), sinon un de moins ;
// chaque Pokémon tire sa catégorie et son set au hasard entre les deux bornes (au-delà du dernier étage, on garde la
// dernière ligne), parmi les espèces de BST au moins minBst (plancher qui adoucit le passage d'une catégorie à la
// suivante). tierByCategory : le set n'est pas tiré mais suit la catégorie à l'envers (catégorie la plus faible
// -> meilleur set)
struct IsaacTrainerParty
{
    u8 maxCount, maxCountPct;
    u8 minCategory, maxCategory;
    u8 minTier, maxTier;  // 0 = set 1
    bool8 tierByCategory;
    u16 minBst;
};

static const struct IsaacTrainerParty sRoomTrainerParties[] =
{
    { 2,  60, ISAAC_CATEGORY_1, ISAAC_CATEGORY_1, 0, 0, FALSE, 210 },
    { 2, 100, ISAAC_CATEGORY_1, ISAAC_CATEGORY_2, 0, 1, TRUE,  265 },  // étage 2 : catégorie 1 en set 2, catégorie 2 en set 1
    { 3,  60, ISAAC_CATEGORY_2, ISAAC_CATEGORY_2, 1, 1, FALSE, 330 },
    { 3,  75, ISAAC_CATEGORY_2, ISAAC_CATEGORY_3, 1, 2, FALSE, 360 },
    { 4,  50, ISAAC_CATEGORY_3, ISAAC_CATEGORY_3, 1, 2, FALSE, 430 },
    { 4,  70, ISAAC_CATEGORY_3, ISAAC_CATEGORY_4, 2, 2, FALSE, 455 },
};

// Équipe du boss selon l'étage. Principe : le boss garde toujours un cran d'avance sur les dresseurs
// de son étage (un Pokémon de plus, catégorie et set au moins égaux), et son dernier Pokémon est un
// « as » d'une catégorie au-dessus du reste, pour finir le combat sur une vraie menace.
// En plus du tableau (voir IsaacBoostTrainerParty et GetIsaacTrainer) :
//  - pas d'Omniboost sur l'équipe, seulement sur l'as : aceBoosts Omniboosts (étage 5) ou une Pierre brillante
//    (aceBrilliant, étage 6 : chromatique, meilleure stat +20 %) ;
//  - l'as du dernier étage est un petit légendaire, choisi selon le thème du boss (sBossLegendaries) ;
//  - l'IA est « Smart » à tous les étages ;
//  - les espèces respectent le même plancher de BST que les dresseurs de l'étage (sRoomTrainerParties).
struct IsaacBossParty
{
    u8 count;  // as compris
    u8 minCategory, maxCategory;
    u8 minTier, maxTier;  // 0 = set 1
    u8 aceCategory, aceTier, aceBoosts;
    bool8 aceBrilliant;
};

static const struct IsaacBossParty sBossParties[] =
{
    { 3, ISAAC_CATEGORY_1, ISAAC_CATEGORY_1, 0, 0, ISAAC_CATEGORY_2,               1, 0, FALSE },
    { 4, ISAAC_CATEGORY_1, ISAAC_CATEGORY_2, 1, 1, ISAAC_CATEGORY_3,               1, 0, FALSE },
    { 4, ISAAC_CATEGORY_2, ISAAC_CATEGORY_3, 1, 1, ISAAC_CATEGORY_3,               1, 0, FALSE },
    { 5, ISAAC_CATEGORY_3, ISAAC_CATEGORY_3, 1, 1, ISAAC_CATEGORY_4,               1, 0, FALSE },
    { 5, ISAAC_CATEGORY_3, ISAAC_CATEGORY_4, 1, 2, ISAAC_CATEGORY_4,               2, 1, FALSE },
    { 6, ISAAC_CATEGORY_4, ISAAC_CATEGORY_4, 1, 2, ISAAC_CATEGORY_LEGENDARY_SMALL, 2, 0, TRUE },
};

// Petit légendaire de l'as du dernier étage, au thème du biome et de son boss
// (une case vide reprend le premier, pour tirer à égalité entre ceux qui sont listés)
static const u16 sBossLegendaries[ISAAC_BIOME_COUNT][3] =
{
    [ISAAC_BIOME_CAVE]   = { SPECIES_REGIROCK, SPECIES_ZAPDOS_GALAR, SPECIES_REGIROCK }, // Pierre (roche, combat)
    [ISAAC_BIOME_DARK]   = { SPECIES_DEOXYS,   SPECIES_MOLTRES_GALAR, SPECIES_DEOXYS },  // Spectra (ténèbres)
    [ISAAC_BIOME_FIRE]   = { SPECIES_MOLTRES,  SPECIES_ENTEI,     SPECIES_RAIKOU },    // Adriane (feu, électrik)
    [ISAAC_BIOME_ICE]    = { SPECIES_ARTICUNO, SPECIES_REGICE,    SPECIES_REGISTEEL }, // Glacia (glace, acier)
    [ISAAC_BIOME_POISON] = { SPECIES_MEW,      SPECIES_MEW,       SPECIES_MEW },       // Koga (poison)
    [ISAAC_BIOME_OCEAN]  = { SPECIES_SUICUNE,  SPECIES_SUICUNE,   SPECIES_SUICUNE },   // Ondine (eau)
    [ISAAC_BIOME_DESERT] = { SPECIES_JIRACHI,  SPECIES_ARTICUNO_GALAR, SPECIES_DEOXYS }, // Giovanni (sol, psy)
    [ISAAC_BIOME_CLIFF]  = { SPECIES_LATIAS,   SPECIES_LATIOS,    SPECIES_ZAPDOS },    // Alizée (vol, dragon)
    [ISAAC_BIOME_FOREST] = { SPECIES_CELEBI,   SPECIES_CELEBI,    SPECIES_CELEBI },    // Erika (plante)
};

static const struct IsaacTrainerParty *GetRoomTrainerParty(u32 floor)
{
    return &sRoomTrainerParties[min(max(floor, 1u), ARRAY_COUNT(sRoomTrainerParties)) - 1];
}

static const struct IsaacBossParty *GetBossParty(void)
{
    return &sBossParties[min(VarGet(VAR_ISAAC_FLOOR), ARRAY_COUNT(sBossParties)) - 1];
}

// Cellule de la salle du boss de l'étage
static void GetBossCell(s32 *outX, s32 *outY)
{
    s32 x, y;

    *outX = *outY = 0;
    for (y = 0; y < ISAAC_GRID_H; y++)
    {
        for (x = 0; x < ISAAC_GRID_W; x++)
        {
            if (GetRoom(x, y) == ROOM_BOSS)
            {
                *outX = x;
                *outY = y;
            }
        }
    }
}

// Hasard propre à une salle de l'étage (toujours le même pour une salle donnée)
static u32 RoomRandom(s32 x, s32 y, u32 salt)
{
    u32 r = VarGet(VAR_ISAAC_SEED) * 2654435761u ^ (VarGet(VAR_ISAAC_FLOOR) << 16) ^ ((y * ISAAC_GRID_W + x + 1) * 0x45D9F3B) ^ salt;

    r ^= r >> 16;
    r *= 0x7FEB352D;
    r ^= r >> 15;
    r *= 0x846CA68B;
    r ^= r >> 16;
    return r;
}

// Contenu de la salle secrète, tiré pour la salle : un dresseur du biome avec l'équipe de l'étage suivant
// (SECRET_TRAINER_PCT %), sinon un butin de sa table (IsaacPickTreasureItem)
enum
{
    SECRET_LOOT,
    SECRET_TRAINER,
};
#define SECRET_TRAINER_PCT  50

static u32 GetSecretRoomContent(s32 x, s32 y)
{
    return RoomRandom(x, y, 0x5EC7) % 100 < SECRET_TRAINER_PCT ? SECRET_TRAINER : SECRET_LOOT;
}

static s32 GetForcedRockSmashRoom(void);

// Dresseur d'une salle normale, avant l'éventuelle salle libérée pour l'entraîneur d'Éclate-Roc
static bool8 NormalRoomRollsTrainer(s32 x, s32 y)
{
    return GetRoom(x, y) == ROOM_NORMAL && RoomRandom(x, y, 0x7A1) % 100 < ISAAC_TRAINER_PCT;
}

static bool8 RoomHasTrainer(s32 x, s32 y)
{
    if (GetRoom(x, y) == ROOM_SECRET)
        return GetSecretRoomContent(x, y) == SECRET_TRAINER;
    return NormalRoomRollsTrainer(x, y) && y * ISAAC_GRID_W + x != GetForcedRockSmashRoom();
}

static const struct IsaacTrainerType *GetRoomTrainerType(s32 x, s32 y)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];

    return &biome->trainers[RoomRandom(x, y, 0x7E3) % biome->trainerCount];
}

// Ajoute un Pokémon à l'équipe du dresseur ou du boss : quel que soit le type de dresseur, une espèce au hasard
// parmi toutes les lignées du biome, de la catégorie voulue, jamais une espèce déjà dans l'équipe (s'il n'en reste
// aucune de cette catégorie, la plus proche, en dessous d'abord), avec le set voulu (variante 50/50). Le petit
// légendaire de l'as du boss vient de sBossLegendaries.
static void AddTrainerMon(enum IsaacCategory category, u32 tier, u32 seed, u32 minBst)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];
    u16 candidates[96];
    u32 count, kept, i, j, k, delta;
    s32 cat;
    u16 species = SPECIES_NONE;

    if (category >= ISAAC_CATEGORY_LEGENDARY_SMALL)
    {
        species = sBossLegendaries[ISAAC_BIOME][seed % ARRAY_COUNT(sBossLegendaries[0])];
    }
    else
    {
        for (delta = 0; delta < ISAAC_CATEGORY_4 && species == SPECIES_NONE; delta++)
        {
            for (i = 0; i < 2 && species == SPECIES_NONE; i++)
            {
                cat = (s32)category + (i == 0 ? -(s32)delta : (s32)delta);
                if (cat < ISAAC_CATEGORY_1 || cat > ISAAC_CATEGORY_4 || (i == 1 && delta == 0))
                    continue;
                count = IsaacAddLinesSpeciesOfCategory(biome->lines, biome->lineCount, cat, candidates, 0, ARRAY_COUNT(candidates));
                // sans les espèces déjà dans l'équipe, ni celles sous le plancher de BST
                for (kept = 0, j = 0; j < count; j++)
                {
                    if (IsaacGetSpeciesBST(candidates[j]) < minBst)
                        continue;
                    for (k = 0; k < sRoomTrainer.partySize && IsaacGetSharedFormBase(sRoomTrainerMons[k].species) != candidates[j]; k++)
                        ;
                    if (k == sRoomTrainer.partySize)
                        candidates[kept++] = candidates[j];
                }
                if (kept != 0)
                    species = IsaacRollSharedForm(candidates[seed % kept], seed >> 12);
            }
        }
        if (species == SPECIES_NONE)
            species = biome->lines[0];
    }
    sRoomTrainerMons[sRoomTrainer.partySize] = *IsaacGetSet(species, tier, (seed >> 24) % ISAAC_SET_VARIANTS);
    sRoomTrainerMons[sRoomTrainer.partySize].lvl = ISAAC_MON_LEVEL;
    sRoomTrainer.partySize++;
}

static void InitTrainer(struct Trainer *trainer, const struct IsaacTrainerType *type, const u8 *name, struct TrainerMon *party)
{
    memset(trainer, 0, sizeof(*trainer));
    trainer->trainerClass = type->trainerClass;
    trainer->trainerPic = type->trainerPic;
    trainer->gender = type->gender;
    trainer->encounterMusic = type->encounterMusic;
    trainer->battleType = TRAINER_BATTLE_TYPE_SINGLES;
    trainer->aiFlags = AI_FLAG_BASIC_TRAINER;
    trainer->multiTeamSize = MULTI_TEAM_SIZE_FULL;
    trainer->party = party;
    StringCopy(trainer->trainerName, name);
}

// Duelliste : un seul Pokémon, d'une catégorie et d'un palier de set au-dessus des meilleurs des dresseurs de l'étage
static const struct IsaacTrainerType sDuelistType =
    { TRAINER_CLASS_BLACK_BELT, TRAINER_PIC_BLACK_BELT, OBJ_EVENT_GFX_BLACK_BELT, TRAINER_GENDER_MALE, TRAINER_ENCOUNTER_MUSIC_INTENSE };
static const struct IsaacTrainerType sDuelistOceanType =
    { TRAINER_CLASS_SWIMMER_M, TRAINER_PIC_SWIMMER_M, OBJ_EVENT_GFX_SWIMMER_M, TRAINER_GENDER_MALE, TRAINER_ENCOUNTER_MUSIC_SWIMMER };

static const struct IsaacTrainerType *GetDuelistType(void)
{
    return gIsaacBiomes[ISAAC_BIOME].ocean ? &sDuelistOceanType : &sDuelistType;
}

static u32 GetDuelistCategory(void)
{
    return min(sRoomTrainerParties[min(VarGet(VAR_ISAAC_FLOOR), ARRAY_COUNT(sRoomTrainerParties)) - 1].maxCategory + 1, ISAAC_CATEGORY_4);
}

static u32 GetDuelistTier(void)
{
    return min(sRoomTrainerParties[min(VarGet(VAR_ISAAC_FLOOR), ARRAY_COUNT(sRoomTrainerParties)) - 1].maxTier + 1, ISAAC_SET_TIERS - 1);
}

bool8 IsIsaacTrainer(u16 trainerId)
{
    return (trainerId == TRAINER_ISAAC_ROOM || trainerId == TRAINER_ISAAC_BOSS || trainerId == TRAINER_ISAAC_DUELIST)
        && IsInRoomMap();
}

// Dresseur de la salle courante (classe et nom selon son type, équipe selon sRoomTrainerParties) ou boss de
// l'étage (champion du biome : 2 Pokémon de catégorie 1 et 1 de catégorie 2, set 2)
const struct Trainer *GetIsaacTrainer(u16 trainerId)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];
    const struct IsaacTrainerType *type;
    const struct IsaacTrainerParty *party;
    const u8 *const *names;
    s32 x, y;
    u32 i, count;

    GetCurrentCell(&x, &y);
    if (trainerId == TRAINER_ISAAC_DUELIST)
    {
        InitTrainer(&sRoomTrainer, GetDuelistType(), COMPOUND_STRING("DUELLISTE"), sRoomTrainerMons);
        sRoomTrainer.aiFlags = AI_FLAG_SMART_TRAINER;
        AddTrainerMon(GetDuelistCategory(), GetDuelistTier(), RoomRandom(x, y, 0xD0E1), 0);
        return &sRoomTrainer;
    }
    if (trainerId == TRAINER_ISAAC_BOSS)
    {
        const struct IsaacBossParty *boss = GetBossParty();

        GetBossCell(&x, &y);  // la même équipe vue d'ailleurs (voyant)
        InitTrainer(&sRoomTrainer, biome->boss, biome->bossName, sRoomTrainerMons);
        sRoomTrainer.aiFlags = AI_FLAG_SMART_TRAINER;
        for (i = 0; i < boss->count - 1; i++)
        {
            u32 roll = RoomRandom(x, y, 0xB051 + i);
            u32 category = boss->minCategory + (roll >> 8) % (boss->maxCategory - boss->minCategory + 1);
            u32 tier = boss->minTier + (roll >> 16) % (boss->maxTier - boss->minTier + 1);

            AddTrainerMon(category, tier, roll, GetRoomTrainerParty(VarGet(VAR_ISAAC_FLOOR))->minBst);
        }
        // L'as, envoyé en dernier
        AddTrainerMon(boss->aceCategory, boss->aceTier, RoomRandom(x, y, 0xB0A5), GetRoomTrainerParty(VarGet(VAR_ISAAC_FLOOR))->minBst);
        return &sRoomTrainer;
    }

    type = GetRoomTrainerType(x, y);
    names = (type->gender == TRAINER_GENDER_FEMALE) ? sFemaleTrainerNames : sMaleTrainerNames;
    InitTrainer(&sRoomTrainer, type, names[RoomRandom(x, y, 0x4A3) % ARRAY_COUNT(sMaleTrainerNames)], sRoomTrainerMons);
    if (VarGet(VAR_ISAAC_FLOOR) == 1)
        sRoomTrainer.aiFlags = AI_FLAG_CHECK_BAD_MOVE;  // premier étage : attaques au hasard, sauf les inutiles
    // dresseur de la salle secrète : l'équipe de l'étage suivant
    party = GetRoomTrainerParty(VarGet(VAR_ISAAC_FLOOR) + (GetRoom(x, y) == ROOM_SECRET));
    count = party->maxCount - (RoomRandom(x, y, 0xC0F7) % 100 >= party->maxCountPct);
    for (i = 0; i < count; i++)
    {
        u32 roll = RoomRandom(x, y, 0x5EC + i);
        u32 category = party->minCategory + (roll >> 8) % (party->maxCategory - party->minCategory + 1);
        u32 tier = party->minTier + (roll >> 16) % (party->maxTier - party->minTier + 1);

        if (party->tierByCategory)
            tier = max((s32)party->maxTier - (s32)(category - party->minCategory), (s32)party->minTier);
        AddTrainerMon(category, tier, roll, party->minBst);
    }
    return &sRoomTrainer;
}

// Argent gagné contre un dresseur ou le boss du donjon (doublé avec la Pièce Porte-Bonheur)
u32 IsaacGetTrainerMoney(void)
{
    u32 money = ISAAC_TRAINER_MONEY_PER_FLOOR * VarGet(VAR_ISAAC_FLOOR);

    return CheckBagHasItem(ITEM_LUCKY_COIN, 1) ? money * 2 : money;
}

// Dresseurs de l'étage encore à battre, en comptant celui de la salle courante (déjà marquée nettoyée)
static u32 CountTrainersLeftOnFloor(void)
{
    s32 x, y, curX, curY;
    u32 left = 0;

    GetCurrentCell(&curX, &curY);
    for (y = 0; y < ISAAC_GRID_H; y++)
        for (x = 0; x < ISAAC_GRID_W; x++)
            if (RoomHasTrainer(x, y) && GetRoom(x, y) != ROOM_SECRET && (!IsCleared(x, y) || (x == curX && y == curY)))
                left++;
    return left;
}

// Butin d'un dresseur vaincu, donné objet par objet (IsaacNextLootItem) :
//  - jusqu'à l'étage TRAINER_LOOT_BOTH_FLOOR - 1 : une Guérison (TRAINER_LOOT_FULL_RESTORE_PCT %) ou un Rappel (toujours
//    le même pour une salle) ;
//  - à partir de l'étage TRAINER_LOOT_BOTH_FLOOR : une Guérison et un Rappel.
// À partir de ISAAC_MASTER_BALL_MIN_FLOOR, une Master Ball, une seule par étage : tant qu'elle n'est pas tombée, chaque
// dresseur la donne avec 1 chance sur le nombre de dresseurs encore à battre (1/8, 1/7... 1/1) ; battre tous les
// dresseurs de l'étage la donne donc à coup sûr. Le dresseur de la salle secrète, qu'on ne peut pas toujours atteindre,
// n'en fait pas partie (ni pour la donner, ni dans le compte). Elle remplace le soin, sauf à partir de
// TRAINER_LOOT_BOTH_FLOOR où elle s'y ajoute.
// En plus, TRAINER_LOOT_RARE_PCT % des dresseurs donnent un butin rare (PickTrainerRareLoot, toujours le même pour une
// salle) : un objet spécial, une très bonne CT, un objet de combat de la boutique ou un Rappel Max.
#define TRAINER_LOOT_FULL_RESTORE_PCT  66
#define TRAINER_LOOT_BOTH_FLOOR        4
#define TRAINER_LOOT_RARE_PCT          10

static u16 PickGoodTM(u32 roll);
static u16 PickTrainerRareLoot(u32 roll);

static EWRAM_DATA u16 sLoot[4] = {0};
static EWRAM_DATA u8 sLootCount = 0;

void IsaacPickTrainerLoot(void)
{
    s32 x, y;
    u32 left;
    bool32 both = VarGet(VAR_ISAAC_FLOOR) >= TRAINER_LOOT_BOTH_FLOOR;

    GetCurrentCell(&x, &y);
    sLootCount = 0;
    if (both)
    {
        sLoot[sLootCount++] = ITEM_FULL_RESTORE;
        sLoot[sLootCount++] = ITEM_REVIVE;
    }
    else
    {
        sLoot[sLootCount++] = (RoomRandom(x, y, 0x1007) % 100 < TRAINER_LOOT_FULL_RESTORE_PCT) ? ITEM_FULL_RESTORE : ITEM_REVIVE;
    }
    if (RoomRandom(x, y, 0x7E57) % 100 < TRAINER_LOOT_RARE_PCT)
        sLoot[sLootCount++] = PickTrainerRareLoot(RoomRandom(x, y, 0x7E58));
    if (VarGet(VAR_ISAAC_FLOOR) < ISAAC_MASTER_BALL_MIN_FLOOR || FlagGet(FLAG_ISAAC_MASTER_BALL_DROPPED)
     || GetRoom(x, y) == ROOM_SECRET)
        return;
    left = CountTrainersLeftOnFloor();
    if (left != 0 && Random() % left == 0)
    {
        if (!both)
            sLoot[0] = ITEM_MASTER_BALL;  // la Master Ball remplace le soin
        else
            sLoot[sLootCount++] = ITEM_MASTER_BALL;
        FlagSet(FLAG_ISAAC_MASTER_BALL_DROPPED);
    }
}

// Objet suivant du butin (dresseur, salle trésor ou secrète) dans VAR_0x8004 (ITEM_NONE quand il n'y en a plus),
// pour `giveitem VAR_0x8004`
void IsaacNextLootItem(void)
{
    u32 i;

    gSpecialVar_0x8004 = ITEM_NONE;
    if (sLootCount == 0)
        return;
    gSpecialVar_0x8004 = sLoot[0];
    for (i = 1; i < sLootCount; i++)
        sLoot[i - 1] = sLoot[i];
    sLootCount--;
}

// Bonus de l'as du boss (voir sBossParties), une fois son équipe créée pour le combat
void IsaacBoostTrainerParty(u16 trainerId, struct Pokemon *party)
{
    const struct IsaacBossParty *boss = GetBossParty();
    struct Pokemon *ace;
    u32 n, stat;

    if (trainerId != TRAINER_ISAAC_BOSS || !IsIsaacTrainer(trainerId) || sRoomTrainer.partySize == 0)
        return;
    IsaacGoldenHeartStart();  // le Cœur Doré utilisé depuis le sac, s'il y en a un
    IsaacBossEvolutionStart();
    ace = &party[sRoomTrainer.partySize - 1];
    for (n = boss->aceBoosts; n != 0; n--)
        IsaacAddOmniboost(ace);
    if (boss->aceBrilliant)
        IsaacUseBrilliantStone(ace, &stat);
}

static struct ObjectEventTemplate *GetObjectTemplate(u8 localId)
{
    u32 i;

    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
        if (gSaveBlock1Ptr->objectEventTemplates[i].localId == localId)
            return &gSaveBlock1Ptr->objectEventTemplates[i];
    return NULL;
}

// Comportement du dresseur d'une salle normale, tiré pour la salle (une chance sur trois chacun) ; celui de la salle
// secrète est toujours un guetteur :
//  - guetteur : il regarde la porte par laquelle le joueur arrive, sa vue couvre toute la salle ;
//  - rôdeur : il tourne sur lui-même et ne voit qu'à ROAMING_TRAINER_SIGHT cases, on peut l'éviter ;
//  - facultatif : il ne voit personne, il faut lui parler et accepter le combat.
// Dans tous les cas, vaincu, il retourne au centre de la salle.
enum
{
    TRAINER_BEHAVIOR_WATCHER,
    TRAINER_BEHAVIOR_ROAMING,
    TRAINER_BEHAVIOR_OPTIONAL,
    TRAINER_BEHAVIOR_COUNT,
};
#define WATCHER_TRAINER_SIGHT  7
#define ROAMING_TRAINER_SIGHT  3

static u32 GetRoomTrainerBehavior(s32 x, s32 y)
{
    if (GetRoom(x, y) != ROOM_NORMAL)
        return TRAINER_BEHAVIOR_WATCHER;
    return RoomRandom(x, y, 0xBE4A) % TRAINER_BEHAVIOR_COUNT;
}

// Au chargement de la salle : place (ou cache) le dresseur, choisit son apparence et son comportement
// (GetRoomTrainerBehavior). Pas encore vaincu, il regarde la porte par laquelle le joueur arrive, ou tourne sur
// lui-même s'il est rôdeur.
static void SetUpRoomTrainer(s32 x, s32 y)
{
    struct ObjectEventTemplate *template = GetObjectTemplate(LOCALID_ISAAC_ROOM_TRAINER);
    u32 behavior;

    if (template == NULL)
        return;
    if (!RoomHasTrainer(x, y))
    {
        template->x = -1000;  // loin hors de la map : jamais affiché
        template->y = -1000;
        return;
    }
    VarSet(VAR_OBJ_GFX_ID_0, GetRoomTrainerType(x, y)->objEventGfx);
    template->x = ROOM_MID_X;
    template->y = ROOM_MID_Y;
    behavior = GetRoomTrainerBehavior(x, y);
    template->trainerType = (behavior == TRAINER_BEHAVIOR_OPTIONAL) ? TRAINER_TYPE_NONE : TRAINER_TYPE_NORMAL;
    template->script = (behavior == TRAINER_BEHAVIOR_OPTIONAL) ? IsaacRoom_EventScript_OptionalTrainer
                                                               : IsaacRoom_EventScript_Trainer;
    template->trainerRange_berryTreeId = (behavior == TRAINER_BEHAVIOR_ROAMING) ? ROAMING_TRAINER_SIGHT : WATCHER_TRAINER_SIGHT;
    if (IsCleared(x, y))
    {
        FlagSet(TRAINER_FLAGS_START + TRAINER_ISAAC_ROOM);
        template->movementType = MOVEMENT_TYPE_FACE_DOWN;
        return;
    }
    // Retour de combat ou partie chargée (pas d'arrivée par une porte) : l'objet et le drapeau du dresseur sont
    // déjà dans le bon état (après une victoire, le drapeau est mis avant que la salle soit marquée nettoyée)
    if (sArrivalFacing == DIR_NONE)
        return;
    FlagClear(TRAINER_FLAGS_START + TRAINER_ISAAC_ROOM);
    if (behavior == TRAINER_BEHAVIOR_ROAMING)
    {
        template->movementType = MOVEMENT_TYPE_ROTATE_CLOCKWISE;
        return;
    }
    switch (sArrivalFacing)  // le joueur marche vers le centre : le dresseur lui fait face
    {
    case DIR_NORTH:
        template->movementType = MOVEMENT_TYPE_FACE_DOWN;
        break;
    case DIR_SOUTH:
        template->movementType = MOVEMENT_TYPE_FACE_UP;
        break;
    case DIR_EAST:
        template->movementType = MOVEMENT_TYPE_FACE_LEFT;
        break;
    case DIR_WEST:
        template->movementType = MOVEMENT_TYPE_FACE_RIGHT;
        break;
    default:
        template->movementType = MOVEMENT_TYPE_FACE_DOWN;
        break;
    }
}

// ---------------------------------------------------------------------------
// Salles de capture et trésor
// ---------------------------------------------------------------------------

// Salle de capture : emplacement, objet et apparence de chaque Pokémon
static const u8 sCaptureLocalIds[CAPTURE_MAX_MONS] = { LOCALID_ISAAC_ROOM_WILD, LOCALID_ISAAC_ROOM_WILD_2, LOCALID_ISAAC_ROOM_WILD_3, LOCALID_ISAAC_ROOM_WILD_4, LOCALID_ISAAC_ROOM_WILD_5 };
static const u16 sCaptureGfxVars[CAPTURE_MAX_MONS] = { VAR_OBJ_GFX_ID_1, VAR_OBJ_GFX_ID_3, VAR_OBJ_GFX_ID_4, VAR_OBJ_GFX_ID_5, VAR_OBJ_GFX_ID_9 };
static const s8 sCaptureX[CAPTURE_MAX_MONS] = { ROOM_MID_X - 2, ROOM_MID_X + 2, ROOM_MID_X - 2, ROOM_MID_X + 2, ROOM_MID_X };
static const s8 sCaptureY[CAPTURE_MAX_MONS] = { ROOM_MID_Y - 1, ROOM_MID_Y - 1, ROOM_MID_Y + 1, ROOM_MID_Y + 1, ROOM_MID_Y };

static EWRAM_DATA u8 sCaptureSlot = 0;  // Pokémon combattu (IsaacPrepareWildBattle -> IsaacCaptureMonDone)

// Nombre de Pokémon de la salle de capture : 3 aux étages 1-2, 4 aux étages 3-4, 5 aux étages 5-6
static u32 GetCaptureMonCount(void)
{
    u32 floor = max(VarGet(VAR_ISAAC_FLOOR), 1u);

    return min(3 + (floor - 1) / 2, (u32)CAPTURE_MAX_MONS);
}

// Catégorie du Pokémon n° slot : étage 1 catégorie 2, 2 catégorie 2 ou 3, 3 catégorie 3, 4 catégorie 3 ou 4,
// 5 catégorie 4, 6 et plus catégorie 4 (75 %) ou légendaire (25 %)
static enum IsaacCategory RollCaptureCategory(s32 x, s32 y, u32 slot)
{
    u32 roll = RoomRandom(x, y, 0xCA70 + slot) % 100;

    switch (VarGet(VAR_ISAAC_FLOOR))
    {
    case 1:
        return ISAAC_CATEGORY_2;
    case 2:
        return roll < 50 ? ISAAC_CATEGORY_2 : ISAAC_CATEGORY_3;
    case 3:
        return ISAAC_CATEGORY_3;
    case 4:
        return roll < 50 ? ISAAC_CATEGORY_3 : ISAAC_CATEGORY_4;
    case 5:
        return ISAAC_CATEGORY_4;
    default:
        return roll < 75 ? ISAAC_CATEGORY_4 : ISAAC_CATEGORY_LEGENDARY_SMALL;  // légendaire : petit ou grand
    }
}

// Une espèce de la catégorie voulue : des lignées du biome, ou n'importe quel légendaire du roster
static u16 PickCaptureSpecies(enum IsaacCategory category, u32 roll)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];
    u16 legendaries[32];
    u32 count = 0, i;
    u16 species;

    if (category == ISAAC_CATEGORY_LEGENDARY_SMALL || category == ISAAC_CATEGORY_LEGENDARY_BIG)
    {
        for (i = 0; i < IsaacGetRosterCount() && count < ARRAY_COUNT(legendaries); i++)
        {
            species = IsaacGetRosterSpecies(i);
            category = IsaacGetSpeciesCategory(species);
            if (category == ISAAC_CATEGORY_LEGENDARY_SMALL || category == ISAAC_CATEGORY_LEGENDARY_BIG)
                legendaries[count++] = species;
        }
        return legendaries[roll % count];
    }
    species = IsaacPickLineSpecies(biome->lines, biome->lineCount, category, roll);
    return species != SPECIES_NONE ? species : biome->lines[0];
}

// Espèces des Pokémon de la salle de capture, toutes différentes (toujours les mêmes pour une salle)
static void GetCaptureSpecies(s32 x, s32 y, u16 *species)
{
    u32 slot, prev, try;
    bool8 taken;

    for (slot = 0; slot < GetCaptureMonCount(); slot++)
    {
        enum IsaacCategory category = RollCaptureCategory(x, y, slot);

        taken = TRUE;
        for (try = 0; try < 16 && taken; try++)
        {
            species[slot] = PickCaptureSpecies(category, RoomRandom(x, y, 0xC5EC + slot * 32 + try));
            taken = FALSE;
            for (prev = 0; prev < slot; prev++)
                if (species[prev] == species[slot])
                    taken = TRUE;
        }
    }
}

// Set des Pokémon de la salle de capture : set 2 jusqu'à l'étage 3, puis set 3
static u32 GetCaptureSetTier(void)
{
    return VarGet(VAR_ISAAC_FLOOR) <= 3 ? 1 : 2;
}

// Place un objet au centre de la salle s'il y est (le boss juste au-dessus du centre),
// sinon loin hors de la map (jamais affiché)
static void PlaceRoomObject(u8 localId, bool8 present)
{
    struct ObjectEventTemplate *template = GetObjectTemplate(localId);

    if (template == NULL)
        return;
    template->x = present ? ROOM_MID_X : -1000;
    template->y = present ? ROOM_MID_Y - (localId == LOCALID_ISAAC_ROOM_BOSS) : -1000;
}

// Boss : apparence du champion du biome ; on lui parle pour combattre. Vaincu, il reste là (et ne recombat pas).
static void SetUpBoss(s32 x, s32 y, bool8 cleared)
{
    VarSet(VAR_OBJ_GFX_ID_2, gIsaacBiomes[ISAAC_BIOME].boss->objEventGfx);
    if (cleared)
        FlagSet(TRAINER_FLAGS_START + TRAINER_ISAAC_BOSS);
    else if (sArrivalFacing != DIR_NONE)  // pas au retour du combat (le drapeau y est déjà le bon)
        FlagClear(TRAINER_FLAGS_START + TRAINER_ISAAC_BOSS);
}

// Avant le combat contre le boss : son nom dans gStringVar1 (texte d'introduction)
void IsaacBufferBossName(void)
{
    StringCopy(gStringVar1, gIsaacBiomes[ISAAC_BIOME].bossName);
}

// Voyant : le boss dans gStringVar1, toute son équipe dans gStringVar2 (dans l'ordre d'envoi, deux par ligne), et son
// as (le dernier) dans gStringVar3
void IsaacBufferBossTeam(void)
{
    const struct Trainer *boss = GetIsaacTrainer(TRAINER_ISAAC_BOSS);
    u8 *end = gStringVar2;
    u32 i;

    StringCopy(gStringVar1, gIsaacBiomes[ISAAC_BIOME].bossName);
    *end = EOS;
    for (i = 0; i < boss->partySize; i++)
    {
        if (i != 0)
            end = StringAppend(end, (i % 2 == 0) ? COMPOUND_STRING(",\l") : COMPOUND_STRING(", "));
        end = StringAppend(end, GetSpeciesName(boss->party[i].species));
    }
    StringCopy(gStringVar3, GetSpeciesName(boss->party[boss->partySize - 1].species));
}

// Boss vaincu : la salle est nettoyée et le passage vers l'étage suivant s'ouvre (la salle est redessinée)
void IsaacOpenBossExit(void)
{
    s32 x, y;

    GetCurrentCell(&x, &y);
    FlagSet(FLAG_ISAAC_CLEARED_FIRST + y * ISAAC_GRID_W + x);
    CarveRoom();
    PaintRoom();
    DrawWholeMapView();
}

// ---------------------------------------------------------------------------
// Infirmière de la salle du boss
// ---------------------------------------------------------------------------
// Le boss vaincu, elle arrive par le passage qui vient de s'ouvrir et se poste juste à côté (le passage reste
// libre). Elle propose un soin payant : PV et statut au complet, K.O. compris. Le prix dépend du nombre de Pokémon à soigner : ISAAC_NURSE_PRICE_PER_MON x étage chacun. Sans assez d'argent
// pour tous, elle soigne dans l'ordre de l'équipe autant de Pokémon que le joueur peut en payer.

#define ISAAC_NURSE_PRICE_PER_MON  600

EWRAM_DATA u8 gIsaacNurseArrivalMovement[ISAAC_ROOM_H] = {0};

static const u8 sDirToFaceMovementType[ISAAC_DIR_COUNT] =
{
    [ISAAC_DIR_NORTH] = MOVEMENT_TYPE_FACE_UP,
    [ISAAC_DIR_EAST] = MOVEMENT_TYPE_FACE_RIGHT,
    [ISAAC_DIR_SOUTH] = MOVEMENT_TYPE_FACE_DOWN,
    [ISAAC_DIR_WEST] = MOVEMENT_TYPE_FACE_LEFT,
};
static const u8 sDirToWalkMovement[ISAAC_DIR_COUNT] =
{
    [ISAAC_DIR_NORTH] = MOVEMENT_ACTION_WALK_NORMAL_UP,
    [ISAAC_DIR_EAST] = MOVEMENT_ACTION_WALK_NORMAL_RIGHT,
    [ISAAC_DIR_SOUTH] = MOVEMENT_ACTION_WALK_NORMAL_DOWN,
    [ISAAC_DIR_WEST] = MOVEMENT_ACTION_WALK_NORMAL_LEFT,
};
static const u8 sDirToFaceMovement[ISAAC_DIR_COUNT] =
{
    [ISAAC_DIR_NORTH] = MOVEMENT_ACTION_FACE_UP,
    [ISAAC_DIR_EAST] = MOVEMENT_ACTION_FACE_RIGHT,
    [ISAAC_DIR_SOUTH] = MOVEMENT_ACTION_FACE_DOWN,
    [ISAAC_DIR_WEST] = MOVEMENT_ACTION_FACE_LEFT,
};

// Chemin de l'infirmière : de la case du passage, elle traverse le mur (2 cases en haut et en bas, 1 sur les
// côtés), fait un pas dans la salle, puis un pas de côté (vers la droite ou le bas) et regarde la salle
static u32 GetNurseSteps(u32 exitDir, u32 *sideDir)
{
    *sideDir = (exitDir == ISAAC_DIR_NORTH || exitDir == ISAAC_DIR_SOUTH) ? ISAAC_DIR_EAST : ISAAC_DIR_SOUTH;
    return (exitDir == ISAAC_DIR_NORTH || exitDir == ISAAC_DIR_SOUTH) ? 3 : 2;
}

static void PlaceNurse(s32 x, s32 y, bool8 arriving)
{
    struct ObjectEventTemplate *template = GetObjectTemplate(LOCALID_ISAAC_ROOM_NURSE);
    u32 exitDir = GetBossExitDir(x, y), inward = (exitDir + 2) % ISAAC_DIR_COUNT, sideDir, steps;

    if (template == NULL)
        return;
    steps = GetNurseSteps(exitDir, &sideDir);
    template->x = sDoorX[exitDir];
    template->y = sDoorY[exitDir];
    if (!arriving)
    {
        template->x += steps * sDirDx[inward] + sDirDx[sideDir];
        template->y += steps * sDirDy[inward] + sDirDy[sideDir];
    }
    template->movementType = sDirToFaceMovementType[inward];
}

// Boss vaincu (après IsaacOpenBossExit) : l'infirmière est placée sur le passage, à faire apparaître avec
// `addobject` puis marcher avec gIsaacNurseArrivalMovement. Suivre de IsaacSettleNurse.
void IsaacPrepareNurseArrival(void)
{
    s32 x, y;
    u32 exitDir, inward, sideDir, steps, n = 0;

    GetCurrentCell(&x, &y);
    exitDir = GetBossExitDir(x, y);
    inward = (exitDir + 2) % ISAAC_DIR_COUNT;
    PlaceNurse(x, y, TRUE);
    for (steps = GetNurseSteps(exitDir, &sideDir); steps != 0; steps--)
        gIsaacNurseArrivalMovement[n++] = sDirToWalkMovement[inward];
    gIsaacNurseArrivalMovement[n++] = sDirToWalkMovement[sideDir];
    gIsaacNurseArrivalMovement[n++] = sDirToFaceMovement[inward];
    gIsaacNurseArrivalMovement[n] = MOVEMENT_ACTION_STEP_END;
}

// Une fois arrivée, elle reste à son poste (même si la caméra la fait disparaître puis réapparaître)
void IsaacSettleNurse(void)
{
    s32 x, y;

    GetCurrentCell(&x, &y);
    PlaceNurse(x, y, FALSE);
}

static u32 GetNursePricePerMon(void)
{
    return ISAAC_NURSE_PRICE_PER_MON * VarGet(VAR_ISAAC_FLOOR);
}

static bool8 MonNeedsHealing(struct Pokemon *mon)
{
    return !GetMonData(mon, MON_DATA_IS_EGG)
        && (GetMonData(mon, MON_DATA_HP) < GetMonData(mon, MON_DATA_MAX_HP) || GetMonData(mon, MON_DATA_STATUS) != 0);
}

static u32 CountMonsNeedingHealing(void)
{
    u32 count = 0, i;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
        if (MonNeedsHealing(&gParties[B_TRAINER_PLAYER][i]))
            count++;
    return count;
}

// Prix du soin de toute l'équipe dans gStringVar2 (gStringVar1 sert à la boîte d'argent), nombre de Pokémon à
// soigner dans gStringVar3 ; VAR_RESULT : ce nombre (0 : personne à soigner)
void IsaacNurseGetPrice(void)
{
    u32 count = CountMonsNeedingHealing();

    ConvertIntToDecimalStringN(gStringVar2, count * GetNursePricePerMon(), STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar3, count, STR_CONV_MODE_LEFT_ALIGN, 1);
    gSpecialVar_Result = count;
}

// Soin payant, Pokémon par Pokémon dans l'ordre de l'équipe tant que le joueur peut payer. VAR_RESULT = nombre de
// Pokémon soignés, VAR_0x8006 = nombre de Pokémon qui en avaient besoin ; gStringVar3 = nombre soignés.
void IsaacNurseTryHeal(void)
{
    u32 price = GetNursePricePerMon(), healed = 0, i, hp, status = 0;

    gSpecialVar_0x8006 = CountMonsNeedingHealing();
    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (!MonNeedsHealing(mon))
            continue;
        if (!IsEnoughMoney(&gSaveBlock1Ptr->money, price))
            break;
        RemoveMoney(&gSaveBlock1Ptr->money, price);
        hp = GetMonData(mon, MON_DATA_MAX_HP);
        SetMonData(mon, MON_DATA_HP, &hp);
        SetMonData(mon, MON_DATA_STATUS, &status);
        healed++;
    }
    ConvertIntToDecimalStringN(gStringVar3, healed, STR_CONV_MODE_LEFT_ALIGN, 1);
    gSpecialVar_Result = healed;
}

// Pokémon de la salle de capture pas encore combattus : à leur place, avec leur apparence ; les autres (et ceux
// des autres salles) loin hors de la map
static void SetUpCaptureMons(s32 x, s32 y)
{
    u16 species[CAPTURE_MAX_MONS];
    struct ObjectEventTemplate *template;
    bool8 capture = (GetRoom(x, y) == ROOM_CAPTURE);
    u32 slot;

    if (capture)
        GetCaptureSpecies(x, y, species);
    for (slot = 0; slot < CAPTURE_MAX_MONS; slot++)
    {
        template = GetObjectTemplate(sCaptureLocalIds[slot]);
        if (template == NULL)
            continue;
        if (capture && slot < GetCaptureMonCount() && !FlagGet(FLAG_ISAAC_CAPTURE_DONE_FIRST + slot))
        {
            template->x = sCaptureX[slot];
            template->y = sCaptureY[slot];
            VarSet(sCaptureGfxVars[slot], OBJ_EVENT_MON + species[slot]);
        }
        else
        {
            template->x = -1000;
            template->y = -1000;
        }
    }
}

// ---------------------------------------------------------------------------
// Boutique de Kecleon
// ---------------------------------------------------------------------------
// Kecleon tient la boutique de l'étage (menu Acheter / Quitter, CreateIsaacMartMenu). Ses articles possibles :
//  - une Master Ball, une Guérison, un Rappel, un Rappel Max, une Super Potion, une Hyper Potion, un Total Soin ;
//  - jusqu'à l'étage SHOP_DISCOUNT_CARD_MAX_FLOOR, la Carte de Réduction (si on ne l'a pas déjà) : -SHOP_DISCOUNT_PCT %
//    sur tous les autres articles jusqu'à la fin de la descente ;
//  - deux objets de combat de sShopBattleItems et deux CT de sShopTMs (tirés pour la salle), un objet spécial
//    (IsaacPickSpecialItem).
// Chacun n'est vraiment en vente qu'avec SHOP_SLOT_PRESENT_PCT % de chances (tiré pour l'étage,
// IsShopSlotPresent), en un seul exemplaire.
// Les prix se comptent en dixièmes de SHOP_PRICE_UNIT_PER_FLOOR x étage (voir sShopPrices) : un étage rapporte de
// l'ordre de 6 (étage 2) à 13 (étages profonds) de ces unités, on ne peut donc pas tout acheter et il faut choisir.
#define SHOP_PRICE_UNIT_PER_FLOOR  1000  // les gains d'un dresseur valaient 1000 x étage quand les prix ont été fixés

enum
{
    SHOP_SLOT_MASTER_BALL,
    SHOP_SLOT_FULL_RESTORE,
    SHOP_SLOT_REVIVE,
    SHOP_SLOT_MAX_REVIVE,
    SHOP_SLOT_SUPER_POTION,
    SHOP_SLOT_HYPER_POTION,
    SHOP_SLOT_FULL_HEAL,
    SHOP_SLOT_ITEM_1,
    SHOP_SLOT_ITEM_2,
    SHOP_SLOT_TM_1,
    SHOP_SLOT_TM_2,
    SHOP_SLOT_SPECIAL,
    SHOP_SLOT_DISCOUNT_CARD,
};
STATIC_ASSERT(SHOP_SLOT_DISCOUNT_CARD + 1 == SHOP_LIMITED_SLOTS, IsaacShopSlotCount);

// Très bons objets de combat
static const u16 sShopBattleItems[] =
{
    ITEM_CHOICE_BAND,
    ITEM_CHOICE_SPECS,
    ITEM_CHOICE_SCARF,
    ITEM_LIFE_ORB,
    ITEM_LEFTOVERS,
    ITEM_ASSAULT_VEST,
    ITEM_FOCUS_SASH,
    ITEM_WEAKNESS_POLICY,
    ITEM_ROCKY_HELMET,
    ITEM_EXPERT_BELT,
    ITEM_LUM_BERRY,
};

// Très bonnes CT (les CT51 et après viennent des générations suivantes, voir include/constants/tms_hms.h)
static const u16 sShopTMs[] =
{
    ITEM_TM_EARTHQUAKE,
    ITEM_TM_ICE_BEAM,
    ITEM_TM_THUNDERBOLT,
    ITEM_TM_FLAMETHROWER,
    ITEM_TM_PSYCHIC,
    ITEM_TM_SHADOW_BALL,
    ITEM_TM_SLUDGE_BOMB,
    ITEM_TM_CALM_MIND,
    ITEM_TM_BULK_UP,
    ITEM_TM_CLOSE_COMBAT,
    ITEM_TM_KNOCK_OFF,
    ITEM_TM_U_TURN,
    ITEM_TM_STONE_EDGE,
    ITEM_TM_SCALD,
    ITEM_TM_VOLT_SWITCH,
    ITEM_TM_DAZZLING_GLEAM,
    ITEM_TM_SWORDS_DANCE,
    ITEM_TM_NASTY_PLOT,
    ITEM_TM_DRAGON_DANCE,
    ITEM_TM_FLARE_BLITZ,
    ITEM_TM_PLAY_ROUGH,
    ITEM_TM_ENERGY_BALL,
    ITEM_TM_FOCUS_BLAST,
    ITEM_TM_FLASH_CANNON,
    ITEM_TM_EARTH_POWER,
    ITEM_TM_HURRICANE,
    ITEM_TM_DRAGON_PULSE,
    ITEM_TM_POISON_JAB,
    ITEM_TM_X_SCISSOR,
    ITEM_TM_IRON_HEAD,
    ITEM_TM_LIQUIDATION,
    ITEM_TM_BUG_BUZZ,
    ITEM_TM_DARK_PULSE,
    ITEM_TM_ROOST,
};

static u16 PickGoodTM(u32 roll)
{
    return sShopTMs[roll % ARRAY_COUNT(sShopTMs)];
}

// Butin rare d'un dresseur (sur 100) : objet spécial 40, très bonne CT 25, objet de combat de la boutique 25, Rappel
// Max 10
static u16 PickTrainerRareLoot(u32 roll)
{
    u32 pick = roll % 100;

    roll /= 100;
    if (pick < 40)
        return IsaacPickSpecialItem(roll);
    if (pick < 65)
        return PickGoodTM(roll);
    if (pick < 90)
        return sShopBattleItems[roll % ARRAY_COUNT(sShopBattleItems)];
    return ITEM_MAX_REVIVE;
}

// Prix en dixièmes des gains d'un dresseur de l'étage
struct IsaacShopPrice
{
    u16 item;
    u16 tenths;
};

static const struct IsaacShopPrice sShopPrices[] =
{
    { ITEM_SUPER_POTION,  1 },
    { ITEM_HYPER_POTION,  2 },
    { ITEM_FULL_HEAL,     1 },
    { ITEM_REVIVE,        4 },
    { ITEM_FULL_RESTORE,  6 },
    { ITEM_MAX_REVIVE,   10 },
    { ITEM_MASTER_BALL,  30 },
    { ITEM_DISCOUNT_CARD, 20 },
};
#define SHOP_DISCOUNT_CARD_MAX_FLOOR  4
#define SHOP_DISCOUNT_PCT            30
#define SHOP_TM_PRICE_TENTHS       12
#define SHOP_ITEM_PRICE_TENTHS     15
#define SHOP_SPECIAL_PRICE_TENTHS  50  // objet spécial : très cher

static EWRAM_DATA u16 sShopItems[SHOP_LIMITED_SLOTS + 1] = {0};

// Article n° slot (SHOP_SLOT_*) de la boutique de la salle courante
static u16 GetShopSlotItem(u32 slot)
{
    s32 x, y;
    u32 roll, first;

    GetCurrentCell(&x, &y);
    roll = RoomRandom(x, y, 0x5409);
    switch (slot)
    {
    case SHOP_SLOT_MASTER_BALL:
        return ITEM_MASTER_BALL;
    case SHOP_SLOT_FULL_RESTORE:
        return ITEM_FULL_RESTORE;
    case SHOP_SLOT_REVIVE:
        return ITEM_REVIVE;
    case SHOP_SLOT_MAX_REVIVE:
        return ITEM_MAX_REVIVE;
    case SHOP_SLOT_SUPER_POTION:
        return ITEM_SUPER_POTION;
    case SHOP_SLOT_HYPER_POTION:
        return ITEM_HYPER_POTION;
    case SHOP_SLOT_FULL_HEAL:
        return ITEM_FULL_HEAL;
    case SHOP_SLOT_ITEM_1:
    case SHOP_SLOT_ITEM_2:  // deux objets différents
        first = roll % ARRAY_COUNT(sShopBattleItems);
        if (slot == SHOP_SLOT_ITEM_1)
            return sShopBattleItems[first];
        return sShopBattleItems[(first + 1 + (roll >> 8) % (ARRAY_COUNT(sShopBattleItems) - 1)) % ARRAY_COUNT(sShopBattleItems)];
    case SHOP_SLOT_TM_1:
    case SHOP_SLOT_TM_2:  // deux CT différentes
        roll = RoomRandom(x, y, 0x5470);
        first = roll % ARRAY_COUNT(sShopTMs);
        if (slot == SHOP_SLOT_TM_1)
            return sShopTMs[first];
        return sShopTMs[(first + 1 + (roll >> 8) % (ARRAY_COUNT(sShopTMs) - 1)) % ARRAY_COUNT(sShopTMs)];
    case SHOP_SLOT_DISCOUNT_CARD:
        return ITEM_DISCOUNT_CARD;
    default:
        return IsaacPickSpecialItem(RoomRandom(x, y, 0x5495));
    }
}

// SHOP_LIMITED_SLOTS si l'objet n'est pas un article de la boutique
static u32 GetShopSlot(u16 item)
{
    u32 slot;

    for (slot = 0; slot < SHOP_LIMITED_SLOTS && GetShopSlotItem(slot) != item; slot++)
        ;
    return slot;
}

// Prix d'un article de la boutique (shop.c)
u32 IsaacGetShopPrice(u16 item)
{
    u32 unit = SHOP_PRICE_UNIT_PER_FLOOR * VarGet(VAR_ISAAC_FLOOR);
    u32 slot = GetShopSlot(item), i;

    if (item != ITEM_DISCOUNT_CARD && CheckBagHasItem(ITEM_DISCOUNT_CARD, 1))
        unit = unit * (100 - SHOP_DISCOUNT_PCT) / 100;

    for (i = 0; i < ARRAY_COUNT(sShopPrices); i++)
        if (sShopPrices[i].item == item)
            return unit * sShopPrices[i].tenths / 10;
    if (slot == SHOP_SLOT_TM_1 || slot == SHOP_SLOT_TM_2)
        return unit * SHOP_TM_PRICE_TENTHS / 10;
    if (slot == SHOP_SLOT_ITEM_1 || slot == SHOP_SLOT_ITEM_2)
        return unit * SHOP_ITEM_PRICE_TENTHS / 10;
    if (slot == SHOP_SLOT_SPECIAL)
        return unit * SHOP_SPECIAL_PRICE_TENTHS / 10;
    return GetItemPrice(item);
}

// Exemplaires encore en vente (0 ou 1 ; 0xFFFF pour un objet qui n'est pas de la boutique)
u16 IsaacGetShopStock(u16 item)
{
    u32 slot = GetShopSlot(item);

    if (slot == SHOP_LIMITED_SLOTS)
        return 0xFFFF;
    return FlagGet(FLAG_ISAAC_SHOP_SOLD_FIRST + slot) ? 0 : 1;
}

void IsaacOnShopPurchase(u16 item, u16 count)
{
    u32 slot = GetShopSlot(item);

    if (slot != SHOP_LIMITED_SLOTS && count != 0)
        FlagSet(FLAG_ISAAC_SHOP_SOLD_FIRST + slot);
}

#define SHOP_SLOT_PRESENT_PCT  60

// L'article n° slot est-il en vente à cet étage ? (toujours la même réponse pour la salle)
static bool8 IsShopSlotPresent(u32 slot)
{
    s32 x, y;

    GetCurrentCell(&x, &y);
    return RoomRandom(x, y, 0x5A70 + slot) % 100 < SHOP_SLOT_PRESENT_PCT;
}

// Ouvre la boutique (suivre de `waitstate`) : seulement les articles en vente à cet étage et pas encore vendus
void IsaacOpenShop(void)
{
    u32 n = 0, i;

    for (i = 0; i < SHOP_LIMITED_SLOTS; i++)
    {
        if (i == SHOP_SLOT_DISCOUNT_CARD
         && (VarGet(VAR_ISAAC_FLOOR) > SHOP_DISCOUNT_CARD_MAX_FLOOR || CheckBagHasItem(ITEM_DISCOUNT_CARD, 1)))
            continue;
        if (IsShopSlotPresent(i) && !FlagGet(FLAG_ISAAC_SHOP_SOLD_FIRST + i))
            sShopItems[n++] = GetShopSlotItem(i);
    }
    sShopItems[n] = ITEM_NONE;  // fin de la liste pour shop.c
    CreateIsaacMartMenu(sShopItems);
}

// VAR_RESULT : TRUE la toute première fois qu'on parle à Kecleon (dans la partie)
void IsaacKecleonGreeting(void)
{
    gSpecialVar_Result = !FlagGet(FLAG_ISAAC_KECLEON_MET);
    FlagSet(FLAG_ISAAC_KECLEON_MET);
}

// ---------------------------------------------------------------------------
// Contenus des salles vides
// ---------------------------------------------------------------------------
// Les salles normales sans dresseur peuvent accueillir un contenu (un personnage au centre), chacun au plus une
// fois par étage, dans des salles différentes tirées pour l'étage (GetRoomContent) :
//  - ROOM_CONTENT_EGG : à l'étage 1 ou 2 (un seul des deux par descente, GetEggFloor), quelqu'un confie un Œuf
//    d'un Pokémon de n'importe quel biome (set 2), de catégorie 3 à l'étage 1 et 4 à l'étage 2 (le plus fort de plusieurs tirages), qui éclot après ISAAC_EGG_HATCH_ROOMS nouvelles
//    salles visitées (environ deux étages, IsaacTickEggs) ;
//  - ROOM_CONTENT_TRADER : un homme échange un Pokémon de l'équipe contre un autre de n'importe quel biome, de la
//    force de ceux de la salle de capture de l'étage (IsaacPrepareTrade) ;
//  - ROOM_CONTENT_DEVIL : à partir de DEVIL_PACT_MIN_FLOOR, pacte du diable, un objet rare (Master Ball, Dé
//    Maudit, Pacte de Sang ou objet spécial, IsaacPrepareDevilPact) contre le sacrifice définitif d'un Pokémon de
//    l'équipe (pas le dernier en état de se battre, pas un Œuf) ;
//  - ROOM_CONTENT_TUTOR : maître des capacités, propose TUTOR_CHOICES attaques fortes (sTutorMoves) que le Pokémon choisi peut
//    apprendre et ne connaît pas ; il en apprend une (en oubliant une autre s'il en connaît déjà 4) ;
//  - ROOM_CONTENT_DUELIST : le duelliste annonce son Pokémon (plus fort que ceux de l'étage, voir GetIsaacTrainer),
//    puis combat en 1 contre 1 contre le Pokémon choisi par le joueur. Perdre ne renvoie pas au hub. Un seul duel :
//    gagné, il donne une Guérison et une Super CT ; perdu, un Rappel pour relever le Pokémon K.O. ;
//  - ROOM_CONTENT_SEER : le voyant annonce l'équipe du boss de l'étage (IsaacBufferBossTeam) ;
//  - ROOM_CONTENT_ROCK_SMASH : un entraîneur apprend Éclate-Roc au Pokémon choisi, qui pourra briser les roches qui
//    ferment la salle secrète (seulement s'il y en a une, et si personne ne connaissait Éclate-Roc en début d'étage) ;
//  - ROOM_CONTENT_HEALER : une vieille dame et son Leveinard soignent complètement un Pokémon (même K.O.), une fois ;
//  - ROOM_CONTENT_EV_TRAINER : un entraîneur met au maximum les IVs (31) et EVs (répartition du set 3 de l'espèce)
//    d'un Pokémon, une fois.
// Une fois le contenu utilisé, la salle est « nettoyée » et le personnage reste là.

#define ISAAC_EGG_HATCH_ROOMS  20

// Étage de l'Œuf de la descente : 1 ou 2, tiré avec l'ordre des biomes
static u32 GetEggFloor(void)
{
    return 1 + ((VarGet(VAR_ISAAC_BIOME_ORDER) * 2654435761u) >> 31);
}

static bool8 FloorHasSecretRoom(void)
{
    s32 x, y;

    for (y = 0; y < ISAAC_GRID_H; y++)
        for (x = 0; x < ISAAC_GRID_W; x++)
            if (GetRoom(x, y) == ROOM_SECRET)
                return TRUE;
    return FALSE;
}

#define DEVIL_PACT_MIN_FLOOR  3  // le pacte du diable n'apparaît qu'à partir de cet étage

static bool8 IsRoomContentAvailable(u32 content)
{
    switch (content)
    {
    case ROOM_CONTENT_EGG:
        return VarGet(VAR_ISAAC_FLOOR) == GetEggFloor();
    case ROOM_CONTENT_DEVIL:
        return VarGet(VAR_ISAAC_FLOOR) >= DEVIL_PACT_MIN_FLOOR;
    case ROOM_CONTENT_ROCK_SMASH:
        return FloorHasSecretRoom() && FlagGet(FLAG_ISAAC_ROCK_SMASH_TEACHER);
    default:
        return TRUE;
    }
}

static bool8 PartyKnowsMove(u16 move)
{
    u32 i;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
        if (!GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG) && MonKnowsMove(&gParties[B_TRAINER_PLAYER][i], move))
            return TRUE;
    return FALSE;
}

// L'entraîneur d'Éclate-Roc est assuré à l'étage ROCK_SMASH_SURE_FLOOR (premier étage avec une salle secrète) quand
// personne ne connaît Éclate-Roc : il passe avant les autres contenus (GetRoomContent), et s'il n'y a aucune salle
// vide, une salle normale tirée pour l'étage perd son dresseur pour l'accueillir (cellule renvoyée ici, -1 sinon)
#define ROCK_SMASH_SURE_FLOOR  ISAAC_SECRET_MIN_FLOOR

static bool8 IsRockSmashTeacherSure(void)
{
    return VarGet(VAR_ISAAC_FLOOR) == ROCK_SMASH_SURE_FLOOR && IsRoomContentAvailable(ROOM_CONTENT_ROCK_SMASH);
}

static s32 GetForcedRockSmashRoom(void)
{
    u8 rooms[ISAAC_GRID_W * ISAAC_GRID_H];
    u32 count = 0;
    s32 x, y;

    if (!IsRockSmashTeacherSure())
        return -1;
    for (y = 0; y < ISAAC_GRID_H; y++)
    {
        for (x = 0; x < ISAAC_GRID_W; x++)
        {
            if (GetRoom(x, y) != ROOM_NORMAL)
                continue;
            if (!NormalRoomRollsTrainer(x, y))
                return -1;  // il y a déjà une salle vide
            rooms[count++] = y * ISAAC_GRID_W + x;
        }
    }
    return count ? rooms[RoomRandom(0, 0, 0x5A5A) % count] : -1;
}

static bool8 IsEmptyRoom(s32 x, s32 y)
{
    return GetRoom(x, y) == ROOM_NORMAL && !RoomHasTrainer(x, y);
}

// Contenu de la salle (ROOM_CONTENT_*) : les contenus disponibles à l'étage sont pris dans un ordre tiré au hasard
// (s'il y a moins de salles vides que de contenus, ce ne sont pas toujours les mêmes qui manquent), chacun dans une
// salle vide différente
static u32 GetRoomContent(s32 x, s32 y)
{
    u8 rooms[ISAAC_GRID_W * ISAAC_GRID_H], contents[ROOM_CONTENT_COUNT];
    u32 roomCount = 0, contentCount = 0, i, pick, tmp;
    s32 cx, cy;

    if (!IsEmptyRoom(x, y))
        return ROOM_CONTENT_NONE;
    for (cy = 0; cy < ISAAC_GRID_H; cy++)
        for (cx = 0; cx < ISAAC_GRID_W; cx++)
            if (IsEmptyRoom(cx, cy))
                rooms[roomCount++] = cy * ISAAC_GRID_W + cx;
    for (i = ROOM_CONTENT_NONE + 1; i < ROOM_CONTENT_COUNT; i++)
        if (IsRoomContentAvailable(i))
            contents[contentCount++] = i;
    for (i = 0; i < contentCount; i++)  // ordre des contenus tiré pour l'étage
    {
        pick = i + RoomRandom(i, 1, 0xC0DE) % (contentCount - i);
        SWAP(contents[i], contents[pick], tmp);
    }
    if (IsRockSmashTeacherSure())  // l'entraîneur d'Éclate-Roc passe en premier
    {
        for (i = 0; contents[i] != ROOM_CONTENT_ROCK_SMASH; i++)
            ;
        SWAP(contents[0], contents[i], tmp);
    }
    for (i = 0; i < contentCount && roomCount != 0; i++)
    {
        pick = RoomRandom(contents[i], 0, 0xC047) % roomCount;
        if (rooms[pick] == y * ISAAC_GRID_W + x)
            return contents[i];
        rooms[pick] = rooms[--roomCount];  // cette salle est prise
    }
    return ROOM_CONTENT_NONE;
}

// Personnage du contenu, au centre de la salle (dans l'océan : un nageur)
static void SetUpRoomContent(s32 x, s32 y)
{
    static const u16 sContentGfx[ROOM_CONTENT_COUNT][2] =
    {
        [ROOM_CONTENT_EGG]    = { OBJ_EVENT_GFX_OLD_WOMAN, OBJ_EVENT_GFX_SWIMMER_F },
        [ROOM_CONTENT_TRADER] = { OBJ_EVENT_GFX_GENTLEMAN, OBJ_EVENT_GFX_SWIMMER_M },
        [ROOM_CONTENT_DEVIL]  = { OBJ_EVENT_GFX_HEX_MANIAC, OBJ_EVENT_GFX_SWIMMER_F },
        [ROOM_CONTENT_TUTOR]  = { OBJ_EVENT_GFX_EXPERT_M,   OBJ_EVENT_GFX_SWIMMER_M },
        [ROOM_CONTENT_DUELIST] = { OBJ_EVENT_GFX_BLACK_BELT, OBJ_EVENT_GFX_SWIMMER_M },
        [ROOM_CONTENT_SEER]   = { OBJ_EVENT_GFX_PSYCHIC_M,  OBJ_EVENT_GFX_SWIMMER_M },
        [ROOM_CONTENT_ROCK_SMASH] = { OBJ_EVENT_GFX_HIKER, OBJ_EVENT_GFX_SWIMMER_M },
        [ROOM_CONTENT_HEALER] = { OBJ_EVENT_GFX_OLD_WOMAN, OBJ_EVENT_GFX_SWIMMER_F },
        [ROOM_CONTENT_EV_TRAINER] = { OBJ_EVENT_GFX_MAN_3, OBJ_EVENT_GFX_SWIMMER_M },
    };
    u32 content = GetRoomContent(x, y);
    struct ObjectEventTemplate *chansey;

    PlaceRoomObject(LOCALID_ISAAC_ROOM_CONTENT, content != ROOM_CONTENT_NONE);
    if (content != ROOM_CONTENT_NONE)
        VarSet(VAR_OBJ_GFX_ID_8, sContentGfx[content][gIsaacBiomes[ISAAC_BIOME].ocean]);
    // le Pokémon de la vieille dame, à côté d'elle : un Leveinard (un Lokhlass dans l'océan)
    PlaceRoomObject(LOCALID_ISAAC_ROOM_CONTENT_2, content == ROOM_CONTENT_HEALER);
    chansey = GetObjectTemplate(LOCALID_ISAAC_ROOM_CONTENT_2);
    if (content == ROOM_CONTENT_HEALER && chansey != NULL)
    {
        chansey->x++;
        VarSet(VAR_OBJ_GFX_ID_A, OBJ_EVENT_MON + (gIsaacBiomes[ISAAC_BIOME].ocean ? SPECIES_LAPRAS : SPECIES_CHANSEY));
    }
}

// Avant de parler au personnage : VAR_RESULT = contenu de la salle, VAR_0x8005 = TRUE s'il a déjà servi
void IsaacGetRoomContent(void)
{
    s32 x, y;

    GetCurrentCell(&x, &y);
    gSpecialVar_Result = GetRoomContent(x, y);
    gSpecialVar_0x8005 = IsCleared(x, y);
}

// Une espèce de la catégorie voulue (1 à 4, au sens de son biome) dans un biome tiré au hasard ; autre que exclude
static u16 PickSpeciesOfAnyBiome(enum IsaacCategory category, u16 exclude, u32 seed)
{
    const struct IsaacBiome *biome;
    u32 try;
    u16 species = SPECIES_NONE;

    for (try = 0; try < 16; try++)
    {
        biome = &gIsaacBiomes[(seed >> 8) % ISAAC_BIOME_COUNT];
        species = IsaacPickLineSpecies(biome->lines, biome->lineCount, category, seed);
        if (species != SPECIES_NONE && species != exclude)
            return species;
        seed = seed * 1103515245 + 12345;
    }
    return species;
}

// Œuf confié : VAR_RESULT = FALSE si l'équipe est pleine. L'espèce est la plus forte (BST) de EGG_SPECIES_DRAWS tirages.
#define EGG_SPECIES_DRAWS  3

void IsaacGiveEgg(void)
{
    struct Pokemon *mon;
    s32 x, y;
    u32 seed, value, i;
    u16 species, candidate;

    gSpecialVar_Result = FALSE;
    if (gPartiesCount[B_TRAINER_PLAYER] >= PARTY_SIZE)
        return;
    GetCurrentCell(&x, &y);
    seed = RoomRandom(x, y, 0xE66);
    for (i = 0, species = SPECIES_NONE; i < EGG_SPECIES_DRAWS; i++)
    {
        candidate = PickSpeciesOfAnyBiome(VarGet(VAR_ISAAC_FLOOR) == 1 ? ISAAC_CATEGORY_3 : ISAAC_CATEGORY_4,
                                          SPECIES_NONE, seed ^ (i * 0x9E3779B9));
        if (species == SPECIES_NONE || IsaacGetSpeciesBST(candidate) > IsaacGetSpeciesBST(species))
            species = candidate;
    }
    mon = &gParties[B_TRAINER_PLAYER][gPartiesCount[B_TRAINER_PLAYER]];
    IsaacCreateMonFromSet(mon, species, 1, (seed >> 16) % ISAAC_SET_VARIANTS, seed);
    value = TRUE;
    SetMonData(mon, MON_DATA_IS_EGG, &value);
    value = ISAAC_EGG_HATCH_ROOMS;  // compteur de salles avant l'éclosion (voir TryToHatchEgg dans daycare.c)
    SetMonData(mon, MON_DATA_FRIENDSHIP, &value);
    SetMonData(mon, MON_DATA_NICKNAME, gText_EggNickname);
    CalculatePlayerPartyCount();
    gSpecialVar_Result = TRUE;
}

// Pokémon de l'équipe en état de se battre un jour (hors Œufs)
static u32 CountPartyFighters(void)
{
    u32 count = 0, i;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
        if (!GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            count++;
    return count;
}

// Équipe pleine quand on obtient un Pokémon (pas de PC) : le Pokémon n° VAR_0x8004 est relâché pour faire de la
// place (son objet revient dans le Sac). VAR_RESULT = FALSE si c'est le dernier en état de se battre ; gStringVar1 =
// son nom.
void IsaacReleasePartyMon(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);

    GetMonNickname(mon, gStringVar1);
    gSpecialVar_Result = GetMonData(mon, MON_DATA_IS_EGG) || CountPartyFighters() >= 2;
    if (!gSpecialVar_Result)
        return;
    if (heldItem != ITEM_NONE)
        AddBagItem(heldItem, 1);
    ZeroMonData(mon);
    CompactPartySlots();
    CalculatePlayerPartyCount();
}

// Nouvelle salle visitée : les Œufs de l'équipe se rapprochent de l'éclosion
static void IsaacTickEggs(void)
{
    u32 i, rooms;

    for (i = 0; i < gPartiesCount[B_TRAINER_PLAYER]; i++)
    {
        if (!GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            continue;
        rooms = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_FRIENDSHIP);
        if (rooms != 0)
        {
            rooms--;
            SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_FRIENDSHIP, &rooms);
        }
    }
}

static EWRAM_DATA u16 sTradeSpecies = SPECIES_NONE;

// Échange avec le Pokémon n° VAR_0x8004 de l'équipe : VAR_RESULT = FALSE pour un Œuf. Sinon, l'offre est celle de la
// salle (toujours la même) : un Pokémon de n'importe quel biome, de la force de ceux de la salle de capture de l'étage
// (RollCaptureCategory, GetCaptureSetTier), d'une autre espèce que celui du joueur. gStringVar1 = le Pokémon du joueur,
// gStringVar2 = l'offre, révélée seulement une fois l'échange conclu.
void IsaacPrepareTrade(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    u16 species = GetMonData(mon, MON_DATA_SPECIES);
    enum IsaacCategory category;
    u32 try, seed;
    s32 x, y;

    gSpecialVar_Result = FALSE;
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return;
    GetCurrentCell(&x, &y);
    category = RollCaptureCategory(x, y, 0);
    for (try = 0; try < 16; try++)
    {
        seed = RoomRandom(x, y, 0x7AD0 + try);
        if (category == ISAAC_CATEGORY_LEGENDARY_SMALL || category == ISAAC_CATEGORY_LEGENDARY_BIG)
            sTradeSpecies = PickCaptureSpecies(category, seed);
        else
            sTradeSpecies = PickSpeciesOfAnyBiome(category, SPECIES_NONE, seed);
        if (sTradeSpecies != species)
            break;
    }
    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, GetSpeciesName(sTradeSpecies));
    gSpecialVar_Result = TRUE;
}

// Échange conclu : le Pokémon n° VAR_0x8004 devient l'offre (set de la salle de capture, variante au hasard) ;
// l'objet qu'il tenait revient dans le Sac
void IsaacDoTrade(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    u32 tier = GetCaptureSetTier();
    u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);
    s32 x, y;
    u32 seed;

    if (heldItem != ITEM_NONE)
        AddBagItem(heldItem, 1);
    GetCurrentCell(&x, &y);
    seed = RoomRandom(x, y, 0x7AD5 + sTradeSpecies);
    IsaacCreateMonFromSet(mon, sTradeSpecies, tier, seed % ISAAC_SET_VARIANTS, seed);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(sTradeSpecies), FLAG_SET_SEEN);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(sTradeSpecies), FLAG_SET_CAUGHT);
}

// Pacte du diable : objet proposé (toujours le même pour la salle) dans VAR_0x8006 et gStringVar2. VAR_RESULT = FALSE
// si l'équipe n'a pas au moins deux Pokémon (hors Œufs) : on ne peut pas sacrifier le dernier.

#define DEVIL_PACT_MASTER_BALL_ODDS  5
// Sinon, sur DEVIL_PACT_ITEM_ODDS : un Dé Maudit, un Pacte de Sang, ou (le reste) un objet spécial
#define DEVIL_PACT_ITEM_ODDS  4

void IsaacPrepareDevilPact(void)
{
    s32 x, y;
    u32 roll;

    GetCurrentCell(&x, &y);
    roll = RoomRandom(x, y, 0xD3E1);
    if (roll % DEVIL_PACT_MASTER_BALL_ODDS == 0)
        gSpecialVar_0x8006 = ITEM_MASTER_BALL;
    else if ((roll >> 4) % DEVIL_PACT_ITEM_ODDS == 0)
        gSpecialVar_0x8006 = ITEM_CURSED_DIE;
    else if ((roll >> 4) % DEVIL_PACT_ITEM_ODDS == 1)
        gSpecialVar_0x8006 = ITEM_BLOOD_PACT;
    else
        gSpecialVar_0x8006 = IsaacPickSpecialItem(roll >> 8);
    CopyItemName(gSpecialVar_0x8006, gStringVar2);
    gSpecialVar_Result = CountPartyFighters() >= 2;
}

// Pokémon n° VAR_0x8004 proposé en sacrifice : VAR_RESULT = FALSE pour un Œuf ; gStringVar1 = son nom
void IsaacCheckDevilSacrifice(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];

    gSpecialVar_Result = !GetMonData(mon, MON_DATA_IS_EGG);
    GetMonNickname(mon, gStringVar1);
}

// Sacrifice : le Pokémon n° VAR_0x8004 quitte l'équipe pour de bon (l'objet qu'il tenait revient dans le Sac)
void IsaacDoDevilPact(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM);

    if (heldItem != ITEM_NONE)
        AddBagItem(heldItem, 1);
    ZeroMonData(mon);
    CompactPartySlots();
    CalculatePlayerPartyCount();
}

// Maître des capacités : attaques fortes qu'il peut enseigner (si le Pokémon peut les apprendre, par CT, tuteur ou
// niveau)
static const u16 sTutorMoves[] =
{
    MOVE_EARTHQUAKE, MOVE_ICE_BEAM, MOVE_THUNDERBOLT, MOVE_FLAMETHROWER, MOVE_PSYCHIC, MOVE_SHADOW_BALL,
    MOVE_SLUDGE_BOMB, MOVE_ENERGY_BALL, MOVE_FOCUS_BLAST, MOVE_FLASH_CANNON, MOVE_EARTH_POWER, MOVE_DRAGON_PULSE,
    MOVE_DARK_PULSE, MOVE_DAZZLING_GLEAM, MOVE_MOONBLAST, MOVE_HURRICANE, MOVE_BUG_BUZZ, MOVE_SCALD, MOVE_SURF,
    MOVE_HYDRO_PUMP, MOVE_FIRE_BLAST, MOVE_THUNDER, MOVE_BLIZZARD, MOVE_GIGA_DRAIN, MOVE_HYPER_VOICE, MOVE_AURA_SPHERE,
    MOVE_CLOSE_COMBAT, MOVE_KNOCK_OFF, MOVE_U_TURN, MOVE_STONE_EDGE, MOVE_FLARE_BLITZ, MOVE_PLAY_ROUGH,
    MOVE_POISON_JAB, MOVE_X_SCISSOR, MOVE_IRON_HEAD, MOVE_LIQUIDATION, MOVE_WATERFALL, MOVE_DRAIN_PUNCH,
    MOVE_CRUNCH, MOVE_BRAVE_BIRD, MOVE_OUTRAGE, MOVE_SEED_BOMB, MOVE_SHADOW_CLAW, MOVE_GUNK_SHOT, MOVE_SUPERPOWER,
    MOVE_DOUBLE_EDGE, MOVE_BODY_SLAM, MOVE_VOLT_SWITCH, MOVE_SWORDS_DANCE, MOVE_NASTY_PLOT, MOVE_DRAGON_DANCE,
    MOVE_CALM_MIND, MOVE_BULK_UP, MOVE_ROOST, MOVE_RECOVER, MOVE_WILL_O_WISP, MOVE_THUNDER_WAVE,
};
#define TUTOR_CHOICES  5

static EWRAM_DATA u16 sTutorChoices[TUTOR_CHOICES] = {0};
static EWRAM_DATA u16 sTutorMove = MOVE_NONE;

static bool8 CanLearnTutorMove(enum Species species, u16 move)
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

// Pokémon n° VAR_0x8004 : VAR_RESULT = nombre d'attaques proposées (0 pour un Œuf ou s'il n'y a rien à lui
// apprendre ; leurs noms avec IsaacBufferTutorChoice). Toujours les mêmes pour ce Pokémon dans cette salle.
void IsaacPrepareTutor(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    u16 species = GetMonData(mon, MON_DATA_SPECIES), candidates[ARRAY_COUNT(sTutorMoves)], tmp;
    u32 count = 0, i, pick;
    s32 x, y;

    gSpecialVar_Result = 0;
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return;
    for (i = 0; i < ARRAY_COUNT(sTutorMoves); i++)
        if (CanLearnTutorMove(species, sTutorMoves[i]) && !MonKnowsMove(mon, sTutorMoves[i]))
            candidates[count++] = sTutorMoves[i];
    GetCurrentCell(&x, &y);
    for (i = 0; i < TUTOR_CHOICES && i < count; i++)
    {
        pick = i + RoomRandom(x, y, 0x7070 + species * TUTOR_CHOICES + i) % (count - i);
        SWAP(candidates[i], candidates[pick], tmp);
        sTutorChoices[i] = candidates[i];
    }
    gSpecialVar_Result = i;
}

// Nom de l'attaque proposée n° VAR_0x8006 dans gStringVar1 (pour le menu, voir IsaacPrepareTutor)
void IsaacBufferTutorChoice(void)
{
    StringCopy(gStringVar1, GetMoveName(sTutorChoices[min(gSpecialVar_0x8006, TUTOR_CHOICES - 1)]));
}

// Attaque n° VAR_0x8006 choisie : gStringVar1 = le Pokémon, gStringVar2 = l'attaque. Apprise tout de suite s'il a une
// place libre (VAR_RESULT = TRUE), sinon il faudra en oublier une (IsaacTutorChooseMoveToForget).
void IsaacTutorChooseMove(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];

    sTutorMove = sTutorChoices[min(gSpecialVar_0x8006, TUTOR_CHOICES - 1)];
    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, GetMoveName(sTutorMove));
    gSpecialVar_Result = (GiveMoveToMon(mon, sTutorMove) == sTutorMove);
}

// Écran du Pokémon pour choisir l'attaque à oublier (VAR_0x8005 : sa place, MAX_MON_MOVES si on renonce). Entre deux
// `fadescreen`, comme chez l'Effaceur de capacités.
void IsaacTutorChooseMoveToForget(void)
{
    ShowSelectMovePokemonSummaryScreen(gParties[B_TRAINER_PLAYER], gSpecialVar_0x8004, CB2_ReturnToField, sTutorMove);
    gFieldCallback = FieldCB_ContinueScriptHandleMusic;
}

// Remplace l'attaque choisie à l'écran du Pokémon ; gStringVar1 = le Pokémon, gStringVar2 = l'attaque (à réécrire :
// l'écran du Pokémon s'en est servi pour afficher ses stats)
void IsaacTutorReplaceMove(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];

    RemoveMonPPBonus(mon, gSpecialVar_0x8005);
    SetMonMoveSlot(mon, sTutorMove, gSpecialVar_0x8005);
    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, GetMoveName(sTutorMove));
}

// Duel : le Pokémon du duelliste dans gStringVar1
void IsaacPrepareDuel(void)
{
    const struct Trainer *duelist = GetIsaacTrainer(TRAINER_ISAAC_DUELIST);

    StringCopy(gStringVar1, GetSpeciesName(duelist->party[0].species));
}

// Pokémon n° VAR_0x8004 choisi pour le duel : VAR_RESULT = FALSE pour un Œuf ou un Pokémon K.O.
void IsaacDuelCheckMon(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];

    gSpecialVar_Result = !GetMonData(mon, MON_DATA_IS_EGG) && GetMonData(mon, MON_DATA_HP) != 0;
}

// Le duel se joue en 1 contre 1 : l'équipe est mise de côté, il ne reste que le Pokémon choisi ; perdre ne renvoie
// pas au hub (B_FLAG_NO_WHITEOUT). IsaacDuelRestoreParty remet tout en place après le combat.
static EWRAM_DATA struct Pokemon sDuelSavedParty[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sDuelSavedCount = 0;
static EWRAM_DATA u8 sDuelSlot = 0;

void IsaacDuelSetUpParty(void)
{
    u32 i;

    sDuelSlot = gSpecialVar_0x8004;
    sDuelSavedCount = gPartiesCount[B_TRAINER_PLAYER];
    memcpy(sDuelSavedParty, gParties[B_TRAINER_PLAYER], sizeof(sDuelSavedParty));
    gParties[B_TRAINER_PLAYER][0] = sDuelSavedParty[sDuelSlot];
    for (i = 1; i < PARTY_SIZE; i++)
        ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
    CalculatePlayerPartyCount();
    FlagSet(B_FLAG_NO_WHITEOUT);
    FlagClear(TRAINER_FLAGS_START + TRAINER_ISAAC_DUELIST);  // le combat a lieu même si un duelliste a déjà été battu
}

void IsaacDuelRestoreParty(void)
{
    sDuelSavedParty[sDuelSlot] = gParties[B_TRAINER_PLAYER][0];
    memcpy(gParties[B_TRAINER_PLAYER], sDuelSavedParty, sizeof(sDuelSavedParty));
    gPartiesCount[B_TRAINER_PLAYER] = sDuelSavedCount;
    CalculatePlayerPartyCount();
    FlagClear(B_FLAG_NO_WHITEOUT);
}

// Ouvre le passage vers la salle secrète de l'étage (roche brisée par Éclate-Roc) : VAR_RESULT = FALSE s'il l'était
// déjà. Si la salle courante y mène, elle est redessinée tout de suite.
void IsaacOpenSecretRoom(void)
{
    gSpecialVar_Result = !FlagGet(FLAG_ISAAC_SECRET_OPEN);
    FlagSet(FLAG_ISAAC_SECRET_OPEN);
    CarveRoom();
    PaintRoom();
    DrawWholeMapView();
}

// Entraîneur d'Éclate-Roc, Pokémon n° VAR_0x8004 (n'importe quelle espèce, comme les CT) : VAR_RESULT = 0 pour un Œuf
// ou s'il la connaît déjà, 1 s'il l'apprend tout de suite (place libre), 2 s'il doit oublier une attaque
// (IsaacTutorChooseMoveToForget puis IsaacTutorReplaceMove). gStringVar1 = le Pokémon, gStringVar2 = l'attaque.
void IsaacPrepareRockSmash(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];

    sTutorMove = MOVE_ROCK_SMASH;
    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, GetMoveName(MOVE_ROCK_SMASH));
    if (GetMonData(mon, MON_DATA_IS_EGG) || MonKnowsMove(mon, MOVE_ROCK_SMASH))
        gSpecialVar_Result = 0;
    else if (GiveMoveToMon(mon, MOVE_ROCK_SMASH) == MOVE_ROCK_SMASH)
        gSpecialVar_Result = 1;
    else
        gSpecialVar_Result = 2;
}

// Bouton A face à une case : si c'est une roche qui ferme la salle secrète et qu'un Pokémon de l'équipe connaît
// Éclate-Roc, script pour la briser (ailleurs, rien : les autres murs ne trahissent pas la salle secrète)
const u8 *IsaacGetWallInteractScript(s16 x, s16 y)
{
    x -= MAP_OFFSET;
    y -= MAP_OFFSET;
    if (!IsInRoomMap() || !InPaintArea(x, y) || sCell[y - PAINT_MIN][x - PAINT_MIN] != CELL_HIDDEN
     || FlagGet(FLAG_ISAAC_SECRET_OPEN) || !PartyKnowsMove(MOVE_ROCK_SMASH))
        return NULL;
    return IsaacRoom_EventScript_SecretWall;
}

// Récompense du boss vaincu (dans VAR_0x8004, toujours la même pour l'étage) : un objet spécial ou une Master Ball,
// une fois sur deux chacun
void IsaacPickBossReward(void)
{
    s32 x, y;
    u32 roll;

    GetCurrentCell(&x, &y);
    roll = RoomRandom(x, y, 0xB0B0);
    gSpecialVar_0x8004 = (roll & 1) ? IsaacPickSpecialItem(roll >> 1) : ITEM_MASTER_BALL;
}

// Butin de la salle secrète, une entrée de la table tirée pour la salle (la Carte de Réduction, si on l'a déjà, est
// remplacée par l'entrée suivante)
enum
{
    SECRET_LOOT_ITEM_AND_TM,  // un très bon objet de combat et une très bonne CT
    SECRET_LOOT_CURSED_DIE,
    SECRET_LOOT_BLOOD_PACT,
    SECRET_LOOT_DISCOUNT_CARD,
    SECRET_LOOT_HEALS,        // une Guérison, un Rappel et un Total Soin
    SECRET_LOOT_COUNT,
};

static void PickSecretRoomLoot(s32 x, s32 y)
{
    u32 roll = RoomRandom(x, y, 0x600D);
    u32 entry = roll % SECRET_LOOT_COUNT;

    if (entry == SECRET_LOOT_DISCOUNT_CARD && CheckBagHasItem(ITEM_DISCOUNT_CARD, 1))
        entry++;
    roll >>= 8;
    switch (entry)
    {
    case SECRET_LOOT_ITEM_AND_TM:
        sLoot[sLootCount++] = sShopBattleItems[roll % ARRAY_COUNT(sShopBattleItems)];
        sLoot[sLootCount++] = PickGoodTM(roll >> 8);
        break;
    case SECRET_LOOT_CURSED_DIE:
        sLoot[sLootCount++] = ITEM_CURSED_DIE;
        break;
    case SECRET_LOOT_BLOOD_PACT:
        sLoot[sLootCount++] = ITEM_BLOOD_PACT;
        break;
    case SECRET_LOOT_DISCOUNT_CARD:
        sLoot[sLootCount++] = ITEM_DISCOUNT_CARD;
        break;
    default:
        sLoot[sLootCount++] = ITEM_FULL_RESTORE;
        sLoot[sLootCount++] = ITEM_REVIVE;
        sLoot[sLootCount++] = ITEM_FULL_HEAL;
        break;
    }
}

// Objet(s) de la salle trésor ou de la salle secrète, à donner un par un (IsaacNextLootItem) : un objet spécial au
// hasard dans une salle trésor, le butin de la table de la salle secrète (PickSecretRoomLoot)
void IsaacPickTreasureItem(void)
{
    s32 x, y;

    GetCurrentCell(&x, &y);
    sLootCount = 0;
    if (GetRoom(x, y) == ROOM_SECRET)
        PickSecretRoomLoot(x, y);
    else
        sLoot[sLootCount++] = IsaacPickSpecialItem(RoomRandom(x, y, 0x7EA5));
}

// Le Pokémon de la vieille dame dans gStringVar2 (Leveinard, Lokhlass dans l'océan)
void IsaacBufferHealerPokemon(void)
{
    StringCopy(gStringVar2, GetSpeciesName(gIsaacBiomes[ISAAC_BIOME].ocean ? SPECIES_LAPRAS : SPECIES_CHANSEY));
}

// Vieille dame et Leveinard : soin complet (PV, statut, PP) du Pokémon n° VAR_0x8004, même K.O. VAR_RESULT = FALSE pour
// un Œuf ou un Pokémon déjà en pleine forme ; gStringVar1 = son nom
void IsaacHealerHealMon(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];

    GetMonNickname(mon, gStringVar1);
    gSpecialVar_Result = MonNeedsHealing(mon);
    if (gSpecialVar_Result)
        HealPokemon(mon);
}

// Entraîneur : IVs à 31 et EVs de la répartition du set 3 de l'espèce (même variante) pour le Pokémon n° VAR_0x8004.
// VAR_RESULT = FALSE pour un Œuf ou un Pokémon déjà au maximum ; gStringVar1 = son nom
void IsaacMaxEvsIvs(void)
{
    // ordre des EVs dans les sets (TRAINER_PARTY_EVS) : PV, Att, Déf, AttSp, DéfSp, Vit
    static const u8 sSetEvOrder[NUM_STATS] = { STAT_HP, STAT_ATK, STAT_DEF, STAT_SPATK, STAT_SPDEF, STAT_SPEED };
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    const struct TrainerMon *set;
    u32 i, value;
    bool32 changed = FALSE;

    GetMonNickname(mon, gStringVar1);
    gSpecialVar_Result = FALSE;
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return;
    set = IsaacGetSet(GetMonData(mon, MON_DATA_SPECIES), ISAAC_SET_TIERS - 1, IsaacGetMonSetVariant(mon));
    for (i = 0; i < NUM_STATS; i++)
    {
        value = MAX_PER_STAT_IVS;
        if (GetMonData(mon, MON_DATA_HP_IV + i) != value)
            changed = TRUE;
        SetMonData(mon, MON_DATA_HP_IV + i, &value);
        if (set != NULL && set->ev != NULL)
        {
            value = set->ev[i];
            if (GetMonData(mon, MON_DATA_HP_EV + sSetEvOrder[i]) != value)
                changed = TRUE;
            SetMonData(mon, MON_DATA_HP_EV + sSetEvOrder[i], &value);
        }
    }
    CalculateMonStats(mon);
    gSpecialVar_Result = changed;
}

// Salle secrète à dresseur : une fois le dresseur vaincu, la récompense de la salle (sa table de butin, comme une salle
// secrète sans dresseur) est au sol jusqu'à ce qu'on la ramasse
static bool8 IsSecretRewardOnFloor(s32 x, s32 y)
{
    return GetRoom(x, y) == ROOM_SECRET && RoomHasTrainer(x, y) && IsCleared(x, y)
        && !FlagGet(FLAG_ISAAC_SECRET_LOOT_TAKEN);
}

// Après la victoire contre le dresseur : VAR_RESULT = TRUE si une récompense vient d'apparaître au sol (salle secrète)
void IsaacCheckSecretReward(void)
{
    s32 x, y;

    GetCurrentCell(&x, &y);
    gSpecialVar_Result = IsSecretRewardOnFloor(x, y);
}

// Au chargement de la salle : Pokémon sauvages (salle de capture), objet des salles trésor et secrète (tant qu'il n'est
// pas ramassé), boss, personnages
static void SetUpRoomObjects(s32 x, s32 y)
{
    u8 room = GetRoom(x, y);
    bool8 cleared = IsCleared(x, y);

    PlaceRoomObject(LOCALID_ISAAC_ROOM_ITEM, (room == ROOM_TREASURE || (room == ROOM_SECRET && !RoomHasTrainer(x, y))) && !cleared);
    if (IsSecretRewardOnFloor(x, y))  // salle secrète à dresseur : la récompense, à l'écart du chemin du dresseur
    {
        struct ObjectEventTemplate *template = GetObjectTemplate(LOCALID_ISAAC_ROOM_ITEM);

        if (template != NULL)
        {
            template->x = ROOM_MID_X - 2;
            template->y = ROOM_MID_Y + 2;
        }
    }
    PlaceRoomObject(LOCALID_ISAAC_ROOM_BOSS, room == ROOM_BOSS);
    PlaceRoomObject(LOCALID_ISAAC_ROOM_NURSE, FALSE);
    PlaceRoomObject(LOCALID_ISAAC_ROOM_SHOPKEEPER, room == ROOM_SHOP);
    VarSet(VAR_OBJ_GFX_ID_6, OBJ_EVENT_MON + SPECIES_KECLEON);
    VarSet(VAR_OBJ_GFX_ID_7, gIsaacBiomes[ISAAC_BIOME].ocean ? OBJ_EVENT_GFX_SWIMMER_F : OBJ_EVENT_GFX_NURSE);  // océan : nageuse
    if (room == ROOM_BOSS && cleared)
        PlaceNurse(x, y, FALSE);
    SetUpCaptureMons(x, y);
    SetUpRoomContent(x, y);
    if (room == ROOM_BOSS)
        SetUpBoss(x, y, cleared);
}

// Avant le combat contre le Pokémon n° VAR_0x8004 de la salle de capture. Suivre de `dowildbattle`.
void IsaacPrepareWildBattle(void)
{
    u16 item = ITEM_NONE;
    u16 species[CAPTURE_MAX_MONS];
    s32 x, y;

    GetCurrentCell(&x, &y);
    sCaptureSlot = min(gSpecialVar_0x8004, CAPTURE_MAX_MONS - 1);
    GetCaptureSpecies(x, y, species);
    CreateScriptedWildMon(species[sCaptureSlot], ISAAC_MON_LEVEL, ITEM_NONE);
    IsaacCreateMonFromSet(&gParties[B_TRAINER_OPPONENT_A][0], species[sCaptureSlot], GetCaptureSetTier(),
                          RoomRandom(x, y, 0x5EEE + sCaptureSlot) % ISAAC_SET_VARIANTS, RoomRandom(x, y, 0x5EED + sCaptureSlot));
    // un Pokémon sauvage ne tient pas d'objet
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM, &item);
}

// Après le combat (capturé, vaincu ou fui) : le Pokémon disparaît pour de bon
void IsaacCaptureMonDone(void)
{
    s32 x, y;

    FlagSet(FLAG_ISAAC_CAPTURE_DONE_FIRST + sCaptureSlot);
    GetCurrentCell(&x, &y);
    SetUpCaptureMons(x, y);
    RemoveObjectEventByLocalIdAndMap(sCaptureLocalIds[sCaptureSlot], gSaveBlock1Ptr->location.mapNum,
                                     gSaveBlock1Ptr->location.mapGroup);
}

// ---------------------------------------------------------------------------
// Déplacements
// ---------------------------------------------------------------------------

static void WarpToRoom(s32 x, s32 y)
{
    SetWarpDestination(MAP_GROUP(MAP_ISAAC_ROOM), MAP_NUM(MAP_ISAAC_ROOM), WARP_ID_NONE, x, y);
    DoWarp();
    ResetInitialPlayerAvatarState();
}

// Ordre des biomes de la prochaine descente, tiré une seule fois (voir FLAG_ISAAC_RUN_PREPARED)
static void PrepareRunBiomeOrder(void)
{
    if (FlagGet(FLAG_ISAAC_RUN_PREPARED))
        return;
    VarSet(VAR_ISAAC_BIOME_ORDER, Random());
    FlagSet(FLAG_ISAAC_RUN_PREPARED);
}

// Montagnard du hub : description du premier biome de la prochaine descente dans gStringVar1
void IsaacBufferFirstBiome(void)
{
    static const u8 *const sBiomeHints[ISAAC_BIOME_COUNT] =
    {
        [ISAAC_BIOME_CAVE]   = COMPOUND_STRING("une grotte rocheuse, pleine de\nPokémon Roche et Combat"),
        [ISAAC_BIOME_DARK]   = COMPOUND_STRING("une grotte hantée par des\nPokémon Spectre et Ténèbres"),
        [ISAAC_BIOME_FIRE]   = COMPOUND_STRING("un chemin brûlant, avec des\nPokémon Feu et Électrik"),
        [ISAAC_BIOME_ICE]    = COMPOUND_STRING("une grotte gelée, avec des\nPokémon Glace et Acier"),
        [ISAAC_BIOME_POISON] = COMPOUND_STRING("un tunnel moussu, plein de\nPokémon Poison et Insecte"),
        [ISAAC_BIOME_OCEAN]  = COMPOUND_STRING("l'océan, plein de\nPokémon Eau"),
        [ISAAC_BIOME_DESERT] = COMPOUND_STRING("un désert brûlant, avec des\nPokémon Sol et Psy"),
        [ISAAC_BIOME_CLIFF]  = COMPOUND_STRING("le haut des falaises, avec des\nPokémon Vol et Dragon"),
        [ISAAC_BIOME_FOREST] = COMPOUND_STRING("une forêt enchantée, pleine de\nPokémon Plante et Fée"),
    };

    PrepareRunBiomeOrder();
    StringCopy(gStringVar1, sBiomeHints[GetBiomeOfFloor(1)]);
}

// Nouvel étage (nouvelle graine) : arrivée au centre de la salle de départ
static void WarpToNewFloor(u32 floor)
{
    if (floor == 1)  // nouvelle descente : l'ordre des biomes annoncé par le Montagnard (sinon tiré maintenant)
    {
        PrepareRunBiomeOrder();
        FlagClear(FLAG_ISAAC_RUN_PREPARED);  // la descente suivante aura un nouvel ordre
        RemoveBagItem(ITEM_DISCOUNT_CARD, 1);  // la Carte de Réduction ne vaut que pour une descente
        RemoveBagItem(ITEM_LUCKY_COIN, 1);     // la Pièce Porte-Bonheur aussi
        IsaacGoldenHeartReset();
    }
    VarSet(VAR_ISAAC_FLOOR, floor);
    VarSet(VAR_ISAAC_SEED, Random());
    ClearVisited();
    ClearClearedRooms();
    FlagClear(FLAG_ISAAC_MASTER_BALL_DROPPED);
    for (u32 i = 0; i < CAPTURE_MAX_MONS; i++)
        FlagClear(FLAG_ISAAC_CAPTURE_DONE_FIRST + i);
    for (u32 i = 0; i < SHOP_LIMITED_SLOTS; i++)
        FlagClear(FLAG_ISAAC_SHOP_SOLD_FIRST + i);
    FlagClear(FLAG_ISAAC_SECRET_OPEN);
    FlagClear(FLAG_ISAAC_SECRET_LOOT_TAKEN);
    if (PartyKnowsMove(MOVE_ROCK_SMASH))
        FlagClear(FLAG_ISAAC_ROCK_SMASH_TEACHER);
    else
        FlagSet(FLAG_ISAAC_ROCK_SMASH_TEACHER);
    SetCurrentCell(ISAAC_START_X, ISAAC_START_Y);
    sArrivalFacing = DIR_SOUTH;
    WarpToRoom(ROOM_MID_X, ROOM_MID_Y);
}

// Porte : salle voisine, devant la porte d'en face, tourné dans le sens de la marche
static void GoThroughDoor(u32 dir)
{
    s32 x, y;
    u32 opposite = (dir + 2) % ISAAC_DIR_COUNT;

    GetCurrentCell(&x, &y);
    SetCurrentCell(x + sDirDx[dir], y + sDirDy[dir]);
    sArrivalFacing = sDirToFacing[dir];
    WarpToRoom(sDoorX[opposite] + sDirDx[dir], sDoorY[opposite] + sDirDy[dir]);
}

// ---------------------------------------------------------------------------
// Mini-carte
// ---------------------------------------------------------------------------

// Visible : visitée, ou voisine d'une salle visitée (sauf la salle secrète tant que son passage est fermé)
static bool8 IsRoomShownOnMinimap(s32 x, s32 y)
{
    u32 dir;

    if (GetRoom(x, y) == ROOM_NONE)
        return FALSE;
    if (IsVisited(x, y))
        return TRUE;
    if (GetRoom(x, y) == ROOM_SECRET && !FlagGet(FLAG_ISAAC_SECRET_OPEN))
        return FALSE;
    for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
        if (IsVisited(x + sDirDx[dir], y + sDirDy[dir]))
            return TRUE;
    return FALSE;
}

static enum Species GetMinimapIconSpecies(u8 room)
{
    switch (room)
    {
    case ROOM_BOSS:
        return SPECIES_MEWTWO;
    case ROOM_TREASURE:
        return SPECIES_PIKACHU;
    case ROOM_SHOP:
        return SPECIES_KECLEON;
    case ROOM_SECRET:
        return SPECIES_UNOWN_QUESTION;
    default:
        return SPECIES_NONE;
    }
}

// Icône centrée sur la salle (x, y : centre de la salle, en pixels de l'écran)
static u8 CreateMinimapIcon(enum Species species, s32 x, s32 y)
{
    u8 spriteId;

    LoadMonIconPalette(species);
    spriteId = CreateMonIcon(species, SpriteCB_MonIcon, x, y - MINIMAP_ICON_RISE, 0, 0);
    if (spriteId != MAX_SPRITES)
        gSprites[spriteId].oam.priority = 0;  // devant la fenêtre
    return spriteId;
}

// Icône de Poké Ball centrée sur la salle de capture (x, y : centre de la salle, en pixels de l'écran)
static u8 CreateMinimapBallIcon(s32 x, s32 y)
{
    u8 spriteId = AddItemIconSprite(MINIMAP_BALL_TAG, MINIMAP_BALL_TAG, ITEM_POKE_BALL);

    // L'icône d'objet (24 x 24) est dans le coin haut gauche d'un sprite de 32 x 32 : on la recentre
    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].x = x + MINIMAP_ITEM_ICON_OFFSET;
        gSprites[spriteId].y = y + MINIMAP_ITEM_ICON_OFFSET;
        gSprites[spriteId].oam.priority = 0;  // devant la fenêtre
    }
    return spriteId;
}

// Dessine la carte dans la fenêtre et crée les icônes (sprites) des salles spéciales : renvoie le nombre
// d'icônes de Pokémon ; *ballSpriteId : icône de la salle de capture (MAX_SPRITES si elle n'est pas visible)
static u32 DrawMinimap(u8 windowId, u8 *iconSpriteIds, u16 *iconSpecies, u8 *ballSpriteId)
{
    u8 title[16], *end;
    s32 x, y, curX, curY, minX = ISAAC_GRID_W, maxX = -1, minY = ISAAC_GRID_H, maxY = -1;
    u32 roomW, roomH, cols, rows, left, top, px, py;
    u32 iconCount = 0;
    enum Species species;

    *ballSpriteId = MAX_SPRITES;
    FillWindowPixelBuffer(windowId, PIXEL_FILL(MINIMAP_COLOR_BG));
    end = ConvertIntToDecimalStringN(StringCopy(title, COMPOUND_STRING("ÉTAGE ")), VarGet(VAR_ISAAC_FLOOR),
                                     STR_CONV_MODE_LEFT_ALIGN, 3);
    *end++ = CHAR_SLASH;
    ConvertIntToDecimalStringN(end, ISAAC_FLOOR_COUNT, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized(windowId, FONT_NORMAL, title,
                                GetStringCenterAlignXOffset(FONT_NORMAL, title, MINIMAP_AREA_W), 0, TEXT_SKIP_DRAW, NULL);

    // Les salles visibles sont centrées dans la fenêtre
    for (y = 0; y < ISAAC_GRID_H; y++)
    {
        for (x = 0; x < ISAAC_GRID_W; x++)
        {
            if (!IsRoomShownOnMinimap(x, y))
                continue;
            minX = min(minX, x);
            maxX = max(maxX, x);
            minY = min(minY, y);
            maxY = max(maxY, y);
        }
    }
    cols = maxX - minX + 1;
    rows = maxY - minY + 1;
    if (cols * (MINIMAP_BIG_ROOM_W + MINIMAP_GAP) - MINIMAP_GAP <= MINIMAP_AREA_W
     && rows * (MINIMAP_BIG_ROOM_H + MINIMAP_GAP) - MINIMAP_GAP <= MINIMAP_AREA_H)
    {
        roomW = MINIMAP_BIG_ROOM_W;
        roomH = MINIMAP_BIG_ROOM_H;
    }
    else
    {
        roomW = MINIMAP_SMALL_ROOM_W;
        roomH = MINIMAP_SMALL_ROOM_H;
    }
    left = (MINIMAP_AREA_W - (cols * (roomW + MINIMAP_GAP) - MINIMAP_GAP)) / 2;
    top = MINIMAP_TITLE_H + (MINIMAP_AREA_H - (rows * (roomH + MINIMAP_GAP) - MINIMAP_GAP)) / 2;

    GetCurrentCell(&curX, &curY);
    for (y = minY; y <= maxY; y++)
    {
        for (x = minX; x <= maxX; x++)
        {
            if (!IsRoomShownOnMinimap(x, y))
                continue;
            px = left + (x - minX) * (roomW + MINIMAP_GAP);
            py = top + (y - minY) * (roomH + MINIMAP_GAP);
            if (x == curX && y == curY)  // contour d'un pixel, dans l'espace entre les salles
                FillWindowPixelRect(windowId, PIXEL_FILL(MINIMAP_COLOR_OUTLINE), px - 1, py - 1, roomW + 2, roomH + 2);
            FillWindowPixelRect(windowId, PIXEL_FILL(IsVisited(x, y) ? MINIMAP_COLOR_VISITED : MINIMAP_COLOR_SEEN),
                                px, py, roomW, roomH);

            if (GetRoom(x, y) == ROOM_CAPTURE)
            {
                *ballSpriteId = CreateMinimapBallIcon(sMinimapWindowTemplate.tilemapLeft * 8 + px + roomW / 2,
                                                      sMinimapWindowTemplate.tilemapTop * 8 + py + roomH / 2);
                continue;
            }
            species = GetMinimapIconSpecies(GetRoom(x, y));
            if (species == SPECIES_NONE || iconCount >= MINIMAP_MAX_ICONS)
                continue;
            iconSpriteIds[iconCount] = CreateMinimapIcon(species, sMinimapWindowTemplate.tilemapLeft * 8 + px + roomW / 2,
                                                         sMinimapWindowTemplate.tilemapTop * 8 + py + roomH / 2);
            iconSpecies[iconCount++] = species;
        }
    }
    return iconCount;
}

#define tWindowId   data[0]
#define tOpened     data[1]
#define tIconCount  data[2]
#define tIconSprite(i)   data[3 + (i)]
#define tIconSpecies(i)  data[3 + MINIMAP_MAX_ICONS + (i)]
#define tBallSprite      data[3 + 2 * MINIMAP_MAX_ICONS]

STATIC_ASSERT(4 + 2 * MINIMAP_MAX_ICONS <= NUM_TASK_DATA, MinimapTaskDataFits);

// Ferme la mini-carte avec A, B ou R (pas à la première image : l'appui sur R qui l'a ouverte compte encore)
static void Task_Minimap(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u32 i;

    if (!tOpened)
    {
        tOpened = TRUE;
        return;
    }
    if (!(JOY_NEW(A_BUTTON | B_BUTTON | R_BUTTON)))
        return;
    PlaySE(SE_WIN_OPEN);
    for (i = 0; i < tIconCount; i++)
    {
        if (tIconSprite(i) != MAX_SPRITES)
            FreeAndDestroyMonIconSprite(&gSprites[tIconSprite(i)]);
    }
    for (i = 0; i < tIconCount; i++)
        FreeMonIconPalette(tIconSpecies(i));
    if (tBallSprite != MAX_SPRITES)
    {
        FreeSpriteTilesByTag(MINIMAP_BALL_TAG);
        FreeSpritePaletteByTag(MINIMAP_BALL_TAG);
        DestroySprite(&gSprites[tBallSprite]);
    }
    ClearStdWindowAndFrameToTransparent(tWindowId, TRUE);
    RemoveWindow(tWindowId);
    DestroyTask(taskId);
    ScriptContext_Enable();
}

// ---------------------------------------------------------------------------
// Points d'entrée
// ---------------------------------------------------------------------------

// Script ON_LOAD de IsaacRoom : (re)construit l'étage et dessine la salle courante
void IsaacPaintRoom(void)
{
    s32 x, y;

    BuildFloorPlan();
    GetCurrentCell(&x, &y);
    if (!IsVisited(x, y))
        IsaacTickEggs();
    SetVisited(x, y);
    CarveRoom();
    PaintRoom();
    SetUpRoomTrainer(x, y);
    SetUpRoomObjects(x, y);
}

// Dresseur vaincu, Pokémon de la salle de capture combattu, Pierre Évolution ramassée : la salle est nettoyée.
// Le Pokémon et la Pierre sont aussi retirés de la map (sinon le jeu les refait apparaître au moindre
// mouvement de caméra après le `removeobject`, leur objet étant toujours placé au centre).
void IsaacSetRoomCleared(void)
{
    s32 x, y;

    GetCurrentCell(&x, &y);
    if (IsSecretRewardOnFloor(x, y))  // la récompense de la salle secrète à dresseur est ramassée
        FlagSet(FLAG_ISAAC_SECRET_LOOT_TAKEN);
    else
        FlagSet(FLAG_ISAAC_CLEARED_FIRST + y * ISAAC_GRID_W + x);
    SetUpRoomObjects(x, y);
}

// Remplit gIsaacTrainerReturnMovement : le dresseur revient en ligne droite au centre de la salle (il s'en est
// éloigné en ligne droite pour approcher le joueur), puis regarde vers le bas
void IsaacPrepareTrainerReturn(void)
{
    struct ObjectEvent *trainer;
    u8 objectEventId;
    s32 dx, dy;
    u32 n = 0;

    if (TryGetObjectEventIdByLocalIdAndMap(LOCALID_ISAAC_ROOM_TRAINER, gSaveBlock1Ptr->location.mapNum,
                                           gSaveBlock1Ptr->location.mapGroup, &objectEventId))
    {
        gIsaacTrainerReturnMovement[0] = MOVEMENT_ACTION_STEP_END;
        return;
    }
    trainer = &gObjectEvents[objectEventId];
    dx = ROOM_MID_X - (trainer->currentCoords.x - MAP_OFFSET);
    dy = ROOM_MID_Y - (trainer->currentCoords.y - MAP_OFFSET);
    for (; dx > 0 && n < ISAAC_ROOM_W; dx--)
        gIsaacTrainerReturnMovement[n++] = MOVEMENT_ACTION_WALK_NORMAL_RIGHT;
    for (; dx < 0 && n < ISAAC_ROOM_W; dx++)
        gIsaacTrainerReturnMovement[n++] = MOVEMENT_ACTION_WALK_NORMAL_LEFT;
    for (; dy > 0 && n < ISAAC_ROOM_W; dy--)
        gIsaacTrainerReturnMovement[n++] = MOVEMENT_ACTION_WALK_NORMAL_DOWN;
    for (; dy < 0 && n < ISAAC_ROOM_W; dy++)
        gIsaacTrainerReturnMovement[n++] = MOVEMENT_ACTION_WALK_NORMAL_UP;
    gIsaacTrainerReturnMovement[n++] = MOVEMENT_ACTION_FACE_DOWN;
    gIsaacTrainerReturnMovement[n] = MOVEMENT_ACTION_STEP_END;
}

// Bouton R dans une salle : ouvre la mini-carte. Suivre de `waitstate`.
void IsaacShowMinimap(void)
{
    u8 taskId = CreateTask(Task_Minimap, 80);
    s16 *data = gTasks[taskId].data;
    u8 iconSpriteIds[MINIMAP_MAX_ICONS];
    u16 iconSpecies[MINIMAP_MAX_ICONS];
    u8 ballSpriteId;
    u32 i;

    tWindowId = AddWindow(&sMinimapWindowTemplate);
    SetStandardWindowBorderStyle(tWindowId, FALSE);
    tIconCount = DrawMinimap(tWindowId, iconSpriteIds, iconSpecies, &ballSpriteId);
    tBallSprite = ballSpriteId;
    for (i = 0; i < tIconCount; i++)
    {
        tIconSprite(i) = iconSpriteIds[i];
        tIconSpecies(i) = iconSpecies[i];
    }
    PutWindowTilemap(tWindowId);
    CopyWindowToVram(tWindowId, COPYWIN_FULL);
}

#undef tWindowId
#undef tOpened
#undef tIconCount
#undef tIconSprite
#undef tIconSpecies
#undef tBallSprite

bool8 TryOpenIsaacMinimap(void)
{
    if (!IsInRoomMap())
        return FALSE;
    ScriptContext_SetupScript(Isaac_EventScript_Minimap);
    return TRUE;
}

// Arrivée dans une salle (ON_WARP_INTO_MAP_TABLE) : le joueur regarde dans le sens de sa marche
void IsaacFaceArrivalDirection(void)
{
    if (sArrivalFacing != DIR_NONE)
        ObjectEventTurn(&gObjectEvents[gPlayerAvatar.objectEventId], sArrivalFacing);
    sArrivalFacing = DIR_NONE;
}

// Sortie déclenchée par TryStartIsaacStepScript. Suivre de `waitstate`.
void IsaacTakeExit(void)
{
    switch (sPendingExit)
    {
    case EXIT_DOOR:
        GoThroughDoor(sPendingDoor);
        break;
    case EXIT_NEXT_FLOOR:
        if (VarGet(VAR_ISAAC_FLOOR) >= ISAAC_FLOOR_COUNT)
        {
            SetWarpDestination(MAP_GROUP(MAP_ISAAC_WIN), MAP_NUM(MAP_ISAAC_WIN), WARP_ID_NONE, WIN_ROOM_X, WIN_ROOM_Y);
            DoWarp();
            ResetInitialPlayerAvatarState();
        }
        else
        {
            WarpToNewFloor(VarGet(VAR_ISAAC_FLOOR) + 1);
        }
        break;
    case EXIT_HUB_ENTRANCE:
        WarpToNewFloor(1);
        break;
    }
    sPendingExit = EXIT_NONE;
}

// Appelé à chaque pas du joueur : entrée du donjon dans le hub, portes des salles, passage du boss
bool8 TryStartIsaacStepScript(s16 x, s16 y)
{
    s32 cellX, cellY;
    u32 dir;

    x -= MAP_OFFSET;
    y -= MAP_OFFSET;
    sPendingExit = EXIT_NONE;
    if (IsInHub())
    {
        if (x == HUB_ENTRANCE_X && y == HUB_ENTRANCE_Y)
        {
            if (CalculatePlayerPartyCount() == 0)  // pas sans Pokémon : le Montagnard retient le joueur
            {
                ScriptContext_SetupScript(Hub_EventScript_NeedPokemon);
                return TRUE;
            }
            sPendingExit = EXIT_HUB_ENTRANCE;
        }
    }
    else if (IsInRoomMap())
    {
        GetCurrentCell(&cellX, &cellY);
        for (dir = 0; dir < ISAAC_DIR_COUNT; dir++)
        {
            if (x != sDoorX[dir] || y != sDoorY[dir])
                continue;
            if (GetRoom(cellX, cellY) == ROOM_BOSS && IsCleared(cellX, cellY) && dir == GetBossExitDir(cellX, cellY))
            {
                sPendingExit = EXIT_NEXT_FLOOR;  // passage ouvert par la victoire contre le boss : étage suivant
            }
            else
            {
                sPendingExit = EXIT_DOOR;
                sPendingDoor = dir;
            }
        }
    }
    if (sPendingExit == EXIT_NONE)
        return FALSE;
    ScriptContext_SetupScript(Isaac_EventScript_Exit);
    return TRUE;
}
