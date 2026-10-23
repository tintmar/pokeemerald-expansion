#!/usr/bin/env python3
"""
Vérifie les sets écrits à la main et génère src/data/isaac_sets.h : pour chaque Pokémon du roster (roster.py), 3 paliers
du plus faible au plus fort, et deux variantes A et B de chaque palier (même force, rôles différents :
physique / spécial, offensif / soutien, autre objet...).
  - variante A : tools/isaac/sets_gen1.py, sets_gen2.py, sets_gen3.py, puis les espèces ajoutées au roster par
    biome (sets_cave.py, sets_dark.py...) (dictionnaire SETS) ;
  - variante B : tools/isaac/sets_gen1b.py, sets_gen2b.py, sets_gen3b.py, sets_caveb.py... (dictionnaire SETS, même
    format).
    Tant qu'une espèce n'a pas de variante B, sa variante B reprend la A.

Un set s'écrit sur une ligne : 'TALENT / OBJET / NATURE / RÉPARTITION / ATTAQUE1 ATTAQUE2 ATTAQUE3 ATTAQUE4'
  - OBJET : '-' pour aucun objet ;
  - NATURE : '-' pour le set 1 (nature neutre, Hardi) ;
  - RÉPARTITION : une clé de SPREADS (EVs du set 3, réduits de moitié pour le set 2 et au tiers pour le set 1).

Paliers :
  1. attaques de base, sans objet (celui écrit dans le set est ignoré), nature neutre, EVs au tiers, IVs 15 ;
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
# avec une exception pour les sets aux autres attaques faibles. La puissance d'une attaque à plusieurs coups est celle
# de tous ses coups (gamedata : total).
TIER2_MAX_POWER = 70
# Exception : la plus forte attaque du set peut aller jusqu'à TIER2_EXCEPTION_POWER si toutes les autres attaques
# offensives du set (après remplacement) ont au plus TIER2_WEAK_POWER de puissance
TIER2_EXCEPTION_POWER = 90
TIER2_WEAK_POWER = 60
TIER2_MIN_POWER = 40
TIER2_MIN_ACCURACY = 85
# Attaques jamais choisies en remplacement (effet trop particulier)
NERF_EXCLUDED = {'DREAM_EATER', 'SNORE', 'SPIT_UP', 'NATURAL_GIFT', 'SYNCHRONOISE', 'BELCH', 'STEEL_ROLLER',
                 'LAST_RESORT', 'FUTURE_SIGHT', 'DOOM_DESIRE', 'SKY_DROP', 'BIDE', 'COUNTER', 'MIRROR_COAT',
                 'METAL_BURST', 'ROLLOUT', 'ICE_BALL', 'UPROAR', 'THRASH', 'OUTRAGE', 'PETAL_DANCE', 'RAGE',
                 'RAZOR_WIND', 'SKULL_BASH', 'SKY_ATTACK', 'SOLAR_BEAM', 'SOLAR_BLADE', 'METEOR_BEAM', 'GEOMANCY',
                 'TERA_BLAST',  # pas de Téracristallisation : une simple attaque Normal spéciale
                 'UPPER_HAND', 'FAKE_OUT', 'FOCUS_PUNCH', 'BURN_UP', 'DOUBLE_SHOCK'}  # échouent hors cas précis
TIER_EV_FRACTION = ((1, 3), (1, 2), (1, 1))  # set 2 nettement en dessous du set 3

# Puissance comptée pour les attaques à puissance variable (selon le poids, la Vitesse, les PV, l'amitié...) et les
# attaques K.O. en un coup
VARIABLE_POWER = {'LOW_KICK': 80, 'GRASS_KNOT': 80, 'HEAVY_SLAM': 80, 'HEAT_CRASH': 80, 'GYRO_BALL': 80,
                  'ELECTRO_BALL': 80, 'FLAIL': 100, 'REVERSAL': 100, 'RETURN': 102, 'FRUSTRATION': 102,
                  'HORN_DRILL': 150, 'GUILLOTINE': 150, 'FISSURE': 150, 'SHEER_COLD': 150}
for _mv, _power in VARIABLE_POWER.items():
    g.MOVES[_mv].total = _power

# Attaques de statut trop fortes pour les sets 1 et 2 (réservées au set 3) : remplacées par la première version plus
# faible que l'espèce peut apprendre (voir nerf_status), dans l'ordre de préférence
STATUS_NERFS = {
    'SPORE':           ['SLEEP_POWDER', 'HYPNOSIS', 'SING', 'GRASS_WHISTLE', 'YAWN', 'STUN_SPORE'],
    'DARK_VOID':       ['HYPNOSIS', 'YAWN'],
    'SHELL_SMASH':     ['WORK_UP', 'GROWTH', 'HOWL', 'HONE_CLAWS', 'AGILITY'],
    'QUIVER_DANCE':    ['CALM_MIND', 'WORK_UP', 'GROWTH'],
    'TAIL_GLOW':       ['CALM_MIND', 'WORK_UP', 'GROWTH'],
    'DRAGON_DANCE':    ['HONE_CLAWS', 'WORK_UP', 'HOWL', 'AGILITY'],
    'SHIFT_GEAR':      ['AUTOTOMIZE', 'AGILITY', 'HONE_CLAWS', 'WORK_UP'],
    'VICTORY_DANCE':   ['BULK_UP', 'WORK_UP', 'HOWL'],
    'BELLY_DRUM':      ['BULK_UP', 'HOWL', 'WORK_UP'],
    'CLANGOROUS_SOUL': ['WORK_UP', 'HOWL', 'BULK_UP'],
    'NO_RETREAT':      ['BULK_UP', 'WORK_UP', 'HOWL'],
    'FILLET_AWAY':     ['AGILITY', 'WORK_UP', 'HOWL'],
    'COTTON_GUARD':    ['IRON_DEFENSE', 'ACID_ARMOR', 'BARRIER', 'REFLECT', 'DEFENSE_CURL'],
}

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

# Pas d'entry hazards dans le jeu (ni de Grosses Bottes) : remplacés à la génération (voir remove_hazards)
HAZARD_MOVES = {'STEALTH_ROCK', 'SPIKES', 'TOXIC_SPIKES', 'STICKY_WEB', 'STONE_AXE', 'CEASELESS_EDGE'}
NERF_EXCLUDED |= HAZARD_MOVES
ITEM_REPLACEMENTS = {'HEAVY_DUTY_BOOTS': 'LEFTOVERS'}
HAZARD_MAX_POWER = [60, 70, 90]  # puissance maximale de l'attaque de remplacement, par palier

SET_FILES = ['sets_gen1', 'sets_gen2', 'sets_gen3', 'sets_cave', 'sets_dark', 'sets_fire', 'sets_ice', 'sets_poison',
             'sets_ocean', 'sets_desert', 'sets_cliff', 'sets_forest']
SET_FILES_B = ['sets_gen1b', 'sets_gen2b', 'sets_gen3b', 'sets_caveb', 'sets_darkb', 'sets_fireb', 'sets_iceb',
               'sets_poisonb', 'sets_oceanb', 'sets_desertb', 'sets_cliffb', 'sets_forestb']

errors = []


variant_letter = 'A'


def error(name, tier, msg):
    errors.append(f'{name} (set {tier + 1}{variant_letter}) : {msg}')


def best_replacement(name, move, taken, allow_over=True):
    """Meilleure attaque apprenable de puissance TIER2_MIN_POWER à TIER2_MAX_POWER et de même catégorie (physique /
    spéciale, pour rester cohérente avec le build) : de même type, sinon d'un autre type, de préférence un type que
    le set n'a pas encore. Pour une attaque du type du Pokémon (STAB), on préfère garder le type : faute de mieux, la
    moins puissante de même type jusqu'à TIER2_EXCEPTION_POWER (l'attaque elle-même, au besoin) plutôt qu'une attaque
    d'un autre type. Une seule attaque par set dépasse ainsi TIER2_MAX_POWER (allow_over)."""
    info = g.MOVES[move]
    used_types = {g.MOVES[mv].type for mv in taken if mv in g.MOVES and g.MOVES[mv].cat != 'STATUS'}
    stab = info.type in g.MONS[name].types

    def pick(same_type, low, high, weakest):
        best = None
        for cand in g.learnset(name):
            c = g.MOVES.get(cand)
            if (c is None or (cand in taken and cand != move) or cand in NERF_EXCLUDED or c.cat != info.cat
                    or (same_type and c.type != info.type)
                    or not low <= c.total <= high
                    or (c.acc != 0 and c.acc < TIER2_MIN_ACCURACY)):
                continue
            key = (c.type not in used_types, -c.total if weakest else c.total, c.acc if c.acc else 101, cand)
            if best is None or key > best[0]:
                best = (key, cand)
        return best[1] if best else None

    return (pick(True, TIER2_MIN_POWER, TIER2_MAX_POWER, False)
            or (stab and allow_over and pick(True, TIER2_MAX_POWER + 1, min(info.total, TIER2_EXCEPTION_POWER), True))
            or pick(False, TIER2_MIN_POWER, TIER2_MAX_POWER, False))


def nerf_status(name, moves):
    """Sets 1 et 2 : remplace les attaques de STATUS_NERFS par leur version plus faible (la première apprenable) ;
    renvoie les attaques et les remplacements"""
    learn = g.learnset(name)
    out = list(moves)
    for i, mv in enumerate(moves):
        for weaker in STATUS_NERFS.get(mv, []):
            if weaker not in out and (weaker in learn or name == 'SMEARGLE'):
                out[i] = weaker
                break
    return out, [(a, b) for a, b in zip(moves, out) if a != b]


def nerf_moves(name, moves):
    """Set 2 : remplace les attaques plus puissantes que TIER2_MAX_POWER, sauf l'exception (TIER2_EXCEPTION_POWER) ;
    renvoie les attaques et les remplacements"""
    power = lambda mv: g.MOVES[mv].total if mv in g.MOVES and g.MOVES[mv].cat != 'STATUS' else 0
    strong = sorted((i for i, mv in enumerate(moves) if power(mv) > TIER2_MAX_POWER), key=lambda i: -power(moves[i]))
    out = list(moves)
    for i in strong:
        over = any(power(mv) > TIER2_MAX_POWER for k, mv in enumerate(out) if k != i and k not in strong[strong.index(i):])
        new = best_replacement(name, moves[i], out, not over)
        if new:
            out[i] = new
    # exception : on garde la plus forte (si <= TIER2_EXCEPTION_POWER) quand le reste du set est faible
    if strong and power(moves[strong[0]]) <= TIER2_EXCEPTION_POWER:
        others = [power(mv) for k, mv in enumerate(out) if k != strong[0]]
        if max(others, default=0) <= TIER2_WEAK_POWER and moves[strong[0]] not in out:
            out[strong[0]] = moves[strong[0]]
    return out, [(a, b) for a, b in zip(moves, out) if a != b]


# Set 1 : léger coup de pouce (voir buff_tier1) : une attaque STAB garantie, une attaque de statut trop faible en
# moins ; attaques ajoutées de puissance TIER1_MIN_POWER à TIER1_MAX_POWER
TIER1_MIN_POWER = 35
TIER1_MAX_POWER = 60
WEAK_STATUS = {'GROWL', 'TAIL_WHIP', 'LEER', 'SAND_ATTACK', 'SMOKESCREEN', 'STRING_SHOT', 'HARDEN', 'DEFENSE_CURL',
               'WITHDRAW', 'SPLASH', 'ODOR_SLEUTH', 'FORESIGHT', 'FOCUS_ENERGY', 'SHARPEN', 'MIST', 'KINESIS',
               'FLASH', 'SWEET_SCENT', 'CHARM', 'TICKLE', 'MUD_SPORT', 'WATER_SPORT', 'BABY_DOLL_EYES', 'TEETER_DANCE',
               'GROWTH', 'MEDITATE', 'DOUBLE_TEAM', 'MINIMIZE', 'LOCK_ON', 'MIND_READER', 'CONVERSION'}
PHYS_SPREADS = {'PHYS', 'PHYS_BULK', 'HP_ATK'}
SPEC_SPREADS = {'SPEC', 'SPEC_BULK'}


def best_tier1_move(name, taken, types, category):
    """Meilleure attaque offensive apprenable de puissance TIER1_MIN_POWER à TIER1_MAX_POWER, d'un des types donnés
    (None : n'importe lequel, de préférence un type que le set n'a pas), de la catégorie donnée (None : les deux)"""
    used = {g.MOVES[mv].type for mv in taken if mv in g.MOVES and g.MOVES[mv].cat != 'STATUS'}
    best = None
    for cand in g.learnset(name):
        c = g.MOVES.get(cand)
        if (c is None or cand in taken or cand in NERF_EXCLUDED or c.cat == 'STATUS'
                or cand in ('SELF_DESTRUCT', 'EXPLOSION') or (types and c.type not in types)
                or (category and c.cat != category)
                or not TIER1_MIN_POWER <= c.total <= TIER1_MAX_POWER or (c.acc != 0 and c.acc < 85)):
            continue
        key = (c.type not in used, c.total, c.acc if c.acc else 101, cand)
        if best is None or key > best[0]:
            best = (key, cand)
    return best[1] if best else None


def buff_tier1(name, moves, spread):
    """Set 1 : ajoute une attaque STAB s'il n'en a pas, remplace une attaque de statut trop faible"""
    out = list(moves)
    types = set(g.MONS[name].types)
    category = 'PHYSICAL' if spread in PHYS_SPREADS else ('SPECIAL' if spread in SPEC_SPREADS else None)
    damaging = lambda mv: mv in g.MOVES and g.MOVES[mv].cat != 'STATUS'

    def slot_to_replace(prefer_weak_damaging):
        weak = [i for i, mv in enumerate(out) if mv in WEAK_STATUS]
        if weak:
            return weak[0]
        if prefer_weak_damaging:
            dmg = [i for i, mv in enumerate(out) if damaging(mv) and g.MOVES[mv].type not in types]
            if dmg:
                return min(dmg, key=lambda i: g.MOVES[out[i]].power)
        return len(out) if len(out) < 4 else None

    if not any(damaging(mv) and g.MOVES[mv].type in types for mv in out):
        new = best_tier1_move(name, out, types, category) or best_tier1_move(name, out, types, None)
        i = slot_to_replace(True)
        if new and i is not None:
            out[i:i + 1] = [new]
    if any(mv in WEAK_STATUS for mv in out):  # de préférence une attaque du type du Pokémon
        new = best_tier1_move(name, out, types, category) or best_tier1_move(name, out, None, category)
        i = slot_to_replace(False)
        if new and i is not None and i < len(out):
            out[i] = new
    return out, [(a, b) for a, b in zip(moves + ['-'] * (len(out) - len(moves)), out) if a != b]


def remove_hazards(name, moves, tier, spread):
    """Remplace les entry hazards par une attaque offensive apprenable de la catégorie du build, de préférence d'un
    type que le set n'a pas encore (même type pour une attaque offensive qui pose des pièges) ; sinon les retire"""
    category = 'PHYSICAL' if spread in PHYS_SPREADS else ('SPECIAL' if spread in SPEC_SPREADS else None)
    out = list(moves)
    changes = []
    for i, mv in enumerate(moves):
        if mv not in HAZARD_MOVES:
            continue
        info = g.MOVES[mv]
        used = {g.MOVES[m].type for m in out if m in g.MOVES and g.MOVES[m].cat != 'STATUS'}
        best = None
        for cand in g.learnset(name) if name != 'SMEARGLE' else g.MOVES:
            c = g.MOVES.get(cand)
            if (c is None or cand in out or cand in HAZARD_MOVES or cand in NERF_EXCLUDED or c.cat == 'STATUS'
                    or cand in ('SELF_DESTRUCT', 'EXPLOSION') or (category and c.cat != category)
                    or not TIER1_MIN_POWER <= c.total <= HAZARD_MAX_POWER[tier] or (c.acc != 0 and c.acc < 85)):
                continue
            same = c.type == info.type if info.cat != 'STATUS' else c.type not in used
            key = (same, c.type in g.MONS[name].types, c.total, cand)
            if best is None or key > best[0]:
                best = (key, cand)
        out[i] = best[1] if best else None
        changes.append((mv, out[i] or '-'))
    return [mv for mv in out if mv], changes


nerf_log = []
buff_log = []
status_log = []
hazard_log = []


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

    item = ITEM_REPLACEMENTS.get(item, item)
    moves, changes = remove_hazards(name, moves, tier, spread)
    hazard_log.extend((name, f'{tier + 1}{variant_letter}', a, b) for a, b in changes)
    if tier < 2:
        moves, changes = nerf_status(name, moves)
        status_log.extend((name, f'{tier + 1}{variant_letter}', a, b) for a, b in changes)
    if tier == 1:
        moves, changes = nerf_moves(name, moves)
        nerf_log.extend((name, variant_letter, a, b) for a, b in changes)
    if tier == 0 and name not in ('DITTO', 'SMEARGLE', 'UNOWN', 'WOBBUFFET', 'WYNAUT'):
        moves, changes = buff_tier1(name, moves, spread)
        buff_log.extend((name, variant_letter, a, b) for a, b in changes)

    num, den = TIER_EV_FRACTION[tier]
    evs = [v * num // den // 4 * 4 for v in SPREADS[spread]]
    if tier == 0:  # set 1 : faible, sans objet
        item = '-'
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
            # set 1 : sans objet, A et B peuvent être identiques pour un Pokémon qui n'a presque pas d'attaques
            if tier > 0 and a and b and a['moves'] == b['moves'] and a['item'] == b['item'] and name != 'UNOWN':
                errors.append(f'{name} (set {tier + 1}) : variantes A et B identiques (attaques et objet)')
    return {name: [[a, sets_b.get(name, parsed)[tier]] for tier, a in enumerate(parsed)]
            for name, parsed in sets_a.items()}, len(sets_b)


def write_header(sets):
    out = ['// Fichier généré par tools/isaac/build_sets.py à partir de tools/isaac/sets_gen*.py :',
           '// ne pas modifier à la main.',
           '// Par Pokémon du roster : 3 paliers, du plus faible (0) au plus fort (2), en deux variantes (A, B).',
           '// sIsaacSetIndex : rang des sets de l\'espèce dans sIsaacSets, plus 1 (0 : pas de sets).',
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
           'static const struct TrainerMon sIsaacSets[][ISAAC_SET_TIERS][ISAAC_SET_VARIANTS] =',
           '{']
    names = [n for n in g.ORDER if n in sets]
    for name in names:
        out.append(f'    // {name}')
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
    out.append('')
    out.append('static const u16 sIsaacSetIndex[NUM_SPECIES] =')
    out.append('{')
    out.extend(f'    [SPECIES_{name}] = {i + 1},' for i, name in enumerate(names))
    out.append('};')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_sets.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')


def main():
    sets, count_b = load_all_sets()
    missing = [n for n in g.ORDER if n not in sets]
    for e in errors:
        print(e)
    print(f'{len(sets)} espèces, {len(missing)} sans sets, {count_b} avec une variante B, {len(errors)} erreurs')
    print(f'set 2 : {len(nerf_log)} attaques remplacées (puissance > {TIER2_MAX_POWER})')
    print(f'set 1 : {len(buff_log)} attaques remplacées ou ajoutées (STAB, statut trop faible)')
    print(f'sets 1 et 2 : {len(status_log)} attaques de statut trop fortes remplacées')
    print(f'tous les sets : {len(hazard_log)} entry hazards remplacés')
    if '--hazard-log' in sys.argv:
        for name, tier, old, new in hazard_log:
            print(f'  {name} ({tier}) : {old} -> {new}')
    if '--status-log' in sys.argv:
        for name, tier, old, new in status_log:
            print(f'  {name} ({tier}) : {old} -> {new}')
    if '--buff-log' in sys.argv:
        for name, letter, old, new in buff_log:
            print(f'  {name} (1{letter}) : {old} -> {new}')
    if '--nerf-log' in sys.argv:
        for name, letter, old, new in nerf_log:
            print(f'  {name} (2{letter}) : {old} -> {new}')
    if '--check' not in sys.argv and not errors:
        write_header(sets)
        print('src/data/isaac_sets.h écrit')
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
