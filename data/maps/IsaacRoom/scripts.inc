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

@ Infirmière (arrivée par le passage une fois le boss vaincu) : soin partiel payant (voir IsaacNurseTryHeal)
IsaacRoom_EventScript_Nurse::
	lock
	faceplayer
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

IsaacRoom_Text_NurseOffer:
	.string "Bravo pour ta victoire !\p"
	.string "Pour {STR_VAR_2}¥, je soigne ton équipe :\n"
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

@ Contenus des salles vides (voir GetRoomContent, ROOM_CONTENT_* dans include/constants/isaac_dungeon.h)
IsaacRoom_EventScript_Content::
	lock
	faceplayer
	special IsaacGetRoomContent
	goto_if_eq VAR_RESULT, ROOM_CONTENT_EGG, IsaacRoom_EventScript_EggGiver
	goto_if_eq VAR_RESULT, ROOM_CONTENT_TRADER, IsaacRoom_EventScript_Trader
	goto_if_eq VAR_RESULT, ROOM_CONTENT_DEVIL, IsaacRoom_EventScript_Devil
	goto_if_eq VAR_RESULT, ROOM_CONTENT_TUTOR, IsaacRoom_EventScript_Tutor
	release
	end

@ Œuf confié (étage 1 ou 2)
IsaacRoom_EventScript_EggGiver::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_EggGiverDone
	msgbox IsaacRoom_Text_EggOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_EggGiverDeclined
	special IsaacGiveEgg
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_EggGiverPartyFull
	playfanfare MUS_OBTAIN_ITEM
	message IsaacRoom_Text_EggReceived
	waitfanfare
	waitmessage
	special IsaacSetRoomCleared
	msgbox IsaacRoom_Text_EggAdvice, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_EggGiverDone::
	msgbox IsaacRoom_Text_EggAdvice, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_EggGiverDeclined::
	msgbox IsaacRoom_Text_EggDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_EggGiverPartyFull::
	msgbox IsaacRoom_Text_EggPartyFull, MSGBOX_DEFAULT
	release
	end

@ Échangeur : un Pokémon contre un autre de même catégorie et même palier de set, de n'importe quel biome
IsaacRoom_EventScript_Trader::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_TraderDone
	msgbox IsaacRoom_Text_TraderIntro, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_TraderDeclined
	special IsaacPrepareTrade
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_TraderEgg
	msgbox IsaacRoom_Text_TraderOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_TraderDeclined
	special IsaacDoTrade
	playfanfare MUS_OBTAIN_ITEM
	message IsaacRoom_Text_TraderTraded
	waitfanfare
	waitmessage
	special IsaacSetRoomCleared
	msgbox IsaacRoom_Text_TraderDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_TraderDone::
	msgbox IsaacRoom_Text_TraderDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_TraderDeclined::
	msgbox IsaacRoom_Text_TraderDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_TraderEgg::
	msgbox IsaacRoom_Text_TraderEgg, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_EggOffer:
	.string "Oh, un dresseur ! J'ai trouvé cet Œuf\n"
	.string "au fond du donjon…\p"
	.string "Je ne peux pas m'en occuper ici.\n"
	.string "Tu veux bien le prendre ?$"

IsaacRoom_Text_EggReceived:
	.string "{PLAYER} reçoit un Œuf !$"

IsaacRoom_Text_EggAdvice:
	.string "Garde-le dans ton équipe : il devrait\n"
	.string "éclore d'ici deux étages environ.\p"
	.string "Ça m'a l'air d'être un Pokémon très\n"
	.string "puissant !$"

IsaacRoom_Text_EggDeclined:
	.string "Dommage… Repasse si tu changes d'avis.$"

IsaacRoom_Text_EggPartyFull:
	.string "Ton équipe est pleine… Reviens avec\n"
	.string "une place libre !$"

IsaacRoom_Text_TraderIntro:
	.string "Je collectionne les Pokémon de tous\n"
	.string "les biomes.\p"
	.string "Je t'échange un des tiens contre un\n"
	.string "des miens, d'une force équivalente.\p"
	.string "Lequel me proposes-tu ?$"

IsaacRoom_Text_TraderOffer:
	.string "Ton {STR_VAR_1} contre mon\n"
	.string "{STR_VAR_2} ? Marché conclu ?$"

IsaacRoom_Text_TraderTraded:
	.string "{PLAYER} échange {STR_VAR_1}\n"
	.string "contre {STR_VAR_2} !$"

IsaacRoom_Text_TraderDone:
	.string "Merci pour l'échange !\n"
	.string "Prends-en bien soin.$"

IsaacRoom_Text_TraderDeclined:
	.string "Comme tu veux. Ma proposition tient\n"
	.string "toujours.$"

IsaacRoom_Text_TraderEgg:
	.string "Un Œuf ? Non merci, je ne prends\n"
	.string "que des Pokémon éclos.$"

@ Pacte du diable : un objet rare contre le sacrifice définitif d'un Pokémon
IsaacRoom_EventScript_Devil::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_DevilDone
	special IsaacPrepareDevilPact
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_DevilTooFew
	msgbox IsaacRoom_Text_DevilOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_DevilDeclined
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_DevilDeclined
	special IsaacCheckDevilSacrifice
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_DevilEgg
	msgbox IsaacRoom_Text_DevilConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_DevilDeclined
	special IsaacDoDevilPact
	playse SE_M_NIGHTMARE
	waitse
	msgbox IsaacRoom_Text_DevilSealed, MSGBOX_DEFAULT
	giveitem VAR_0x8006
	special IsaacSetRoomCleared
	release
	end

IsaacRoom_EventScript_DevilDone::
	msgbox IsaacRoom_Text_DevilDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_DevilTooFew::
	msgbox IsaacRoom_Text_DevilTooFew, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_DevilDeclined::
	msgbox IsaacRoom_Text_DevilDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_DevilEgg::
	msgbox IsaacRoom_Text_DevilEgg, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_DevilOffer:
	.string "Héhéhé… Tu veux du pouvoir ?\p"
	.string "Cet objet est à toi :\n"
	.string "{STR_VAR_2}.\p"
	.string "En échange, l'un de tes Pokémon\n"
	.string "restera ici… pour toujours.\p"
	.string "Marché conclu ?$"

IsaacRoom_Text_DevilConfirm:
	.string "Tu abandonnes {STR_VAR_1} ?\n"
	.string "Il n'y aura pas de retour…$"

IsaacRoom_Text_DevilSealed:
	.string "{STR_VAR_1} disparaît dans l'ombre…\p"
	.string "Le pacte est scellé. Héhéhé…$"

IsaacRoom_Text_DevilDone:
	.string "Le pacte est scellé.\n"
	.string "Je n'ai plus rien à t'offrir…$"

IsaacRoom_Text_DevilTooFew:
	.string "Héhéhé… Tu n'as qu'un seul Pokémon.\n"
	.string "Reviens quand tu auras de quoi payer.$"

IsaacRoom_Text_DevilDeclined:
	.string "Héhéhé… L'offre tient toujours.$"

IsaacRoom_Text_DevilEgg:
	.string "Un Œuf ? Ça ne vaut rien pour moi.$"

@ Maître des capacités : 3 attaques fortes au choix pour le Pokémon choisi
IsaacRoom_EventScript_Tutor::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_TutorDone
	msgbox IsaacRoom_Text_TutorIntro, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_TutorDeclined
	special IsaacPrepareTutor
	goto_if_eq VAR_RESULT, 0, IsaacRoom_EventScript_TutorNothing
	message IsaacRoom_Text_TutorWhich
	waitmessage
	dynmultipush IsaacRoom_Text_StrVar1, 0
	goto_if_lt VAR_RESULT, 2, IsaacRoom_EventScript_TutorMenu
	dynmultipush IsaacRoom_Text_StrVar2, 1
	goto_if_lt VAR_RESULT, 3, IsaacRoom_EventScript_TutorMenu
	dynmultipush IsaacRoom_Text_StrVar3, 2
IsaacRoom_EventScript_TutorMenu::
	dynmultistack 0, 0, FALSE, 3, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	closemessage
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, IsaacRoom_EventScript_TutorDeclined
	copyvar VAR_0x8006, VAR_RESULT
	special IsaacTutorChooseMove
	goto_if_eq VAR_RESULT, TRUE, IsaacRoom_EventScript_TutorLearned
	msgbox IsaacRoom_Text_TutorForget, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	special IsaacTutorChooseMoveToForget
	fadescreen FADE_FROM_BLACK
	goto_if_eq VAR_0x8005, MAX_MON_MOVES, IsaacRoom_EventScript_TutorDeclined
	special IsaacTutorReplaceMove
IsaacRoom_EventScript_TutorLearned::
	playfanfare MUS_LEVEL_UP
	message IsaacRoom_Text_TutorLearned
	waitfanfare
	waitmessage
	special IsaacSetRoomCleared
	msgbox IsaacRoom_Text_TutorDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_TutorDone::
	msgbox IsaacRoom_Text_TutorDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_TutorDeclined::
	msgbox IsaacRoom_Text_TutorDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_TutorNothing::
	msgbox IsaacRoom_Text_TutorNothing, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_StrVar1:
	.string "{STR_VAR_1}$"

IsaacRoom_Text_StrVar2:
	.string "{STR_VAR_2}$"

IsaacRoom_Text_StrVar3:
	.string "{STR_VAR_3}$"

IsaacRoom_Text_TutorIntro:
	.string "Je suis maître des capacités.\n"
	.string "J'enseigne une seule attaque par étage.\p"
	.string "Quel Pokémon veux-tu entraîner ?$"

IsaacRoom_Text_TutorWhich:
	.string "Laquelle de ces attaques ?$"

IsaacRoom_Text_TutorForget:
	.string "{STR_VAR_1} connaît déjà 4 attaques.\n"
	.string "Laquelle doit-il oublier pour\l"
	.string "{STR_VAR_2} ?$"

IsaacRoom_Text_TutorLearned:
	.string "{STR_VAR_1} apprend {STR_VAR_2} !$"

IsaacRoom_Text_TutorDone:
	.string "Bien joué. Utilise-la avec sagesse.$"

IsaacRoom_Text_TutorDeclined:
	.string "Reviens quand tu veux.$"

IsaacRoom_Text_TutorNothing:
	.string "Je n'ai rien à lui apprendre…$"

