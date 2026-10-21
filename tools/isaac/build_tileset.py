#!/usr/bin/env python3
"""
Génère le tileset secondaire du donjon, data/tilesets/secondary/isaac_cave/ :
  - les metatiles 0x200 à 0x23F du tileset Cave (Route Victoire), aux mêmes numéros : sol, falaises,
    rochers, échelle... (le reste de Cave n'est pas repris, ça libère de la place) ;
  - à partir de 0x240 (ISAAC_MAT_FIRST), les paillassons posés devant les portes des salles spéciales :
    tapis rouge et or avec la tête du Pokémon de la mini-carte (Mewtwo boss, Pikachu trésor, Kecleon
    boutique) ou une Poké Ball (capture). Les têtes sont découpées dans les sprites de face du jeu.

Chaque paillasson existe en 3 orientations (la tête regarde vers la porte) :
  - HORIZONTAL (3 x 2 metatiles) : portes nord et sud, tête à l'endroit ;
  - WEST (2 x 3 metatiles) : porte ouest, tête tournée d'un quart de tour vers la gauche ;
  - EAST (2 x 3 metatiles) : porte est, tête tournée d'un quart de tour vers la droite.
Metatile du paillasson n° s (0 boss, 1 trésor, 2 boutique, 3 capture), orientation o (0, 1, 2),
case i (de gauche à droite puis de haut en bas) : ISAAC_MAT_FIRST + (s * 3 + o) * 6 + i.

Chaque motif a sa palette : couleurs 1 à 4 = tapis (communes), 5 à 15 = tête.

Usage : python3 tools/isaac/build_tileset.py
"""
import os, struct
from PIL import Image, ImageDraw

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
CAVE = os.path.join(ROOT, 'data/tilesets/secondary/cave')
OUT = os.path.join(ROOT, 'data/tilesets/secondary/isaac_cave')

KEEP_METATILES = 0x40           # metatiles de Cave repris (0x200 à 0x23F)
NUM_PRIMARY_TILES = 512
NUM_SECONDARY_TILES = 512
NUM_SECONDARY_METATILES = 512
FLOOR_ATTRIBUTES = 0x0008       # comme le sol de la grotte (0x201)

# Motifs : espèce (sprite de face) ou Poké Ball, palette secondaire libre dans les metatiles repris
MATS = [
    ('mewtwo', 7),
    ('pikachu', 8),
    ('kecleon', 9),
    ('pokeball', 11),
]
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


def main():
    os.makedirs(os.path.join(OUT, 'palettes'), exist_ok=True)
    cave_tiles = Image.open(os.path.join(CAVE, 'tiles.png'))
    cave_meta = struct.unpack('<%dH' % (os.path.getsize(os.path.join(CAVE, 'metatiles.bin')) // 2),
                              open(os.path.join(CAVE, 'metatiles.bin'), 'rb').read())
    cave_attr = struct.unpack('<%dH' % (os.path.getsize(os.path.join(CAVE, 'metatile_attributes.bin')) // 2),
                              open(os.path.join(CAVE, 'metatile_attributes.bin'), 'rb').read())

    tiles = [tuple([0] * 64)]  # tuile 0 vide
    tile_index = {tiles[0]: 0}

    def add_tile(pixels):
        if pixels not in tile_index:
            tile_index[pixels] = len(tiles)
            tiles.append(pixels)
        return tile_index[pixels]

    # 1. Metatiles repris de Cave (mêmes numéros), tuiles secondaires renumérotées
    cave_w = cave_tiles.width // 8
    remap = {}
    metatiles, attributes = [], []
    for m in range(KEEP_METATILES):
        for e in cave_meta[m * 8: m * 8 + 8]:
            t = e & 0x3FF
            if t >= NUM_PRIMARY_TILES:
                if t not in remap:
                    s = t - NUM_PRIMARY_TILES
                    box = ((s % cave_w) * 8, (s // cave_w) * 8)
                    pixels = tuple(cave_tiles.getpixel((box[0] + x, box[1] + y)) for y in range(8) for x in range(8))
                    remap[t] = NUM_PRIMARY_TILES + add_tile(pixels)
                e = (e & ~0x3FF) | remap[t]
            metatiles.append(e)
        attributes.append(cave_attr[m])

    # 2. Paillassons
    palettes = [read_pal(os.path.join(CAVE, 'palettes', '%02d.pal' % i)) for i in range(16)]
    for name, pal_num in MATS:
        horizontal, colors = mat_with_head(name)
        palettes[pal_num] = [(0, 0, 0)] + colors
        for im in (horizontal, horizontal.rotate(90, expand=True), horizontal.rotate(-90, expand=True)):
            w, h = im.width // 16, im.height // 16
            for my in range(h):
                for mx in range(w):
                    block = im.crop((mx * 16, my * 16, mx * 16 + 16, my * 16 + 16))
                    for t in tiles_of(block):
                        metatiles.append((pal_num << 12) | (NUM_PRIMARY_TILES + add_tile(t)))
                    metatiles.extend([0, 0, 0, 0])  # calque du dessus vide
                    attributes.append(FLOOR_ATTRIBUTES)

    assert len(tiles) <= NUM_SECONDARY_TILES, f'{len(tiles)} tuiles'
    assert len(attributes) <= NUM_SECONDARY_METATILES, f'{len(attributes)} metatiles'

    # tiles.png : 16 tuiles par ligne, palette en niveaux de gris (seuls les indices comptent)
    rows = (len(tiles) + 15) // 16
    sheet = Image.new('P', (128, rows * 8), 0)
    sheet.putpalette(cave_tiles.getpalette())
    for i, t in enumerate(tiles):
        for p, v in enumerate(t):
            sheet.putpixel(((i % 16) * 8 + p % 8, (i // 16) * 8 + p // 8), v)
    sheet.save(os.path.join(OUT, 'tiles.png'))
    open(os.path.join(OUT, 'metatiles.bin'), 'wb').write(struct.pack('<%dH' % len(metatiles), *metatiles))
    open(os.path.join(OUT, 'metatile_attributes.bin'), 'wb').write(struct.pack('<%dH' % len(attributes), *attributes))
    for i, pal in enumerate(palettes):
        write_pal(os.path.join(OUT, 'palettes', '%02d.pal' % i), pal)
    print(f'{len(tiles)} tuiles, {len(attributes)} metatiles')


if __name__ == '__main__':
    main()
