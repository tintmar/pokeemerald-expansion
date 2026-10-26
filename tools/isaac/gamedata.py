"""
Données du jeu utiles aux sets : espèces 1 à 386 (types, talents, stats de base), attaques, attaques
apprenables (src/data/pokemon/all_learnables.json), objets et natures.
"""
import json, os, re

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')


def read(path):
    return open(os.path.join(ROOT, path), encoding='utf-8').read()


GEN_LATEST = 9  # include/config/general.h : les réglages *_UPDATED_* valent GEN_LATEST (= GEN_9)
_GEN_VALUES = {'LATEST': GEN_LATEST, 'CHAMPIONS': GEN_LATEST + 1}


_COMPARISON = r'[A-Z][A-Z0-9_]*\s*(?:>=|<=|==|!=|>|<)\s*GEN_\w+'


def _compare(m):
    op, gen = m.group(1), m.group(2)
    n = _GEN_VALUES.get(gen, int(gen) if gen.isdigit() else GEN_LATEST)
    return str({'>=': GEN_LATEST >= n, '>': GEN_LATEST > n, '<=': GEN_LATEST <= n, '<': GEN_LATEST < n,
                '==': GEN_LATEST == n, '!=': GEN_LATEST != n}[op])


def resolve_ternaries(text):
    # « P_UPDATED_X >= GEN_6 ? a : b », « B_UPDATED_X == GEN_CHAMPIONS ? a : b », « X >= GEN_6 || X == GEN_1 ? a : b » :
    # le jeu est configuré sur la dernière génération (GEN_LATEST)
    def choose(m):
        cond = re.sub(r'[A-Z][A-Z0-9_]*\s*(>=|<=|==|!=|>|<)\s*GEN_(\w+)', _compare, m.group(1))
        ok = eval(cond.replace('||', ' or ').replace('&&', ' and '))
        return m.group(2) if ok else m.group(3)
    # la condition peut être entre parenthèses : « (X >= GEN_9) ? a : b »
    pattern = r'\(?(%s(?:\s*(?:\|\||&&)\s*%s)*)\)?\s*\?\s*(\w+)\s*:\s*(\w+)' % (_COMPARISON, _COMPARISON)
    return re.sub(pattern, choose, text)


class Record:
    pass


# --- Espèces du roster : 1 à 386, puis roster.EXTRA_SPECIES --------------------------------------
_species_h = read('include/constants/species.h')
SPECIES_NUM = {}
for _name, _num in re.findall(r'SPECIES_(\w+)\s*=\s*(\d+),', _species_h):
    SPECIES_NUM.setdefault(_name, int(_num))
SPECIES = [None] * 387
for _name, _n in SPECIES_NUM.items():
    if 1 <= _n <= 386 and SPECIES[_n] is None:
        SPECIES[_n] = _name
assert all(SPECIES[1:]), 'espèce manquante'

import roster as _roster
for _name in _roster.EXTRA_SPECIES:
    assert SPECIES_NUM.get(_name, 0) > 386, f'roster : {_name} inconnue ou déjà dans les 386'
ROSTER = SPECIES[1:] + sorted(_roster.EXTRA_SPECIES, key=lambda n: SPECIES_NUM[n])

_info = resolve_ternaries(''.join(read(f'src/data/pokemon/species_info/gen_{g}_families.h') for g in range(1, 10)))
_blocks = {}
for _m in re.finditer(r'\[SPECIES_(\w+)\]\s*=\s*\{', _info):
    if _m.group(1) in _blocks:
        continue
    _end = _info.find('\n    [SPECIES_', _m.end())
    _blocks[_m.group(1)] = _info[_m.end(): _end if _end > 0 else len(_info)]

# Macros à paramètres (Flabébé et ses évolutions décrivent leurs stats dans des macros, une par couleur) : développées
# dans les blocs qui n'ont pas leurs stats en clair
_MACROS = {}
for _m in re.finditer(r'#define\s+(\w+)\(([^)]*)\)((?:[^\n]*\\\n)*[^\n]*)', _info):
    _MACROS[_m.group(1)] = ([p.strip() for p in _m.group(2).split(',')], _m.group(3).replace('\\\n', '\n'))


def _expand_macros(block):
    for _ in range(4):
        def sub(m):
            params, body = _MACROS[m.group(1)]
            args = [a.strip() for a in m.group(2).split(',')]
            for p, a in zip(params, args):
                body = re.sub(r'\b%s\b' % re.escape(p), a, body)
            return re.sub(r'\s*##\s*', '', body)
        new = re.sub(r'\b(%s)\(([^()]*)\)' % '|'.join(map(re.escape, _MACROS)), sub, block)
        if new == block:
            break
        block = new
    return block


for _name, _b in list(_blocks.items()):
    if '.baseHP' not in _b and _MACROS:
        _blocks[_name] = _expand_macros(_b)

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
# Arceus (forme Normal) : ses formes sont décrites par une macro à paramètres (ARCEUS_SPECIES_INFO)
_ARCEUS_BLOCK = """
        .baseHP        = 120,
        .baseAttack    = 120,
        .baseDefense   = 120,
        .baseSpeed     = 120,
        .baseSpAttack  = 120,
        .baseSpDefense = 120,
        .types = MON_TYPES(TYPE_NORMAL),
        .abilities = { ABILITY_MULTITYPE, ABILITY_NONE, ABILITY_NONE },
"""
SHORT_NAMES = {'CASTFORM_NORMAL': 'CASTFORM', 'DEOXYS_NORMAL': 'DEOXYS'}  # nom de l'espèce dans le code C
LEARN_NAMES = {'DEOXYS': 'DEOXYS_NORMAL'}  # nom dans all_learnables.json

MONS = {}
ORDER = []
for _name in ROSTER:
    _b = _UNOWN_BLOCK if _name == 'UNOWN' else _ARCEUS_BLOCK if _name == 'ARCEUS_NORMAL' else _blocks[_name]
    mon = Record()
    mon.name, mon.num = SHORT_NAMES.get(_name, _name), SPECIES_NUM[_name]
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
    # puissance réelle des attaques à plusieurs coups : coups fixes (strikeCount), 2 à 5 coups (multiHit, environ 3),
    # Triple Pied et Triple Axel (puissance doublée puis triplée : 6 fois la puissance)
    _e = re.search(r'\.effect\s*=\s*EFFECT_(\w+)', _b)
    _s = re.search(r'\.strikeCount\s*=\s*(\d+)', _b)
    mv.effect = _e.group(1) if _e else ''
    mv.hits = 6 if mv.effect == 'TRIPLE_KICK' else (3 if re.search(r'\.multiHit\s*=\s*TRUE', _b)
                                                     else (int(_s.group(1)) if _s else 1))
    mv.total = mv.power * mv.hits
    MOVES[mv.name] = mv

_learn = json.loads(read('src/data/pokemon/all_learnables.json'))


# Alias des espèces (SPECIES_FLABEBE = SPECIES_FLABEBE_RED) : all_learnables.json les range parfois sous l'alias
_ALIASES = {}
for _alias, _target in re.findall(r'SPECIES_(\w+)\s*=\s*SPECIES_(\w+),', _species_h):
    _ALIASES.setdefault(_target, []).append(_alias)


def learnset(name):
    key = LEARN_NAMES.get(name, name)
    for alias in [key] + _ALIASES.get(key, []):
        if alias in _learn:
            key = alias
            break
    return {m[5:] for m in _learn.get(key, [])} | set(_roster.EXTRA_LEARNABLE.get(name, []))


ITEMS = {i[5:] for i in re.findall(r'\bITEM_[A-Z0-9_]+', read('include/constants/items.h'))}
NATURES = {n[7:] for n in re.findall(r'\bNATURE_[A-Z]+', read('include/constants/pokemon.h'))}
