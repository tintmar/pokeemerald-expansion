#!/usr/bin/env python3
"""
Sets par biome d'après data/pokemon_sets_<biome>.txt (draft v3) : pour chaque espèce du biome (biome_roster.py),
4 tiers de sets (1 début de partie ... 4 boss / joueur), chacun en deux variantes A et B.
Les fichiers donnent le talent, l'objet, la nature et les attaques ; les IVs et les EVs sont calculés ici :
  - IVs progressifs par tier (TIER_IVS) ;
  - EVs cohérents avec le set (voir spread : stat d'attaque des attaques offensives, défense selon la nature et le
    rôle, Vitesse sauf nature qui la baisse), proportion croissante par tier (TIER_EV_FRACTION).

SETS : espèce -> [tier 1..4][variante A, B] -> {'ability', 'item', 'nature', 'evs', 'iv', 'moves'} (format de
build_sets.py). ERRORS : noms inconnus du jeu (bloquants). WARNINGS : attaque non apprenable, talent non disponible,
tier d'attaque différent de move_tiers.py, règle de tier non respectée.

Usage : python3 tools/isaac/biome_sets.py   (affiche les erreurs, avertissements et EVs calculés)
"""
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g
import biome_roster
import move_tiers

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data')
BIOME_FILES = {b: f'pokemon_sets_{b.lower()}.txt' for b in biome_roster.BIOMES}

TIER_IVS = (10, 17, 24, 31)
TIER_EV_FRACTION = ((1, 4), (1, 2), (3, 4), (1, 1))
FORBIDDEN_ITEMS = {'EVIOLITE'}

# Natures : stat augmentée, stat baissée (indices de SPREAD : PV, Att, Déf, Vit, AttSp, DéfSp)
HP, ATK, DEF, SPE, SPA, SPD = range(6)
NATURE_STATS = {
    'LONELY': (ATK, DEF), 'BRAVE': (ATK, SPE), 'ADAMANT': (ATK, SPA), 'NAUGHTY': (ATK, SPD),
    'BOLD': (DEF, ATK), 'RELAXED': (DEF, SPE), 'IMPISH': (DEF, SPA), 'LAX': (DEF, SPD),
    'TIMID': (SPE, ATK), 'HASTY': (SPE, DEF), 'JOLLY': (SPE, SPA), 'NAIVE': (SPE, SPD),
    'MODEST': (SPA, ATK), 'MILD': (SPA, DEF), 'QUIET': (SPA, SPE), 'RASH': (SPA, SPD),
    'CALM': (SPD, ATK), 'GENTLE': (SPD, DEF), 'SASSY': (SPD, SPE), 'CAREFUL': (SPD, SPA),
}
# Attaques offensives qui n'utilisent pas l'Attaque / l'Attaque Spé. du lanceur
NO_ATTACK_STAT = {'FOUL_PLAY', 'BODY_PRESS', 'SEISMIC_TOSS', 'NIGHT_SHADE', 'SUPER_FANG', 'FINAL_GAMBIT', 'COUNTER',
                  'MIRROR_COAT', 'METAL_BURST', 'ENDEAVOR', 'PSYWAVE', 'DRAGON_RAGE', 'SONIC_BOOM', 'RUINATION'}
# Rôles défensifs (ligne « Variants » du fichier) : servent pour les sets de nature neutre
DEFENSIVE_ROLE = re.compile(r'Defensive|Stall|Support|wall', re.I)

SETS, ERRORS, WARNINGS = {}, [], []


def constant(name):
    return move_tiers.move_constant(name)


def spread(name, nature, moves, role, profile):
    """EVs du tier 4 (252 / 252 / 4) d'après le set ; role : rôle de la variante (« Physical », « Defensive »...),
    profile : profil de l'espèce (« Offensive (special) »...), utilisés pour les natures neutres"""
    mon = g.MONS[name]
    up, down = NATURE_STATS.get(nature, (None, None))
    phys = [mv for mv in moves if mv in g.MOVES and g.MOVES[mv].cat == 'PHYSICAL' and mv not in NO_ATTACK_STAT]
    spec = [mv for mv in moves if mv in g.MOVES and g.MOVES[mv].cat == 'SPECIAL' and mv not in NO_ATTACK_STAT]
    evs = [0] * 6

    # Stat d'attaque : celle de la nature, sinon celle du rôle ou du profil, sinon celle de la majorité des attaques
    # (à égalité : la meilleure de base)
    hint = next((stat for text in (role, profile) for word, stat in (('physical', ATK), ('special', SPA))
                 if re.search(word, text, re.I) and not re.search('physical' if stat == SPA else 'special', text, re.I)),
                None)
    if up in (ATK, SPA) and (phys if up == ATK else spec):
        attack = up
    elif up is None and hint is not None and (phys if hint == ATK else spec):
        attack = hint
    elif phys or spec:
        attack = ATK if (len(phys), mon.atk) > (len(spec), mon.spa) else SPA
    else:
        attack = None
    defensive = up in (DEF, SPD) or (up is None and DEFENSIVE_ROLE.search(role)) or attack is None
    if 'BODY_PRESS' in moves:
        defensive = True
    fast = down != SPE and (up == SPE or mon.spe >= 60)

    if name == 'SHEDINJA':  # 1 PV : rien à gagner en PV ni en défense
        evs[ATK], evs[SPE], evs[DEF] = 252, 252, 4
    elif defensive:
        evs[HP] = 252
        if 'BODY_PRESS' in moves or up == DEF:
            evs[DEF], evs[SPD] = 252, 4
        elif up == SPD:
            evs[SPD], evs[DEF] = 252, 4
        elif up == SPE:  # soutien rapide
            evs[SPE], evs[DEF] = 252, 4
        else:  # défensif équilibré
            evs[DEF], evs[SPD] = 128, 128
    elif up == SPE and attack is not None and phys and spec and down not in (ATK, SPA):  # mixte rapide
        evs[ATK], evs[SPA], evs[SPE] = 128, 128, 252
    else:
        evs[attack] = 252
        if fast:
            evs[SPE], evs[HP] = 252, 4
        else:  # attaquant lent (ou Distorsion) : PV
            evs[HP], evs[DEF if mon.dfn <= mon.spd else SPD] = 252, 4
    return evs


def check_tier_rules(name, label, tier, tiers, moves):
    """Règles de tier du fichier (« HOW TO READ ») : tier 1 = T1 + au plus une T2 ; tier 2 = T2 / Sit + au plus une T3 ;
    tier 3 = T3 / Sit + au plus une T4 ; tier 4 = T3 / T4 / Sit. Exception : une attaque [Pivot] de T2 aux tiers 3 et 4."""
    over = {1: 2, 2: 3, 3: 4, 4: None}[tier]
    allowed = {1: {1}, 2: {2, 'SITUATIONAL'}, 3: {3, 'SITUATIONAL'}, 4: {3, 4, 'SITUATIONAL'}}[tier]
    n_over = 0
    for mv, t in zip(moves, tiers):
        rec = move_tiers.MOVES.get(mv)
        pivot = rec is not None and 'Pivot' in rec.tags and t == 2 and tier >= 3
        if t == over:
            n_over += 1
        elif t not in allowed and not pivot and not (tier == 1 and t == 'SITUATIONAL'):
            WARNINGS.append(f'{name} [{label}] : {mv} (T{t}) hors des tiers du set {tier}')
    if n_over > 1:
        WARNINGS.append(f'{name} [{label}] : {n_over} attaques de tier {over}')


def parse_file(biome, path):
    text = open(path, encoding='utf-8').read()
    blocks = re.split(r'\n=+\n(?=#\d+ )', text)[1:]
    species = [n for n, _ in biome_roster.BIOMES[biome]]
    if len(blocks) != len(species):
        ERRORS.append(f'{biome} : {len(blocks)} Pokémon dans le fichier, {len(species)} dans biome_roster.py')
    for name, block in zip(species, blocks):
        head = re.match(r'#\d+ (\S+)', block).group(1)
        if not name.startswith(constant(head)):
            ERRORS.append(f'{biome} : {head} dans le fichier, {name} attendu')
            continue
        profile = re.search(r'Profile: (.*)', block).group(1)
        role = re.search(r'Variants: (.*)', block).group(1)
        roles = dict(re.findall(r'\b([AB]) = ([^/]+)', role))
        tiers = [[None, None] for _ in range(4)]
        for m in re.finditer(r'\[(\d)([AB])\] .*\n\s+\S.*? @ (.+?)\s+\|\s+Ability: (.+?)\s+\|\s+Nature: (\w+)\s*\n'
                             r'\s+Moves: (.+)', block):
            tier, letter, item, ability, nature, moves_txt = m.groups()
            tier, label = int(tier), m.group(1) + letter
            item = 'NONE' if item == '(no item)' else constant(item)
            ability, nature = constant(ability), nature.upper()
            moves_txt = re.sub(r'\s+\(.*\)\s*$', '', moves_txt)
            moves, move_tiers_txt = [], []
            for part in moves_txt.split(' / '):
                mm = re.match(r'(.+?)\s*\[(T\d|Sit)\]$', part.strip())
                if not mm:
                    ERRORS.append(f'{name} [{label}] : attaque illisible {part!r}')
                    continue
                moves.append(constant(mm.group(1)))
                move_tiers_txt.append('SITUATIONAL' if mm.group(2) == 'Sit' else int(mm.group(2)[1]))

            mon = g.MONS[name]
            if item != 'NONE' and item not in g.ITEMS:
                ERRORS.append(f'{name} [{label}] : objet {item} inconnu')
            if item in FORBIDDEN_ITEMS:
                ERRORS.append(f'{name} [{label}] : objet {item} interdit')
            if nature not in g.NATURES:
                ERRORS.append(f'{name} [{label}] : nature {nature} inconnue')
            if ('ABILITY_' + ability) not in g.read('include/constants/abilities.h'):
                ERRORS.append(f'{name} [{label}] : talent {ability} inconnu')
            elif ability not in mon.abilities:
                WARNINGS.append(f'{name} [{label}] : talent {ability} non disponible ({", ".join(mon.abilities)})')
            learn = g.learnset(name)
            for mv, t in zip(moves, move_tiers_txt):
                if mv not in g.MOVES:
                    ERRORS.append(f'{name} [{label}] : attaque {mv} inconnue')
                    continue
                if mv not in learn and name != 'SMEARGLE':
                    WARNINGS.append(f'{name} [{label}] : {mv} non apprenable')
                rec = move_tiers.MOVES.get(mv)
                if rec is None:
                    WARNINGS.append(f'{name} [{label}] : {mv} absente de moves_by_tier.txt')
                elif rec.tier != t:
                    WARNINGS.append(f'{name} [{label}] : {mv} notée T{t}, T{rec.tier} dans moves_by_tier.txt')
            check_tier_rules(name, label, tier, move_tiers_txt, moves)
            if len(set(moves)) != len(moves):
                ERRORS.append(f'{name} [{label}] : attaque en double')

            num, den = TIER_EV_FRACTION[tier - 1]
            evs = [v * num // den // 4 * 4 for v in spread(name, nature, moves, roles.get(letter, ''), profile)]
            tiers[tier - 1]['AB'.index(letter)] = {
                'ability': ability, 'item': item, 'nature': nature, 'evs': evs, 'iv': TIER_IVS[tier - 1],
                'moves': moves + ['NONE'] * (4 - len(moves))}
        if any(s is None for t in tiers for s in t):
            ERRORS.append(f'{name} : sets manquants ({biome})')
            continue
        SETS[name] = tiers


for _biome, _file in BIOME_FILES.items():
    parse_file(_biome, os.path.join(DATA, _file))

if __name__ == '__main__':
    for e in ERRORS:
        print('ERREUR', e)
    for w in WARNINGS:
        print('avertissement', w)
    print(f'{len(SETS)} espèces, {len(ERRORS)} erreurs, {len(WARNINGS)} avertissements')
    if '--evs' in sys.argv:
        for name, tiers in SETS.items():
            for t, (a, b) in enumerate(tiers):
                print(f'{name} {t + 1}A {a["nature"]:8} {a["evs"]}   {t + 1}B {b["nature"]:8} {b["evs"]}')
