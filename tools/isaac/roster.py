"""
Roster du donjon : les espèces de biome_roster.BIOMES (les 386 premiers Pokémon, plus des espèces des générations
suivantes ; noms des constantes SPECIES_*). Chaque espèce du roster a une catégorie (build_categories.py) et des sets
(build_sets.py) ; le jeu ne fait apparaître, évoluer ou échanger que des espèces du roster.
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import biome_roster

# Espèces au-delà des 386 premières (gamedata.ROSTER les ajoute aux 386)
_BASE_FORMS = {'CASTFORM', 'DEOXYS'}  # formes des 386 (CASTFORM_NORMAL, DEOXYS_NORMAL)
def _extra_species():
    import re
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
    nums = dict(re.findall(r'SPECIES_(\w+)\s*=\s*(\d+),', open(os.path.join(root, 'include/constants/species.h')).read()))
    return [n for n in biome_roster.CATEGORY if n not in _BASE_FORMS and int(nums.get(n, 0)) > 386]


EXTRA_SPECIES = _extra_species()
# Formes régionales « à poids partagé » : plus aucune dans le roster (le jeu tire l'espèce d'origine puis choisit au
# hasard entre elle et ses formes, forme -> espèce d'origine)
SHARED_FORMS = {}

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
