@ Salle du donjon : redessinée par isaac_dungeon.c à chaque chargement de la map
IsaacRoom_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, IsaacRoom_OnLoad
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, IsaacRoom_OnWarp
	.byte 0

IsaacRoom_OnLoad:
	special IsaacPaintRoom
	end

IsaacRoom_OnWarp:
	map_script_2 VAR_TEMP_0, 0, IsaacRoom_EventScript_FaceArrival
	.2byte 0

IsaacRoom_EventScript_FaceArrival:
	special IsaacFaceArrivalDirection
	end

@ Entrée du hub, portes et échelle des salles (déclenchées par isaac_dungeon.c)
Isaac_EventScript_Exit::
	lockall
	special IsaacTakeExit
	waitstate
	releaseall
	end

@ Bouton R dans une salle : mini-carte de l'étage, fermée par A, B ou R
Isaac_EventScript_Minimap::
	lockall
	playse SE_WIN_OPEN
	special IsaacShowMinimap
	waitstate
	releaseall
	end

@ Dresseur d'une salle normale (placé par isaac_dungeon.c) : il regarde la porte par laquelle on arrive,
@ s'approche et lance le combat ; vaincu, il retourne au centre de la salle
IsaacRoom_EventScript_Trainer::
	trainerbattle_single TRAINER_ISAAC_ROOM, IsaacRoom_Text_TrainerIntro, IsaacRoom_Text_TrainerDefeat, IsaacRoom_EventScript_TrainerDefeated
	msgbox IsaacRoom_Text_TrainerPostBattle, MSGBOX_AUTOCLOSE
	end

IsaacRoom_EventScript_TrainerDefeated::
	special IsaacSetRoomCleared
	special IsaacPrepareTrainerReturn
	applymovement LOCALID_ISAAC_ROOM_TRAINER, gIsaacTrainerReturnMovement
	waitmovement 0
	release
	end

IsaacRoom_Text_TrainerIntro:
	.string "Personne ne traverse cette salle\n"
	.string "sans combattre !$"

IsaacRoom_Text_TrainerDefeat:
	.string "Bien joué…$"

IsaacRoom_Text_TrainerPostBattle:
	.string "La salle est à toi.\n"
	.string "Continue ta descente !$"
