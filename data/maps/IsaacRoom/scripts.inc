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

@ Dresseur d'une salle (placé par isaac_dungeon.c, comportement : voir GetRoomTrainerBehavior) : quand il voit le
@ joueur, il s'approche et lance le combat ; s'il est facultatif (IsaacRoom_EventScript_OptionalTrainer, choisi par
@ SetUpRoomTrainer), il faut lui parler et accepter. Vaincu, il donne un objet de son butin et retourne au centre de la
@ salle. Dans une salle secrète, la récompense de la salle apparaît en plus au sol.
@ Le script doit commencer par trainerbattle : la vue des dresseurs y lit le dresseur et s'il est déjà vaincu.
IsaacRoom_EventScript_Trainer::
	trainerbattle_single TRAINER_ISAAC_ROOM, IsaacRoom_Text_TrainerIntro, IsaacRoom_Text_TrainerDefeat, IsaacRoom_EventScript_TrainerDefeated
	msgbox IsaacRoom_Text_TrainerPostBattle, MSGBOX_AUTOCLOSE
	end

IsaacRoom_EventScript_OptionalTrainer::
	goto_if_defeated TRAINER_ISAAC_ROOM, IsaacRoom_EventScript_Trainer
	lock
	faceplayer
	msgbox IsaacRoom_Text_TrainerChallenge, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_TrainerDeclined
	trainerbattle_single TRAINER_ISAAC_ROOM, IsaacRoom_Text_TrainerAccepted, IsaacRoom_Text_TrainerDefeat, IsaacRoom_EventScript_TrainerDefeated
	release
	end

IsaacRoom_EventScript_TrainerDeclined::
	msgbox IsaacRoom_Text_TrainerDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_TrainerDefeated::
	special IsaacSetRoomCleared
	special IsaacPickTrainerLoot
IsaacRoom_EventScript_TrainerLoot::
	special IsaacNextLootItem
	goto_if_eq VAR_0x8004, ITEM_NONE, IsaacRoom_EventScript_TrainerReturn
	giveitem VAR_0x8004
	goto IsaacRoom_EventScript_TrainerLoot
IsaacRoom_EventScript_TrainerReturn::
	special IsaacCheckSecretReward
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_TrainerWalkBack
	addobject LOCALID_ISAAC_ROOM_ITEM
IsaacRoom_EventScript_TrainerWalkBack::
	special IsaacPrepareTrainerReturn
	applymovement LOCALID_ISAAC_ROOM_TRAINER, gIsaacTrainerReturnMovement
	waitmovement 0
	release
	end

IsaacRoom_Text_TrainerIntro:
	.string "No one crosses this room\n"
	.string "without a battle!$"

IsaacRoom_Text_TrainerDefeat:
	.string "Well played…$"

IsaacRoom_Text_TrainerChallenge:
	.string "Want to take me on?\n"
	.string "No hard feelings if you pass…$"

IsaacRoom_Text_TrainerAccepted:
	.string "Perfect! En garde!$"

IsaacRoom_Text_TrainerDeclined:
	.string "Too bad. Come back anytime.$"

IsaacRoom_Text_TrainerPostBattle:
	.string "The room is yours.\n"
	.string "Keep heading down!$"

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
IsaacRoom_EventScript_WildMon5::
	setvar VAR_0x8004, 4
	goto IsaacRoom_EventScript_WildMon

IsaacRoom_EventScript_WildMon:
	lock
	faceplayer
	special IsaacPrepareWildBattle
	dowildbattle
	special IsaacCaptureMonDone
	release
	end

@ Salle de capture : le personnage de l'entrée donne une Master Ball par étage (voir PlaceCaptureGuide)
IsaacRoom_EventScript_CaptureGuide::
	lock
	faceplayer
	special IsaacCaptureGuideHasBall
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_CaptureGuideDone
	msgbox IsaacRoom_Text_CaptureGuideGive, MSGBOX_DEFAULT
	giveitem ITEM_MASTER_BALL
	goto_if_eq VAR_0x8007, FALSE, IsaacRoom_EventScript_CaptureGuideBagFull  @ sac plein (message de giveitem)
	special IsaacCaptureGuideGaveBall
	msgbox IsaacRoom_Text_CaptureGuideAfter, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_CaptureGuideBagFull::
	release
	end

IsaacRoom_EventScript_CaptureGuideDone::
	msgbox IsaacRoom_Text_CaptureGuideAfter, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_CaptureGuideGive:
	.string "Wild Pokémon live in this room!\p"
	.string "Here, take this. It never fails,\n"
	.string "so save it for the one you want most!$"

IsaacRoom_Text_CaptureGuideAfter:
	.string "Choose wisely! A Master Ball\n"
	.string "catches any Pokémon, guaranteed.$"

@ Salles trésor et secrète : un objet spécial ou, dans une salle secrète, le butin de sa table (voir
@ IsaacPickTreasureItem), donné objet par objet
IsaacRoom_EventScript_Treasure::
	lock
	special IsaacPickTreasureItem
IsaacRoom_EventScript_TreasureLoot::
	special IsaacNextLootItem
	goto_if_eq VAR_0x8004, ITEM_NONE, IsaacRoom_EventScript_TreasureTaken
	giveitem VAR_0x8004
	goto IsaacRoom_EventScript_TreasureLoot
IsaacRoom_EventScript_TreasureTaken::
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
	special IsaacPickBossReward
	goto_if_eq VAR_0x8004, ITEM_NONE, IsaacRoom_EventScript_BossNurse
	msgbox IsaacRoom_Text_BossReward, MSGBOX_DEFAULT
	giveitem VAR_0x8004
IsaacRoom_EventScript_BossNurse::
	special IsaacPrepareNurseArrival
	addobject LOCALID_ISAAC_ROOM_NURSE
	applymovement LOCALID_ISAAC_ROOM_NURSE, gIsaacNurseArrivalMovement
	waitmovement 0
	special IsaacSettleNurse
	msgbox IsaacRoom_Text_BossPostBattle, MSGBOX_DEFAULT
	release
	end

@ Infirmière (arrivée par le passage une fois le boss vaincu) : soin payant, par Pokémon (voir IsaacNurseTryHeal)
IsaacRoom_EventScript_Nurse::
	lock
	faceplayer
	special IsaacNurseGetPrice
	goto_if_eq VAR_RESULT, 0, IsaacRoom_EventScript_NurseNoNeed
	showmoneybox 0, 0
	msgbox IsaacRoom_Text_NurseOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_NurseDeclined
	special IsaacNurseTryHeal
	goto_if_eq VAR_RESULT, 0, IsaacRoom_EventScript_NurseNoMoney
	updatemoneybox
	playse SE_SHOP
	msgbox IsaacRoom_Text_NurseHealing, MSGBOX_DEFAULT
	playfanfare MUS_HEAL
	waitfanfare
	hidemoneybox
	goto_if_lt VAR_RESULT, VAR_0x8006, IsaacRoom_EventScript_NursePartial
	msgbox IsaacRoom_Text_NurseHealed, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_NursePartial::
	msgbox IsaacRoom_Text_NursePartial, MSGBOX_DEFAULT
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
	.string "Congratulations on your win!\p"
	.string "I can heal your Pokémon, even the\n"
	.string "fainted ones: full HP.\p"
	.string "{STR_VAR_3} Pokémon to heal: ¥{STR_VAR_2}.\n"
	.string "Is that all right?$"

IsaacRoom_Text_NurseHealing:
	.string "I'll take care of your Pokémon.$"

IsaacRoom_Text_NurseHealed:
	.string "There you go! Good luck ahead.$"

IsaacRoom_Text_NurseNoNeed:
	.string "Your team is in great shape.\n"
	.string "Good luck ahead!$"

IsaacRoom_Text_NurseDeclined:
	.string "As you wish. Come see me before\n"
	.string "heading down if you change your mind.$"

IsaacRoom_Text_NursePartial:
	.string "You didn't have enough money\n"
	.string "for everyone…\p"
	.string "I could heal {STR_VAR_3} of them, in\n"
	.string "party order. Good luck!$"

IsaacRoom_Text_NurseNoMoney:
	.string "Sorry, you don't have\n"
	.string "enough money…$"

IsaacRoom_Text_BossIntro:
	.string "I am {STR_VAR_1}, and I guard\n"
	.string "this floor.\p"
	.string "No one goes any deeper\n"
	.string "without beating me!$"

IsaacRoom_Text_BossDefeat:
	.string "Your strength is impressive…$"

IsaacRoom_Text_BossReward:
	.string "You've earned this.$"

IsaacRoom_Text_BossPostBattle:
	.string "The way to the next floor\n"
	.string "is open. Go on down!$"

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
	.string "Welcome, welcome!\p"
	.string "… Why that face? Never seen\n"
	.string "a talking Kecleon before?\p"
	.string "Hiding behind human shop\n"
	.string "counters for so long, I\l"
	.string "picked up their language…\p"
	.string "… and above all, their prices!\n"
	.string "Kehehe!$"

IsaacRoom_Text_KecleonHello:
	.string "Welcome! Take a look at\n"
	.string "what I've got today!$"

IsaacRoom_Text_KecleonBye:
	.string "Come back anytime!\n"
	.string "If you can find me… Kehehe!$"

@ Contenus des salles vides (voir GetRoomContent, ROOM_CONTENT_* dans include/constants/isaac_dungeon.h)
IsaacRoom_EventScript_Content::
	lock
	faceplayer
	special IsaacGetRoomContent
	goto_if_eq VAR_RESULT, ROOM_CONTENT_EGG, IsaacRoom_EventScript_EggGiver
	goto_if_eq VAR_RESULT, ROOM_CONTENT_TRADER, IsaacRoom_EventScript_Trader
	goto_if_eq VAR_RESULT, ROOM_CONTENT_DEVIL, IsaacRoom_EventScript_Devil
	goto_if_eq VAR_RESULT, ROOM_CONTENT_TUTOR, IsaacRoom_EventScript_Tutor
	goto_if_eq VAR_RESULT, ROOM_CONTENT_DUELIST, IsaacRoom_EventScript_Duelist
	goto_if_eq VAR_RESULT, ROOM_CONTENT_SEER, IsaacRoom_EventScript_Seer
	goto_if_eq VAR_RESULT, ROOM_CONTENT_ROCK_SMASH, IsaacRoom_EventScript_RockSmashTeacher
	goto_if_eq VAR_RESULT, ROOM_CONTENT_HEALER, IsaacRoom_EventScript_Healer
	goto_if_eq VAR_RESULT, ROOM_CONTENT_EV_TRAINER, IsaacRoom_EventScript_EvTrainer
	release
	end

@ Œuf confié (étage 1 ou 2)
IsaacRoom_EventScript_EggGiver::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_EggGiverDone
	msgbox IsaacRoom_Text_EggOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_EggGiverDeclined
	special IsaacGiveEgg
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_EggGiverPartyFull
IsaacRoom_EventScript_EggGiverReceived::
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

@ Équipe pleine (pas de PC) : relâcher un Pokémon pour faire de la place
IsaacRoom_EventScript_EggGiverPartyFull::
	msgbox IsaacRoom_Text_EggPartyFull, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_EggGiverDeclined
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_EggGiverDeclined
	special IsaacReleasePartyMon
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_EggGiverLastMon
	msgbox IsaacRoom_Text_MonReleased, MSGBOX_DEFAULT
	special IsaacGiveEgg
	goto IsaacRoom_EventScript_EggGiverReceived

IsaacRoom_EventScript_EggGiverLastMon::
	msgbox IsaacRoom_Text_CantReleaseLastMon, MSGBOX_DEFAULT
	release
	end

@ Échangeur : un Pokémon contre un autre de n'importe quel biome, de la force de ceux de la salle de capture
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
	.string "Oh, a Trainer! I found this Egg\n"
	.string "deep in the dungeon…\p"
	.string "I can't look after it down here.\n"
	.string "Would you take it?$"

IsaacRoom_Text_EggReceived:
	.string "{PLAYER} received an Egg!$"

IsaacRoom_Text_EggAdvice:
	.string "Keep it in your party: it should\n"
	.string "hatch in about two floors.\p"
	.string "It looks like a very powerful\n"
	.string "Pokémon!$"

IsaacRoom_Text_EggDeclined:
	.string "Too bad… Come back if you\n"
	.string "change your mind.$"

IsaacRoom_Text_EggPartyFull:
	.string "Your party is full…\n"
	.string "Release a Pokémon to take the Egg?$"

IsaacRoom_Text_MonReleased:
	.string "{STR_VAR_1} was released.\n"
	.string "Bye-bye, {STR_VAR_1}!$"

IsaacRoom_Text_CantReleaseLastMon:
	.string "You can't release your last\n"
	.string "Pokémon!$"

IsaacRoom_Text_TraderIntro:
	.string "I collect Pokémon from every\n"
	.string "biome.\p"
	.string "I'll trade one of yours for one\n"
	.string "of mine, as strong as the ones in\l"
	.string "this floor's capture rooms.\p"
	.string "Which one will you offer?$"

IsaacRoom_Text_TraderOffer:
	.string "Your {STR_VAR_1} for one of my\n"
	.string "Pokémon? Do we have a deal?$"

IsaacRoom_Text_TraderTraded:
	.string "{PLAYER} traded {STR_VAR_1}\n"
	.string "for {STR_VAR_2}!$"

IsaacRoom_Text_TraderDone:
	.string "Thanks for the trade!\n"
	.string "Take good care of it.$"

IsaacRoom_Text_TraderDeclined:
	.string "As you wish. My offer still\n"
	.string "stands.$"

IsaacRoom_Text_TraderEgg:
	.string "An Egg? No thanks, I only take\n"
	.string "hatched Pokémon.$"

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
	.string "Heheheh… You want power?\p"
	.string "This item is yours:\n"
	.string "{STR_VAR_2}.\p"
	.string "In exchange, one of your Pokémon\n"
	.string "stays here… forever.\p"
	.string "Do we have a deal?$"

IsaacRoom_Text_DevilConfirm:
	.string "You're giving up {STR_VAR_1}?\n"
	.string "There's no going back…$"

IsaacRoom_Text_DevilSealed:
	.string "{STR_VAR_1} vanishes into the dark…\p"
	.string "The pact is sealed. Heheheh…$"

IsaacRoom_Text_DevilDone:
	.string "The pact is sealed.\n"
	.string "I have nothing more to offer…$"

IsaacRoom_Text_DevilTooFew:
	.string "Heheheh… You only have one Pokémon.\n"
	.string "Come back when you can pay.$"

IsaacRoom_Text_DevilDeclined:
	.string "Heheheh… The offer still stands.$"

IsaacRoom_Text_DevilEgg:
	.string "An Egg? That's worthless to me.$"

@ Maître des capacités : 5 attaques fortes au choix pour le Pokémon choisi
IsaacRoom_EventScript_Tutor::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_TutorDone
	msgbox IsaacRoom_Text_TutorIntro, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_TutorDeclined
	special IsaacPrepareTutor
	goto_if_eq VAR_RESULT, 0, IsaacRoom_EventScript_TutorNothing
	copyvar VAR_0x8007, VAR_RESULT
	message IsaacRoom_Text_TutorWhich
	waitmessage
	setvar VAR_0x8006, 0
IsaacRoom_EventScript_TutorPushChoice::
	special IsaacBufferTutorChoice
	dynmultipush IsaacRoom_Text_StrVar1, VAR_0x8006
	addvar VAR_0x8006, 1
	goto_if_lt VAR_0x8006, VAR_0x8007, IsaacRoom_EventScript_TutorPushChoice
	dynmultistack 0, 0, FALSE, 5, FALSE, 0, DYN_MULTICHOICE_CB_NONE
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

IsaacRoom_Text_TutorIntro:
	.string "I am a Move Master.\n"
	.string "I teach a single move per floor.\p"
	.string "Which Pokémon do you want to train?$"

IsaacRoom_Text_TutorWhich:
	.string "Which of these moves?$"

IsaacRoom_Text_TutorForget:
	.string "{STR_VAR_1} already knows 4 moves.\n"
	.string "Which one should it forget for\l"
	.string "{STR_VAR_2}?$"

IsaacRoom_Text_TutorLearned:
	.string "{STR_VAR_1} learned {STR_VAR_2}!$"

IsaacRoom_Text_TutorDone:
	.string "Well done. Use it wisely.$"

IsaacRoom_Text_TutorDeclined:
	.string "Come back anytime.$"

IsaacRoom_Text_TutorNothing:
	.string "I have nothing to teach it…$"

@ Duelliste : il annonce son Pokémon, puis combat en 1 contre 1 contre le Pokémon choisi (perdre ne renvoie pas au
@ hub). Un seul duel : gagné, une Restore Max et une Super CT ; perdu, une Restore Max pour relever le Pokémon K.O.
IsaacRoom_EventScript_Duelist::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_DuelistDone
	special IsaacPrepareDuel
	msgbox IsaacRoom_Text_DuelistOffer, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_DuelistDeclined
	msgbox IsaacRoom_Text_DuelistChoose, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_DuelistDeclined
	special IsaacDuelCheckMon
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_DuelistCantFight
	special IsaacDuelSetUpParty
	trainerbattle_single TRAINER_ISAAC_DUELIST, IsaacRoom_Text_DuelistIntro, IsaacRoom_Text_DuelistDefeat, IsaacRoom_EventScript_DuelistAfter
	end

IsaacRoom_EventScript_DuelistAfter::
	copyvar VAR_0x8006, VAR_RESULT
	special IsaacDuelRestoreParty
	special IsaacSetRoomCleared
	goto_if_ne VAR_0x8006, B_OUTCOME_WON, IsaacRoom_EventScript_DuelistLost
	msgbox IsaacRoom_Text_DuelistWon, MSGBOX_DEFAULT
	giveitem ITEM_MAX_RESTORE
	giveitem ITEM_ABILITY_TRAINER
	release
	end

IsaacRoom_EventScript_DuelistLost::
	msgbox IsaacRoom_Text_DuelistLost, MSGBOX_DEFAULT
	giveitem ITEM_MAX_RESTORE
	release
	end

IsaacRoom_EventScript_DuelistDone::
	msgbox IsaacRoom_Text_DuelistDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_DuelistDeclined::
	msgbox IsaacRoom_Text_DuelistDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_DuelistCantFight::
	msgbox IsaacRoom_Text_DuelistCantFight, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_DuelistOffer:
	.string "A duel, one Pokémon each!\p"
	.string "My {STR_VAR_1} is waiting for an\n"
	.string "opponent worthy of it.\p"
	.string "Do you accept the challenge?$"

IsaacRoom_Text_DuelistChoose:
	.string "Choose your champion!$"

IsaacRoom_Text_DuelistIntro:
	.string "En garde!$"

IsaacRoom_Text_DuelistDefeat:
	.string "What a duel! You win.$"

IsaacRoom_Text_DuelistWon:
	.string "I keep my word: here is your\n"
	.string "reward.$"

IsaacRoom_Text_DuelistLost:
	.string "Ha! Nice try, but a duel\n"
	.string "can't be replayed.\p"
	.string "Here, to get your Pokémon\n"
	.string "back on its feet.$"

IsaacRoom_Text_DuelistDone:
	.string "A memorable duel. Safe travels!$"

IsaacRoom_Text_DuelistDeclined:
	.string "Too bad… My {STR_VAR_1} will wait.$"

IsaacRoom_Text_DuelistCantFight:
	.string "That Pokémon can't battle!$"

@ Voyant : ouvre le passage vers la salle secrète de l'étage
IsaacRoom_EventScript_Seer::
	special IsaacBufferBossTeam
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_SeerAgain
	msgbox IsaacRoom_Text_SeerVision, MSGBOX_DEFAULT
	special IsaacSetRoomCleared
	release
	end

IsaacRoom_EventScript_SeerAgain::
	msgbox IsaacRoom_Text_SeerAgain, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_SeerVision:
	.string "Hmm… I see {STR_VAR_1}, waiting\n"
	.string "at the end of this floor…\p"
	.string "Their team:\n"
	.string "{STR_VAR_2}.\p"
	.string "Their ace, saved for last:\n"
	.string "{STR_VAR_3}!$"

IsaacRoom_Text_SeerAgain:
	.string "Beware of {STR_VAR_1}'s ace:\n"
	.string "{STR_VAR_3}!$"

@ Entraîneur d'Éclate-Roc : apprend Éclate-Roc au Pokémon choisi
IsaacRoom_EventScript_RockSmashTeacher::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_RockSmashTeacherDone
	msgbox IsaacRoom_Text_RockSmashIntro, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_TutorDeclined
	special IsaacPrepareRockSmash
	goto_if_eq VAR_RESULT, 0, IsaacRoom_EventScript_RockSmashCant
	goto_if_eq VAR_RESULT, 1, IsaacRoom_EventScript_RockSmashLearned
	msgbox IsaacRoom_Text_TutorForget, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	special IsaacTutorChooseMoveToForget
	fadescreen FADE_FROM_BLACK
	goto_if_eq VAR_0x8005, MAX_MON_MOVES, IsaacRoom_EventScript_TutorDeclined
	special IsaacTutorReplaceMove
IsaacRoom_EventScript_RockSmashLearned::
	playfanfare MUS_LEVEL_UP
	message IsaacRoom_Text_TutorLearned
	waitfanfare
	waitmessage
	special IsaacSetRoomCleared
	msgbox IsaacRoom_Text_RockSmashHint, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_RockSmashTeacherDone::
	msgbox IsaacRoom_Text_RockSmashHint, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_RockSmashCant::
	msgbox IsaacRoom_Text_RockSmashCant, MSGBOX_DEFAULT
	release
	end

@ Roche qui ferme la salle secrète, avec un Pokémon qui connaît Éclate-Roc (voir IsaacGetWallInteractScript)
IsaacRoom_EventScript_SecretWall::
	lockall
	checkfieldmove FIELD_MOVE_ROCK_SMASH
	bufferpartymonnick STR_VAR_1, VAR_RESULT
	msgbox IsaacRoom_Text_SecretWallAsk, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, IsaacRoom_EventScript_SecretWallEnd
	msgbox IsaacRoom_Text_SecretWallSmash, MSGBOX_DEFAULT
	playse SE_M_ROCK_THROW
	special IsaacOpenSecretRoom
	waitse
IsaacRoom_EventScript_SecretWallEnd::
	releaseall
	end

IsaacRoom_Text_RockSmashIntro:
	.string "Some rocks in this dungeon sound\n"
	.string "hollow… Hidden rooms lie behind.\p"
	.string "I can teach Rock Smash to one\n"
	.string "of your Pokémon. Which one?$"

IsaacRoom_Text_RockSmashHint:
	.string "Check the rocks blocking the rooms:\n"
	.string "if one sounds hollow, smash it!$"

IsaacRoom_Text_RockSmashCant:
	.string "I have nothing to teach it…$"

IsaacRoom_Text_SecretWallAsk:
	.string "This rock sounds hollow…\n"
	.string "Should {STR_VAR_1} use Rock Smash?$"

IsaacRoom_Text_SecretWallSmash:
	.string "{STR_VAR_1} used Rock Smash!$"

@ Vieille dame et son Leveinard : soin complet d'un Pokémon (même K.O.), une fois
IsaacRoom_EventScript_Healer::
	special IsaacBufferHealerPokemon
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_HealerDone
	msgbox IsaacRoom_Text_HealerIntro, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_HealerDeclined
	special IsaacHealerHealMon
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_HealerNoNeed
	playfanfare MUS_HEAL
	message IsaacRoom_Text_HealerHealed
	waitfanfare
	waitmessage
	special IsaacSetRoomCleared
	special IsaacBufferHealerPokemon
	msgbox IsaacRoom_Text_HealerDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_HealerDone::
	msgbox IsaacRoom_Text_HealerDone, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_HealerDeclined::
	msgbox IsaacRoom_Text_HealerDeclined, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_HealerNoNeed::
	msgbox IsaacRoom_Text_HealerNoNeed, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_Chansey::
	lock
	faceplayer
	special IsaacBufferHealerPokemon
	msgbox IsaacRoom_Text_Chansey, MSGBOX_DEFAULT
	release
	end

@ Entraîneur : IVs et EVs au maximum pour un Pokémon, une fois
IsaacRoom_EventScript_EvTrainer::
	goto_if_eq VAR_0x8005, TRUE, IsaacRoom_EventScript_EvTrainerDone
	msgbox IsaacRoom_Text_EvTrainerIntro, MSGBOX_DEFAULT
	special ChoosePartyMon
	goto_if_ge VAR_0x8004, PARTY_SIZE, IsaacRoom_EventScript_TutorDeclined
	special IsaacMaxEvsIvs
	goto_if_eq VAR_RESULT, FALSE, IsaacRoom_EventScript_EvTrainerNoNeed
	playfanfare MUS_LEVEL_UP
	message IsaacRoom_Text_EvTrainerDone
	waitfanfare
	waitmessage
	special IsaacSetRoomCleared
	release
	end

IsaacRoom_EventScript_EvTrainerDone::
	msgbox IsaacRoom_Text_EvTrainerAfter, MSGBOX_DEFAULT
	release
	end

IsaacRoom_EventScript_EvTrainerNoNeed::
	msgbox IsaacRoom_Text_EvTrainerNoNeed, MSGBOX_DEFAULT
	release
	end

IsaacRoom_Text_HealerIntro:
	.string "My {STR_VAR_2} and I heal\n"
	.string "Pokémon in trouble.\p"
	.string "Which of yours needs us?$"

IsaacRoom_Text_HealerHealed:
	.string "{STR_VAR_1} is in top shape!$"

IsaacRoom_Text_HealerDone:
	.string "{STR_VAR_2} is out of energy for\n"
	.string "today. Safe travels!$"

IsaacRoom_Text_HealerDeclined:
	.string "Come back anytime, dear.$"

IsaacRoom_Text_HealerNoNeed:
	.string "That one is already in top shape!$"

IsaacRoom_Text_Chansey:
	.string "{STR_VAR_2}: Chaaan-sey!$"

IsaacRoom_Text_EvTrainerIntro:
	.string "I'm a personal trainer.\n"
	.string "One session with me and your\l"
	.string "Pokémon reaches its full potential.\p"
	.string "Which one do you want to train?$"

IsaacRoom_Text_EvTrainerDone:
	.string "{STR_VAR_1} reached its full\n"
	.string "potential!$"

IsaacRoom_Text_EvTrainerAfter:
	.string "One session per client!\n"
	.string "Good luck.$"

IsaacRoom_Text_EvTrainerNoNeed:
	.string "That one is already maxed out!$"

