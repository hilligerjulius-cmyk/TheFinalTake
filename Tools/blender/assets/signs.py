"""Extruded 3D lettering for every static sign text of the shells (optional stand-ins for the
UTextRenderComponents). Generated from the Text() records: same location, pitch/yaw and world size;
identical texts share one mesh and get several placements."""
import re

from ftb import layout as L
from ftb.core import FONT_BLOCK, FONT_SIGN
from ftb.registry import P, asset

AREAS = {"studio": ("Studio/Signs", "SM_Sign_Studio_", "AFTStudioShell::Text()"), "city": ("City/Signs", "SM_Sign_City_", "AFTCityShell::Text()")}


def _slug(s):
    s = re.sub(r"[^A-Za-z0-9]+", "_", s.upper()).strip("_")
    return s[:40] or "SIGN"


def _groups():
    out = {}
    for area in AREAS:
        for r in L.records(area):
            if r["kind"] != "text":
                continue
            key = (area, r["text"], round(r["size"], 1), tuple(r["color"]))
            out.setdefault(key, []).append(r)
    return out


def _make(key, recs):
    area, text, size, color = key
    folder, prefix, fn = AREAS[area]
    name = prefix + _slug(text)
    taken = {s for s in _NAMES}
    if name in taken:
        name = "%s_%d" % (name, int(size))
    k = 2
    while name in taken:
        name = "%s_%d" % (prefix + _slug(text), k)
        k += 1
    _NAMES.add(name)
    col = tuple(c / 255.0 for c in color)
    lines = sorted({r["line"] for r in recs})
    files = sorted({r["file"].split("/")[-1] for r in recs})

    @asset(name, folder,
           desc="3D lettering '%s' (world size %g) in the sign colour." % (text, size),
           replaces=["%s '%s' (%s:%s) - optional; hide the UTextRenderComponent when using this mesh" % (fn, text, "/".join(files), ",".join(str(l) for l in lines))],
           placements=lambda: [P(r["loc"], (r["pitch"], r["yaw"], 0), note="%s:%d" % (r["file"].split("/")[-1], r["line"])) for r in recs],
           pivot="text centre (like the TextRender: horizontally centred, vertically text-centre); faces +X at yaw 0",
           check=False, view=(1, 0.25, 0.1), tags=["sign"])
    def build(a):
        depth = max(0.6, min(6.0, size * 0.07))
        font = FONT_BLOCK if size >= 40 else FONT_SIGN
        a.text(text, size, depth, at=(0, 0, 0), col=col, font=font, glow=1.5)
    return build


_NAMES = set()
for _k, _recs in sorted(_groups().items(), key=lambda kv: (kv[0][0], kv[0][1], kv[0][2])):
    _make(_k, _recs)
