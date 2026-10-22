"""
Roster du donjon : les 386 premiers Pokémon, plus les espèces des générations suivantes de EXTRA_SPECIES (noms
des constantes SPECIES_*). Chaque espèce du roster a une catégorie (build_categories.py) et des sets
(build_sets.py) ; le jeu ne fait apparaître, évoluer ou échanger que des espèces du roster.
"""

EXTRA_SPECIES = [
]

# Attaques que les sets peuvent donner en plus de celles qu'apprend l'espèce : celles du type gagné par les espèces
# retypées (src/data/pokemon/species_info : Noarfang Spectre, Pharamp Dragon...). Les sets donnent leurs attaques
# directement au Pokémon : ces attaques n'ont pas besoin d'être dans ses learnsets du jeu.
EXTRA_LEARNABLE = {
    'SANDSLASH': ['IRON_HEAD'],
    'RAPIDASH': ['SPIRIT_BREAK'],
    'NINETALES': ['MOONBLAST', 'DAZZLING_GLEAM', 'DRAINING_KISS'],
    'KINGLER': ['IRON_HEAD'],
    'MAROWAK': ['SHADOW_BONE', 'SHADOW_SNEAK', 'SHADOW_CLAW'],
    'TAUROS': ['BRICK_BREAK'],
    'NOCTOWL': ['HEX'],
    'AMPHAROS': ['DRAGON_BREATH'],
    'SUNFLORA': ['EMBER', 'FLAMETHROWER', 'FIRE_BLAST'],
    'LINOONE': ['BITE', 'NIGHT_SLASH'],
    'BEAUTIFLY': ['FAIRY_WIND', 'DAZZLING_GLEAM', 'MOONBLAST'],
    'GLALIE': ['ROCK_TOMB', 'ROCK_SLIDE', 'STONE_EDGE'],
    'CHIMECHO': ['FLASH_CANNON'],
}
