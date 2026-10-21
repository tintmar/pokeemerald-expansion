#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_camera.h"
#include "fieldmap.h"
#include "data.h"
#include "international_string_util.h"
#include "isaac_dungeon.h"
#include "isaac_pokemon.h"
#include "item_icon.h"
#include "menu.h"
#include "pokemon_icon.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "script_pokemon_util.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/battle_ai.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/map_event_ids.h"
#include "constants/opponents.h"
#include "constants/trainers.h"
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
 *      (au moins 2, jamais le boss). Les portes qui y mènent ressemblent à de la roche mais on peut
 *      passer à travers.
 *
 * Tout ne dépend que de VAR_ISAAC_SEED et VAR_ISAAC_FLOOR : l'étage est régénéré à l'identique à chaque
 * chargement (retour de combat, de menu...). VAR_ISAAC_ROOM est la cellule où se trouve le joueur,
 * VAR_ISAAC_VISITED_0 à 4 les salles déjà visitées (un bit par cellule).
 *
 * DRESSEURS : une salle normale a ISAAC_TRAINER_PCT % de chances d'avoir un dresseur du biome, au centre. Il
 * regarde la porte par laquelle le joueur arrive : le moteur le fait réagir, s'approcher et lancer le combat
 * dès l'arrivée. Vaincu, il retourne au centre et la salle est « nettoyée » (FLAG_ISAAC_CLEARED_FIRST + cellule).
 * Pour l'instant : 2 Pokémon de catégorie 1 de ses lignées, set 1. Vaincu, il donne ISAAC_TRAINER_MONEY_PER_FLOOR x
 * la profondeur, et un objet de son butin (IsaacPickTrainerLoot).
 *
 * SALLE DE CAPTURE : un Pokémon sauvage du biome (catégorie 2, set 2) se promène ; lui parler lance le
 * combat, après quoi il disparaît. SALLE TRÉSOR : une Pierre Évolution au sol.
 *
 * MINI-CARTE (bouton R dans une salle) : comme Isaac, on voit les salles visitées et celles qui les
 * touchent (pas la salle secrète tant qu'on n'y est pas entré), avec l'icône des salles spéciales.
 */

#define ISAAC_MAX_ROOMS        20
#define ISAAC_SHOP_MIN_FLOOR   2
#define ISAAC_SECRET_MIN_FLOOR 3
#define ISAAC_MAX_TRIES        200  // graines essayées pour un étage valide

#define ISAAC_TRAINER_PCT    75  // % de salles normales avec un dresseur
#define ISAAC_TRAINER_MONEY_PER_FLOOR  1000  // argent gagné contre un dresseur (ou le boss) : x profondeur
#define ISAAC_BIOME          ISAAC_BIOME_CAVE  // un seul biome pour l'instant

// Salles nettoyées (dresseur vaincu) : un drapeau par cellule, dans des drapeaux inutilisés du jeu
#define FLAG_ISAAC_CLEARED_FIRST  FLAG_UNUSED_0x264
STATIC_ASSERT(FLAG_ISAAC_CLEARED_FIRST + ISAAC_GRID_W * ISAAC_GRID_H - 1 <= FLAG_UNUSED_0x2BB, IsaacClearedFlagsAreUnused);

#define ISAAC_START_X  (ISAAC_GRID_W / 2)
#define ISAAC_START_Y  (ISAAC_GRID_H / 2)

// Salle IsaacRoom : murs de 1 case à gauche et à droite, 2 en haut et en bas
#define ROOM_MIN_X  1
#define ROOM_MAX_X  (ISAAC_ROOM_W - 2)
#define ROOM_MIN_Y  2
#define ROOM_MAX_Y  (ISAAC_ROOM_H - 3)
#define ROOM_MID_X  (ISAAC_ROOM_W / 2)
#define ROOM_MID_Y  ((ROOM_MIN_Y + ROOM_MAX_Y) / 2)

// Metatiles (0x200 et plus : tileset secondaire Cave)
#define TILE_FLOOR       0x201
#define TILE_ROCK_1      0x202
#define TILE_ROCK_2      0x203
#define TILE_CLIFF_TL    0x208
#define TILE_CLIFF_T     0x209
#define TILE_CLIFF_TR    0x20A
#define TILE_CLIFF_L     0x210
#define TILE_CLIFF_C     0x211
#define TILE_CLIFF_R     0x212
#define TILE_CLIFF_BL    0x218
#define TILE_CLIFF_B     0x219
#define TILE_CLIFF_BR    0x21A
#define TILE_CLIFF_IN_SE 0x21B  // coins rentrants : sol en diagonale
#define TILE_CLIFF_IN_SW 0x21C
#define TILE_CLIFF_IN_NE 0x223
#define TILE_CLIFF_IN_NW 0x224

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
    CELL_HIDDEN,  // porte de la salle secrète : roche, mais on passe
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

// 6 à 8 salles au 1er étage, 9 à 11 au 2e, 13 à 15 au 3e... jusqu'à ISAAC_MAX_ROOMS
static u32 GetTargetRoomCount(u32 floor)
{
    u32 count = 3 + floor * 10 / 3 + IsaacRandom() % 3;

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
    u32 needed = (floor >= ISAAC_SHOP_MIN_FLOOR) ? 4 : 3;  // boss, trésor, capture (+ boutique)
    u32 deadCount;

    GrowFloor(target);
    if (sFloor.roomCount < target)
        return FALSE;
    deadCount = FindDeadEnds(deadX, deadY);
    if (deadCount < needed)
        return FALSE;

    // Boss sur le cul-de-sac le plus éloigné, les autres spéciales au hasard parmi le reste
    sFloor.room[deadY[0]][deadX[0]] = ROOM_BOSS;
    PlaceOnFreeDeadEnd(deadX, deadY, deadCount, ROOM_TREASURE);
    PlaceOnFreeDeadEnd(deadX, deadY, deadCount, ROOM_CAPTURE);
    if (floor >= ISAAC_SHOP_MIN_FLOOR)
        PlaceOnFreeDeadEnd(deadX, deadY, deadCount, ROOM_SHOP);
    if (floor >= ISAAC_SECRET_MIN_FLOOR)
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
        if (there == ROOM_SECRET && here != ROOM_SECRET)
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

static u16 GetCliffTile(s32 x, s32 y)
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

static void PaintRoom(void)
{
    s32 x, y, cellX, cellY;
    u16 tile;

    ComputeBigRocks();
    for (y = PAINT_MIN; y < PAINT_MIN + PAINT_H; y++)
    {
        for (x = PAINT_MIN; x < PAINT_MIN + PAINT_W; x++)
        {
            if (IsBigRockAt(x, y))
                tile = GetCliffTile(x, y);
            else if (IsWallAt(x, y))  // bout de mur trop fin pour une falaise : rocher isolé
                tile = (IsaacRandom() & 1) ? TILE_ROCK_1 : TILE_ROCK_2;
            else
                tile = TILE_FLOOR;

            if (sCell[y - PAINT_MIN][x - PAINT_MIN] == CELL_WALL)
                SetTile(x, y, tile | MAPGRID_IMPASSABLE);
            else
                SetTile(x, y, tile | PACK_ELEVATION(FLOOR_ELEVATION));
        }
    }

    GetCurrentCell(&cellX, &cellY);
    PaintDoorMats(cellX, cellY);
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
static EWRAM_DATA struct TrainerMon sRoomTrainerMons[3] = {0};  // dresseur : 2, boss : 3

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

static bool8 RoomHasTrainer(s32 x, s32 y)
{
    return GetRoom(x, y) == ROOM_NORMAL && RoomRandom(x, y, 0x7A1) % 100 < ISAAC_TRAINER_PCT;
}

static const struct IsaacTrainerType *GetRoomTrainerType(s32 x, s32 y)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];

    return &biome->trainers[RoomRandom(x, y, 0x7E3) % biome->trainerCount];
}

// Espèce de la catégorie voulue dans les lignées du dresseur (sinon dans celles du biome)
static u16 PickTrainerSpecies(const struct IsaacTrainerType *type, enum IsaacCategory category, u32 roll)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];
    u16 species = IsaacPickLineSpecies(type->lines, type->lineCount, category, roll);

    if (species == SPECIES_NONE)
        species = IsaacPickLineSpecies(biome->lines, biome->lineCount, category, roll);
    return species != SPECIES_NONE ? species : biome->lines[0];
}

// Ajoute à l'équipe un Pokémon de la catégorie et du set voulus, d'une espèce pas encore dans l'équipe si possible
static void AddTrainerMon(struct Trainer *trainer, struct TrainerMon *party, const struct IsaacTrainerType *type,
                          enum IsaacCategory category, u32 tier, u32 seed)
{
    u16 species = SPECIES_NONE;
    u32 try, i;
    bool8 taken = TRUE;

    for (try = 0; try < 8 && taken; try++)
    {
        species = PickTrainerSpecies(type, category, seed + try * 0x9E3779B9);
        taken = FALSE;
        for (i = 0; i < trainer->partySize; i++)
            if (party[i].species == species)
                taken = TRUE;
    }
    party[trainer->partySize] = *IsaacGetSet(species, tier);
    party[trainer->partySize].lvl = ISAAC_MON_LEVEL;
    trainer->partySize++;
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

bool8 IsIsaacTrainer(u16 trainerId)
{
    return (trainerId == TRAINER_ISAAC_ROOM || trainerId == TRAINER_ISAAC_BOSS) && IsInRoomMap();
}

// Dresseur de la salle courante (classe et nom selon son type, 2 Pokémon de catégorie 1, set 1) ou boss de
// l'étage (champion du biome : 2 Pokémon de catégorie 1 et 1 de catégorie 2, set 2)
const struct Trainer *GetIsaacTrainer(u16 trainerId)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];
    const struct IsaacTrainerType *type;
    const u8 *const *names;
    s32 x, y;

    GetCurrentCell(&x, &y);
    if (trainerId == TRAINER_ISAAC_BOSS)
    {
        InitTrainer(&sRoomTrainer, biome->boss, biome->bossName, sRoomTrainerMons);
        sRoomTrainer.aiFlags = AI_FLAG_SMART_TRAINER;
        AddTrainerMon(&sRoomTrainer, sRoomTrainerMons, biome->boss, ISAAC_CATEGORY_1, 1, RoomRandom(x, y, 0xB051));
        AddTrainerMon(&sRoomTrainer, sRoomTrainerMons, biome->boss, ISAAC_CATEGORY_1, 1, RoomRandom(x, y, 0xB052));
        AddTrainerMon(&sRoomTrainer, sRoomTrainerMons, biome->boss, ISAAC_CATEGORY_2, 1, RoomRandom(x, y, 0xB053));
        return &sRoomTrainer;
    }

    type = GetRoomTrainerType(x, y);
    names = (type->gender == TRAINER_GENDER_FEMALE) ? sFemaleTrainerNames : sMaleTrainerNames;
    InitTrainer(&sRoomTrainer, type, names[RoomRandom(x, y, 0x4A3) % ARRAY_COUNT(sMaleTrainerNames)], sRoomTrainerMons);
    AddTrainerMon(&sRoomTrainer, sRoomTrainerMons, type, ISAAC_CATEGORY_1, 0, RoomRandom(x, y, 0x5EC));
    AddTrainerMon(&sRoomTrainer, sRoomTrainerMons, type, ISAAC_CATEGORY_1, 0, RoomRandom(x, y, 0x5ED));
    return &sRoomTrainer;
}

// Argent gagné contre un dresseur ou le boss du donjon
u32 IsaacGetTrainerMoney(void)
{
    return ISAAC_TRAINER_MONEY_PER_FLOOR * VarGet(VAR_ISAAC_FLOOR);
}

// Butin d'un dresseur vaincu (toujours le même pour une salle) : Guérison ou Rappel, moitié-moitié.
// VAR_0x8004 = l'objet, pour `giveitem VAR_0x8004`.
void IsaacPickTrainerLoot(void)
{
    static const u16 sLoot[] = { ITEM_FULL_RESTORE, ITEM_REVIVE };
    s32 x, y;

    GetCurrentCell(&x, &y);
    gSpecialVar_0x8004 = sLoot[RoomRandom(x, y, 0x1007) % ARRAY_COUNT(sLoot)];
}

static struct ObjectEventTemplate *GetObjectTemplate(u8 localId)
{
    u32 i;

    for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
        if (gSaveBlock1Ptr->objectEventTemplates[i].localId == localId)
            return &gSaveBlock1Ptr->objectEventTemplates[i];
    return NULL;
}

// Au chargement de la salle : place (ou cache) le dresseur et choisit son apparence. Pas encore vaincu, il
// regarde la porte par laquelle le joueur arrive (la vue du dresseur couvre toute la salle).
static void SetUpRoomTrainer(s32 x, s32 y)
{
    struct ObjectEventTemplate *template = GetObjectTemplate(LOCALID_ISAAC_ROOM_TRAINER);

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

// Pokémon sauvage de la salle de capture : catégorie 2, des lignées du biome, set 2
static u16 GetCaptureSpecies(s32 x, s32 y)
{
    const struct IsaacBiome *biome = &gIsaacBiomes[ISAAC_BIOME];
    u16 species = IsaacPickLineSpecies(biome->lines, biome->lineCount, ISAAC_CATEGORY_2, RoomRandom(x, y, 0xCA7));

    return species != SPECIES_NONE ? species : biome->lines[0];
}

// Pour l'instant, toujours le set 2
static u32 GetCaptureSetTier(s32 x, s32 y)
{
    return 1;
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

// Au chargement de la salle : Pokémon sauvage (salle de capture) et Pierre Évolution (salle trésor), tant que la
// salle n'est pas nettoyée
static void SetUpRoomObjects(s32 x, s32 y)
{
    u8 room = GetRoom(x, y);
    bool8 cleared = IsCleared(x, y);

    PlaceRoomObject(LOCALID_ISAAC_ROOM_WILD, room == ROOM_CAPTURE && !cleared);
    PlaceRoomObject(LOCALID_ISAAC_ROOM_ITEM, room == ROOM_TREASURE && !cleared);
    PlaceRoomObject(LOCALID_ISAAC_ROOM_BOSS, room == ROOM_BOSS);
    if (room == ROOM_CAPTURE)
        VarSet(VAR_OBJ_GFX_ID_1, OBJ_EVENT_MON + GetCaptureSpecies(x, y));
    if (room == ROOM_BOSS)
        SetUpBoss(x, y, cleared);
}

// Avant le combat contre le Pokémon de la salle de capture. Suivre de `dowildbattle`.
void IsaacPrepareWildBattle(void)
{
    s32 x, y;
    u16 species;

    GetCurrentCell(&x, &y);
    species = GetCaptureSpecies(x, y);
    CreateScriptedWildMon(species, ISAAC_MON_LEVEL, ITEM_NONE);
    IsaacCreateMonFromSet(&gParties[B_TRAINER_OPPONENT_A][0], species, GetCaptureSetTier(x, y), RoomRandom(x, y, 0x5EED));
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

// Nouvel étage (nouvelle graine) : arrivée au centre de la salle de départ
static void WarpToNewFloor(u32 floor)
{
    VarSet(VAR_ISAAC_FLOOR, floor);
    VarSet(VAR_ISAAC_SEED, Random());
    ClearVisited();
    ClearClearedRooms();
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

// Visible : visitée, ou voisine d'une salle visitée (sauf la salle secrète, qu'il faut avoir visitée)
static bool8 IsRoomShownOnMinimap(s32 x, s32 y)
{
    u32 dir;

    if (GetRoom(x, y) == ROOM_NONE)
        return FALSE;
    if (IsVisited(x, y))
        return TRUE;
    if (GetRoom(x, y) == ROOM_SECRET)
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
    u8 title[16];
    s32 x, y, curX, curY, minX = ISAAC_GRID_W, maxX = -1, minY = ISAAC_GRID_H, maxY = -1;
    u32 roomW, roomH, cols, rows, left, top, px, py;
    u32 iconCount = 0;
    enum Species species;

    *ballSpriteId = MAX_SPRITES;
    FillWindowPixelBuffer(windowId, PIXEL_FILL(MINIMAP_COLOR_BG));
    ConvertIntToDecimalStringN(StringCopy(title, COMPOUND_STRING("ÉTAGE ")), VarGet(VAR_ISAAC_FLOOR),
                               STR_CONV_MODE_LEFT_ALIGN, 3);
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
        WarpToNewFloor(VarGet(VAR_ISAAC_FLOOR) + 1);
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
