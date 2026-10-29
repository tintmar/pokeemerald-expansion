#ifndef GUARD_CONSTANTS_VARS_H
#define GUARD_CONSTANTS_VARS_H

#include "constants/vars_frlg.h"

#define VARS_START 0x4000

// temporary vars
// The first 0x10 vars are temporary--they are cleared every time a map is loaded.
#define TEMP_VARS_START            0x4000
#define VAR_TEMP_0                 (TEMP_VARS_START + 0x0)
#define VAR_TEMP_1                 (TEMP_VARS_START + 0x1)
#define VAR_TEMP_2                 (TEMP_VARS_START + 0x2)
#define VAR_TEMP_3                 (TEMP_VARS_START + 0x3) // Note: Used when the player checks a TV
#define VAR_TEMP_4                 (TEMP_VARS_START + 0x4)
#define VAR_TEMP_5                 (TEMP_VARS_START + 0x5)
#define VAR_TEMP_6                 (TEMP_VARS_START + 0x6)
#define VAR_TEMP_7                 (TEMP_VARS_START + 0x7)
#define VAR_TEMP_8                 (TEMP_VARS_START + 0x8)
#define VAR_TEMP_9                 (TEMP_VARS_START + 0x9)
#define VAR_TEMP_A                 (TEMP_VARS_START + 0xA)
#define VAR_TEMP_B                 (TEMP_VARS_START + 0xB)
#define VAR_TEMP_C                 (TEMP_VARS_START + 0xC)
#define VAR_TEMP_D                 (TEMP_VARS_START + 0xD)
#define VAR_TEMP_E                 (TEMP_VARS_START + 0xE)
#define VAR_TEMP_F                 (TEMP_VARS_START + 0xF)
#define TEMP_VARS_END              VAR_TEMP_F
#define NUM_TEMP_VARS              (TEMP_VARS_END - TEMP_VARS_START + 1)

// object gfx id vars
// These 0x10 vars are used to dynamically control a map object's sprite.
// For example, the rival's sprite id is dynamically set based on the player's gender.
// See VarGetObjectEventGraphicsId().
#define VAR_OBJ_GFX_ID_0           0x4010
#define VAR_OBJ_GFX_ID_1           0x4011
#define VAR_OBJ_GFX_ID_2           0x4012
#define VAR_OBJ_GFX_ID_3           0x4013
#define VAR_OBJ_GFX_ID_4           0x4014
#define VAR_OBJ_GFX_ID_5           0x4015
#define VAR_OBJ_GFX_ID_6           0x4016
#define VAR_OBJ_GFX_ID_7           0x4017
#define VAR_OBJ_GFX_ID_8           0x4018
#define VAR_OBJ_GFX_ID_9           0x4019
#define VAR_OBJ_GFX_ID_A           0x401A
#define VAR_OBJ_GFX_ID_B           0x401B
#define VAR_OBJ_GFX_ID_C           0x401C
#define VAR_OBJ_GFX_ID_D           0x401D
#define VAR_OBJ_GFX_ID_E           0x401E
#define VAR_OBJ_GFX_ID_F           0x401F

// general purpose vars
#define VAR_MAIN_GAME_JUNK                               0x4020 // donjon : les variables du jeu principal (jamais joué) y pointent toutes
#define VAR_REPEL_STEP_COUNT                             0x4021
#define VAR_ICE_STEP_COUNT                               0x4022
#define VAR_STARTER_MON                                  0x4023 // 0=Treecko, 1=Torchic, 2=Mudkip
#define VAR_MIRAGE_RND_H                                 0x4024 // Unused if OW_USE_DAILY_SEED_FOR_VANILLA_VARIABLES is TRUE
#define VAR_MIRAGE_RND_L                                 0x4025 // Unused if OW_USE_DAILY_SEED_FOR_VANILLA_VARIABLES is TRUE
#define VAR_SECRET_BASE_MAP                              0x4026
#define VAR_CYCLING_ROAD_RECORD_COLLISIONS               0x4027
#define VAR_CYCLING_ROAD_RECORD_TIME_L                   0x4028
#define VAR_CYCLING_ROAD_RECORD_TIME_H                   0x4029
#define VAR_FRIENDSHIP_STEP_COUNTER                      0x402A
#define VAR_POISON_STEP_COUNTER                          0x402B
#define VAR_RESET_RTC_ENABLE                             0x402C
#define VAR_ENIGMA_BERRY_AVAILABLE                       0x402D
#define VAR_WONDER_NEWS_STEP_COUNTER                     0x402E

#define VAR_FRONTIER_MANIAC_FACILITY                     0x402F
#define VAR_FRONTIER_GAMBLER_CHALLENGE                   0x4030
#define VAR_FRONTIER_GAMBLER_SET_CHALLENGE               0x4031
#define VAR_ISAAC_MAX_RESTORE_LUCK                       0x4032 // Retard des Restores Max des dresseurs, en centièmes + 1000 (isaac_dungeon.c)
#define VAR_FRONTIER_GAMBLER_STATE                       0x4033

#define VAR_DEOXYS_ROCK_STEP_COUNT                       0x4034
#define VAR_DEOXYS_ROCK_LEVEL                            0x4035
#define VAR_PC_BOX_TO_SEND_MON                           0x4036
#define VAR_ABNORMAL_WEATHER_LOCATION                    0x4037
#define VAR_ABNORMAL_WEATHER_STEP_COUNTER                0x4038
#define VAR_SHOULD_END_ABNORMAL_WEATHER                  0x4039
#define VAR_FARAWAY_ISLAND_STEP_COUNTER                  0x403A
#define VAR_REGICE_STEPS_1                               0x403B
#define VAR_REGICE_STEPS_2                               0x403C
#define VAR_REGICE_STEPS_3                               0x403D
#define VAR_ALTERING_CAVE_WILD_SET                       0x403E
#define VAR_DISTRIBUTE_EON_TICKET                        0x403F // This var is read and written, but is always zero. The only way to obtain the Eon Ticket in Emerald is via Record Mixing
#define VAR_DAYS                                         0x4040
#define VAR_FANCLUB_FAN_COUNTER                          0x4041
#define VAR_FANCLUB_LOSE_FAN_TIMER                       0x4042
#define VAR_DEPT_STORE_FLOOR                             0x4043
#define VAR_UNUSED_0x4044                                0x4044 // libre (était VAR_TRICK_HOUSE_LEVEL, jeu principal)
#define VAR_POKELOT_PRIZE_ITEM                           0x4045
#define VAR_NATIONAL_DEX                                 0x4046
#define VAR_SEEDOT_SIZE_RECORD                           0x4047
#define VAR_ASH_GATHER_COUNT                             0x4048
#define VAR_BIRCH_STATE                                  0x4049
#define VAR_CRUISE_STEP_COUNT                            0x404A
#define VAR_POKELOT_RND1                                 0x404B // Unused if OW_USE_DAILY_SEED_FOR_VANILLA_VARIABLES is TRUE
#define VAR_POKELOT_RND2                                 0x404C // Unused if OW_USE_DAILY_SEED_FOR_VANILLA_VARIABLES is TRUE
#define VAR_UNUSED_0x404D                                0x404D // libre (était VAR_POKELOT_PRIZE_PLACE, jeu principal)
#define VAR_ISAAC_SEED                                   0x404E // Graine de l'étage du donjon (isaac_dungeon.c)
#define VAR_LOTAD_SIZE_RECORD                            0x404F
#define VAR_UNUSED_0x4050                                0x4050 // libre (était VAR_LITTLEROOT_TOWN_STATE, jeu principal)
#define VAR_UNUSED_0x4051                                0x4051 // libre (était VAR_OLDALE_TOWN_STATE, jeu principal)
#define VAR_UNUSED_0x4052                                0x4052 // libre (était VAR_DEWFORD_TOWN_STATE, jeu principal)
#define VAR_UNUSED_0x4053                                0x4053 // libre (était VAR_LAVARIDGE_TOWN_STATE, jeu principal)
#define VAR_CURRENT_SECRET_BASE                          0x4054 // was probably allocated for VAR_FALLARBOR_TOWN_STATE at one point
#define VAR_UNUSED_0x4055                                0x4055 // libre (était VAR_VERDANTURF_TOWN_STATE, jeu principal)
#define VAR_UNUSED_0x4056                                0x4056 // libre (était VAR_PACIFIDLOG_TOWN_STATE, jeu principal)
#define VAR_UNUSED_0x4057                                0x4057 // libre (était VAR_PETALBURG_CITY_STATE, jeu principal)
#define VAR_UNUSED_0x4058                                0x4058 // libre (était VAR_SLATEPORT_CITY_STATE, jeu principal)
#define VAR_UNUSED_0x4059                                0x4059 // libre (était VAR_MAUVILLE_CITY_STATE, jeu principal)
#define VAR_UNUSED_0x405A                                0x405A // libre (était VAR_RUSTBORO_CITY_STATE, jeu principal)
#define VAR_UNUSED_0x405B                                0x405B // libre (était VAR_FORTREE_CITY_STATE, jeu principal)
#define VAR_UNUSED_0x405C                                0x405C // libre (était VAR_LILYCOVE_CITY_STATE, jeu principal)
#define VAR_MOSSDEEP_CITY_STATE                          0x405D
#define VAR_SOOTOPOLIS_CITY_STATE                        0x405E
#define VAR_UNUSED_0x405F                                0x405F // libre (était VAR_EVER_GRANDE_CITY_STATE, jeu principal)
#define VAR_UNUSED_0x4060                                0x4060 // libre (était VAR_ROUTE101_STATE, jeu principal)
#define VAR_UNUSED_0x4061                                0x4061 // libre (était VAR_ROUTE102_STATE, jeu principal)
#define VAR_UNUSED_0x4062                                0x4062 // libre (était VAR_ROUTE103_STATE, jeu principal)
#define VAR_UNUSED_0x4063                                0x4063 // libre (était VAR_ROUTE104_STATE, jeu principal)
#define VAR_UNUSED_0x4064                                0x4064 // libre (était VAR_ROUTE105_STATE, jeu principal)
#define VAR_UNUSED_0x4065                                0x4065 // libre (était VAR_ROUTE106_STATE, jeu principal)
#define VAR_UNUSED_0x4066                                0x4066 // libre (était VAR_ROUTE107_STATE, jeu principal)
#define VAR_UNUSED_0x4067                                0x4067 // libre (était VAR_ROUTE108_STATE, jeu principal)
#define VAR_UNUSED_0x4068                                0x4068 // libre (était VAR_ROUTE109_STATE, jeu principal)
#define VAR_UNUSED_0x4069                                0x4069 // libre (était VAR_ROUTE110_STATE, jeu principal)
#define VAR_UNUSED_0x406A                                0x406A // libre (était VAR_ROUTE111_STATE, jeu principal)
#define VAR_UNUSED_0x406B                                0x406B // libre (était VAR_ROUTE112_STATE, jeu principal)
#define VAR_UNUSED_0x406C                                0x406C // libre (était VAR_ROUTE113_STATE, jeu principal)
#define VAR_UNUSED_0x406D                                0x406D // libre (était VAR_ROUTE114_STATE, jeu principal)
#define VAR_UNUSED_0x406E                                0x406E // libre (était VAR_ROUTE115_STATE, jeu principal)
#define VAR_UNUSED_0x406F                                0x406F // libre (était VAR_ROUTE116_STATE, jeu principal)
#define VAR_UNUSED_0x4070                                0x4070 // libre (était VAR_ROUTE117_STATE, jeu principal)
#define VAR_UNUSED_0x4071                                0x4071 // libre (était VAR_ROUTE118_STATE, jeu principal)
#define VAR_UNUSED_0x4072                                0x4072 // libre (était VAR_ROUTE119_STATE, jeu principal)
#define VAR_UNUSED_0x4073                                0x4073 // libre (était VAR_ROUTE120_STATE, jeu principal)
#define VAR_UNUSED_0x4074                                0x4074 // libre (était VAR_ROUTE121_STATE, jeu principal)
#define VAR_UNUSED_0x4075                                0x4075 // libre (était VAR_ROUTE122_STATE, jeu principal)
#define VAR_UNUSED_0x4076                                0x4076 // libre (était VAR_ROUTE123_STATE, jeu principal)
#define VAR_UNUSED_0x4077                                0x4077 // libre (était VAR_ROUTE124_STATE, jeu principal)
#define VAR_UNUSED_0x4078                                0x4078 // libre (était VAR_ROUTE125_STATE, jeu principal)
#define VAR_UNUSED_0x4079                                0x4079 // libre (était VAR_ROUTE126_STATE, jeu principal)
#define VAR_UNUSED_0x407A                                0x407A // libre (était VAR_ROUTE127_STATE, jeu principal)
#define VAR_UNUSED_0x407B                                0x407B // libre (était VAR_ROUTE128_STATE, jeu principal)
#define VAR_UNUSED_0x407C                                0x407C // libre (était VAR_ROUTE129_STATE, jeu principal)
#define VAR_UNUSED_0x407D                                0x407D // libre (était VAR_ROUTE130_STATE, jeu principal)
#define VAR_UNUSED_0x407E                                0x407E // libre (était VAR_ROUTE131_STATE, jeu principal)
#define VAR_UNUSED_0x407F                                0x407F // libre (était VAR_ROUTE132_STATE, jeu principal)
#define VAR_UNUSED_0x4080                                0x4080 // libre (était VAR_ROUTE133_STATE, jeu principal)
#define VAR_UNUSED_0x4081                                0x4081 // libre (était VAR_ROUTE134_STATE, jeu principal)
#define VAR_UNUSED_0x4082                                0x4082 // libre (était VAR_LITTLEROOT_HOUSES_STATE_MAY, jeu principal)
#define VAR_ISAAC_FLOOR                                  0x4083 // Étage du donjon (1 = premier)
#define VAR_UNUSED_0x4084                                0x4084 // libre (était VAR_BIRCH_LAB_STATE, jeu principal)
#define VAR_UNUSED_0x4085                                0x4085 // libre (était VAR_PETALBURG_GYM_STATE, jeu principal)
#define VAR_CONTEST_HALL_STATE                           0x4086
#define VAR_CABLE_CLUB_STATE                             0x4087
#define VAR_CONTEST_TYPE                                 0x4088
#define VAR_SECRET_BASE_INITIALIZED                      0x4089
#define VAR_UNUSED_0x408A                                0x408A // libre (était VAR_CONTEST_PRIZE_PICKUP, jeu principal)
#define VAR_ISAAC_ROOM                                   0x408B // Cellule de la salle courante (y * ISAAC_GRID_W + x)
#define VAR_UNUSED_0x408C                                0x408C // libre (était VAR_LITTLEROOT_HOUSES_STATE_BRENDAN, jeu principal)
#define VAR_UNUSED_0x408D                                0x408D // libre (était VAR_LITTLEROOT_RIVAL_STATE, jeu principal)
#define VAR_UNUSED_0x408E                                0x408E // libre (était VAR_BOARD_BRINEY_BOAT_STATE, jeu principal)
#define VAR_UNUSED_0x408F                                0x408F // libre (était VAR_DEVON_CORP_3F_STATE, jeu principal)
#define VAR_UNUSED_0x4090                                0x4090 // libre (était VAR_BRINEY_HOUSE_STATE, jeu principal)
#define VAR_ISAAC_VISITED_0                              0x4091 // Salles visitées de l'étage, cellules 0-15 (isaac_dungeon.c)
#define VAR_UNUSED_0x4092                                0x4092 // libre (était VAR_LITTLEROOT_INTRO_STATE, jeu principal)
#define VAR_UNUSED_0x4093                                0x4093 // libre (était VAR_MAUVILLE_GYM_STATE, jeu principal)
#define VAR_UNUSED_0x4094                                0x4094 // libre (était VAR_LILYCOVE_MUSEUM_2F_STATE, jeu principal)
#define VAR_LILYCOVE_FAN_CLUB_STATE                      0x4095
#define VAR_UNUSED_0x4096                                0x4096 // libre (était VAR_BRINEY_LOCATION, jeu principal)
#define VAR_INIT_SECRET_BASE                             0x4097
#define VAR_UNUSED_0x4098                                0x4098 // libre (était VAR_PETALBURG_WOODS_STATE, jeu principal)
#define VAR_UNUSED_0x4099                                0x4099 // libre (était VAR_LILYCOVE_CONTEST_LOBBY_STATE, jeu principal)
#define VAR_RUSTURF_TUNNEL_STATE                         0x409A
#define VAR_ISAAC_VISITED_1                              0x409B // Salles visitées, cellules 16-31
#define VAR_UNUSED_0x409C                                0x409C // libre (était VAR_ELITE_4_STATE, jeu principal)
#define VAR_ISAAC_VISITED_2                              0x409D // Salles visitées, cellules 32-47
#define VAR_UNUSED_0x409E                                0x409E // libre (était VAR_MOSSDEEP_SPACE_CENTER_STAIR_GUARD_STATE, jeu principal)
#define VAR_UNUSED_0x409F                                0x409F // libre (était VAR_MOSSDEEP_SPACE_CENTER_STATE, jeu principal)
#define VAR_UNUSED_0x40A0                                0x40A0 // libre (était VAR_SLATEPORT_HARBOR_STATE, jeu principal)
#define VAR_ISAAC_VISITED_3                              0x40A1 // Salles visitées, cellules 48-63
#define VAR_UNUSED_0x40A2                                0x40A2 // libre (était VAR_SEAFLOOR_CAVERN_STATE, jeu principal)
#define VAR_UNUSED_0x40A3                                0x40A3 // libre (était VAR_CABLE_CAR_STATION_STATE, jeu principal)
#define VAR_UNUSED_0x40A4                                0x40A4 // libre (était VAR_SAFARI_ZONE_STATE, jeu principal)
#define VAR_UNUSED_0x40A5                                0x40A5 // libre (était VAR_TRICK_HOUSE_BEING_WATCHED_STATE, jeu principal)
#define VAR_UNUSED_0x40A6                                0x40A6 // libre (était VAR_TRICK_HOUSE_FOUND_TRICK_MASTER, jeu principal)
#define VAR_UNUSED_0x40A7                                0x40A7 // libre (était VAR_TRICK_HOUSE_ENTRANCE_STATE, jeu principal)
#define VAR_ISAAC_VISITED_4                              0x40A8 // Salles visitées, cellules 64-71
#define VAR_CYCLING_CHALLENGE_STATE                      0x40A9
#define VAR_UNUSED_0x40AA                                0x40AA // libre (était VAR_SLATEPORT_MUSEUM_1F_STATE, jeu principal)
#define VAR_UNUSED_0x40AB                                0x40AB // libre (était VAR_TRICK_HOUSE_PUZZLE_1_STATE, jeu principal)
#define VAR_UNUSED_0x40AC                                0x40AC // libre (était VAR_TRICK_HOUSE_PUZZLE_2_STATE, jeu principal)
#define VAR_UNUSED_0x40AD                                0x40AD // libre (était VAR_TRICK_HOUSE_PUZZLE_3_STATE, jeu principal)
#define VAR_UNUSED_0x40AE                                0x40AE // libre (était VAR_TRICK_HOUSE_PUZZLE_4_STATE, jeu principal)
#define VAR_UNUSED_0x40AF                                0x40AF // libre (était VAR_TRICK_HOUSE_PUZZLE_5_STATE, jeu principal)
#define VAR_UNUSED_0x40B0                                0x40B0 // libre (était VAR_TRICK_HOUSE_PUZZLE_6_STATE, jeu principal)
#define VAR_UNUSED_0x40B1                                0x40B1 // libre (était VAR_TRICK_HOUSE_PUZZLE_7_STATE, jeu principal)
#define VAR_UNUSED_0x40B2                                0x40B2 // libre (était VAR_TRICK_HOUSE_PUZZLE_8_STATE, jeu principal)
#define VAR_WEATHER_INSTITUTE_STATE                      0x40B3
#define VAR_SS_TIDAL_STATE                               0x40B4
#define VAR_UNUSED_0x40B5                                0x40B5 // libre (était VAR_TRICK_HOUSE_ENTER_FROM_CORRIDOR, jeu principal)
#define VAR_UNUSED_0x40B6                                0x40B6 // libre (était VAR_TRICK_HOUSE_PUZZLE_7_STATE_2, jeu principal)
#define VAR_UNUSED_0x40B7                                0x40B7 // libre (était VAR_SLATEPORT_FAN_CLUB_STATE, jeu principal)
#define VAR_ISAAC_BIOME_ORDER                            0x40B8 // Ordre des biomes de la descente (graine, isaac_dungeon.c)
#define VAR_UNUSED_0x40B9                                0x40B9 // libre (était VAR_MT_PYRE_STATE, jeu principal)
#define VAR_UNUSED_0x40BA                                0x40BA // libre (était VAR_NEW_MAUVILLE_STATE, jeu principal)
#define VAR_ISAAC_ASCENSION                              0x40BB // Niveau d'ascension choisi pour les descentes (isaac_dungeon.c)
#define VAR_BRAVO_TRAINER_BATTLE_TOWER_ON                0x40BC
#define VAR_UNUSED_0x40BD                                0x40BD // libre (était VAR_JAGGED_PASS_ASH_WEATHER, jeu principal)
#define VAR_UNUSED_0x40BE                                0x40BE // libre (était VAR_GLASS_WORKSHOP_STATE, jeu principal)
#define VAR_UNUSED_0x40BF                                0x40BF // libre (était VAR_METEOR_FALLS_STATE, jeu principal)
#define VAR_UNUSED_0x40C0                                0x40C0 // libre (était VAR_SOOTOPOLIS_MYSTERY_EVENTS_STATE, jeu principal)
#define VAR_UNUSED_0x40C1                                0x40C1 // libre (était VAR_TRICK_HOUSE_PRIZE_PICKUP, jeu principal)
#define VAR_PACIFIDLOG_TM_RECEIVED_DAY                   0x40C2
#define VAR_UNUSED_0x40C3                                0x40C3 // libre (était VAR_VICTORY_ROAD_1F_STATE, jeu principal)
#define VAR_UNUSED_0x40C4                                0x40C4 // libre (était VAR_FOSSIL_RESURRECTION_STATE, jeu principal)
#define VAR_UNUSED_0x40C5                                0x40C5 // libre (était VAR_WHICH_FOSSIL_REVIVED, jeu principal)
#define VAR_UNUSED_0x40C6                                0x40C6 // libre (était VAR_STEVENS_HOUSE_STATE, jeu principal)
#define VAR_UNUSED_0x40C7                                0x40C7 // libre (était VAR_OLDALE_RIVAL_STATE, jeu principal)
#define VAR_UNUSED_0x40C8                                0x40C8 // libre (était VAR_JAGGED_PASS_STATE, jeu principal)
#define VAR_UNUSED_0x40C9                                0x40C9 // libre (était VAR_SCOTT_PETALBURG_ENCOUNTER, jeu principal)
#define VAR_SKY_PILLAR_STATE                             0x40CA
#define VAR_MIRAGE_TOWER_STATE                           0x40CB
#define VAR_UNUSED_0x40CC                                0x40CC // libre (était VAR_FOSSIL_MANIAC_STATE, jeu principal)
#define VAR_UNUSED_0x40CD                                0x40CD // libre (était VAR_CABLE_CLUB_TUTORIAL_STATE, jeu principal)
#define VAR_FRONTIER_BATTLE_MODE                         0x40CE
#define VAR_FRONTIER_FACILITY                            0x40CF
#define VAR_UNUSED_0x40D0                                0x40D0 // libre (était VAR_HAS_ENTERED_BATTLE_FRONTIER, jeu principal)
#define VAR_UNUSED_0x40D1                                0x40D1 // libre (était VAR_SCOTT_STATE, jeu principal)
#define VAR_UNUSED_0x40D2                                0x40D2 // libre (était VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE, jeu principal)
#define VAR_UNUSED_0x40D3                                0x40D3 // libre (était VAR_DEX_UPGRADE_JOHTO_STARTER_STATE, jeu principal)
#define VAR_UNUSED_0x40D4                                0x40D4 // libre (était VAR_SS_TIDAL_SCOTT_STATE, jeu principal)
#define VAR_UNUSED_0x40D5                                0x40D5 // libre (était VAR_ROAMER_POKEMON, jeu principal)
#define VAR_TRAINER_HILL_IS_ACTIVE                       0x40D6
#define VAR_UNUSED_0x40D7                                0x40D7 // libre (était VAR_SKY_PILLAR_RAYQUAZA_CRY_DONE, jeu principal)
#define VAR_UNUSED_0x40D8                                0x40D8 // libre (était VAR_SOOTOPOLIS_WALLACE_STATE, jeu principal)
#define VAR_UNUSED_0x40D9                                0x40D9 // libre (était VAR_HAS_TALKED_TO_SEAFLOOR_CAVERN_ENTRANCE_GRUNT, jeu principal)
#define VAR_UNUSED_0x40DA                                0x40DA // libre (était VAR_REGISTER_BIRCH_STATE, jeu principal)
#define VAR_ISAAC_ASCENSION_MAX                          0x40DB // Plus haut niveau d'ascension débloqué
#define VAR_ISAAC_HUB_SCENE                              0x40DC // Scène du hub en attente (ISAAC_HUB_SCENE_*)
#define VAR_GIFT_PICHU_SLOT                              0x40DD
#define VAR_UNUSED_0x40DE                                0x40DE // libre (était VAR_GIFT_UNUSED_1, jeu principal)
#define VAR_UNUSED_0x40DF                                0x40DF // libre (était VAR_GIFT_UNUSED_2, jeu principal)
#define VAR_UNUSED_0x40E0                                0x40E0 // libre (était VAR_GIFT_UNUSED_3, jeu principal)
#define VAR_UNUSED_0x40E1                                0x40E1 // libre (était VAR_GIFT_UNUSED_4, jeu principal)
#define VAR_UNUSED_0x40E2                                0x40E2 // libre (était VAR_GIFT_UNUSED_5, jeu principal)
#define VAR_UNUSED_0x40E3                                0x40E3 // libre (était VAR_GIFT_UNUSED_6, jeu principal)
#define VAR_UNUSED_0x40E4                                0x40E4 // libre (était VAR_GIFT_UNUSED_7, jeu principal)
#define VAR_ISAAC_PROGRESSION_PITY                       0x40E5 // Anti-malchance des récompenses de combat (isaac_pokemon.c)
#define VAR_DAILY_SLOTS                                  0x40E6
#define VAR_DAILY_WILDS                                  0x40E7
#define VAR_DAILY_BLENDER                                0x40E8
#define VAR_DAILY_PLANTED_BERRIES                        0x40E9
#define VAR_DAILY_PICKED_BERRIES                         0x40EA
#define VAR_DAILY_ROULETTE                               0x40EB
#define VAR_SECRET_BASE_STEP_COUNTER                     0x40EC // Used by Secret Base TV programs
#define VAR_SECRET_BASE_LAST_ITEM_USED                   0x40ED // Used by Secret Base TV programs
#define VAR_SECRET_BASE_LOW_TV_FLAGS                     0x40EE // Used by Secret Base TV programs
#define VAR_SECRET_BASE_HIGH_TV_FLAGS                    0x40EF // Used by Secret Base TV programs
#define VAR_SECRET_BASE_IS_NOT_LOCAL                     0x40F0 // Set to TRUE while in another player's secret base.
#define VAR_DAILY_BP                                     0x40F1
#define VAR_WALLY_CALL_STEP_COUNTER                      0x40F2
#define VAR_SCOTT_FORTREE_CALL_STEP_COUNTER              0x40F3
#define VAR_ROXANNE_CALL_STEP_COUNTER                    0x40F4
#define VAR_SCOTT_BF_CALL_STEP_COUNTER                   0x40F5
#define VAR_RIVAL_RAYQUAZA_CALL_STEP_COUNTER             0x40F6
#define VAR_ISAAC_SHOP_SITUATIONAL                       0x40F7 // Objets situationnels de la boutique de l'étage, figés à la première visite
#define VAR_ISAAC_FALLEN_CROWN                           0x40F8 // Pokémon couronné relâché après le combat (isaac_pokemon.c)
#define VAR_ISAAC_FALLEN_CROWN_PENDING                   0x40F9 // 1 : message du Pokémon couronné relâché en attente
#define VAR_ISAAC_CAPTURE_GIFT_PENDING                   0x40FA // 1 : le Pokémaniac de la salle de capture va donner la Master Ball
#define VAR_ISAAC_SECRET_ABILITY_SEEN_0                  0x40FB // Talents secrets déjà proposés dans la descente (isaac_pokemon.c)
#define VAR_ISAAC_SECRET_ABILITY_SEEN_1                  0x40FC // Talents secrets déjà proposés dans la descente (isaac_pokemon.c)
#define VAR_ISAAC_SECRET_ABILITY_SEEN_2                  0x40FD // Talents secrets déjà proposés dans la descente (isaac_pokemon.c)
#define VAR_ISAAC_SECRET_ABILITY_SEEN_3                  0x40FE // Talents secrets déjà proposés dans la descente (isaac_pokemon.c)
#define VAR_ISAAC_RIVAL_ROOM                             0x40FF // Salle du rival choisie en cours d'étage (cellule + 1, 0 : pas encore)

#define VARS_END                                         0x40FF

// Donjon : le jeu principal n'est jamais joué. Ses variables (scripts de ses maps seulement, ou jamais lues) pointent
// toutes vers VAR_MAIN_GAME_JUNK pour que ces scripts compilent encore ; leurs places sont libres (VAR_UNUSED_*).
#define VAR_RECYCLE_GOODS                                VAR_MAIN_GAME_JUNK
#define VAR_FRONTIER_GAMBLER_AMOUNT_BET                  VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_LEVEL                            VAR_MAIN_GAME_JUNK
#define VAR_POKELOT_PRIZE_PLACE                          VAR_MAIN_GAME_JUNK
#define VAR_LITTLEROOT_TOWN_STATE                        VAR_MAIN_GAME_JUNK
#define VAR_OLDALE_TOWN_STATE                            VAR_MAIN_GAME_JUNK
#define VAR_DEWFORD_TOWN_STATE                           VAR_MAIN_GAME_JUNK
#define VAR_LAVARIDGE_TOWN_STATE                         VAR_MAIN_GAME_JUNK
#define VAR_VERDANTURF_TOWN_STATE                        VAR_MAIN_GAME_JUNK
#define VAR_PACIFIDLOG_TOWN_STATE                        VAR_MAIN_GAME_JUNK
#define VAR_PETALBURG_CITY_STATE                         VAR_MAIN_GAME_JUNK
#define VAR_SLATEPORT_CITY_STATE                         VAR_MAIN_GAME_JUNK
#define VAR_MAUVILLE_CITY_STATE                          VAR_MAIN_GAME_JUNK
#define VAR_RUSTBORO_CITY_STATE                          VAR_MAIN_GAME_JUNK
#define VAR_FORTREE_CITY_STATE                           VAR_MAIN_GAME_JUNK
#define VAR_LILYCOVE_CITY_STATE                          VAR_MAIN_GAME_JUNK
#define VAR_EVER_GRANDE_CITY_STATE                       VAR_MAIN_GAME_JUNK
#define VAR_ROUTE101_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE102_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE103_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE104_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE105_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE106_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE107_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE108_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE109_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE110_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE111_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE112_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE113_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE114_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE115_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE116_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE117_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE118_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE119_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE120_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE121_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE122_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE123_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE124_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE125_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE126_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE127_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE128_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE129_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE130_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE131_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE132_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE133_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_ROUTE134_STATE                               VAR_MAIN_GAME_JUNK
#define VAR_LITTLEROOT_HOUSES_STATE_MAY                  VAR_MAIN_GAME_JUNK
#define VAR_BIRCH_LAB_STATE                              VAR_MAIN_GAME_JUNK
#define VAR_PETALBURG_GYM_STATE                          VAR_MAIN_GAME_JUNK
#define VAR_CONTEST_PRIZE_PICKUP                         VAR_MAIN_GAME_JUNK
#define VAR_LITTLEROOT_HOUSES_STATE_BRENDAN              VAR_MAIN_GAME_JUNK
#define VAR_LITTLEROOT_RIVAL_STATE                       VAR_MAIN_GAME_JUNK
#define VAR_BOARD_BRINEY_BOAT_STATE                      VAR_MAIN_GAME_JUNK
#define VAR_DEVON_CORP_3F_STATE                          VAR_MAIN_GAME_JUNK
#define VAR_BRINEY_HOUSE_STATE                           VAR_MAIN_GAME_JUNK
#define VAR_LITTLEROOT_INTRO_STATE                       VAR_MAIN_GAME_JUNK
#define VAR_MAUVILLE_GYM_STATE                           VAR_MAIN_GAME_JUNK
#define VAR_LILYCOVE_MUSEUM_2F_STATE                     VAR_MAIN_GAME_JUNK
#define VAR_BRINEY_LOCATION                              VAR_MAIN_GAME_JUNK
#define VAR_PETALBURG_WOODS_STATE                        VAR_MAIN_GAME_JUNK
#define VAR_LILYCOVE_CONTEST_LOBBY_STATE                 VAR_MAIN_GAME_JUNK
#define VAR_ELITE_4_STATE                                VAR_MAIN_GAME_JUNK
#define VAR_MOSSDEEP_SPACE_CENTER_STAIR_GUARD_STATE      VAR_MAIN_GAME_JUNK
#define VAR_MOSSDEEP_SPACE_CENTER_STATE                  VAR_MAIN_GAME_JUNK
#define VAR_SLATEPORT_HARBOR_STATE                       VAR_MAIN_GAME_JUNK
#define VAR_SEAFLOOR_CAVERN_STATE                        VAR_MAIN_GAME_JUNK
#define VAR_CABLE_CAR_STATION_STATE                      VAR_MAIN_GAME_JUNK
#define VAR_SAFARI_ZONE_STATE                            VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_BEING_WATCHED_STATE              VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_FOUND_TRICK_MASTER               VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_ENTRANCE_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_SLATEPORT_MUSEUM_1F_STATE                    VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_1_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_2_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_3_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_4_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_5_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_6_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_7_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_8_STATE                   VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_ENTER_FROM_CORRIDOR              VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PUZZLE_7_STATE_2                 VAR_MAIN_GAME_JUNK
#define VAR_SLATEPORT_FAN_CLUB_STATE                     VAR_MAIN_GAME_JUNK
#define VAR_MT_PYRE_STATE                                VAR_MAIN_GAME_JUNK
#define VAR_NEW_MAUVILLE_STATE                           VAR_MAIN_GAME_JUNK
#define VAR_JAGGED_PASS_ASH_WEATHER                      VAR_MAIN_GAME_JUNK
#define VAR_GLASS_WORKSHOP_STATE                         VAR_MAIN_GAME_JUNK
#define VAR_METEOR_FALLS_STATE                           VAR_MAIN_GAME_JUNK
#define VAR_SOOTOPOLIS_MYSTERY_EVENTS_STATE              VAR_MAIN_GAME_JUNK
#define VAR_TRICK_HOUSE_PRIZE_PICKUP                     VAR_MAIN_GAME_JUNK
#define VAR_VICTORY_ROAD_1F_STATE                        VAR_MAIN_GAME_JUNK
#define VAR_FOSSIL_RESURRECTION_STATE                    VAR_MAIN_GAME_JUNK
#define VAR_WHICH_FOSSIL_REVIVED                         VAR_MAIN_GAME_JUNK
#define VAR_STEVENS_HOUSE_STATE                          VAR_MAIN_GAME_JUNK
#define VAR_OLDALE_RIVAL_STATE                           VAR_MAIN_GAME_JUNK
#define VAR_JAGGED_PASS_STATE                            VAR_MAIN_GAME_JUNK
#define VAR_SCOTT_PETALBURG_ENCOUNTER                    VAR_MAIN_GAME_JUNK
#define VAR_FOSSIL_MANIAC_STATE                          VAR_MAIN_GAME_JUNK
#define VAR_CABLE_CLUB_TUTORIAL_STATE                    VAR_MAIN_GAME_JUNK
#define VAR_HAS_ENTERED_BATTLE_FRONTIER                  VAR_MAIN_GAME_JUNK
#define VAR_SCOTT_STATE                                  VAR_MAIN_GAME_JUNK
#define VAR_SLATEPORT_OUTSIDE_MUSEUM_STATE               VAR_MAIN_GAME_JUNK
#define VAR_DEX_UPGRADE_JOHTO_STARTER_STATE              VAR_MAIN_GAME_JUNK
#define VAR_SS_TIDAL_SCOTT_STATE                         VAR_MAIN_GAME_JUNK
#define VAR_ROAMER_POKEMON                               VAR_MAIN_GAME_JUNK
#define VAR_SKY_PILLAR_RAYQUAZA_CRY_DONE                 VAR_MAIN_GAME_JUNK
#define VAR_SOOTOPOLIS_WALLACE_STATE                     VAR_MAIN_GAME_JUNK
#define VAR_HAS_TALKED_TO_SEAFLOOR_CAVERN_ENTRANCE_GRUNT VAR_MAIN_GAME_JUNK
#define VAR_REGISTER_BIRCH_STATE                         VAR_MAIN_GAME_JUNK
#define VAR_GIFT_UNUSED_1                                VAR_MAIN_GAME_JUNK
#define VAR_GIFT_UNUSED_2                                VAR_MAIN_GAME_JUNK
#define VAR_GIFT_UNUSED_3                                VAR_MAIN_GAME_JUNK
#define VAR_GIFT_UNUSED_4                                VAR_MAIN_GAME_JUNK
#define VAR_GIFT_UNUSED_5                                VAR_MAIN_GAME_JUNK
#define VAR_GIFT_UNUSED_6                                VAR_MAIN_GAME_JUNK
#define VAR_GIFT_UNUSED_7                                VAR_MAIN_GAME_JUNK
#define VARS_COUNT                                       (VARS_END - VARS_START + 1)

#define SPECIAL_VARS_START            0x8000
// special vars
// They are commonly used as parameters to commands, or return values from commands.
#define VAR_0x8000                    0x8000
#define VAR_0x8001                    0x8001
#define VAR_0x8002                    0x8002
#define VAR_0x8003                    0x8003
#define VAR_0x8004                    0x8004
#define VAR_0x8005                    0x8005
#define VAR_0x8006                    0x8006
#define VAR_0x8007                    0x8007
#define VAR_0x8008                    0x8008
#define VAR_0x8009                    0x8009
#define VAR_0x800A                    0x800A
#define VAR_0x800B                    0x800B
#define VAR_FACING                    0x800C
#define VAR_RESULT                    0x800D
#define VAR_ITEM_ID                   0x800E
#define VAR_LAST_TALKED               0x800F
#define VAR_CONTEST_RANK              0x8010
#define VAR_CONTEST_CATEGORY          0x8011
#define VAR_MON_BOX_ID                0x8012
#define VAR_MON_BOX_POS               0x8013
#define VAR_UNUSED_0x8014             0x8014
#define VAR_TRAINER_BATTLE_OPPONENT_A 0x8015 // Alias of TRAINER_BATTLE_PARAM.opponentA

#define SPECIAL_VARS_END              0x8015

// If an overworld trigger uses this pseudo-variable as the trigger check,
// then the script will be run using RunScriptImmediately instead of in the
// global script context. This means it will run faster, but cannot do any
// cutscenes nor call a wait command. Used for weather effects in vanilla.
#define TRIGGER_RUN_IMMEDIATELY   0

// Temp var aliases
#define VAR_TEMP_CHALLENGE_STATUS  VAR_TEMP_0

#define VAR_TEMP_MIXED_RECORDS         VAR_TEMP_0
#define VAR_TEMP_RECORD_MIX_GIFT_ITEM  VAR_TEMP_1

#define VAR_TEMP_PLAYING_PYRAMID_MUSIC  VAR_TEMP_E

#define VAR_TEMP_FRONTIER_TUTOR_SELECTION  VAR_TEMP_D
#define VAR_TEMP_FRONTIER_TUTOR_ID         VAR_TEMP_E

#define VAR_TEMP_TRANSFERRED_SPECIES  VAR_TEMP_1

#if TESTING
#define TESTING_VARS_START                  0x9000
#define TESTING_VAR_DIFFICULTY              (TESTING_VARS_START + 0x0)
#define TESTING_VAR_TRAINER_SLIDES          (TESTING_VARS_START + 0x1)
#define TESTING_VAR_UNUSED_2                (TESTING_VARS_START + 0x2)
#define TESTING_VAR_UNUSED_3                (TESTING_VARS_START + 0x3)
#define TESTING_VAR_UNUSED_4                (TESTING_VARS_START + 0x4)
#define TESTING_VAR_UNUSED_5                (TESTING_VARS_START + 0x5)
#define TESTING_VAR_UNUSED_6                (TESTING_VARS_START + 0x6)
#define TESTING_VAR_UNUSED_7                (TESTING_VARS_START + 0x7)
#endif // TESTING

#endif // GUARD_CONSTANTS_VARS_H
