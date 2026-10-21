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
