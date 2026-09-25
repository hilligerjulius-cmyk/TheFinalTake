"""The seven downtown buildings from AFTCityShell::Building(), one mesh each."""

from ftb import detail as D
from ftb import layout as L
from ftb import palette as C
from ftb.registry import P, asset

F = "City/Buildings"


def _calls():
    return L.kit_calls("city", "Building")


def _slug(s):
    return "".join(w.capitalize() for w in s.replace("&", " ").split())


def _building(call):
    args = call["call"]["args"]
    X0, X1, north, H, col_lin, sign, sign_lin, seed = args
    col = L.lin_to_srgb(col_lin)
    scol = L.lin_to_srgb(sign_lin)
    front = 300.0 if north else -600.0
    d = -1.0 if north else 1.0  # towards the road
    mid, W = (X0 + X1) / 2, X1 - X0
    name = "SM_City_Bld_%s" % _slug(sign)
    pv = (mid, front, 0.0)

    def covers():
        for c in _calls():
            if c["call"]["args"][5] == sign:
                return c["records"]
        return []

    @asset(name, F,
           desc="%s building (%d x %d cm, %s side): %s facade with cornice, pilasters and belt course, framed windows (lit pattern from the code), shop front with display props, striped awning with a scalloped valance, neon sign board, door with canopy, roof furniture from the code." % (sign, W, H, "north" if north else "south", "navy/plum"),
           replaces=["AFTCityShell::Building('%s') (FTCity.cpp:187-257, called from FTCity.cpp:%d)" % (sign, call["call"]["line"])],
           placements=lambda: [P(pv)], covers=covers,
           pivot="world-aligned, pivot (%g, %g, 0) = middle of the street facade at ground level; the facade faces %s" % (mid, front, "-Y" if north else "+Y"),
           integration="Sign text stays a TextRender (or use the SM_Sign_City_* letters).", view=(0.35, d * 1.0, 0.45))
    def build(a):
        a.records(covers(), pivot=pv, bevel=2)
        F_ = lambda x, z, out: (x - mid, d * out, z)
        shop_w = W * 0.58
        shop_x = X0 + W * 0.36
        door_x = X0 + W * 0.8
        # pilasters + belt course
        for x in (X0 + 12, X1 - 12):
            a.box((24, 10, H - 40), at=F_(x, (H - 40) / 2, 5), col=C.shade(col, 1.25), bevel=3)
        a.box((W, 10, 18), at=F_(mid, 480, 5), col=C.shade(col, 1.35), bevel=3)
        a.box((W + 10, 16, 14), at=F_(mid, H - 58, 8), col=C.shade(col, 1.2), bevel=3)
        for x in range(int(X0) + 60, int(X1) - 40, 90):
            a.box((20, 14, 16), at=F_(x, H - 48, 10), col=C.shade(col, 1.45), bevel=3)
        # window frames on the upper floors
        for r in covers():
            if r["line"] != 233 or r["center"][2] < 500:
                continue
            x, z = r["center"][0], r["center"][2]
            a.box((8, 6, 138), at=F_(x - 54, z, 4), col=C.CREAM_DARK, bevel=1.5)
            a.box((8, 6, 138), at=F_(x + 54, z, 4), col=C.CREAM_DARK, bevel=1.5)
            a.box((116, 6, 10), at=F_(x, z + 68, 4), col=C.CREAM_DARK, bevel=1.5)
            a.box((4, 5, 128), at=F_(x, z, 3), col=C.shade(col, 0.7), bevel=0.6)
            a.box((100, 5, 4), at=F_(x, z + 18, 3), col=C.shade(col, 0.7), bevel=0.6)
        # shop display props (in front of the glowing window)
        rng = a.rng
        kind = sign.split()[0]
        for k in range(4):
            x = shop_x - shop_w * 0.36 + k * shop_w * 0.24
            if kind == "RECORDS":
                a.cyl(26, 2, at=F_(x, 150, 9), rot=(0, 0, 90), col=C.INK, sides=16, bevel=0.5)
                a.cyl(8, 2.4, at=F_(x, 150, 9.5), rot=(0, 0, 90), col=[C.CORAL, C.CYAN, C.YELLOW, C.MAGENTA][k], sides=10)
            elif kind == "PIZZA":
                a.cyl(24, 3, at=F_(x, 120, 9), rot=(0, 0, 90), col=C.SAND, sides=12, bevel=1)
                for j in range(4):
                    a.cyl(4, 1, at=F_(x + rng.uniform(-12, 12), 120 + rng.uniform(-12, 12), 11), rot=(0, 0, 90), col=C.RED, sides=8)
            elif kind == "DINER":
                a.cyl(10, 26, at=F_(x, 80, 12), col=C.CHROME, sides=10, bevel=1, rough=0.2)
                a.cyl(16, 6, at=F_(x, 96, 12), col=C.RED, sides=12, bevel=2)
            elif kind == "CAMERA":
                a.box((34, 20, 24), at=F_(x, 110, 12), col=C.CHARCOAL, bevel=4)
                a.cyl(8, 14, at=F_(x, 110, 22), rot=(0, 0, 90), col=C.GREY_DARK, sides=10)
            elif kind == "LAUNDROMAT":
                a.box((44, 16, 50), at=F_(x, 95, 12), col=C.WHITE, bevel=5)
                a.cyl(14, 4, at=F_(x, 98, 20), rot=(0, 0, 90), col=C.SKY_BLUE, sides=14, bevel=1)
            elif kind == "PAWN":
                a.cyl(10, 30, at=F_(x, 110, 12), col=C.BRASS, sides=10, bevel=1, rough=0.3)
                a.sphere(10, at=F_(x, 132, 12), col=C.BRASS, segs=10, rings=6, rough=0.3)
            else:
                a.box((30, 16, 40), at=F_(x, 100, 12), col=[C.CORAL, C.TEAL, C.YELLOW, C.CREAM][k], bevel=4)
        a.box((shop_w, 20, 10), at=F_(shop_x, 62, 10), col=C.shade(col, 0.8), bevel=2)
        # scalloped awning valance (code stripes sit at z 300, out 58)
        n = 12
        for s in range(n):
            x = shop_x - shop_w / 2 + (s + 0.5) * shop_w / n
            a.cyl(shop_w / n * 0.5, 3, at=F_(x, 283, 112), rot=(0, 0, 90), col=scol if (s // 2) % 2 == 0 else C.CREAM, sides=10, bevel=0.5)
        for s in (-1, 1):
            a.tube([F_(shop_x + s * shop_w / 2, 250, 1), F_(shop_x + s * shop_w / 2, 290, 100)], 2, col=C.CHARCOAL, sides=6)
        # door canopy + step + number
        a.box((170, 60, 10), at=F_(door_x, 275, 30), col=C.shade(col, 1.3), bevel=3)
        a.box((170, 30, 8), at=F_(door_x, 4, 16), col=C.PLAZA_STONE, bevel=2)
        a.cyl(10, 3, at=F_(door_x + 90, 230, 6), rot=(0, 0, 90), col=C.BRASS, sides=12, bevel=1, rough=0.3)
        # fire escape on the tall ones
        if H >= 1750:
            fx = X0 + W * 0.3
            for z in range(560, int(H) - 300, 230):
                a.box((260, 70, 5), at=F_(fx, z, 38), col=C.CHARCOAL, bevel=1.2)
                a.box((260, 4, 4), at=F_(fx, z + 50, 72), col=C.CHARCOAL, bevel=0.8)
                for k in range(9):
                    a.box((3, 3, 50), at=F_(fx - 125 + k * 31, z + 25, 72), col=C.CHARCOAL, bevel=0)
                if z + 230 < H - 300:
                    a.box((34, 60, 235), at=F_(fx + (60 if (z // 230) % 2 else -60), z + 115, 38), rot=(0, 0, 0), col=C.GREY_DARK, bevel=1.5)
        # detail pass: rolling-shutter box over the shop window, stone base course, posters + tags on the pilasters,
        # a drainpipe and utility meters on the side walls, house number and mail slot, gutter under the cornice
        fn = "-y" if north else "+y"
        a.box((shop_w + 10, 22, 24), at=F_(shop_x, 262, 12), col=C.shade(col, 0.85), bevel=3)
        a.box((shop_w + 6, 2, 4), at=F_(shop_x, 252, 23), col=C.GREY_DARK, bevel=0.5)
        a.box((W, 6, 44), at=F_(mid, 22, 3), col=C.shade(col, 0.72), bevel=1.5)
        for x in range(int(X0) + 40, int(X1) - 20, 80):
            a.box((0.8, 7, 44), at=F_(x, 22, 3.2), col=C.shade(col, 0.5), bevel=0, jitter=0, wear=False)
        for k, x in enumerate((X0 + 12, X1 - 12)):
            a.box((18, 1, 26), at=F_(x, 170 + k * 20, 10.6), rot=(0, 0, rng.uniform(-5, 5)), col=[C.CREAM, C.SKY_BLUE][k], bevel=0.1, jitter=0)
            a.box((14, 0.6, 3), at=F_(x, 178 + k * 20, 11.2), col=C.CORAL, bevel=0, jitter=0, wear=False)
            a.box((rng.uniform(16, 22), 0.6, rng.uniform(6, 10)), at=F_(x, 60, 10.4), rot=(0, 0, rng.uniform(-10, 10)),
                  col=[C.MAGENTA, C.CYAN, C.YELLOW][(k + len(sign)) % 3], bevel=0, jitter=0)
        a.box((W + 12, 18, 10), at=F_(mid, H - 72, 14), col=C.GREY, bevel=2, rough=0.5)
        a.box((34, 5, 14), at=F_(door_x + 60, 200, 3), col=C.BRASS, bevel=1, rough=0.3)
        a.box((24, 3, 3), at=F_(door_x - 50, 110, 3), col=C.BRASS, bevel=0.5, rough=0.3)
        for sx, xw in ((-1, X0), (1, X1)):
            a.tube([(xw - mid + sx * 8, d * -30, 0), (xw - mid + sx * 8, d * -30, H - 80), (xw - mid + sx * 8, d * -10, H - 60)], 5, col=C.GREY_DARK, sides=8, rough=0.5)
            for z in range(150, int(H) - 100, 300):
                a.box((10, 14, 6), at=(xw - mid + sx * 6, d * -30, z), col=C.GREY, bevel=1)
            if sx < 0:
                a.box((10, 50, 70), at=(xw - mid - 5, d * -120, 130), col=C.GREY, bevel=2, rough=0.5)
                for k in range(2):
                    a.cyl(8, 3, at=(xw - mid - 10.5, d * -110 + k * d * -22, 140), rot=(90, 0, 0), col=C.GLASS, sides=12, bevel=0.5)
                a.cyl(2, 120, at=(xw - mid - 5, d * -120, 35), col=C.GREY, sides=6)
            D.vent(a, (xw - mid, d * -200, 260), "+x" if sx > 0 else "-x", 60, 40, slats=5, col=C.shade(col, 0.8))
        # side walls and back: cornice + belt course wrapped round the corners, framed windows (some lit), a back
        # door with a lamp and a fire-escape ladder (the code builds the walls as one plain box, 2000 cm deep)
        depth = 2000.0
        for sx in (-1, 1):
            xw = sx * W / 2
            n = "+x" if sx > 0 else "-x"
            a.box((12, depth - 24, 44), at=(xw + sx * 6, d * -(depth / 2 + 12), H - 22), col=C.shade(col, 1.4), bevel=3)
            a.box((10, depth - 24, 18), at=(xw + sx * 5, d * -(depth / 2 + 12), 480), col=C.shade(col, 1.35), bevel=3)
            for z in range(760, int(H) - 250, 300):
                for off in (420, 980, 1560):
                    c = (xw, d * -off, z)
                    lit = rng.random() < 0.35
                    a.box(D._size(n, 90, 120, 1.0), at=D.on(c, n, dn=0.5), col=C.WINDOW_WARM if lit else C.WINDOW_DARK, glow=0.8 if lit else None)
                    D.border(a, c, n, 102, 132, bar=6, t=2.0, col=C.CREAM_DARK)
                    a.box(D._size(n, 110, 7, 6), at=D.on(c, n, 0, -68, 3), col=C.CREAM, bevel=1.5)
        nb = "+y" if d < 0 else "-y"
        yb = d * -depth
        for z in range(460, int(H) - 250, 300):
            for x in range(int(-W / 2) + 150, int(W / 2) - 100, 280):
                c = (x, yb, z)
                lit = rng.random() < 0.25
                a.box(D._size(nb, 80, 100, 1.0), at=D.on(c, nb, dn=0.5), col=C.WINDOW_WARM if lit else C.WINDOW_DARK, glow=0.6 if lit else None)
                D.border(a, c, nb, 90, 110, bar=5, t=1.6, col=C.shade(col, 1.3))
        dc = (W * 0.25, yb, 110)
        D.plate(a, dc, nb, 110, 220, t=3, col=C.CHARCOAL, screws=False)
        D.handle(a, D.on(dc, nb, -30, 0, 3), nb, 26, along="v", r=1.6, standoff=4, col=C.GREY)
        a.box(D._size(nb, 30, 18, 14), at=D.on(dc, nb, 0, 135, 7), col=C.GREY_DARK, bevel=2)
        a.box(D._size(nb, 18, 8, 1), at=D.on(dc, nb, 0, 128, 14.5), col=C.WINDOW_WARM, glow=4)
        for sy in (-1, 1):
            a.box(D._size(nb, 5, H - 300, 5), at=D.on((-W * 0.3, yb, H / 2), nb, sy * 22, 0, 20), col=C.CHARCOAL, bevel=1)
        for z in range(300, int(H) - 150, 32):
            a.box(D._size(nb, 44, 2.4, 2.4), at=D.on((-W * 0.3, yb, z), nb, 0, 0, 20), col=C.CHARCOAL, bevel=0.5)
        a.box((W + 20, 30, 16), at=(0, yb + d * -8, H + 6), col=C.shade(col, 1.2), bevel=3)
        # AC boxes on a few windows
        for r in covers():
            if r["line"] == 233 and r["center"][2] > 700 and L.lin_to_srgb(r["color"])[0] < 0.5 and rng.random() < 0.3:
                a.box((50, 36, 34), at=F_(r["center"][0], r["center"][2] - 50, 20), col=C.GREY, bevel=4)
                a.box((2, 30, 24), at=F_(r["center"][0], r["center"][2] - 50, 38.5), col=C.GREY_DARK, bevel=0.5)
    return build


for _c in _calls():
    _building(_c)
