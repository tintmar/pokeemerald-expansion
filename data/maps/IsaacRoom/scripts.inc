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
@ s'approche et lance le combat ; vaincu, il donne un objet de son butin et retourne au centre de la salle
IsaacRoom_EventScript_Trainer::
	trainerbattle_single TRAINER_ISAAC_ROOM, IsaacRoom_Text_TrainerIntro, IsaacRoom_Text_TrainerDefeat, IsaacRoom_EventScript_TrainerDefeated
	msgbox IsaacRoom_Text_TrainerPostBattle, MSGBOX_AUTOCLOSE
	end

IsaacRoom_EventScript_TrainerDefeated::
	special IsaacSetRoomCleared
	special IsaacPickTrainerLoot
	giveitem VAR_0x8004
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

@ Salle de capture : un Pokémon sauvage du biome (placé par isaac_dungeon.c). Après le combat (capturé, vaincu
@ ou fui), il disparaît.
IsaacRoom_EventScript_WildMon::
	lock
	faceplayer
	special IsaacPrepareWildBattle
	dowildbattle
	special IsaacSetRoomCleared
	removeobject LOCALID_ISAAC_ROOM_WILD
	release
	end

@ Salle trésor : pour l'instant (tests), les trois objets spéciaux
IsaacRoom_EventScript_Treasure::
	lock
	giveitem ITEM_EVOLUTION_STONE
	giveitem ITEM_OMNIBOOST
	giveitem ITEM_ABILITY_TRAINER
	special IsaacSetRoomCleared
	removeobject LOCALID_ISAAC_ROOM_ITEM
	release
	end

@ Salle du boss : le champion du biome (placé par isaac_dungeon.c). Il faut lui parler pour combattre ; vaincu,
@ l'échelle de l'étage suivant apparaît au centre de la salle.
IsaacRoom_EventScript_Boss::
	trainerbattle_single TRAINER_ISAAC_BOSS, IsaacRoom_Text_BossIntro, IsaacRoom_Text_BossDefeat, IsaacRoom_EventScript_BossDefeated
	msgbox IsaacRoom_Text_BossPostBattle, MSGBOX_AUTOCLOSE
	end

IsaacRoom_EventScript_BossDefeated::
	playse SE_M_STRENGTH
	special IsaacRevealLadder
	waitse
	msgbox IsaacRoom_Text_BossPostBattle, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_BossIntro:
	.string "Je suis PIERRE, le gardien\n"
	.string "de cet étage.\p"
	.string "Ma défense de roc est solide.\n"
	.string "Voyons si tu peux la briser !$"

IsaacRoom_Text_BossDefeat:
	.string "Ta force est impressionnante…$"

IsaacRoom_Text_BossPostBattle:
	.string "Le passage vers l'étage suivant\n"
	.string "est ouvert. Descends !$"
