Hub_MapScripts::
	.byte 0

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
	msgbox Hub_Text_HikerMasterBall, MSGBOX_DEFAULT
	giveitem ITEM_MASTER_BALL
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

Hub_Text_HikerMasterBall:
	.string "And take this: a MASTER BALL.\n"
	.string "Save it for a truly special Pokémon.$"

Hub_Text_HikerSupplies:
	.string "And some healing for the road.$"

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
