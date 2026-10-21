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
	special IsaacPrepareNurseArrival
	addobject LOCALID_ISAAC_ROOM_NURSE
	applymovement LOCALID_ISAAC_ROOM_NURSE, gIsaacNurseArrivalMovement
	waitmovement 0
	special IsaacSettleNurse
	msgbox IsaacRoom_Text_BossPostBattle, MSGBOX_DEFAULT
	release
	end

@ Infirmière (arrivée par le passage une fois le boss vaincu) : PP rendus gratuitement, puis soin partiel payant
@ (voir IsaacNurseTryHeal)
IsaacRoom_EventScript_Nurse::
	lock
	faceplayer
	special IsaacNurseRestorePP
	msgbox IsaacRoom_Text_NursePP, MSGBOX_DEFAULT
	special IsaacNurseGetPrice
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_NurseNoNeed
	showmoneybox 0, 0
	msgbox IsaacRoom_Text_NurseOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_NurseDeclined
	special IsaacNurseTryHeal
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_NurseNoMoney
	updatemoneybox
	playse SE_SHOP
	msgbox IsaacRoom_Text_NurseHealing, MSGBOX_DEFAULT
	playfanfare MUS_HEAL
	waitfanfare
	hidemoneybox
	msgbox IsaacRoom_Text_NurseHealed, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_NurseNoNeed::
	msgbox IsaacRoom_Text_NurseNoNeed, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_NurseDeclined::
	hidemoneybox
	msgbox IsaacRoom_Text_NurseDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_NurseNoMoney::
	msgbox IsaacRoom_Text_NurseNoMoney, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

IsaacRoom_Text_NursePP:
	.string "Bravo pour ta victoire !\n"
	.string "Je t'ai rendu tous tes PP, c'est\l"
	.string "offert.$"

IsaacRoom_Text_NurseOffer:
	.string "Pour {STR_VAR_1}¥, je soigne ton équipe :\n"
	.string "PV au complet pour ceux qui tiennent\l"
	.string "debout, moitié des PV pour les K.O.\p"
	.string "Ça te va ?$"

IsaacRoom_Text_NurseHealing:
	.string "Je m'occupe de tes Pokémon.$"

IsaacRoom_Text_NurseHealed:
	.string "Voilà ! Bonne chance pour la suite.$"

IsaacRoom_Text_NurseNoNeed:
	.string "Ton équipe est en pleine forme.\n"
	.string "Bonne chance pour la suite !$"

IsaacRoom_Text_NurseDeclined:
	.string "Comme tu veux. Reviens me voir\n"
	.string "avant de descendre si tu changes d'avis.$"

IsaacRoom_Text_NurseNoMoney:
	.string "Désolée, tu n'as pas assez\n"
	.string "d'argent…$"

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

@ Boutique : Kecleon, qui parle comme un humain (blague la toute première fois). Menu Acheter / Quitter
@ (voir IsaacOpenShop)
IsaacRoom_EventScript_Shopkeeper::
	lock
	faceplayer
	special IsaacKecleonGreeting
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_ShopkeeperHello
	msgbox IsaacRoom_Text_KecleonFirstTime, MSGBOX_DEFAULT
	goto IsaacRoom_EventScript_ShopkeeperOpen

IsaacRoom_EventScript_ShopkeeperHello::
	msgbox IsaacRoom_Text_KecleonHello, MSGBOX_DEFAULT
IsaacRoom_EventScript_ShopkeeperOpen::
	special IsaacOpenShop
	waitstate
	msgbox IsaacRoom_Text_KecleonBye, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_KecleonFirstTime:
	.string "Bienvenue, bienvenue !\p"
	.string "… Pourquoi cette tête ? Tu n'as\n"
	.string "jamais vu un Kecleon qui parle ?\p"
	.string "À force de me camoufler derrière\n"
	.string "les comptoirs des humains, j'ai\l"
	.string "appris leur langue…\p"
	.string "… et surtout leurs prix ! Kehehe !$"

IsaacRoom_Text_KecleonHello:
	.string "Bienvenue ! Regarde un peu\n"
	.string "ce que j'ai aujourd'hui !$"

IsaacRoom_Text_KecleonBye:
	.string "Reviens quand tu veux !\n"
	.string "Enfin, si tu me trouves… Kehehe !$"

