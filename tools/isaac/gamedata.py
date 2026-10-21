"""
Données du jeu utiles aux sets : espèces 1 à 386 (types, talents, stats de base), attaques, attaques
apprenables (src/data/pokemon/all_learnables.json), objets et natures.
"""
import json, os, re

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')


def read(path):
    return open(os.path.join(ROOT, path), encoding='utf-8').read()


def resolve_ternaries(text):
    # « P_UPDATED_X >= GEN_6 ? a : b » : le jeu est configuré sur la dernière génération -> a
    return re.sub(r'[A-Z][A-Z0-9_]*\s*>=\s*GEN_\d+\s*\?\s*([\w]+)\s*:\s*([\w]+)', r'\1', text)


class Record:
    pass


# --- Espèces 1 à 386 -------------------------------------------------------------------------
_species_h = read('include/constants/species.h')
SPECIES = [None] * 387
for _name, _num in re.findall(r'SPECIES_(\w+)\s*=\s*(\d+),', _species_h):
    _n = int(_num)
    if 1 <= _n <= 386 and SPECIES[_n] is None:
        SPECIES[_n] = _name
assert all(SPECIES[1:]), 'espèce manquante'

_info = resolve_ternaries(''.join(read(f'src/data/pokemon/species_info/gen_{g}_families.h') for g in range(1, 4)))
_blocks = {}
for _m in re.finditer(r'\[SPECIES_(\w+)\]\s*=\s*\{', _info):
    if _m.group(1) in _blocks:
        continue
    _end = _info.find('\n    [SPECIES_', _m.end())
    _blocks[_m.group(1)] = _info[_m.end(): _end if _end > 0 else len(_info)]

_DEFINES = {}
for _name, _value in re.findall(r'#define\s+(\w+)\s+([^\n]+)', _info):
    _DEFINES.setdefault(_name, _value.strip())


def _eval_value(expr):
    expr = expr.strip()
    if '?' in expr:
        expr = expr.split('?', 1)[1].split(':', 1)[0]
    token = re.match(r'[\s(]*(\w+)', expr).group(1)
    return int(token) if token.isdigit() else _eval_value(_DEFINES[token])


def _resolve_token(token):
    token = token.strip()
    while not token.startswith(('TYPE_', 'ABILITY_')):
        define = _DEFINES[token]
        if '?' in define:
            define = define.split('?', 1)[1].split(':', 1)[0]
        token = re.match(r'[\s(]*(\w+)', define).group(1)
    return token


def _stat(block, field):
    return _eval_value(re.search(r'\.%s\s*=\s*([^,\n]+)' % field, block).group(1))


# Zarbi est décrit par une macro : son bloc est recopié ici
_UNOWN_BLOCK = """
        .baseHP = 48, .baseAttack = 72, .baseDefense = 48, .baseSpeed = 48, .baseSpAttack = 72, .baseSpDefense = 48,
        .types = MON_TYPES(TYPE_PSYCHIC),
        .abilities = { ABILITY_LEVITATE, ABILITY_NONE, ABILITY_NONE },
"""
SHORT_NAMES = {'CASTFORM_NORMAL': 'CASTFORM', 'DEOXYS_NORMAL': 'DEOXYS'}  # nom de l'espèce dans le code C
LEARN_NAMES = {'DEOXYS': 'DEOXYS_NORMAL'}  # nom dans all_learnables.json

MONS = {}
ORDER = []
for _n in range(1, 387):
    _name = SPECIES[_n]
    _b = _UNOWN_BLOCK if _name == 'UNOWN' else _blocks[_name]
    mon = Record()
    mon.name, mon.num = SHORT_NAMES.get(_name, _name), _n
    mon.hp, mon.atk, mon.dfn = _stat(_b, 'baseHP'), _stat(_b, 'baseAttack'), _stat(_b, 'baseDefense')
    mon.spe, mon.spa, mon.spd = _stat(_b, 'baseSpeed'), _stat(_b, 'baseSpAttack'), _stat(_b, 'baseSpDefense')
    _types = re.search(r'\.types\s*=\s*([^\n]+)', _b).group(1).rstrip(',')
    while _types.strip() in _DEFINES:
        _types = _DEFINES[_types.strip()]
    _inner = re.search(r'MON_TYPES\(([^)]*)\)|\{([^}]*)\}', _types)
    mon.types = sorted({_resolve_token(t)[5:] for t in (_inner.group(1) or _inner.group(2)).split(',')})
    _abilities = re.search(r'\.abilities\s*=\s*([^\n]+)', _b).group(1).rstrip(',')
    while _abilities.strip() in _DEFINES:
        _abilities = _DEFINES[_abilities.strip()]
    _inner = re.search(r'\{([^}]*)\}', _abilities).group(1)
    mon.abilities = [a[8:] for a in (_resolve_token(t) for t in _inner.split(',') if t.strip()) if a != 'ABILITY_NONE']
    MONS[mon.name] = mon
    ORDER.append(mon.name)

# --- Attaques ----------------------------------------------------------------------------------
_moves_h = resolve_ternaries(read('src/data/moves_info.h'))
MOVES = {}
for _m in re.finditer(r'\[MOVE_(\w+)\]\s*=\s*\{', _moves_h):
    _end = _moves_h.find('\n    [MOVE_', _m.end())
    _b = _moves_h[_m.end(): _end if _end > 0 else len(_moves_h)]
    mv = Record()
    mv.name = _m.group(1)
    _p = re.search(r'\.power\s*=\s*(\d+)', _b)
    mv.power = int(_p.group(1)) if _p else 0
    _a = re.search(r'\.accuracy\s*=\s*(\d+)', _b)
    mv.acc = int(_a.group(1)) if _a else 0
    _t = re.search(r'\.type\s*=\s*TYPE_(\w+)', _b)
    mv.type = _t.group(1) if _t else 'NORMAL'
    _c = re.search(r'\.category\s*=\s*DAMAGE_CATEGORY_(\w+)', _b)
    mv.cat = _c.group(1) if _c else 'STATUS'
    MOVES[mv.name] = mv

_learn = json.loads(read('src/data/pokemon/all_learnables.json'))


def learnset(name):
    return {m[5:] for m in _learn.get(LEARN_NAMES.get(name, name), [])}


ITEMS = {i[5:] for i in re.findall(r'\bITEM_[A-Z0-9_]+', read('include/constants/items.h'))}
NATURES = {n[7:] for n in re.findall(r'\bNATURE_[A-Z]+', read('include/constants/pokemon.h'))}
