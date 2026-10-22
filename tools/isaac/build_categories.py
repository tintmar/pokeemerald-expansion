#!/usr/bin/env python3
"""
Génère src/data/isaac_categories.h : la catégorie de chaque Pokémon du roster (roster.py) selon son BST
(total des stats de base), et la liste du roster.
  - les non légendaires sont répartis en 4 catégories par quartile de BST (calculés sur eux seuls) :
    BST <= Q1 -> catégorie 1, <= Q2 -> 2, <= Q3 -> 3, au-delà -> 4 ;
  - les légendaires et fabuleux sont à part : « petits » (BST < 620 : Électhor, Suicune, Mew...) et
    « grands » (BST >= 620 : Mewtwo, Lugia, Kyogre...).

Usage : python3 tools/isaac/build_categories.py
"""
import os, statistics, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g

LEGENDARIES = {'ARTICUNO', 'ZAPDOS', 'MOLTRES', 'MEWTWO', 'MEW', 'RAIKOU', 'ENTEI', 'SUICUNE', 'LUGIA', 'HO_OH',
               'CELEBI', 'REGIROCK', 'REGICE', 'REGISTEEL', 'LATIAS', 'LATIOS', 'KYOGRE', 'GROUDON', 'RAYQUAZA',
               'JIRACHI', 'DEOXYS'}
BIG_LEGENDARY_BST = 620


def bst(name):
    m = g.MONS[name]
    return m.hp + m.atk + m.dfn + m.spa + m.spd + m.spe


def main():
    regular = [n for n in g.ORDER if n not in LEGENDARIES]
    q1, q2, q3 = statistics.quantiles([bst(n) for n in regular], n=4, method='inclusive')
    categories = {}
    for name in g.ORDER:
        b = bst(name)
        if name in LEGENDARIES:
            categories[name] = 'LEGENDARY_BIG' if b >= BIG_LEGENDARY_BST else 'LEGENDARY_SMALL'
        else:
            categories[name] = '1' if b <= q1 else '2' if b <= q2 else '3' if b <= q3 else '4'

    out = ['// Fichier généré par tools/isaac/build_categories.py : ne pas modifier à la main.',
           '// Catégorie de chaque Pokémon selon son BST : quartiles des non légendaires (BST <= %d, <= %d, <= %d, au-delà),' % (q1, q2, q3),
           '// légendaires à part (petits : BST < %d, grands : BST >= %d).' % (BIG_LEGENDARY_BST, BIG_LEGENDARY_BST),
           '',
           'static const u8 sIsaacSpeciesCategories[NUM_SPECIES] =',
           '{']
    for name in g.ORDER:
        out.append(f'    [SPECIES_{name}] = ISAAC_CATEGORY_{categories[name]},  // BST {bst(name)}')
    out.append('};')
    out.append('')
    out.append('// Toutes les espèces du roster, dans l\'ordre du Pokédex national')
    out.append('static const u16 sIsaacRoster[] =')
    out.append('{')
    out.extend(f'    SPECIES_{name},' for name in g.ORDER)
    out.append('};')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_categories.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')

    counts = {c: sum(1 for v in categories.values() if v == c) for c in ('1', '2', '3', '4', 'LEGENDARY_SMALL', 'LEGENDARY_BIG')}
    print(f'seuils : {q1:.0f} / {q2:.0f} / {q3:.0f}', counts)


if __name__ == '__main__':
    main()
