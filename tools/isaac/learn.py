#!/usr/bin/env python3
"""Aide à l'écriture des sets : talents, stats et attaques apprenables d'une espèce.
Usage : python3 tools/isaac/learn.py ESPÈCE [MOT...]   (MOT : ne garder que les attaques qui le contiennent)"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g

name = sys.argv[1].upper()
mon = g.MONS[name]
print(f'{name} {"/".join(mon.types)}  {mon.hp}/{mon.atk}/{mon.dfn}/{mon.spa}/{mon.spd}/{mon.spe}  talents : {", ".join(mon.abilities)}')
filters = [f.upper() for f in sys.argv[2:]]
moves = sorted(g.learnset(name), key=lambda m: (g.MOVES[m].cat, -g.MOVES[m].power) if m in g.MOVES else ('', 0))
for cat in ('PHYSICAL', 'SPECIAL', 'STATUS'):
    names = [m for m in moves if m in g.MOVES and g.MOVES[m].cat == cat and (not filters or any(f in m for f in filters))]
    if names:
        print(f'{cat}: ' + ' '.join(f'{m}({g.MOVES[m].power})' if cat != 'STATUS' else m for m in names))
