#!/usr/bin/env python3
"""
Génère src/data/isaac_move_tiers.h : le tier de chaque attaque d'après data/moves_by_tier.txt (move_tiers.py), pour la
progression par les combats (IsaacPickBattleProgression) : 1 à 4, ISAAC_MOVE_TIER_SITUATIONAL pour les attaques
situationnelles, 0 pour les attaques sans tier (écartées).

Usage : python3 tools/isaac/build_move_tiers.py
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g
import move_tiers


def main():
    out = ['// Fichier généré par tools/isaac/build_move_tiers.py (d\'après tools/isaac/data/moves_by_tier.txt) :',
           '// ne pas modifier à la main. Tier de chaque attaque : 1 à 4, ISAAC_MOVE_TIER_SITUATIONAL, 0 sans tier.',
           '',
           'static const u8 sIsaacMoveTiers[MOVES_COUNT] =',
           '{']
    for name, rec in move_tiers.MOVES.items():
        tier = 'ISAAC_MOVE_TIER_SITUATIONAL' if rec.tier == 'SITUATIONAL' else rec.tier
        out.append(f'    [MOVE_{name}] = {tier},')
    out.append('};')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_move_tiers.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    print(f'{len(move_tiers.MOVES)} attaques, src/data/isaac_move_tiers.h écrit')


if __name__ == '__main__':
    main()
