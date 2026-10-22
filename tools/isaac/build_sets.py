#!/usr/bin/env python3
"""
Vérifie les sets écrits à la main et génère src/data/isaac_sets.h : pour chaque Pokémon (1 à 386), 3 paliers
du plus faible au plus fort, et deux variantes A et B de chaque palier (même force, rôles différents :
physique / spécial, offensif / soutien, autre objet...).
  - variante A : tools/isaac/sets_gen1.py, sets_gen2.py, sets_gen3.py (dictionnaire SETS) ;
  - variante B : tools/isaac/sets_gen1b.py, sets_gen2b.py, sets_gen3b.py (dictionnaire SETS, même format).
    Tant qu'une espèce n'a pas de variante B, sa variante B reprend la A.

Un set s'écrit sur une ligne : 'TALENT / OBJET / NATURE / RÉPARTITION / ATTAQUE1 ATTAQUE2 ATTAQUE3 ATTAQUE4'
  - OBJET : '-' pour aucun objet ;
  - NATURE : '-' pour le set 1 (nature neutre, Hardi) ;
  - RÉPARTITION : une clé de SPREADS (EVs du set 3, réduits de moitié pour le set 2 et au tiers pour le set 1).

Paliers :
  1. attaques de base, petite baie, nature neutre, EVs au tiers, IVs 15 ;
  2. bonnes attaques, objet correct (Baie Sitrus, objet de type, Restes...), EVs à moitié, IVs 20 ;
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
TIER_IVS = (15, 20, 31)
# Set 2 : attaques offensives plafonnées en puissance (les plus fortes sont réservées au set 3). Chaque attaque plus
# puissante que TIER2_MAX_POWER est remplacée par la meilleure que l'espèce peut apprendre, de même type et de même
# catégorie, de puissance TIER2_MIN_POWER à TIER2_MAX_POWER et de précision >= TIER2_MIN_ACCURACY (voir nerf_moves),
# avec une exception pour les sets aux autres attaques faibles.
TIER2_MAX_POWER = 80
# Exception : la plus forte attaque du set peut aller jusqu'à TIER2_EXCEPTION_POWER si toutes les autres attaques
# offensives du set (après remplacement) ont au plus TIER2_WEAK_POWER de puissance
TIER2_EXCEPTION_POWER = 100
TIER2_WEAK_POWER = 65
TIER2_MIN_POWER = 40
TIER2_MIN_ACCURACY = 85
# Attaques jamais choisies en remplacement (effet trop particulier)
NERF_EXCLUDED = {'DREAM_EATER', 'SNORE', 'SPIT_UP', 'NATURAL_GIFT', 'SYNCHRONOISE', 'BELCH', 'STEEL_ROLLER',
                 'LAST_RESORT', 'FUTURE_SIGHT', 'DOOM_DESIRE', 'SKY_DROP', 'BIDE', 'COUNTER', 'MIRROR_COAT',
                 'METAL_BURST', 'ROLLOUT', 'ICE_BALL', 'UPROAR', 'THRASH', 'OUTRAGE', 'PETAL_DANCE', 'RAGE',
                 'RAZOR_WIND', 'SKULL_BASH', 'SKY_ATTACK', 'SOLAR_BEAM', 'SOLAR_BLADE', 'METEOR_BEAM', 'GEOMANCY',
                 'TERA_BLAST'}  # pas de Téracristallisation : une simple attaque Normal spéciale
TIER_EV_FRACTION = ((1, 3), (1, 2), (1, 1))  # set 2 nettement en dessous du set 3

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
SET_FILES_B = ['sets_gen1b', 'sets_gen2b', 'sets_gen3b']

errors = []


variant_letter = 'A'


def error(name, tier, msg):
    errors.append(f'{name} (set {tier + 1}{variant_letter}) : {msg}')


def best_replacement(name, move, taken):
    """Meilleure attaque apprenable de puissance TIER2_MIN_POWER à TIER2_MAX_POWER et de même catégorie (physique /
    spéciale, pour rester cohérente avec le build) : de même type, sinon d'un autre type, de préférence un type que
    le set n'a pas encore"""
    info = g.MOVES[move]
    used_types = {g.MOVES[mv].type for mv in taken if mv in g.MOVES and g.MOVES[mv].cat != 'STATUS'}
    for same_type in (True, False):
        best = None
        for cand in g.learnset(name):
            c = g.MOVES.get(cand)
            if (c is None or cand in taken or cand in NERF_EXCLUDED or c.cat != info.cat
                    or (same_type and c.type != info.type)
                    or not TIER2_MIN_POWER <= c.power <= TIER2_MAX_POWER
                    or (c.acc != 0 and c.acc < TIER2_MIN_ACCURACY)):
                continue
            key = (c.type not in used_types, c.power, c.acc if c.acc else 101, cand)
            if best is None or key > best[0]:
                best = (key, cand)
        if best:
            return best[1]
    return None


def nerf_moves(name, moves):
    """Set 2 : remplace les attaques plus puissantes que TIER2_MAX_POWER, sauf l'exception (TIER2_EXCEPTION_POWER) ;
    renvoie les attaques et les remplacements"""
    power = lambda mv: g.MOVES[mv].power if mv in g.MOVES and g.MOVES[mv].cat != 'STATUS' else 0
    strong = sorted((i for i, mv in enumerate(moves) if power(mv) > TIER2_MAX_POWER), key=lambda i: -power(moves[i]))
    out = list(moves)
    for i in strong:
        new = best_replacement(name, moves[i], out)
        if new:
            out[i] = new
    # exception : on garde la plus forte (si <= TIER2_EXCEPTION_POWER) quand le reste du set est faible
    if strong and power(moves[strong[0]]) <= TIER2_EXCEPTION_POWER:
        others = [power(mv) for k, mv in enumerate(out) if k != strong[0]]
        if max(others, default=0) <= TIER2_WEAK_POWER and moves[strong[0]] not in out:
            out[strong[0]] = moves[strong[0]]
    return out, [(a, b) for a, b in zip(moves, out) if a != b]


nerf_log = []


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

    if tier == 1:
        moves, changes = nerf_moves(name, moves)
        nerf_log.extend((name, variant_letter, a, b) for a, b in changes)

    num, den = TIER_EV_FRACTION[tier]
    evs = [v * num // den // 4 * 4 for v in SPREADS[spread]]
    return {'ability': ability, 'item': item if item != '-' else 'NONE', 'nature': nature, 'evs': evs,
            'iv': TIER_IVS[tier], 'moves': moves + ['NONE'] * (4 - len(moves))}


def load_sets(files):
    sets = {}
    for module in files:
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


def load_all_sets():
    global variant_letter
    variant_letter = 'A'
    sets_a = load_sets(SET_FILES)
    variant_letter = 'B'
    sets_b = load_sets(SET_FILES_B)
    for name, parsed in sets_b.items():
        if name not in sets_a:
            errors.append(f'{name} : variante B sans variante A')
            continue
        for tier, (a, b) in enumerate(zip(sets_a[name], parsed)):
            if a and b and a['moves'] == b['moves'] and a['item'] == b['item'] and name != 'UNOWN':
                errors.append(f'{name} (set {tier + 1}) : variantes A et B identiques (attaques et objet)')
    return {name: [[a, sets_b.get(name, parsed)[tier]] for tier, a in enumerate(parsed)]
            for name, parsed in sets_a.items()}, len(sets_b)


def write_header(sets):
    out = ['// Fichier généré par tools/isaac/build_sets.py à partir de tools/isaac/sets_gen*.py :',
           '// ne pas modifier à la main.',
           '// Par Pokémon (1 à 386) : 3 paliers, du plus faible (0) au plus fort (2), en deux variantes (A, B).',
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
           'static const struct TrainerMon sIsaacSets[SPECIES_DEOXYS + 1][ISAAC_SET_TIERS][ISAAC_SET_VARIANTS] =',
           '{']
    for name in g.ORDER:
        if name not in sets:
            continue
        out.append(f'    [SPECIES_{name}] =')
        out.append('    {')
        for variants in sets[name]:
            out.append('        {')
            for s in variants:
                evs = ', '.join(map(str, s['evs']))
                moves = ', '.join(s['moves'])
                out.append(f"            ISAAC_SET({name}, {s['ability']}, {s['item']}, {s['nature']}, {s['iv']}, {evs}, {moves}),")
            out.append('        },')
        out.append('    },')
    out.append('};')
    out.append('')
    out.append('#undef ISAAC_SET')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_sets.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')


def main():
    sets, count_b = load_all_sets()
    missing = [n for n in g.ORDER if n not in sets]
    for e in errors:
        print(e)
    print(f'{len(sets)} espèces, {len(missing)} sans sets, {count_b} avec une variante B, {len(errors)} erreurs')
    print(f'set 2 : {len(nerf_log)} attaques remplacées (puissance > {TIER2_MAX_POWER})')
    if '--nerf-log' in sys.argv:
        for name, letter, old, new in nerf_log:
            print(f'  {name} (2{letter}) : {old} -> {new}')
    if '--check' not in sys.argv and not errors:
        write_header(sets)
        print('src/data/isaac_sets.h écrit')
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
