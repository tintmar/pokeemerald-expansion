#ifndef GUARD_CONSTANTS_TMS_HMS_H
#define GUARD_CONSTANTS_TMS_HMS_H

#define FOREACH_TM(F) \
    F(FOCUS_PUNCH) \
    F(DRAGON_CLAW) \
    F(WATER_PULSE) \
    F(CALM_MIND) \
    F(ROAR) \
    F(TOXIC) \
    F(HAIL) \
    F(BULK_UP) \
    F(BULLET_SEED) \
    F(HIDDEN_POWER) \
    F(SUNNY_DAY) \
    F(TAUNT) \
    F(ICE_BEAM) \
    F(BLIZZARD) \
    F(HYPER_BEAM) \
    F(LIGHT_SCREEN) \
    F(PROTECT) \
    F(RAIN_DANCE) \
    F(GIGA_DRAIN) \
    F(SAFEGUARD) \
    F(FRUSTRATION) \
    F(SOLAR_BEAM) \
    F(IRON_TAIL) \
    F(THUNDERBOLT) \
    F(THUNDER) \
    F(EARTHQUAKE) \
    F(RETURN) \
    F(DIG) \
    F(PSYCHIC) \
    F(SHADOW_BALL) \
    F(BRICK_BREAK) \
    F(DOUBLE_TEAM) \
    F(REFLECT) \
    F(SHOCK_WAVE) \
    F(FLAMETHROWER) \
    F(SLUDGE_BOMB) \
    F(SANDSTORM) \
    F(FIRE_BLAST) \
    F(ROCK_TOMB) \
    F(AERIAL_ACE) \
    F(TORMENT) \
    F(FACADE) \
    F(SECRET_POWER) \
    F(REST) \
    F(ATTRACT) \
    F(THIEF) \
    F(STEEL_WING) \
    F(SKILL_SWAP) \
    F(SNATCH) \
    F(OVERHEAT) \
    /* Donjon : CT des générations suivantes (TM51 et après), vendues par Kecleon (voir isaac_dungeon.c) */ \
    F(CLOSE_COMBAT) \
    F(KNOCK_OFF) \
    F(U_TURN) \
    F(STONE_EDGE) \
    F(SCALD) \
    F(VOLT_SWITCH) \
    F(DAZZLING_GLEAM) \
    F(SWORDS_DANCE) \
    F(NASTY_PLOT) \
    F(DRAGON_DANCE) \
    F(FLARE_BLITZ) \
    F(PLAY_ROUGH) \
    F(ENERGY_BALL) \
    F(FOCUS_BLAST) \
    F(FLASH_CANNON) \
    F(EARTH_POWER) \
    F(HURRICANE) \
    F(DRAGON_PULSE) \
    F(POISON_JAB) \
    F(X_SCISSOR) \
    F(IRON_HEAD) \
    F(LIQUIDATION) \
    F(BUG_BUZZ) \
    F(DARK_PULSE) \
    F(ROOST)

#define FOREACH_HM(F) \
    F(CUT) \
    F(FLY) \
    F(SURF) \
    F(STRENGTH) \
    F(FLASH) \
    F(ROCK_SMASH) \
    F(WATERFALL) \
    F(DIVE)

#define FOREACH_TMHM(F) \
    FOREACH_TM(F) \
    FOREACH_HM(F)

#endif
