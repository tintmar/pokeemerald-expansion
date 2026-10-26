#!/usr/bin/env python3
"""
Génère src/data/isaac_categories.h : la catégorie de chaque Pokémon du roster (roster.py), et la liste du roster ;
et src/data/isaac_biome_species.h : les espèces de chaque biome.
Les catégories viennent de biome_roster.py et sont vérifiées sur le BST (total des stats de base) du jeu :
  - catégories 1 à 7 : septiles de BST des non légendaires (SEPTILES : BST <= 288 -> 1, ... au-delà de 515 -> 7) ;
  - catégorie 8 (ISAAC_CATEGORY_LEGENDARY_SMALL) : légendaires et fabuleux de BST <= 600 (Électhor, Suicune, Mew...) ;
  - catégorie 9 (ISAAC_CATEGORY_LEGENDARY_BIG) : légendaires de BST > 600 (Mewtwo, Lugia, Kyogre...).

Usage : python3 tools/isaac/build_categories.py
"""
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g
import roster
import biome_roster

SEPTILES = (288, 325, 395, 440, 481, 515)
SMALL_LEGENDARY_MAX_BST = 600
ENUM_NAMES = {8: 'LEGENDARY_SMALL', 9: 'LEGENDARY_BIG'}


def bst(name):
    m = g.MONS[name]
    return m.hp + m.atk + m.dfn + m.spa + m.spd + m.spe


def bst_category(name):
    b = bst(name)
    if name in biome_roster.LEGENDARIES:
        return 8 if b <= SMALL_LEGENDARY_MAX_BST else 9
    return 1 + sum(b > t for t in SEPTILES)


def main():
    categories = {}
    for name in g.ORDER:
        categories[name] = biome_roster.CATEGORY[name]
        assert categories[name] == bst_category(name), f'{name} : catégorie {categories[name]}, BST {bst(name)}'

    out = ['// Fichier généré par tools/isaac/build_categories.py : ne pas modifier à la main.',
           '// Catégorie de chaque Pokémon (tools/isaac/biome_roster.py) : septiles de BST des non légendaires (BST <= '
           + ', <= '.join(map(str, SEPTILES)) + ', au-delà),',
           '// légendaires à part (petits : BST <= %d, grands : au-delà).' % SMALL_LEGENDARY_MAX_BST,
           '',
           'static const u8 sIsaacSpeciesCategories[NUM_SPECIES] =',
           '{']
    for name in g.ORDER:
        enum = ENUM_NAMES.get(categories[name], str(categories[name]))
        out.append(f'    [SPECIES_{name}] = ISAAC_CATEGORY_{enum},  // BST {bst(name)}')
    out.append('};')
    out.append('')
    out.append('// Toutes les espèces du roster, dans l\'ordre du Pokédex national')
    out.append('static const u16 sIsaacRoster[] =')
    out.append('{')
    out.extend(f'    SPECIES_{name},' for name in g.ORDER)
    out.append('};')
    out.append('')
    out.append('// Formes régionales du roster : { forme, espèce d\'origine si son poids est partagé avec elle (SPECIES_NONE :')
    out.append('// lignée à part) }')
    out.append('static const u16 sIsaacRegionalForms[][2] =')
    out.append('{')
    forms = [name for name in g.ORDER if re.search(r'_(ALOLA|GALAR|HISUI|PALDEA)(_|$)', name)]
    for name in forms:
        out.append(f'    {{ SPECIES_{name}, SPECIES_{roster.SHARED_FORMS.get(name, "NONE")} }},')
    if not forms:
        out.append('    { SPECIES_NONE, SPECIES_NONE },  // aucune')
    out.append('};')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_categories.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')

    # Espèces de chaque biome (toutes, évolutions comprises), de la catégorie la plus faible à la plus forte
    out = ['// Fichier généré par tools/isaac/build_categories.py (d\'après tools/isaac/biome_roster.py) : ne pas modifier',
           '// à la main. Espèces de chaque biome, de la catégorie 1 à la catégorie 9.', '']
    for biome, species in biome_roster.BIOMES.items():
        out.append(f'static const u16 s{biome.capitalize()}Species[] =')
        out.append('{')
        for cat in range(1, 10):
            names = [f'SPECIES_{name},' for name, c in species if c == cat]
            out.append(f'    // catégorie {cat}')
            line = '   '
            for n in names:
                if len(line) + len(n) + 1 > 118:
                    out.append(line)
                    line = '   '
                line += ' ' + n
            out.append(line)
        out.append('};')
        out.append('')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_biome_species.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out))

    counts = {c: sum(1 for v in categories.values() if v == c) for c in range(1, 10)}
    print(f'{len(g.ORDER)} espèces, par catégorie :', counts)


if __name__ == '__main__':
    main()
