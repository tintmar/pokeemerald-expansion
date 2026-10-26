#!/usr/bin/env python3
"""
Génère src/data/isaac_move_tiers.h d'après data/moves_by_tier.txt (move_tiers.py), pour la progression par les
combats (IsaacPickBattleProgression) :
  - sIsaacMoveTiers : le tier de chaque attaque, 1 à 4, ISAAC_MOVE_TIER_SITUATIONAL pour les attaques situationnelles, 0
    pour les attaques sans tier (écartées) ;
  - sIsaacMoveRoles : le rôle des attaques de statut (ISAAC_MOVE_ROLE_*, voir role), d'après leurs étiquettes et leur
    effet ; une attaque offensive qui fait perdre un tour (recharge, tour de charge) est aussi ISAAC_MOVE_ROLE_COMBO.

Usage : python3 tools/isaac/build_move_tiers.py
"""
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as g
import move_tiers


def role(rec):
    """Rôle d'une attaque de statut : boost d'attaque (physique, spéciale ou les deux, la Vitesse comptant pour les
    deux), boost de défense, soin, protection, statut infligé, combo (inutile seule : Blabla Dodo, Dévorêve, Métronome...),
    autre"""
    if rec.cat != 'STATUS':
        if re.search(r'cannot move next turn|Charges|on turn 1\. Hits turn 2|Dives underwater|Flies up|Digs underground|Bounces turn 1|Hits two turns after|Fails if the user takes damage|Fails if there is no terrain', rec.effect):
            return 'ISAAC_MOVE_ROLE_COMBO'
        return '0'
    if rec.tags & {'Copy / Random', 'Sleep Combo'}:
        return 'ISAAC_MOVE_ROLE_COMBO'
    if rec.tags & {'Setup', 'Stat Boost'}:
        text = rec.effect
        special = re.search(r'Sp\. ?Atk|SpA\b|Sp\. Attack', text)
        rest = re.sub(r'Sp\. ?(Atk|Def|Attack|Defense)|SpA\b|SpD\b', '', text)
        physical = re.search(r'\bAttack\b|\bAtk\b', rest)
        speed = re.search(r'\bSpeed\b|\bSpe\b', rest) and 'lowers' not in text.lower().split('raises')[0]
        flags = []
        if physical or (speed and not special):
            flags.append('ISAAC_MOVE_ROLE_SETUP_PHYSICAL')
        if special or (speed and not physical):
            flags.append('ISAAC_MOVE_ROLE_SETUP_SPECIAL')
        if flags:
            return ' | '.join(flags)
        if re.search(r'\bDef|\bSp\. Def|\bSpD', text):
            return 'ISAAC_MOVE_ROLE_DEF_BOOST'
        return 'ISAAC_MOVE_ROLE_OTHER'
    if 'Recovery' in rec.tags:
        return 'ISAAC_MOVE_ROLE_RECOVERY'
    if 'Protection' in rec.tags:
        return 'ISAAC_MOVE_ROLE_PROTECTION'
    if rec.tags & {'Status Inflict', 'Sleep'}:
        return 'ISAAC_MOVE_ROLE_STATUS'
    return 'ISAAC_MOVE_ROLE_OTHER'


def main():
    out = ['// Fichier généré par tools/isaac/build_move_tiers.py (d\'après tools/isaac/data/moves_by_tier.txt) :',
           '// ne pas modifier à la main. Tier de chaque attaque (1 à 4, ISAAC_MOVE_TIER_SITUATIONAL, 0 sans tier) et rôle',
           '// des attaques de statut (ISAAC_MOVE_ROLE_*).',
           '',
           'static const u8 sIsaacMoveTiers[MOVES_COUNT] =',
           '{']
    for name, rec in move_tiers.MOVES.items():
        tier = 'ISAAC_MOVE_TIER_SITUATIONAL' if rec.tier == 'SITUATIONAL' else rec.tier
        out.append(f'    [MOVE_{name}] = {tier},')
    out.append('};')
    out.append('')
    out.append('static const u8 sIsaacMoveRoles[MOVES_COUNT] =')
    out.append('{')
    for name, rec in move_tiers.MOVES.items():
        if role(rec) != '0':
            out.append(f'    [MOVE_{name}] = {role(rec)},')
    out.append('};')
    path = os.path.join(g.ROOT, 'src', 'data', 'isaac_move_tiers.h')
    open(path, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    print(f'{len(move_tiers.MOVES)} attaques, src/data/isaac_move_tiers.h écrit')


if __name__ == '__main__':
    main()
