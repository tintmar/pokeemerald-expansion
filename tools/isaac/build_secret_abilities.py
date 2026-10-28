#!/usr/bin/env python3
"""
Génère src/data/isaac_secret_abilities.h d'après data/secret_abilities.txt : les talents que le révélateur de talents
peut proposer (section USED), avec leur adéquation (SECRET_FIT_*) et son type. La section UNUSED (talents trop forts)
est seulement vérifiée.

Usage : python3 tools/isaac/build_secret_abilities.py
"""
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g

MAX_USED = 64  # un bit par talent dans VAR_ISAAC_SECRET_ABILITY_SEEN_0..3
FITS = {'CONTACT', 'PUNCH', 'BITE', 'SLICE', 'PULSE', 'SOUND', 'RECOIL', 'MULTIHIT', 'SECONDARY', 'WEAK_MOVES',
        'LOW_ACC', 'STAB', 'TYPE', 'NORMALIZE', 'WEATHER', 'PHYSICAL', 'SPECIAL', 'ATTACKER', 'STATUS_MOVES',
        'HEALING', 'BULK', 'FRAIL', 'RESIST', 'WEAKNESSES', 'SPEED', 'SLOW', 'ANY'}
TYPE_COUNT = {'TYPE': (1, 1), 'NORMALIZE': (1, 1), 'WEATHER': (1, 1), 'RESIST': (1, 2)}  # (min, max), sinon aucun


def parse():
    abilities = set(re.findall(r'\bABILITY_([A-Z0-9_]+)', g.read('include/constants/abilities.h')))
    types = set(re.findall(r'\bTYPE_([A-Z]+)\b', g.read('include/constants/pokemon.h')))
    used, unused, errors, section = [], [], [], None
    for n, line in enumerate(g.read('tools/isaac/data/secret_abilities.txt').split('\n'), 1):
        line = line.split('#')[0].rstrip()
        if line.startswith('USED'):
            section = used
            continue
        if line.startswith('UNUSED'):
            section = unused
            continue
        if not line.strip() or section is None:
            continue
        words = line.split()
        if words[0] not in abilities:
            errors.append(f'ligne {n} : talent {words[0]} inconnu')
        if words[0] in [w[0] for w in used + unused]:
            errors.append(f'ligne {n} : {words[0]} en double')
        if section is unused:
            unused.append(words)
            continue
        fit, params = (words[1] if len(words) > 1 else None), words[2:]
        if fit not in FITS:
            errors.append(f'ligne {n} : adéquation {fit} inconnue')
        elif not TYPE_COUNT.get(fit, (0, 0))[0] <= len(params) <= TYPE_COUNT.get(fit, (0, 0))[1]:
            errors.append(f'ligne {n} : mauvais nombre de types pour {fit}')
        errors += [f'ligne {n} : type {t} inconnu' for t in params if t not in types]
        used.append(words)
    if len(used) > MAX_USED:
        errors.append(f'{len(used)} talents utilisés, {MAX_USED} au plus')
    return used, unused, errors


def main():
    used, unused, errors = parse()
    if errors:
        sys.exit('\n'.join(errors))
    out = ['// Fichier généré par tools/isaac/build_secret_abilities.py (d\'après tools/isaac/data/secret_abilities.txt) :',
           '// ne pas modifier à la main. Talents que le révélateur de talents peut proposer, avec leur adéquation.',
           '',
           'static const struct IsaacSecretAbility sIsaacSecretAbilities[] =',
           '{']
    for words in used:
        params = [f'TYPE_{t}' for t in words[2:]] + ['TYPE_NONE'] * (2 - len(words[2:]))
        out.append(f'    {{ ABILITY_{words[0]}, SECRET_FIT_{words[1]}, {{ {params[0]}, {params[1]} }} }},')
    out.append('};')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_secret_abilities.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    print(f'{len(used)} talents utilisés, {len(unused)} non utilisés, src/data/isaac_secret_abilities.h écrit')


if __name__ == '__main__':
    main()
