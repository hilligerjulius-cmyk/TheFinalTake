"""Upper level: long stair to the projection room, the projection room itself, its ceiling and plush seats."""

from ftb import layout as L
from ftb import palette as C
from ftb.core import bm_box, bulge, xf
from ftb.registry import P, asset

F = "Studio/Projection"
UP = "AFTStudioShell::BuildUpperLevel()"


def rec(*ranges, where=None):
    return lambda: L.select("studio", lines=list(ranges), where=where)


@asset("SM_Stage4_ProjectionStairs", "Studio/Stage4",
       desc="27-step stair along the west wall up to the projection level: steel stringer, nosed treads with tape, yellow handrail on posts with ball caps.",
       replaces=[UP + " Stairs(27 steps), handrail and its six posts"],
       placements=lambda: [P((850, -1480, -120))], covers=rec((935, 942)),
       pivot="stair footprint centre on the stage floor (850, -1480, -120); climbs towards -X",
       integration="Step collision stays with the code CubeSolids; the handrail Blocker stays invisible.", view=(0.4, 1, 0.5))
def projection_stairs(a):
    pv = (850, -1480, -120)
    a.records(L.select("studio", lines=[(935, 942)]), pivot=pv, bevel=1.2)
    # stringer on the open side (Y -1365)
    a.poly([(-450, 0), (450, 0), (450, 20), (-450, 540)], 10, at=(0, 120, 0), rot=(0, 90, 0), col=C.GREY_DARK, bevel=1.5)
    for k in range(27):
        x = 1300 - 33.3 * (k + 0.5) - pv[0]
        top = 20 * (k + 1)
        a.box((5, 226, 3), at=(x + 14, 0, top + 1), col=C.GREY, bevel=1, rough=0.4)
    # handrail: tube over the code's rail + ball caps on the posts
    for r in L.select("studio", lines=[(941, 941)]):
        c = r["center"]
        a.sphere(5, at=(c[0] - pv[0], c[1] - pv[1], c[2] - pv[2] + 52), col=C.YELLOW, segs=8, rings=5)


@asset("SM_Projection_Room", F,
       desc="Projection room: landing, dark wood floor with a patterned red carpet, plum walls with padded curtains, booth window frame, door surround and the balcony railing with a brass handrail on turned balusters.",
       replaces=[UP + " landing, floor, carpet, north wall + booth glass, west door wall, balcony railing + brass rail"],
       placements=lambda: [P((-175, -1250, 420))], covers=rec((944, 947), (949, 956)),
       pivot="world-aligned, pivot (-175, -1250, 420) = projection floor level",
       integration="Ceiling is SM_Projection_Ceiling; the AFTDoor 'Door_Projection' and AFTProjector stay actors.", view=(0.6, 0.8, 0.9))
def projection_room(a):
    pv = (-175, -1250, 420)
    a.records(L.select("studio", lines=[(944, 947), (949, 956)]), pivot=pv, bevel=1.2)
    # carpet pattern: rows of little gold diamonds
    for x in range(-560, 220, 80):
        for y in range(-1560, -920, 80):
            a.box((10, 10, 0.4), at=(x - pv[0], y - pv[1], 1.3), rot=(0, 45, 0), col=C.BRASS, bevel=0, ao=False)
    # padded curtain panels on the south stage wall (Y -1600 face, room side +Y) between X -600 and 240
    for x in range(-590, 220, 60):
        b = bm_box(56, 8, 290, 3, 1)
        bulge(b, 2, axis=1)
        a.add(b, xf((x + 28 - pv[0], -1596 - pv[1], 165)), C.shade(C.PROJ_WALL, 1.25), rough=0.95)
    # booth window frame (glass X -300..100, Z 510..690 at Y -905)
    for z in (510, 690):
        a.box((420, 16, 10), at=(-100 - pv[0], -905 - pv[1] - 14, z - pv[2]), col=C.BRASS, bevel=2, rough=0.35)
    for x in (-300, 100):
        a.box((10, 16, 190), at=(x - pv[0], -905 - pv[1] - 14, 600 - pv[2]), col=C.BRASS, bevel=2, rough=0.35)
    # balusters under the brass rail (X 250, Y -1330..-900)
    for y in range(-1320, -900, 30):
        a.lathe([(3, 0), (4.5, 8), (2.5, 30), (4, 60), (2.5, 88), (4, 96)], at=(250 - pv[0] - 10, y - pv[1], 0), col=C.BRASS, sides=8, rough=0.35)
    # door surround on the room side
    for y in (-1560, -1360):
        a.box((10, 14, 224), at=(240 - pv[0] - 6, y - pv[1], 112), col=C.BRASS, bevel=2, rough=0.35)
    a.box((10, 214, 12), at=(240 - pv[0] - 6, -1460 - pv[1], 226), col=C.BRASS, bevel=2, rough=0.35)
    # wall sconces
    for x in (-450, -50):
        a.box((6, 12, 26), at=(x - pv[0], -910 - pv[1] - 5, 230), col=C.BRASS, bevel=1.5, rough=0.35)
        a.cone(10, 16, at=(x - pv[0], -910 - pv[1] - 14, 244), r_top=5, rot=(180, 0, 0), col=C.CREAM, sides=10, glow=6)


@asset("SM_Projection_Ceiling", F,
       desc="Dark plum projection-room ceiling with three rafters.", replaces=[UP + " projection ceiling slab"],
       placements=lambda: [P((-175, -1250, 420))], covers=rec((948, 948)),
       pivot="world-aligned, pivot (-175, -1250, 420); underside at Z 740 (world)", view=(0.3, 0.4, -0.8))
def projection_ceiling(a):
    pv = (-175, -1250, 420)
    a.records(L.select("studio", lines=[(948, 948)]), pivot=pv, bevel=1.2)
    for x in (-450, -150, 150):
        a.box((18, 700, 20), at=(x - pv[0], 0, 740 - pv[2] - 10), col=C.shade(C.PROJ_CEIL, 1.4), bevel=2)


def _seats():
    return [(-300 - r * 150, -1450 + s * 120, 420) for r in range(2) for s in range(4)]


@asset("SM_Projection_Seat", F,
       desc="Plush screening-room armchair: carpet-red shell, pillowed coral seat and back cushions, navy armrests with brass cup holders.",
       replaces=[UP + " 8 cinema seats (base, back, cushions, armrests, cup holders)"],
       placements=lambda: [P(s) for s in _seats()],
       covers=lambda: [L.select("studio", lines=[(958, 972)], where=lambda r, s=s: abs(r["center"][0] - s[0]) < 45 and abs(r["center"][1] - s[1]) < 60) for s in _seats()],
       per_placement=True, pivot="seat point on the floor; faces +X (towards the balcony), back at -X", view=(1, 0.6, 0.5))
def projection_seat(a):
    a.box((68, 88, 46), at=(0, 0, 24), col=C.CARPET, bevel=5)
    c = bm_box(64, 74, 14, 6, 2)
    bulge(c, 2.5, axis=2)
    a.add(c, xf((2, 0, 53)), C.CORAL, rough=0.95)
    b = bm_box(18, 90, 76, 7, 2)
    bulge(b, 2, axis=0)
    a.add(b, xf((-37, 0, 72), (-8, 0, 0)), C.CORAL_DARK, rough=0.95)
    c2 = bm_box(12, 74, 48, 5, 2)
    bulge(c2, 2.5, axis=0)
    a.add(c2, xf((-26, 0, 84), (-8, 0, 0)), C.CARPET, rough=0.95)
    for sy in (-1, 1):
        a.box((74, 10, 10), at=(0, sy * 45, 61), col=C.NAVY, bevel=4)
        a.box((12, 10, 40), at=(30, sy * 45, 40), col=C.NAVY, bevel=3)
        a.torus(4.5, 1.6, at=(23, sy * 45, 67), col=C.BRASS, major=12, minor=5, rough=0.3)
        a.cyl(4, 5, at=(23, sy * 45, 64), col=C.CHARCOAL, sides=10)
