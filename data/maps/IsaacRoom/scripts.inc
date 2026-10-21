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

@ Entrée du hub, portes des salles et passage ouvert par le boss (déclenchées par isaac_dungeon.c)
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

@ Salle de capture : des Pokémon sauvages du biome (placés par isaac_dungeon.c, CAPTURE_MAX_MONS au plus). Après
@ le combat (capturé, vaincu ou fui), le Pokémon disparaît.
IsaacRoom_EventScript_WildMon1::
	setvar VAR_0x8004, 0
	goto IsaacRoom_EventScript_WildMon
IsaacRoom_EventScript_WildMon2::
	setvar VAR_0x8004, 1
	goto IsaacRoom_EventScript_WildMon
IsaacRoom_EventScript_WildMon3::
	setvar VAR_0x8004, 2
	goto IsaacRoom_EventScript_WildMon
IsaacRoom_EventScript_WildMon4::
	setvar VAR_0x8004, 3
	goto IsaacRoom_EventScript_WildMon

IsaacRoom_EventScript_WildMon:
	lock
	faceplayer
	special IsaacPrepareWildBattle
	dowildbattle
	special IsaacCaptureMonDone
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
@ un passage vers l'étage suivant s'ouvre dans le mur d'en face de la porte.
IsaacRoom_EventScript_Boss::
	special IsaacBufferBossName
	trainerbattle_single TRAINER_ISAAC_BOSS, IsaacRoom_Text_BossIntro, IsaacRoom_Text_BossDefeat, IsaacRoom_EventScript_BossDefeated
	msgbox IsaacRoom_Text_BossPostBattle, MSGBOX_AUTOCLOSE
	end

IsaacRoom_EventScript_BossDefeated::
	playse SE_M_STRENGTH
	special IsaacOpenBossExit
	waitse
	msgbox IsaacRoom_Text_BossReward, MSGBOX_DEFAULT
	special IsaacPickBossReward
	giveitem VAR_0x8004
	msgbox IsaacRoom_Text_BossPostBattle, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_BossIntro:
	.string "Je suis {STR_VAR_1}, et je garde\n"
	.string "cet étage.\p"
	.string "Personne ne descend plus bas\n"
	.string "sans me battre !$"

IsaacRoom_Text_BossDefeat:
	.string "Ta force est impressionnante…$"

IsaacRoom_Text_BossReward:
	.string "Tu as mérité ceci.$"

IsaacRoom_Text_BossPostBattle:
	.string "Le passage vers l'étage suivant\n"
	.string "est ouvert. Descends !$"
