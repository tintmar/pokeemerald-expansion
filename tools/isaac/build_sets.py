#!/usr/bin/env python3
"""
Vérifie les sets écrits à la main (tools/isaac/sets_gen1.py, sets_gen2.py, sets_gen3.py) et génère
src/data/isaac_sets.h : 3 sets par Pokémon (1 à 386), du plus faible au plus fort.

Un set s'écrit sur une ligne : 'TALENT / OBJET / NATURE / RÉPARTITION / ATTAQUE1 ATTAQUE2 ATTAQUE3 ATTAQUE4'
  - OBJET : '-' pour aucun objet ;
  - NATURE : '-' pour le set 1 (nature neutre, Hardi) ;
  - RÉPARTITION : une clé de SPREADS (EVs du set 3, réduits aux 2/3 pour le set 2 et au tiers pour le set 1).

Paliers :
  1. attaques de base, petite baie, nature neutre, EVs au tiers, IVs 15 ;
  2. bonnes attaques, objet correct (Baie Sitrus, objet de type, Restes...), EVs aux 2/3, IVs 24 ;
  3. set compétitif (Orbe Vie, Choix, Restes...), EVs complets, IVs 31. Pas d'Évoluroc.

Usage : python3 tools/isaac/build_sets.py [--check]   (--check : vérifie sans écrire le header)
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g

# EVs du set 3 : PV, Attaque, Défense, Vitesse, Attaque Spé., Défense Spé.
SPREADS = {
    'PHYS':      (4, 252, 0, 252, 0, 0),    # attaquant physique rapide
    'SPEC':      (4, 0, 0, 252, 252, 0),    # attaquant spécial rapide
    'MIXED':     (0, 128, 0, 252, 128, 0),  # attaquant mixte rapide
    'PHYS_BULK': (252, 252, 4, 0, 0, 0),    # attaquant physique lent
    'SPEC_BULK': (252, 0, 0, 0, 252, 4),    # attaquant spécial lent
    'DEF':       (252, 0, 252, 0, 0, 4),    # défensif physique
    'SPDEF':     (252, 0, 4, 0, 0, 252),    # défensif spécial
    'BAL':       (252, 0, 128, 0, 0, 128),  # défensif équilibré
    'HP_SPE':    (252, 0, 4, 252, 0, 0),    # soutien rapide
    'HP_ATK':    (252, 252, 0, 4, 0, 0),    # = PHYS_BULK, Vitesse plutôt que Défense
}
TIER_IVS = (15, 24, 31)
TIER_EV_FRACTION = ((1, 3), (2, 3), (1, 1))

# Objets réservés aux sets les plus forts
TOP_ITEMS = {'LIFE_ORB', 'CHOICE_BAND', 'CHOICE_SPECS', 'CHOICE_SCARF', 'ASSAULT_VEST', 'FOCUS_SASH',
             'WEAKNESS_POLICY', 'LIGHT_BALL', 'THICK_CLUB', 'HEAVY_DUTY_BOOTS', 'ROCKY_HELMET', 'LOADED_DICE',
             'BOOSTER_ENERGY', 'CLEAR_AMULET', 'COVERT_CLOAK', 'EXPERT_BELT', 'SOUL_DEW', 'LEEK', 'STICK'}
MID_ITEMS = {'LEFTOVERS', 'SITRUS_BERRY', 'LUM_BERRY', 'BLACK_SLUDGE', 'FLAME_ORB', 'TOXIC_ORB',
             'MUSCLE_BAND', 'WISE_GLASSES', 'SCOPE_LENS', 'METAL_POWDER', 'QUICK_POWDER', 'DEEP_SEA_TOOTH',
             'DEEP_SEA_SCALE', 'LUCKY_PUNCH', 'SHELL_BELL', 'KINGS_ROCK', 'RAZOR_CLAW', 'WIDE_LENS', 'ZOOM_LENS',
             'BRIGHT_POWDER', 'LAX_INCENSE', 'DAMP_ROCK', 'HEAT_ROCK', 'SMOOTH_ROCK', 'ICY_ROCK', 'LIGHT_CLAY',
             'MENTAL_HERB', 'WHITE_HERB', 'POWER_HERB', 'AIR_BALLOON', 'RED_CARD', 'EJECT_BUTTON', 'EJECT_PACK',
             'SALAC_BERRY', 'LIECHI_BERRY', 'PETAYA_BERRY', 'APICOT_BERRY', 'GANLON_BERRY', 'STARF_BERRY',
             'CUSTAP_BERRY', 'IAPAPA_BERRY', 'WIKI_BERRY', 'AGUAV_BERRY', 'FIGY_BERRY', 'MAGO_BERRY',
             'BLUNDER_POLICY', 'THROAT_SPRAY', 'ROOM_SERVICE', 'ADRENALINE_ORB', 'PROTECTIVE_PADS', 'PUNCHING_GLOVE',
             'TERRAIN_EXTENDER', 'GRIP_CLAW', 'BINDING_BAND', 'BIG_ROOT', 'SHED_SHELL', 'FLOAT_STONE', 'METRONOME',
             'QUICK_CLAW', 'FOCUS_BAND', 'SOFT_SAND', 'HARD_STONE', 'MIRACLE_SEED', 'BLACK_GLASSES', 'BLACK_BELT',
             'MAGNET', 'MYSTIC_WATER', 'SHARP_BEAK', 'POISON_BARB', 'NEVER_MELT_ICE', 'SPELL_TAG', 'TWISTED_SPOON',
             'CHARCOAL', 'DRAGON_FANG', 'SILK_SCARF', 'SILVER_POWDER', 'METAL_COAT', 'FAIRY_FEATHER', 'SEA_INCENSE',
             'ROSE_INCENSE', 'ODD_INCENSE', 'ROCK_INCENSE', 'WAVE_INCENSE'}

# Objets interdits : l'Évoluroc ne sert plus à rien dès que le Pokémon évolue
FORBIDDEN_ITEMS = {'EVIOLITE'}

SET_FILES = ['sets_gen1', 'sets_gen2', 'sets_gen3']

errors = []


def error(name, tier, msg):
    errors.append(f'{name} (set {tier + 1}) : {msg}')


def parse_set(name, tier, line):
    fields = [f.strip() for f in line.split('/')]
    if len(fields) != 5:
        error(name, tier, f'format : {line!r}')
        return None
    ability, item, nature, spread, moves = fields
    moves = moves.split()
    mon = g.MONS[name]
    learn = g.learnset(name)

    if ability not in mon.abilities:
        error(name, tier, f'talent {ability} (possibles : {", ".join(mon.abilities)})')
    if item != '-' and item not in g.ITEMS:
        error(name, tier, f'objet {item} inconnu')
    if item in FORBIDDEN_ITEMS:
        error(name, tier, f'objet {item} interdit')
    if tier == 0 and item in TOP_ITEMS | MID_ITEMS:
        error(name, tier, f'objet {item} trop fort pour le set 1')
    if tier == 1 and item in TOP_ITEMS:
        error(name, tier, f'objet {item} trop fort pour le set 2')
    if tier == 0:
        if nature != '-':
            error(name, tier, 'le set 1 a une nature neutre : « - »')
        nature = 'HARDY'
    elif nature not in g.NATURES:
        error(name, tier, f'nature {nature}')
    if spread not in SPREADS:
        error(name, tier, f'répartition {spread}')
        spread = 'PHYS'
    if not 1 <= len(moves) <= 4:
        error(name, tier, f'{len(moves)} attaques')
    if len(set(moves)) != len(moves):
        error(name, tier, 'attaque en double')
    for mv in moves:
        if mv not in g.MOVES:
            error(name, tier, f'attaque {mv} inconnue')
        elif mv not in learn and name != 'SMEARGLE':  # Queulorior : attaques obtenues par Gribouille
            error(name, tier, f'{mv} non apprenable')

    num, den = TIER_EV_FRACTION[tier]
    evs = [v * num // den // 4 * 4 for v in SPREADS[spread]]
    return {'ability': ability, 'item': item if item != '-' else 'NONE', 'nature': nature, 'evs': evs,
            'iv': TIER_IVS[tier], 'moves': moves + ['NONE'] * (4 - len(moves))}


def load_sets():
    sets = {}
    for module in SET_FILES:
        try:
            mod = __import__(module)
        except ModuleNotFoundError:
            continue
        for name, lines in mod.SETS.items():
            if name not in g.MONS:
                errors.append(f'{name} : espèce inconnue ({module})')
                continue
            if name in sets:
                errors.append(f'{name} : défini deux fois')
            if len(lines) != 3:
                errors.append(f'{name} : {len(lines)} sets au lieu de 3')
            parsed = [parse_set(name, tier, line) for tier, line in enumerate(lines)]
            if len({tuple(p['moves']) + (p['item'],) for p in parsed if p}) < len(parsed) and name != 'UNOWN':
                errors.append(f'{name} : deux sets identiques (attaques et objet)')
            sets[name] = parsed
    return sets


def write_header(sets):
    out = ['// Fichier généré par tools/isaac/build_sets.py à partir de tools/isaac/sets_gen*.py :',
           '// ne pas modifier à la main.',
           '// 3 sets par Pokémon (1 à 386), du plus faible (0) au plus fort (2).',
           '',
           '// ISAAC_SET(espèce, talent, objet, nature, IVs, EVs PV / Att / Déf / Vit / AttSp / DéfSp, 4 attaques)',
           '#define ISAAC_SET(sp, ab, it, nat, ivs, hp, at, df, spe, sa, sd, m1, m2, m3, m4)  \\',
           '    {                                                                         \\',
           '        .species = SPECIES_##sp,                                              \\',
           '        .ability = ABILITY_##ab,                                              \\',
           '        .heldItem = ITEM_##it,                                                \\',
           '        .nature = NATURE_##nat,                                               \\',
           '        .ev = TRAINER_PARTY_EVS(hp, at, df, spe, sa, sd),                     \\',
           '        .iv = TRAINER_PARTY_IVS(ivs, ivs, ivs, ivs, ivs, ivs),                \\',
           '        .moves = { MOVE_##m1, MOVE_##m2, MOVE_##m3, MOVE_##m4 },              \\',
           '        .gender = TRAINER_MON_RANDOM_GENDER,                                  \\',
           '        .ball = POKEBALL_COUNT,                                               \\',
           '        .dynamaxLevel = MAX_DYNAMAX_LEVEL,                                    \\',
           '    }',
           '',
           'static const struct TrainerMon sIsaacSets[SPECIES_DEOXYS + 1][ISAAC_SET_TIERS] =',
           '{']
    for name in g.ORDER:
        if name not in sets:
            continue
        out.append(f'    [SPECIES_{name}] =')
        out.append('    {')
        for s in sets[name]:
            evs = ', '.join(map(str, s['evs']))
            moves = ', '.join(s['moves'])
            out.append(f"        ISAAC_SET({name}, {s['ability']}, {s['item']}, {s['nature']}, {s['iv']}, {evs}, {moves}),")
        out.append('    },')
    out.append('};')
    out.append('')
    out.append('#undef ISAAC_SET')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_sets.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')


def main():
    sets = load_sets()
    missing = [n for n in g.ORDER if n not in sets]
    for e in errors:
        print(e)
    print(f'{len(sets)} espèces, {len(missing)} sans sets, {len(errors)} erreurs')
    if '--check' not in sys.argv and not errors:
        write_header(sets)
        print('src/data/isaac_sets.h écrit')
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
