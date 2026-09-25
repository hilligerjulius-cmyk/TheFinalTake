"""Detail kit for the quality pass: small functional details placed on the faces of existing parts.

Everything is in Unreal space (cm, X forward, Y right, Z up). A face is given by its centre and its outward normal
as an axis string: "+x", "-x", "+y", "-y", "+z", "-z". Sizes on a face are (w along u, h along v) with
    +/-x: u = Y, v = Z      +/-y: u = X, v = Z      +/-z: u = X, v = Y
Details stay small (a few cm proud) so silhouettes and the fit against the code layout do not change.
"""
import math

from mathutils import Vector

from . import palette as C

# FRotator that turns local +Z (the axis of cylinders/cones) onto the normal
NORMAL_ROT = {"+x": (-90, 0, 0), "-x": (90, 0, 0), "+y": (0, 0, 90), "-y": (0, 0, -90), "+z": (0, 0, 0), "-z": (180, 0, 0)}
_AX = {"x": 0, "y": 1, "z": 2}
_UV = {"x": (1, 2), "y": (0, 2), "z": (0, 1)}


def frame(normal):
    """(n, u, v) unit vectors for a face normal."""
    sgn = 1.0 if normal[0] == "+" else -1.0
    ax = _AX[normal[1]]
    n = Vector((0, 0, 0))
    n[ax] = sgn
    iu, iv = _UV[normal[1]]
    u = Vector((0, 0, 0))
    u[iu] = 1.0
    v = Vector((0, 0, 0))
    v[iv] = 1.0
    return n, u, v


def _size(normal, w, h, t):
    iu, iv = _UV[normal[1]]
    s = [0.0, 0.0, 0.0]
    s[_AX[normal[1]]] = t
    s[iu] = w
    s[iv] = h
    return tuple(s)


def on(center, normal, du=0.0, dv=0.0, dn=0.0):
    """Point on a face: centre + du*u + dv*v + dn*n."""
    n, u, v = frame(normal)
    return Vector(center) + u * du + v * dv + n * dn


# ------------------------------------------------------------------------------------------- hardware

def screw(a, at, normal, r=1.1, col=None, slot=True):
    col = col or C.CHROME
    n, u, v = frame(normal)
    p = Vector(at)
    a.cyl(r, 0.9, at=p + n * 0.45, rot=NORMAL_ROT[normal], col=col, sides=8, bevel=0.35, rough=0.3)
    if slot and r >= 0.9:
        a.box(_size(normal, r * 1.5, 0.3, 0.3), at=p + n * 0.95, col=C.INK, bevel=0, jitter=0, wear=False)


def screws_rect(a, center, normal, w, h, inset=2.5, r=1.0, col=None):
    for su in (-1, 1):
        for sv in (-1, 1):
            screw(a, on(center, normal, su * (w / 2 - inset), sv * (h / 2 - inset)), normal, r, col)


def rivets(a, p0, p1, normal, n=6, r=0.8, col=None):
    col = col or C.GREY
    p0, p1 = Vector(p0), Vector(p1)
    nn = frame(normal)[0]
    for k in range(n):
        p = p0.lerp(p1, (k + 0.5) / n)
        a.sphere(r, at=p + nn * (r * 0.3), col=col, segs=6, rings=3, rough=0.35)


def bolt_ring(a, center, normal, R, n=6, r=1.0, col=None):
    nn, u, v = frame(normal)
    for k in range(n):
        ang = 2 * math.pi * k / n
        screw(a, Vector(center) + (u * math.cos(ang) + v * math.sin(ang)) * R, normal, r, col, slot=False)


def plate(a, center, normal, w, h, t=0.8, col=None, screws=True, bevel=0.4, **kw):
    """Thin plate lying on a face (label, patch, cover); optional corner screws."""
    col = col or C.GREY
    a.box(_size(normal, w, h, t), at=on(center, normal, dn=t / 2), col=col, bevel=bevel, **kw)
    if screws and min(w, h) > 6:
        screws_rect(a, on(center, normal, dn=t), normal, w, h, inset=min(2.2, min(w, h) * 0.2), r=0.7)


def label(a, center, normal, w, h, col=None, ink=None, lines=3):
    """Printed label: pale plate with a header band and a few text lines."""
    col = col or C.CREAM
    ink = ink or C.INK
    plate(a, center, normal, w, h, 0.5, col, screws=False, bevel=0.2)
    a.box(_size(normal, w * 0.92, h * 0.22, 0.2), at=on(center, normal, 0, h * 0.3, 0.6), col=C.CORAL, bevel=0, jitter=0, wear=False)
    for k in range(lines):
        lw = w * (0.8 - 0.18 * (k % 2))
        a.box(_size(normal, lw, max(0.35, h * 0.06), 0.2), at=on(center, normal, -(w * 0.8 - lw) / 2, h * (0.05 - 0.17 * k), 0.6),
              col=ink, bevel=0, jitter=0, wear=False)


def vent(a, center, normal, w, h, slats=5, col=None, frame_col=None, depth=1.2):
    """Louvred vent: frame + angled slats."""
    col = col or C.GREY_DARK
    frame_col = frame_col or col
    border(a, center, normal, w, h, bar=max(1.0, min(w, h) * 0.08), t=depth, col=frame_col)
    a.box(_size(normal, w * 0.9, h * 0.9, 0.3), at=on(center, normal, dn=0.15), col=C.INK, bevel=0, jitter=0, wear=False)
    for k in range(slats):
        dv = -h / 2 + (k + 0.5) * h / slats
        a.box(_size(normal, w * 0.86, h / slats * 0.45, depth * 0.8), at=on(center, normal, 0, dv, depth * 0.4), col=col, bevel=0.2)


def grille_holes(a, center, normal, w, h, pitch=3.0, r=0.7, col=None):
    """Speaker/perforated grille: dark dots on a face."""
    col = col or C.INK
    nu, nv = max(1, int(w / pitch)), max(1, int(h / pitch))
    for i in range(nu):
        for j in range(nv):
            p = on(center, normal, -w / 2 + (i + 0.5) * w / nu, -h / 2 + (j + 0.5) * h / nv, 0.1)
            a.cyl(r, 0.3, at=p, rot=NORMAL_ROT[normal], col=col, sides=6, bevel=0, jitter=0, wear=False)


def border(a, center, normal, w, h, bar=2.0, t=1.0, col=None, bevel=0.4):
    """Raised rectangular frame on a face (4 bars)."""
    col = col or C.GREY
    for sv in (-1, 1):
        a.box(_size(normal, w, bar, t), at=on(center, normal, 0, sv * (h / 2 - bar / 2), t / 2), col=col, bevel=bevel)
    for su in (-1, 1):
        a.box(_size(normal, bar, h - 2 * bar, t), at=on(center, normal, su * (w / 2 - bar / 2), 0, t / 2), col=col, bevel=bevel)


def inset_panel(a, center, normal, w, h, col, frame_col=None, bar=2.5, t=1.2, recess=0.94):
    """Panel look: raised frame around a slightly darker, slightly proud centre panel."""
    frame_col = frame_col or C.shade(col, 1.12)
    border(a, center, normal, w, h, bar, t, frame_col)
    a.box(_size(normal, w - 2 * bar, h - 2 * bar, t * 0.35), at=on(center, normal, dn=t * 0.18), col=C.shade(col, recess), bevel=0.3)


def seams(a, center, normal, w, h, n=3, along="u", col=None, width=0.6):
    """Dark panel seams across a face."""
    col = col or C.INK
    for k in range(1, n + 1):
        d = -0.5 + k / (n + 1)
        if along == "u":
            a.box(_size(normal, w, width, 0.25), at=on(center, normal, 0, d * h, 0.12), col=col, bevel=0, jitter=0, wear=False)
        else:
            a.box(_size(normal, width, h, 0.25), at=on(center, normal, d * w, 0, 0.12), col=col, bevel=0, jitter=0, wear=False)


def hazard(a, center, normal, w, h, stripes=6, t=0.5, c1=None, c2=None):
    c1, c2 = c1 or C.YELLOW, c2 or C.INK
    plate(a, center, normal, w, h, t, c1, screws=False, bevel=0.2)
    n, u, v = frame(normal)
    for k in range(stripes):
        du = -w / 2 + (k + 0.5) * w / stripes
        a.box(_size(normal, w / stripes * 0.45, h * 1.05, 0.2), at=on(center, normal, du, 0, t + 0.1), rot=_rot_in_plane(normal, 35),
              col=c2, bevel=0, jitter=0, wear=False)


def _rot_in_plane(normal, deg):
    """FRotator spinning a face-aligned box by deg around the face normal."""
    ax = normal[1]
    if ax == "z":
        return (0, deg, 0)
    if ax == "x":
        return (0, 0, deg)
    return (deg, 0, 0)


def handle(a, center, normal, length, along="u", r=1.2, standoff=4.0, col=None):
    """Bar handle on standoffs."""
    col = col or C.CHROME
    n, u, v = frame(normal)
    d = u if along == "u" else v
    p = Vector(center)
    p0, p1 = p - d * (length / 2), p + d * (length / 2)
    pts = [p0, p0 + n * (standoff * 0.7), p0 + n * standoff + d * r, p1 + n * standoff - d * r, p1 + n * (standoff * 0.7), p1]
    a.tube(pts, r, col=col, sides=8, rough=0.3)
    for q in (p0, p1):
        a.cyl(r * 1.6, 0.8, at=q + n * 0.4, rot=NORMAL_ROT[normal], col=C.shade(col, 0.8), sides=8, bevel=0.2)


def knob(a, at, normal, r=2.0, h=1.6, col=None):
    col = col or C.GREY_DARK
    n = frame(normal)[0]
    a.cyl(r, h, at=Vector(at) + n * (h / 2), rot=NORMAL_ROT[normal], col=col, sides=10, bevel=min(0.5, h * 0.3), rough=0.5)
    a.box(_size(normal, 0.5, r * 1.1, 0.4), at=Vector(at) + n * (h + 0.1) + frame(normal)[2] * (r * 0.35), col=C.CREAM, bevel=0, jitter=0, wear=False)


def hinge(a, at, axis, length=10.0, r=1.2, col=None):
    """Barrel hinge along an axis ("x", "y" or "z")."""
    col = col or C.GREY
    rot = {"x": (-90, 0, 0), "y": (0, 0, 90), "z": (0, 0, 0)}[axis]
    a.cyl(r, length, at=at, rot=rot, col=col, sides=8, bevel=0.3, rough=0.35)
    d = Vector((0, 0, 0))
    d[_AX[axis]] = 1.0
    for s in (-1, 1):
        a.cyl(r * 1.25, 1.0, at=Vector(at) + d * (s * length * 0.25), rot=rot, col=C.shade(col, 0.85), sides=8, bevel=0.2)


def feet(a, pts, r=2.2, h=1.6, col=None):
    """Little rubber feet under a unit (pts = (x, y, z_bottom))."""
    col = col or C.RUBBER
    for (x, y, z) in pts:
        a.cyl(r, h, at=(x, y, z + h / 2), col=col, sides=10, bevel=0.5, rough=0.9)


def corner_guards(a, center, size, col=None, s=5.0, t=0.8):
    """Metal corner caps on the 8 corners of a box."""
    col = col or C.GREY
    c = Vector(center)
    for sx in (-1, 1):
        for sy in (-1, 1):
            for sz in (-1, 1):
                p = c + Vector((sx * (size[0] / 2 - s / 2 + t), sy * (size[1] / 2 - s / 2 + t), sz * (size[2] / 2 - s / 2 + t)))
                a.box((s, s, s), at=p, col=col, bevel=min(1.2, s * 0.25), rough=0.35)


def cable(a, pts, r=0.8, col=None, sag=0.0, segs=6):
    """Cable through points; sag pulls the in-between points down (cm)."""
    col = col or C.RUBBER
    ps = [Vector(p) for p in pts]
    out = []
    for i in range(len(ps) - 1):
        for k in range(segs):
            t = k / segs
            q = ps[i].lerp(ps[i + 1], t)
            q.z -= sag * math.sin(math.pi * t)
            out.append(q)
    out.append(ps[-1])
    a.tube(out, r, col=col, sides=6, rough=0.85)


def led(a, at, normal, r=0.8, col=None, glow=3.0):
    col = col or C.GREEN
    n = frame(normal)[0]
    a.cyl(r * 1.5, 0.6, at=Vector(at) + n * 0.3, rot=NORMAL_ROT[normal], col=C.CHARCOAL, sides=8, bevel=0.1)
    a.sphere(r, at=Vector(at) + n * 0.7, col=col, segs=6, rings=4, glow=glow)


def stencil_number(a, center, normal, text, size, col=None, depth=0.3, font=None):
    """Painted stencil text flat on a face (small, thin)."""
    from .core import FONT_BLOCK
    col = col or C.INK
    rot = {"+x": (0, 0, 0), "-x": (0, 180, 0), "+y": (0, 90, 0), "-y": (0, -90, 0), "+z": (90, 0, 0), "-z": (-90, 0, 0)}[normal]
    a.text(text, size, depth, at=on(center, normal, dn=depth / 2 + 0.05), rot=rot, col=col, font=font or FONT_BLOCK)


def scuffs(a, center, normal, w, h, n=5, col=None, rng=None, t=0.15):
    """A few flat darker scuff marks (wear) on a face."""
    import random
    rng = rng or random.Random(int(abs(center[0] * 7 + center[1] * 13 + center[2] * 17)) % 99991)
    col = col or C.INK
    for _ in range(n):
        du, dv = rng.uniform(-w / 2 * 0.85, w / 2 * 0.85), rng.uniform(-h / 2 * 0.85, h / 2 * 0.85)
        sw, sh = rng.uniform(2, 7), rng.uniform(0.4, 1.4)
        a.box(_size(normal, sw, sh, t), at=on(center, normal, du, dv, t / 2), rot=_rot_in_plane(normal, rng.uniform(-40, 40)),
              col=col, bevel=0, jitter=0, wear=False)


# ------------------------------------------------------------------------------------------- frames

class Local:
    """Proxy of an Asset that places everything in a local frame (at, rot), e.g. details on a raked panel or on a
    rotated part: D.Local(a, (-4, 0, 95), (-16, 0, 0)).box(...) is a box in the panel's own axes."""

    def __init__(self, a, at=(0, 0, 0), rot=(0, 0, 0)):
        from .core import xf
        self._a = a
        self._m = xf(at, rot)

    def add(self, bm, matrix=None, *args, **kw):
        from mathutils import Matrix
        return self._a.add(bm, self._m @ (matrix if matrix is not None else Matrix.Identity(4)), *args, **kw)

    def __getattr__(self, name):
        from .core import Asset
        attr = getattr(Asset, name, None)
        if callable(attr):
            return attr.__get__(self)
        return getattr(self._a, name)


# ------------------------------------------------------------------------------------------- more parts

def wheel(a, at, r, width, axis="y", tire=None, hub=None, treads=0, bolts=4):
    """Tyre + rim + hub cap + wheel bolts; axis = the axle direction ("x", "y" or "z")."""
    tire = tire or C.RUBBER
    hub = hub or C.GREY
    rot = {"x": (-90, 0, 0), "y": (0, 0, 90), "z": (0, 0, 0)}[axis]
    at = Vector(at)
    d = Vector((0, 0, 0))
    d[_AX[axis]] = 1.0
    sides = 12 if r < 12 else 16
    a.cyl(r, width, at=at, rot=rot, col=tire, sides=sides, bevel=min(2.5, r * 0.22, width * 0.3), rough=0.9)
    for s in (-1, 1):
        a.cyl(r * 0.55, 0.8, at=at + d * (s * width / 2), rot=rot, col=hub, sides=sides, bevel=0.3, rough=0.4)
        a.cyl(r * 0.22, 1.4, at=at + d * (s * (width / 2 + 0.4)), rot=rot, col=C.shade(hub, 0.75), sides=8, bevel=0.3, rough=0.35)
        if bolts and r >= 6:
            n = frame(("+" if s > 0 else "-") + axis)
            for k in range(bolts):
                ang = 2 * math.pi * k / bolts
                p = at + d * (s * (width / 2 + 0.6)) + (n[1] * math.cos(ang) + n[2] * math.sin(ang)) * (r * 0.36)
                a.cyl(max(0.45, r * 0.05), 0.8, at=p, rot=rot, col=C.CHROME, sides=6, bevel=0, rough=0.3)
    if treads:
        n, u, v = frame("+" + axis)
        for k in range(treads):
            ang = 2 * math.pi * k / treads
            q = at + (u * math.cos(ang) + v * math.sin(ang)) * (r - 0.2)
            deg = math.degrees(ang)
            trot = {"x": (0, 0, deg), "y": (-deg, 0, 0), "z": (0, deg, 0)}[axis]
            size = [1.6, 1.6, 1.6]
            size[_AX[axis]] = width * 0.8
            a.box(tuple(size), at=q, rot=trot, col=C.shade(tire, 0.85), bevel=0.3, rough=0.95)


def caster(a, at, r=5.0, col=None, plate_col=None):
    """Swivel caster below a cart: mounting plate, fork and wheel; at = floor contact point."""
    col = col or C.RUBBER
    plate_col = plate_col or C.GREY
    x, y, z = at
    a.box((r * 1.9, r * 1.9, 1.0), at=(x, y, z + 2 * r + 3.3), col=plate_col, bevel=0.3, rough=0.4)
    a.cyl(r * 0.5, 1.6, at=(x, y, z + 2 * r + 2.2), col=C.shade(plate_col, 0.8), sides=8, bevel=0.3)
    for s in (-1, 1):
        a.box((r * 0.7, 0.8, r * 1.4), at=(x - r * 0.25, y + s * (r * 0.55), z + r + 1.2), col=plate_col, bevel=0.2, rough=0.4)
    wheel(a, (x - r * 0.3, y, z + r), r, r * 0.8, "y", tire=col, bolts=0)


def jacks(a, center, normal, cols=4, rows=2, pitch=4.0, r=1.1, col=None):
    """Patch-bay sockets: a grid of ringed holes on a face."""
    col = col or C.GREY
    for i in range(cols):
        for j in range(rows):
            p = on(center, normal, (i - (cols - 1) / 2) * pitch, (j - (rows - 1) / 2) * pitch)
            a.cyl(r, 0.8, at=p + frame(normal)[0] * 0.4, rot=NORMAL_ROT[normal], col=col, sides=8, bevel=0.2, rough=0.35)
            a.cyl(r * 0.5, 0.3, at=p + frame(normal)[0] * 0.9, rot=NORMAL_ROT[normal], col=C.INK, sides=6, bevel=0, jitter=0, wear=False)


def tape(a, center, normal, w, h, deg=0.0, col=None, t=0.3):
    """Strip of gaffer tape (slightly rotated) - the quick-fix look of a film set."""
    col = col or C.GREY
    a.box(_size(normal, w, h, t), at=on(center, normal, dn=t / 2), rot=_rot_in_plane(normal, deg), col=col, bevel=0.05,
          rough=0.95, jitter=0)


def rack_unit(a, center, normal, w, h, col=None, knobs=3, leds=2, rng=None):
    """Rack/equipment front: screwed plate with knobs and status LEDs."""
    import random
    rng = rng or random.Random(int(abs(center[0] * 3 + center[1] * 5 + center[2] * 11)) % 9973)
    col = col or C.GREY_DARK
    plate(a, center, normal, w, h, 0.8, col, screws=True)
    for k in range(knobs):
        du = -w / 2 + w * (k + 1) / (knobs + leds + 1)
        knob(a, on(center, normal, du, 0, 0.8), normal, r=min(h * 0.22, 2.4), h=1.4, col=C.shade(C.CHARCOAL, rng.uniform(0.9, 1.3)))
    for k in range(leds):
        du = -w / 2 + w * (knobs + k + 1) / (knobs + leds + 1)
        led(a, on(center, normal, du, h * 0.12, 0.8), normal, r=0.6, col=[C.GREEN, C.AMBER, C.RED][rng.randrange(3)], glow=2.0)


def bracket(a, at, normal, w=6.0, h=6.0, t=1.2, col=None):
    """L-shaped angle bracket fixing something to a face (plate on the face + gusset)."""
    col = col or C.GREY
    n, u, v = frame(normal)
    a.box(_size(normal, w, h, t), at=on(at, normal, dn=t / 2), col=col, bevel=0.3, rough=0.4)
    a.box(_size(normal, t, h, w * 0.8), at=on(at, normal, dn=w * 0.4), col=col, bevel=0.3, rough=0.4)
    for sv in (-1, 1):
        screw(a, on(at, normal, -w * 0.3, sv * h * 0.28, t), normal, r=0.6)


def cable_tie_run(a, pts, r=0.8, col=None, ties=3, sag=0.0):
    """Cable with a few ties/clips along it."""
    col = col or C.RUBBER
    cable(a, pts, r, col, sag)
    ps = [Vector(p) for p in pts]
    for k in range(ties):
        t = (k + 1) / (ties + 1) * (len(ps) - 1)
        i = min(int(t), len(ps) - 2)
        q = ps[i].lerp(ps[i + 1], t - i)
        a.sphere(r * 1.35, at=q, col=C.shade(col, 1.6), segs=6, rings=3, rough=0.6)
