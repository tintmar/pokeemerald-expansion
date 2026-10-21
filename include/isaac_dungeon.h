#ifndef GUARD_ISAAC_DUNGEON_H
#define GUARD_ISAAC_DUNGEON_H

#define ISAAC_GRID_W  9  // cellules d'un étage (une salle par cellule)
#define ISAAC_GRID_H  8

#define ISAAC_ROOM_W  15  // taille de la map IsaacRoom (cases)
#define ISAAC_ROOM_H  11

struct MapHeader;
struct Pokemon;

void IsaacPaintRoom(void);
u32 IsaacGetCurrentBiome(void);
bool8 IsaacApplyBiomeToMapHeader(struct MapHeader *header, u16 *layoutId);
enum MapType IsaacGetRoomMapType(void);
u16 IsaacGetBiomeMusic(void);
void IsaacFaceArrivalDirection(void);
void IsaacTakeExit(void);
bool8 TryStartIsaacStepScript(s16 x, s16 y);
void IsaacShowMinimap(void);
void IsaacSetRoomCleared(void);
void IsaacPrepareTrainerReturn(void);
void IsaacOpenBossExit(void);
void IsaacBufferBossName(void);
void IsaacPrepareWildBattle(void);
void IsaacCaptureMonDone(void);
void IsaacPickTrainerLoot(void);
void IsaacPickBossReward(void);
void IsaacPrepareNurseArrival(void);
void IsaacSettleNurse(void);
void IsaacNurseGetPrice(void);
void IsaacNurseTryHeal(void);
u32 IsaacGetShopPrice(u16 item);
u16 IsaacGetShopStock(u16 item);
void IsaacOnShopPurchase(u16 item, u16 count);
void IsaacOpenShop(void);
void IsaacKecleonGreeting(void);
void IsaacGetRoomContent(void);
void IsaacGiveEgg(void);
void IsaacPrepareTrade(void);
void IsaacDoTrade(void);
void IsaacPrepareDevilPact(void);
void IsaacCheckDevilSacrifice(void);
void IsaacDoDevilPact(void);
void IsaacPrepareTutor(void);
void IsaacTutorChooseMove(void);
void IsaacTutorChooseMoveToForget(void);
void IsaacTutorReplaceMove(void);
void IsaacPrepareDuel(void);
void IsaacDuelCheckMon(void);
void IsaacDuelSetUpParty(void);
void IsaacDuelRestoreParty(void);
void IsaacPickDuelReward(void);
void IsaacBoostTrainerParty(u16 trainerId, struct Pokemon *party);

extern u8 gIsaacTrainerReturnMovement[];
extern u8 gIsaacNurseArrivalMovement[];
bool8 TryOpenIsaacMinimap(void);

#endif // GUARD_ISAAC_DUNGEON_H
