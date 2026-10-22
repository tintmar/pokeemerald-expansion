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
	giveitem ITEM_SUPER_POTION, 2
	giveitem ITEM_REVIVE, 2
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
	.string "Tu veux descendre dans la grotte ?\n"
	.string "Pas sans Pokémon !$"

Hub_Text_HikerChoose:
	.string "Choisis-en deux parmi ceux-là.$"

Hub_Text_HikerCancel:
	.string "Tu hésites ? Reviens me voir\n"
	.string "quand tu seras décidé.$"

Hub_Text_HikerMasterBall:
	.string "Et prends ça : une MASTER BALL.\n"
	.string "Garde-la pour un Pokémon d'exception.$"

Hub_Text_HikerSupplies:
	.string "Et quelques soins pour la route.$"

Hub_Text_HikerFirstBiome:
	.string "Je suis descendu jeter un œil :\n"
	.string "en bas, ça commence par…\p"
	.string "{STR_VAR_1} !\p"
	.string "Prépare ton équipe en conséquence.$"

Hub_Text_HikerGoodLuck:
	.string "Bonne chance là-dessous !$"

Hub_Text_HikerStop:
	.string "Hé ! Tu ne vas pas entrer\n"
	.string "sans Pokémon ? Viens me voir !$"
