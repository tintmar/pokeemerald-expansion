"""
Tiers des attaques d'après data/moves_by_tier.txt (draft v3) : 1 faible (début de partie), 2 moyen, 3 fort (une
meilleure option de tier 4 du même type existe), 4 optimal, 'SITUATIONAL' (utilitaire : soins, météo, statut...).
Pas encore utilisé par les générateurs : c'est une donnée de référence.

MOVES : constante MOVE_* (sans le préfixe) -> Record(tier, type, cat, power, acc, pp, priority, effect, tags,
better), où tags est l'ensemble des étiquettes entre crochets ('Pivot', 'Priority', 'Exclusive: X line'...) et better
la liste des attaques de tier 4 dont une attaque de tier 3 est une version plus faible (-> T4: ...), vide sinon.
OMITTED : attaques écartées (entry hazards, attaques de combat en double).
"""
import os, re

_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'data', 'moves_by_tier.txt')
_TIERS = {'TIER 1': 1, 'TIER 2': 2, 'TIER 3': 3, 'TIER 4': 4, 'SITUATIONAL': 'SITUATIONAL'}
_CATS = {'Phys': 'PHYSICAL', 'Spec': 'SPECIAL', 'Stat': 'STATUS'}


class Record:
    def __repr__(self):
        return f'Record({self.__dict__})'


def move_constant(name):
    """Nom affiché ('Will-O-Wisp', "King's Shield") -> nom de constante sans MOVE_ ('WILL_O_WISP', 'KINGS_SHIELD')"""
    n = re.sub(r"[’']", '', name.upper())
    n = re.sub(r'[^A-Z0-9]+', '_', n).strip('_')
    return n


def _parse():
    moves, omitted = {}, []
    tier = type_ = None
    in_omitted = False
    text = open(_PATH, encoding='utf-8').read()
    for line in text.split('\n'):
        head = re.match(r'(TIER \d|SITUATIONAL|OMITTED)', line)
        if head:
            tier = _TIERS.get(head.group(1))
            in_omitted = head.group(1) == 'OMITTED'
            continue
        m = re.match(r'\s+-- ([A-Z]+) \(\d+\) --', line)
        if m:
            type_ = m.group(1)
            continue
        if tier is None:
            if in_omitted and line.startswith('    '):  # listes des attaques écartées
                omitted.extend(move_constant(n.strip()) for n in line.split(',') if n.strip())
            continue
        m = re.match(r'    (\S.*?)\s{2,}(Phys|Spec|Stat)\s+(\S+)\s+(\S+)\s+(\d+)\s{1,3}([+-]\d)?\s+(.*)$', line)
        if not m:
            continue
        r = Record()
        r.tier, r.type, r.cat = tier, type_, _CATS[m.group(2)]
        r.power = m.group(3) if m.group(3) in ('var', 'fix', '—') else int(m.group(3).rstrip('*'))  # 102* : Retour
        r.acc = None if m.group(4) == '—' else int(m.group(4).rstrip('%'))
        r.pp, r.priority = int(m.group(5)), int(m.group(6) or 0)
        effect = m.group(7).strip()
        better = re.search(r'->\s*T4:\s*(.+)$', effect)
        r.better = [move_constant(b) for b in better.group(1).split('/')] if better else []
        effect = re.sub(r'\s*->\s*T4:.*$', '', effect)
        r.tags = set(re.findall(r'\[([^\]]+)\]', effect))
        r.effect = re.sub(r'\s*\[[^\]]+\]\s*', ' ', effect).strip()
        moves[move_constant(m.group(1))] = r
    return moves, omitted


MOVES, OMITTED = _parse()

if __name__ == '__main__':
    from collections import Counter
    print(len(MOVES), 'attaques :', Counter(r.tier for r in MOVES.values()))
