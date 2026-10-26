Hub_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Hub_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Hub_OnFrame
	.byte 0

@ Scott n'est là qu'après la première descente réussie (niveau d'ascension 1 débloqué)
Hub_OnTransition:
	call_if_eq VAR_ISAAC_ASCENSION_MAX, 0, Hub_EventScript_HideScott
	call_if_ne VAR_ISAAC_ASCENSION_MAX, 0, Hub_EventScript_ShowScott
	end

Hub_EventScript_HideScott:
	setflag FLAG_HIDE_HUB_SCOTT
	return

Hub_EventScript_ShowScott:
	clearflag FLAG_HIDE_HUB_SCOTT
	return

Hub_OnFrame:
	map_script_2 VAR_ISAAC_HUB_SCENE, ISAAC_HUB_SCENE_SCOTT_INTRO, Hub_EventScript_ScottIntro
	.2byte 0

@ Retour de la première descente réussie : Scott interpelle le joueur
Hub_EventScript_ScottIntro::
	lockall
	playse SE_PIN
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_FaceRight
	waitmovement 0
	msgbox Hub_Text_ScottIntro, MSGBOX_DEFAULT
	setvar VAR_ISAAC_HUB_SCENE, ISAAC_HUB_SCENE_NONE
	releaseall
	end

@ Scott : choix du niveau d'ascension parmi ceux débloqués
Hub_EventScript_Scott::
	lock
	faceplayer
	special IsaacBuildAscensionMenu
	msgbox Hub_Text_ScottAsk, MSGBOX_DEFAULT
	dynmultistack 0, 0, FALSE, 6, FALSE, VAR_ISAAC_ASCENSION, DYN_MULTICHOICE_CB_NONE
	special IsaacSetAscension
	msgbox Hub_Text_ScottSet, MSGBOX_DEFAULT
	release
	end

@ Montagnard devant l'entrée du donjon : sans Pokémon, il en fait choisir ISAAC_STARTER_PICKS parmi
@ ISAAC_STARTER_CHOICES (écran de l'Usine de Combat) et donne une Master Ball, 2 Super Potions et 2 Rappels
Hub_EventScript_Hiker::
	lock
	faceplayer
	specialvar VAR_RESULT, CalculatePlayerPartyCount
	goto_if_ne VAR_RESULT, 0, Hub_EventScript_HikerGoodLuck
	msgbox Hub_Text_HikerOffer, MSGBOX_DEFAULT
	special IsaacBufferFirstBiome
	msgbox Hub_Text_HikerFirstBiome, MSGBOX_DEFAULT
	msgbox Hub_Text_HikerChoose, MSGBOX_DEFAULT
	closemessage
	special DoIsaacStarterSelectScreen
	waitstate
	@ écran quitté avec B : rien n'a été donné
	specialvar VAR_RESULT, CalculatePlayerPartyCount
	goto_if_eq VAR_RESULT, 0, Hub_EventScript_HikerCancel
	setflag FLAG_SYS_POKEMON_GET
	msgbox Hub_Text_HikerSupplies, MSGBOX_DEFAULT
	giveitem ITEM_MAX_RESTORE, 2
	msgbox Hub_Text_HikerGoodLuck, MSGBOX_DEFAULT
	release
	end

Hub_EventScript_HikerCancel::
	msgbox Hub_Text_HikerCancel, MSGBOX_DEFAULT
	release
	end

Hub_EventScript_HikerGoodLuck::
	special IsaacBufferFirstBiome
	msgbox Hub_Text_HikerFirstBiome, MSGBOX_DEFAULT
	msgbox Hub_Text_HikerGoodLuck, MSGBOX_DEFAULT
	release
	end

@ Entrée du donjon sans Pokémon (déclenché par isaac_dungeon.c) : le Montagnard retient le joueur
Hub_EventScript_NeedPokemon::
	lockall
	msgbox Hub_Text_HikerStop, MSGBOX_DEFAULT
	closemessage
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkDown
	waitmovement 0
	releaseall
	end

Hub_Text_HikerOffer:
	.string "Want to head down into the cave?\n"
	.string "Not without Pokémon!$"

Hub_Text_HikerChoose:
	.string "Pick two of these.$"

Hub_Text_HikerCancel:
	.string "Having second thoughts? Come back\n"
	.string "when you've made up your mind.$"

Hub_Text_HikerSupplies:
	.string "Take some healing for the road.$"

Hub_Text_HikerFirstBiome:
	.string "I went down to take a look:\n"
	.string "down there, it starts with…\p"
	.string "{STR_VAR_1}!\p"
	.string "Prepare your team accordingly.$"

Hub_Text_HikerGoodLuck:
	.string "Good luck down there!$"

Hub_Text_HikerStop:
	.string "Hey! You're not going in there\n"
	.string "without Pokémon? Come talk to me!$"

Hub_Text_ScottIntro:
	.string "SCOTT: Hey there! You made it all\n"
	.string "the way to the bottom! Impressive!\p"
	.string "I'm SCOTT. I'm always on the lookout\n"
	.string "for strong Trainers.\p"
	.string "How about a tougher challenge?\n"
	.string "Talk to me before heading down,\l"
	.string "and pick your Ascension level.\p"
	.string "Each run you clear unlocks\n"
	.string "the next one!$"

Hub_Text_ScottAsk:
	.string "SCOTT: You're at Ascension\n"
	.string "{STR_VAR_1}. Which level do you want?$"

Hub_Text_ScottSet:
	.string "SCOTT: Ascension {STR_VAR_1} it is!\n"
	.string "Good luck down there!$"
