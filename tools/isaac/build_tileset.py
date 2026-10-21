#!/usr/bin/env python3
"""
Génère les tilesets secondaires du donjon (un par décor de biome), dans data/tilesets/secondary/isaac_*/.

Tous ont la même numérotation de metatiles, pour que isaac_dungeon.c dessine les salles de la même façon :
  - 0x200 à 0x20F : les rôles de ROLES (sol, rochers, bords et coins de falaise), pris dans le tileset
    d'origine du décor (correspondances de BIOME_TILESETS, vérifiées sur des salles rendues) ; un metatile
    d'origine en dessous de 0x200 est pris tel quel dans le tileset primaire General ;
  - 0x210 : un panneau posé sur le sol (salle de la victoire) ;
  - 0x211 : un banc de sable d'une case dans l'eau (boutique de l'océan) ;
  - 0x212, 0x213 : deux décorations d'une case du décor (rochers, glace...), posées dans les coins des salles ;
  - à partir de 0x240 (ISAAC_MAT_FIRST) : les paillassons devant les portes des salles spéciales : tapis rouge
    et or avec la tête du Pokémon de la mini-carte (Mewtwo boss, Pikachu trésor, Kecleon boutique) ou une Poké
    Ball (capture), découpées dans les sprites de face du jeu.

Chaque paillasson existe en 3 orientations (la tête regarde vers la porte) :
  - HORIZONTAL (3 x 2 metatiles) : portes nord et sud, tête à l'endroit ;
  - WEST (2 x 3 metatiles) : porte ouest, tête tournée d'un quart de tour vers la gauche ;
  - EAST (2 x 3 metatiles) : porte est, tête tournée d'un quart de tour vers la droite.
Metatile du paillasson n° s (0 boss, 1 trésor, 2 boutique, 3 capture), orientation o (0, 1, 2),
case i (de gauche à droite puis de haut en bas) : ISAAC_MAT_FIRST + (s * 3 + o) * 6 + i.

Palettes : celles du décor (renumérotées à partir de 6), puis une par motif de paillasson : couleurs 1 à 4 =
tapis (communes), 5 à 15 = tête. Le sol n'a pas le comportement d'origine (pas de glace qui glisse).

Usage : python3 tools/isaac/build_tileset.py
"""
import os, struct
from PIL import Image, ImageDraw

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
SECONDARY = os.path.join(ROOT, 'data/tilesets/secondary')
PRIMARY = os.path.join(ROOT, 'data/tilesets/primary/general')

# Rôles des metatiles 0x200 à 0x20F (même ordre que les TILE_* de isaac_dungeon.c)
DECOS = ['DECO_1', 'DECO_2']  # décorations des coins de salle, à partir de DECO_FIRST
DECO_FIRST = 0x212
ROLES = ['FLOOR', 'ROCK_1', 'ROCK_2', 'TL', 'T', 'TR', 'L', 'C', 'R', 'BL', 'B', 'BR', 'IN_SE', 'IN_SW', 'IN_NE', 'IN_NW']
SIGN_METATILE = 0x210
MAT_FIRST = 0x240
PRIMARY_SIGN = 0x003  # panneau du tileset General
SANDBAR_WATER = 0x170  # eau de General
SANDBAR_TOP = [0x59D6, 0x55D0, 0x51D6, 0x5DD0]  # coins de rive (tuiles 0x1D6 et 0x1D0 de General retournées)

# Décors : (tileset produit, tileset d'origine, metatile d'origine pour chaque rôle)
BIOME_TILESETS = [
    ('isaac_cave', 'cave', dict(  # Route Victoire (grotte standard et grotte obscure)
        FLOOR=0x201, ROCK_1=0x202, ROCK_2=0x203, TL=0x208, T=0x209, TR=0x20A, L=0x210, C=0x211, R=0x212,
        BL=0x218, B=0x219, BR=0x21A, IN_SE=0x21B, IN_SW=0x21C, IN_NE=0x223, IN_NW=0x224, DECO_1=0x202, DECO_2=0x203)),
    ('isaac_fire', 'lavaridge', dict(  # Chemin Ardent
        FLOOR=0x308, ROCK_1=0x271, ROCK_2=0x271, TL=0x30A, T=0x30C, TR=0x30B, L=0x306, C=0x271, R=0x307,
        BL=0x30E, B=0x274, BR=0x30F, IN_SE=0x27B, IN_SW=0x27C, IN_NE=0x27F, IN_NW=0x27E, DECO_1=0x2C4, DECO_2=0x300)),
    ('isaac_mystic', 'meteor_falls', dict(  # Chutes Météore : terrasses lavande, faces de falaise sombres
        FLOOR=0x201, ROCK_1=0x231, ROCK_2=0x231, TL=0x286, T=0x286, TR=0x286, L=0x228, C=0x201, R=0x212,
        BL=0x231, B=0x231, BR=0x231, IN_SE=0x213, IN_SW=0x214, IN_NE=0x286, IN_NW=0x286, DECO_1=0x231, DECO_2=0x231)),
    ('isaac_ice', 'cave', dict(  # Grotte Tréfonds, salle gelée (sol de glace pointillée, qui ne glisse pas)
        FLOOR=0x385, ROCK_1=0x381, ROCK_2=0x381, TL=0x378, T=0x379, TR=0x37A, L=0x380, C=0x381, R=0x382,
        BL=0x388, B=0x389, BR=0x38A, IN_SE=0x37B, IN_SW=0x37C, IN_NE=0x383, IN_NW=0x384, DECO_1=0x37D, DECO_2=0x278)),
    ('isaac_poison', 'rusturf_tunnel', dict(  # Tunnel Mérazon (moussu)
        FLOOR=0x201, ROCK_1=0x211, ROCK_2=0x211, TL=0x208, T=0x209, TR=0x20A, L=0x210, C=0x211, R=0x212,
        BL=0x218, B=0x219, BR=0x21A, IN_SE=0x21B, IN_SW=0x21C, IN_NE=0x223, IN_NW=0x224, DECO_1=0x203, DECO_2=0x206)),
    # Océan de Hoenn (Routes 105, 134) : eau animée, murs en crêtes rocheuses de la mer, rocher immergé pour les
    # bouts de mur isolés ; tout dans le tileset primaire General (le tileset d'origine ne sert pas). Tous les rôles
    # ont le comportement de l'eau : le joueur surfe partout, même à travers les rochers des portes secrètes (les
    # murs sont bloqués par leur collision). Les crêtes n'ont que deux coins rentrants (sol au nord-ouest 0x0B4, au
    # sud-est 0x074) : les deux autres prennent le centre de la roche.
    ('isaac_ocean', 'pacifidlog', dict(
        FLOOR=0x170, ROCK_1=0x0BD, ROCK_2=0x0BD, TL=0x172, T=0x17B, TR=0x174, L=0x17A, C=0x071, R=0x17C,
        BL=0x182, B=0x079, BR=0x184, IN_SE=0x074, IN_SW=0x071, IN_NE=0x071, IN_NW=0x0B4, DECO_1=0x0BD, DECO_2=0x0BD)),
]
SURF_TILESETS = {'isaac_ocean'}  # décors où le joueur surfe : tous les rôles ont le comportement du sol (eau)
NUM_PRIMARY_TILES = 512
NUM_SECONDARY_TILES = 512
NUM_SECONDARY_METATILES = 512
FLOOR_ATTRIBUTES = 0x0008       # comme le sol de la grotte (0x201)

# Motifs des paillassons : espèce (sprite de face) ou Poké Ball
MATS = ['mewtwo', 'pikachu', 'kecleon', 'pokeball']
FIRST_SECONDARY_PALETTE = 6
NUM_PALETTES = 13  # 0 à 5 : General, 6 à 12 : tileset secondaire
# Tête dans le sprite de face 64 x 64 (x0, y0, x1, y1), et zones à effacer (x0, y0) : tout ce qui est à
# droite de x0 et sous y0 (queue de Pikachu)
HEADS = {'pikachu': (16, 10, 47, 33), 'mewtwo': (25, 1, 45, 21), 'kecleon': (18, 10, 46, 34)}
CUTS = {'pikachu': (39, 22)}

# Tapis : couleurs 1 à 4 de chaque palette
MAT_COLORS = [(120, 45, 45), (80, 28, 30), (224, 184, 96), (168, 120, 56)]
BASE, DARK, GOLD, GOLD2 = 1, 2, 3, 4
HEAD_COLORS = 11  # couleurs 5 à 15
MAT_W, MAT_H = 48, 32


def read_pal(path):
    lines = open(path).read().split('\n')[3:19]
    return [tuple(int(c) for c in line.split()[:3]) for line in lines if line.strip()]


def write_pal(path, colors):
    colors = (list(colors) + [(0, 0, 0)] * 16)[:16]
    with open(path, 'w', newline='\r\n') as f:
        f.write('JASC-PAL\n0100\n16\n')
        for c in colors:
            f.write('%d %d %d\n' % c)


def front_sprite(name):
    p = Image.open(os.path.join(ROOT, 'graphics/pokemon', name, 'anim_front.png')).crop((0, 0, 64, 64))
    rgba = p.convert('RGBA')
    alpha = Image.new('L', (64, 64))
    alpha.putdata([0 if v == 0 else 255 for v in p.getdata()])
    rgba.putalpha(alpha)
    if name in CUTS:
        x0, y0 = CUTS[name]
        for y in range(y0, 64):
            for x in range(x0, 64):
                rgba.putpixel((x, y), (0, 0, 0, 0))
    return rgba.crop(HEADS[name])


def pokeball(size=18):
    im = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([0, 0, size - 1, size - 1], fill=(240, 240, 240, 255), outline=(32, 32, 40, 255))
    d.pieslice([0, 0, size - 1, size - 1], 180, 360, fill=(224, 48, 48, 255), outline=(32, 32, 40, 255))
    d.line([(1, size // 2), (size - 2, size // 2)], fill=(32, 32, 40, 255), width=2)
    c = size // 2
    d.ellipse([c - 4, c - 4, c + 3, c + 3], fill=(32, 32, 40, 255))
    d.ellipse([c - 2, c - 2, c + 1, c + 1], fill=(250, 250, 250, 255))
    return im


def quantize_head(head):
    """Tête RGBA -> (indices 5..15 ou 0 si transparent, palette de la tête)"""
    opaque = [c for c in head.getdata() if c[3]]
    sample = Image.new('RGB', (len(opaque), 1))
    sample.putdata([c[:3] for c in opaque])
    q = sample.quantize(colors=HEAD_COLORS, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = (q.getpalette() + [0] * HEAD_COLORS * 3)[:HEAD_COLORS * 3]  # moins de couleurs pour la Poké Ball
    colors = [tuple(pal[i * 3: i * 3 + 3]) for i in range(HEAD_COLORS)]

    def nearest(c):
        return min(range(HEAD_COLORS), key=lambda i: sum((a - b) ** 2 for a, b in zip(colors[i], c[:3])))

    indices = [0 if c[3] == 0 else 5 + nearest(c) for c in head.getdata()]
    return indices, colors


def mat_with_head(name):
    """Paillasson horizontal 48 x 32 (indices de palette) + palette du motif"""
    head = pokeball() if name == 'pokeball' else front_sprite(name)
    indices, colors = quantize_head(head)
    im = Image.new('P', (MAT_W, MAT_H), BASE)
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, MAT_W - 1, MAT_H - 1], outline=DARK)
    d.rectangle([2, 2, MAT_W - 3, MAT_H - 3], outline=GOLD)
    d.rectangle([3, 3, MAT_W - 4, MAT_H - 4], outline=GOLD2)
    for x in range(1, MAT_W - 1, 2):  # franges
        d.point((x, 0), fill=GOLD)
        d.point((x, MAT_H - 1), fill=GOLD)
    ox, oy = (MAT_W - head.width) // 2, (MAT_H - head.height) // 2
    for i, v in enumerate(indices):
        if v:
            im.putpixel((ox + i % head.width, oy + i // head.width), v)
    return im, MAT_COLORS + colors


def tiles_of(im):
    """Tuiles 8 x 8 d'une image (indices), ligne par ligne"""
    out = []
    for ty in range(im.height // 8):
        for tx in range(im.width // 8):
            out.append(tuple(im.getpixel((tx * 8 + x, ty * 8 + y)) for y in range(8) for x in range(8)))
    return out


def read_u16(path):
    data = open(path, 'rb').read()
    return list(struct.unpack('<%dH' % (len(data) // 2), data))


def build(out_name, source_name, mapping, mats):
    """Un tileset du donjon à partir d'un tileset d'origine ; mats : paillassons déjà dessinés"""
    src = os.path.join(SECONDARY, source_name)
    out = os.path.join(SECONDARY, out_name)
    os.makedirs(os.path.join(out, 'palettes'), exist_ok=True)
    src_tiles = Image.open(os.path.join(src, 'tiles.png'))
    src_meta = read_u16(os.path.join(src, 'metatiles.bin'))
    src_attr = read_u16(os.path.join(src, 'metatile_attributes.bin'))
    src_pals = [read_pal(os.path.join(src, 'palettes', '%02d.pal' % i)) for i in range(16)]
    primary_meta = read_u16(os.path.join(PRIMARY, 'metatiles.bin'))
    primary_attr = read_u16(os.path.join(PRIMARY, 'metatile_attributes.bin'))

    tiles = [tuple([0] * 64)]  # tuile 0 vide
    tile_index = {tiles[0]: 0}

    def add_tile(pixels):
        if pixels not in tile_index:
            tile_index[pixels] = len(tiles)
            tiles.append(pixels)
        return tile_index[pixels]

    # Palettes du décor utilisées par les rôles : renumérotées à partir de 6
    pal_remap = {}
    for role in ROLES + DECOS:
        if mapping[role] < NUM_PRIMARY_TILES:
            continue  # metatile de General : palettes de General
        m = mapping[role] - NUM_PRIMARY_TILES
        for e in src_meta[m * 8: m * 8 + 8]:
            pal = e >> 12
            if pal >= FIRST_SECONDARY_PALETTE and pal not in pal_remap:
                pal_remap[pal] = FIRST_SECONDARY_PALETTE + len(pal_remap)
    assert FIRST_SECONDARY_PALETTE + len(pal_remap) + len(mats) <= NUM_PALETTES, f'{out_name} : trop de palettes'

    src_w = src_tiles.width // 8
    tile_remap = {}

    def convert(e):
        t, pal = e & 0x3FF, e >> 12
        if t >= NUM_PRIMARY_TILES:
            if t not in tile_remap:
                s = t - NUM_PRIMARY_TILES
                box = ((s % src_w) * 8, (s // src_w) * 8)
                pixels = tuple(src_tiles.getpixel((box[0] + x, box[1] + y)) for y in range(8) for x in range(8))
                tile_remap[t] = NUM_PRIMARY_TILES + add_tile(pixels)
            t = tile_remap[t]
        pal = pal_remap.get(pal, pal)
        return (e & 0x0C00) | (pal << 12) | t  # garde les retournements

    # 1. Rôles (0x200 à 0x20F)
    metatiles, attributes = [], []
    for role in ROLES:
        if mapping[role] < NUM_PRIMARY_TILES:  # metatile de General, tel quel (comportement compris)
            m = mapping[role]
            metatiles.extend(primary_meta[m * 8: m * 8 + 8])
            attributes.append(primary_attr[m])
            continue
        m = mapping[role] - NUM_PRIMARY_TILES
        metatiles.extend(convert(e) for e in src_meta[m * 8: m * 8 + 8])
        # Sol : comportement du sol de la grotte, mais type de calque de la source (un sol qui a aussi un calque
        # du dessus doit le garder derrière les sprites)
        attributes.append((FLOOR_ATTRIBUTES | (src_attr[m] & 0xF000)) if role == 'FLOOR' else src_attr[m])
    floor_behavior = attributes[0] & 0x0FFF  # paillassons : même comportement que le sol (eau : on y surfe)
    if out_name in SURF_TILESETS:  # tout est de l'eau (voir BIOME_TILESETS)
        attributes = [(a & 0xF000) | floor_behavior for a in attributes]

    # 2. Panneau sur le sol (0x210) : sol dessous, panneau de General dessus
    floor = metatiles[0:4]
    metatiles.extend(floor + primary_meta[PRIMARY_SIGN * 8: PRIMARY_SIGN * 8 + 4])
    attributes.append(0x1000)  # calque du dessus derrière le joueur, comme les rochers

    # 2 bis. Banc de sable d'une case dans l'eau (0x211, sous Kecleon dans l'océan) : eau de General dessous, un coin
    # de rive des îlots de sable de General dans chaque quart dessus
    metatiles.extend(primary_meta[SANDBAR_WATER * 8: SANDBAR_WATER * 8 + 4] + SANDBAR_TOP)
    attributes.append(0x1000 | floor_behavior)

    # 2 ter. Décorations des coins de salle (DECO_FIRST et suivants), comme les rôles
    assert len(attributes) == DECO_FIRST - NUM_PRIMARY_TILES
    for role in DECOS:
        m = mapping[role]
        if m < NUM_PRIMARY_TILES:
            metatiles.extend(primary_meta[m * 8: m * 8 + 8])
            attributes.append((primary_attr[m] & 0xF000) | floor_behavior)
        else:
            m -= NUM_PRIMARY_TILES
            metatiles.extend(convert(e) for e in src_meta[m * 8: m * 8 + 8])
            attributes.append((src_attr[m] & 0xF000) | floor_behavior)

    # 3. Rien jusqu'aux paillassons
    while len(attributes) < MAT_FIRST - NUM_PRIMARY_TILES:
        metatiles.extend([0] * 8)
        attributes.append(0)

    # 4. Paillassons : une palette par motif après celles du décor
    palettes = [[(0, 0, 0)] * 16 for _ in range(16)]
    for old, new in pal_remap.items():
        palettes[new] = src_pals[old]
    for k, (images, colors) in enumerate(mats):
        pal_num = FIRST_SECONDARY_PALETTE + len(pal_remap) + k
        palettes[pal_num] = [(0, 0, 0)] + colors
        for im in images:
            w, h = im.width // 16, im.height // 16
            for my in range(h):
                for mx in range(w):
                    block = im.crop((mx * 16, my * 16, mx * 16 + 16, my * 16 + 16))
                    for t in tiles_of(block):
                        metatiles.append((pal_num << 12) | (NUM_PRIMARY_TILES + add_tile(t)))
                    metatiles.extend([0, 0, 0, 0])  # calque du dessus vide
                    attributes.append(floor_behavior)

    assert len(tiles) <= NUM_SECONDARY_TILES, f'{out_name} : {len(tiles)} tuiles'
    assert len(attributes) <= NUM_SECONDARY_METATILES, f'{out_name} : {len(attributes)} metatiles'

    # tiles.png : 16 tuiles par ligne (seuls les indices comptent)
    rows = (len(tiles) + 15) // 16
    sheet = Image.new('P', (128, rows * 8), 0)
    sheet.putpalette(src_tiles.getpalette())
    for i, t in enumerate(tiles):
        for p, v in enumerate(t):
            sheet.putpixel(((i % 16) * 8 + p % 8, (i // 16) * 8 + p // 8), v)
    sheet.save(os.path.join(out, 'tiles.png'))
    open(os.path.join(out, 'metatiles.bin'), 'wb').write(struct.pack('<%dH' % len(metatiles), *metatiles))
    open(os.path.join(out, 'metatile_attributes.bin'), 'wb').write(struct.pack('<%dH' % len(attributes), *attributes))
    for i, pal in enumerate(palettes):
        write_pal(os.path.join(out, 'palettes', '%02d.pal' % i), pal)
    print(f'{out_name} : {len(tiles)} tuiles, {len(attributes)} metatiles, {len(pal_remap)} + {len(mats)} palettes')


def main():
    mats = []
    for name in MATS:
        horizontal, colors = mat_with_head(name)
        mats.append(((horizontal, horizontal.rotate(90, expand=True), horizontal.rotate(-90, expand=True)), colors))
    for out_name, source_name, mapping in BIOME_TILESETS:
        build(out_name, source_name, mapping, mats)


if __name__ == '__main__':
    main()
