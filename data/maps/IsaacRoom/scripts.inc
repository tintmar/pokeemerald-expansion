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
	.string "Personne ne traverse cette salle\n"
	.string "sans combattre !$"

IsaacRoom_Text_TrainerDefeat:
	.string "Bien joué…$"

IsaacRoom_Text_TrainerChallenge:
	.string "Tu veux te mesurer à moi ?\n"
	.string "Je ne t'en voudrai pas de passer…$"

IsaacRoom_Text_TrainerAccepted:
	.string "Parfait ! En garde !$"

IsaacRoom_Text_TrainerDeclined:
	.string "Dommage. Reviens quand tu veux.$"

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
	.string "Bravo pour ta victoire !\p"
	.string "Je soigne tes Pokémon, même ceux qui\n"
	.string "sont K.O. : PV au complet.\p"
	.string "{STR_VAR_3} Pokémon à soigner : {STR_VAR_2}¥.\n"
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

IsaacRoom_Text_NursePartial:
	.string "Tu n'avais pas assez d'argent pour\n"
	.string "tout le monde…\p"
	.string "J'ai pu en soigner {STR_VAR_3}, dans l'ordre\n"
	.string "de ton équipe. Bonne chance !$"

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
	.string "Ton équipe est pleine…\n"
	.string "Relâcher un Pokémon pour prendre l'Œuf ?$"

IsaacRoom_Text_MonReleased:
	.string "{STR_VAR_1} a été relâché.\n"
	.string "Au revoir, {STR_VAR_1} !$"

IsaacRoom_Text_CantReleaseLastMon:
	.string "Tu ne peux pas relâcher ton dernier\n"
	.string "Pokémon !$"

IsaacRoom_Text_TraderIntro:
	.string "Je collectionne les Pokémon de tous\n"
	.string "les biomes.\p"
	.string "Je t'échange un des tiens contre un\n"
	.string "des miens, d'une force équivalente.\p"
	.string "Lequel me proposes-tu ?$"

IsaacRoom_Text_TraderOffer:
	.string "Ton {STR_VAR_1} contre un de mes\n"
	.string "Pokémon de force comparable.\p"
	.string "Marché conclu ?$"

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

@ Duelliste : il annonce son Pokémon, puis combat en 1 contre 1 contre le Pokémon choisi (perdre ne renvoie pas au
@ hub). Un seul duel : gagné, une Guérison et une Super CT ; perdu, un Rappel pour relever le Pokémon K.O.
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
	giveitem ITEM_FULL_RESTORE
	giveitem ITEM_ABILITY_TRAINER
	release
	end

IsaacRoom_EventScript_DuelistLost::
	msgbox IsaacRoom_Text_DuelistLost, MSGBOX_DEFAULT
	giveitem ITEM_REVIVE
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
	.string "Un duel, un seul Pokémon chacun !\p"
	.string "Mon {STR_VAR_1} attend un adversaire\n"
	.string "digne de lui.\p"
	.string "Tu relèves le défi ?$"

IsaacRoom_Text_DuelistChoose:
	.string "Choisis ton champion !$"

IsaacRoom_Text_DuelistIntro:
	.string "En garde !$"

IsaacRoom_Text_DuelistDefeat:
	.string "Quel duel ! Tu as gagné.$"

IsaacRoom_Text_DuelistWon:
	.string "Je n'ai qu'une parole : voici ta\n"
	.string "récompense.$"

IsaacRoom_Text_DuelistLost:
	.string "Ha ! Belle tentative, mais un duel\n"
	.string "ne se rejoue pas.\p"
	.string "Tiens, pour remettre ton Pokémon\n"
	.string "sur pied.$"

IsaacRoom_Text_DuelistDone:
	.string "Un duel mémorable. Bonne route !$"

IsaacRoom_Text_DuelistDeclined:
	.string "Dommage… Mon {STR_VAR_1} t'attendra.$"

IsaacRoom_Text_DuelistCantFight:
	.string "Ce Pokémon ne peut pas se battre !$"

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
	.string "Hmm… Je vois {STR_VAR_1}, qui t'attend\n"
	.string "au bout de cet étage…\p"
	.string "Son équipe :\n"
	.string "{STR_VAR_2}.\p"
	.string "Son as, gardé pour la fin :\n"
	.string "{STR_VAR_3} !$"

IsaacRoom_Text_SeerAgain:
	.string "Méfie-toi de l'as de {STR_VAR_1} :\n"
	.string "{STR_VAR_3} !$"

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
	.string "Certaines roches de ce donjon sonnent\n"
	.string "creux… Il y a des salles cachées derrière.\p"
	.string "Je peux apprendre Éclate-Roc à l'un\n"
	.string "de tes Pokémon. Lequel ?$"

IsaacRoom_Text_RockSmashHint:
	.string "Parle aux roches qui ferment les salles :\n"
	.string "si l'une sonne creux, brise-la !$"

IsaacRoom_Text_RockSmashCant:
	.string "Je n'ai rien à lui apprendre…$"

IsaacRoom_Text_SecretWallAsk:
	.string "Cette roche sonne creux…\n"
	.string "{STR_VAR_1} utilise Éclate-Roc ?$"

IsaacRoom_Text_SecretWallSmash:
	.string "{STR_VAR_1} utilise Éclate-Roc !$"

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
	.string "Mon {STR_VAR_2} et moi soignons les\n"
	.string "Pokémon en difficulté.\p"
	.string "Lequel des tiens a besoin de nous ?$"

IsaacRoom_Text_HealerHealed:
	.string "{STR_VAR_1} est en pleine forme !$"

IsaacRoom_Text_HealerDone:
	.string "{STR_VAR_2} n'a plus d'énergie pour\n"
	.string "aujourd'hui. Bonne route !$"

IsaacRoom_Text_HealerDeclined:
	.string "Reviens quand tu veux, mon petit.$"

IsaacRoom_Text_HealerNoNeed:
	.string "Celui-là est déjà en pleine forme !$"

IsaacRoom_Text_Chansey:
	.string "{STR_VAR_2} : Grrrou !$"

IsaacRoom_Text_EvTrainerIntro:
	.string "Je suis entraîneur personnel.\n"
	.string "Une séance avec moi et ton Pokémon\l"
	.string "atteint tout son potentiel.\p"
	.string "Lequel veux-tu entraîner ?$"

IsaacRoom_Text_EvTrainerDone:
	.string "{STR_VAR_1} a atteint tout son\n"
	.string "potentiel !$"

IsaacRoom_Text_EvTrainerAfter:
	.string "Une seule séance par client !\n"
	.string "Bonne chance.$"

IsaacRoom_Text_EvTrainerNoNeed:
	.string "Celui-là est déjà au maximum !$"

