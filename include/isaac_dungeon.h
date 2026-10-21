#ifndef GUARD_ISAAC_DUNGEON_H
#define GUARD_ISAAC_DUNGEON_H

#define ISAAC_GRID_W  9  // cellules d'un étage (une salle par cellule)
#define ISAAC_GRID_H  8

#define ISAAC_ROOM_W  15  // taille de la map IsaacRoom (cases)
#define ISAAC_ROOM_H  11

void IsaacPaintRoom(void);
void IsaacFaceArrivalDirection(void);
void IsaacTakeExit(void);
bool8 TryStartIsaacStepScript(s16 x, s16 y);
void IsaacShowMinimap(void);
void IsaacSetRoomCleared(void);
void IsaacPrepareTrainerReturn(void);
void IsaacRevealLadder(void);

extern u8 gIsaacTrainerReturnMovement[];
bool8 TryOpenIsaacMinimap(void);

#endif // GUARD_ISAAC_DUNGEON_H
