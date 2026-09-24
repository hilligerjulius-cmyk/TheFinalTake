"""Asset registry: every mesh, what it replaces in the code, and where it goes in the map."""
from dataclasses import dataclass, field
from typing import Callable, List, Optional

from . import layout as L

ASSETS = []


@dataclass
class Spec:
    name: str
    folder: str
    build: Callable
    desc: str = ""
    replaces: List[str] = field(default_factory=list)
    # [{"loc": [x, y, z], "rot": [p, y, r], "scale": [sx, sy, sz], "note": str}] Unreal world space
    placements: Optional[Callable] = None
    # layout records this asset stands in for; per placement if `per_placement` is True
    covers: Optional[Callable] = None
    per_placement: bool = False
    view: tuple = (1.0, 0.85, 0.62)  # Unreal space, from the target towards the camera
    pivot: str = "bottom centre"
    integration: str = ""
    tags: List[str] = field(default_factory=list)
    check: bool = True
    # code parts [(shape, loc, size, rot)] in the asset's local space (actors): bounds check
    expect: Optional[Callable] = None


def asset(name, folder, desc="", replaces=None, placements=None, covers=None, per_placement=False,
          view=(1.0, 0.85, 0.62), pivot="bottom centre", integration="", tags=None, check=True):
    def deco(fn):
        ASSETS.append(Spec(name, folder, fn, desc, replaces or [], placements, covers, per_placement, view, pivot,
                           integration, tags or [], check))
        return fn
    return deco


def P(loc, rot=(0, 0, 0), scale=(1, 1, 1), note=""):
    return {"loc": [float(x) for x in loc], "rot": [float(x) for x in rot], "scale": [float(x) for x in scale], "note": note}


def at_origin(pivot):
    """Unique architecture: modelled in world orientation around a pivot; placed with no rotation."""
    return lambda: [P(pivot)]


def lines(area, *ranges, where=None, **kw):
    return lambda: L.select(area, lines=list(ranges), where=where, **kw)


def kit(area, name, to_placement, line_range=None, where=None):
    """Placements + per-placement coverage from every call of a shell helper (Plant, Crate, ...)."""
    def calls():
        out = []
        for c in L.kit_calls(area, name, line_range):
            if where and not where(c["call"]["args"]):
                continue
            out.append(c)
        return out

    def placements():
        return [to_placement(c["call"]["args"], c["call"]) for c in calls()]

    def covers():
        return [c["records"] for c in calls()]

    return placements, covers
