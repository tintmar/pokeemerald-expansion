#!/usr/bin/env python3
"""
Vérifie la répartition du roster dans les biomes (src/data/isaac_biomes.h), en refaisant le parcours des lignées du
jeu (GetLinesSpecies de isaac_pokemon.c) : depuis chaque espèce de base, ses évolutions dans le roster, sans passer
d'une espèce à une forme régionale (les formes régionales ont leur lignée, ou un poids partagé avec leur espèce
d'origine : roster.SHARED_FORMS).
  - chaque espèce du roster (sauf légendaires et formes à poids partagé) est dans une seule lignée ;
  - chaque biome a des espèces des 4 catégories (quarts de BST des espèces du biome) ;
  - les espèces d'origine des formes à poids partagé sont bien dans une lignée.

Usage : python3 tools/isaac/check_roster.py [-v]   (-v : espèces de chaque biome par catégorie)
"""
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g
import roster

REGIONAL = re.compile(r'_(ALOLA|GALAR|HISUI|PALDEA)(_|$)')
CANONICAL = {alias: target for target, aliases in g._ALIASES.items() for alias in aliases}
NOT_IN_LINES = {'SHEDINJA'}  # « clone » de Ninjask (EVO_SPLIT_FROM_EVO) : jamais tiré dans une lignée


def canonical(name):
    name = CANONICAL.get(name, name)
    return g.SHORT_NAMES.get(name, name)


def evolutions(name):
    block = g._blocks.get(g.LEARN_NAMES.get(name, name)) or g._blocks.get(name, '')
    m = re.search(r'\.evolutions\s*=\s*EVOLUTION\((.*?)\)\s*,\s*\n', block, re.S)
    if not m:
        return []
    out = []
    for method, target in re.findall(r'\{EVO_(\w+),\s*[^,]*,\s*SPECIES_(\w+)', m.group(1)):
        target = canonical(target)
        if method in ('NONE', 'SPLIT_FROM_EVO') or target not in g.MONS or target in out:
            continue
        if REGIONAL.search(target) and not REGIONAL.search(name):
            continue
        out.append(target)
    return out


def bst(name):
    m = g.MONS[name]
    return m.hp + m.atk + m.dfn + m.spa + m.spd + m.spe


def main():
    src = open(os.path.join(g.ROOT, 'src/data/isaac_biomes.h'), encoding='utf-8').read()
    biomes = {name: [canonical(s) for s in re.findall(r'SPECIES_(\w+)', body)]
              for name, body in re.findall(r'static const u16 s(\w+)Lines\[\] =\s*\{(.*?)\};', src, re.S)}
    errors = []
    owner = {}
    for biome, lines in biomes.items():
        species = []
        for base in lines:
            if base not in g.MONS:
                errors.append(f'{biome} : {base} hors du roster')
                continue
            todo = [base]
            while todo:
                s = todo.pop(0)
                if s in species:
                    continue
                species.append(s)
                todo += evolutions(s)
        for s in species:
            if s in owner:
                errors.append(f'{s} dans deux biomes : {owner[s]} et {biome}')
            owner[s] = biome
        ranked = sorted(species, key=bst)
        cats = {}
        for i, s in enumerate(ranked):
            first = next(j for j in range(len(ranked)) if bst(ranked[j]) == bst(s))
            cats.setdefault(1 + first * 4 // len(ranked), []).append(s)
        missing = [c for c in (1, 2, 3, 4) if c not in cats]
        if missing:
            errors.append(f'{biome} : aucune espèce de catégorie {missing}')
        shared = sum(1 for f, b in roster.SHARED_FORMS.items() if b in species)
        print(f'{biome:7} {len(lines):3} lignées, {len(species):3} espèces (+{shared} formes partagées), '
              f'par catégorie : {[len(cats.get(c, [])) for c in (1, 2, 3, 4)]}')
        if '-v' in sys.argv:
            for c in (1, 2, 3, 4):
                print(f'    {c} : {" ".join(cats.get(c, []))}')
    legendaries = set(re.findall(r"'(\w+)'", open(os.path.join(os.path.dirname(__file__), 'build_categories.py')).read().split('BIG_LEGENDARY_BST')[0]))
    legendaries |= set(roster.EXTRA_LEGENDARIES)
    for s in g.ORDER:
        if s in owner or s in legendaries or s in roster.SHARED_FORMS or s in NOT_IN_LINES:
            continue
        errors.append(f'{s} : dans aucune lignée')
    for form, base in roster.SHARED_FORMS.items():
        if form not in g.MONS:
            errors.append(f'{form} : forme partagée hors du roster')
        if base not in owner:
            errors.append(f'{form} : son espèce d\'origine {base} n\'est dans aucune lignée')
    for e in errors:
        print(e)
    print(f'{len(g.ORDER)} espèces, {len(errors)} erreurs')
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
