"""Actor meshes vs. the C++ constructors.

The actors (stations, props, doors, cinema parts) are built in their constructors from FTVis::MakePart calls.
layout/cppactor.py replays those constructors (and OnConstruction, with each instance's settings from the map
builder), so every mesh here gets
  * a bounds check against the exact code parts it replaces (Spec.expect), and
  * its level placements computed from the code: actor transform x component transform (sub-components such as
    lamp heads, levers and reels, which the modules could not place by hand).
Imported last by build_all.py; it only annotates specs registered by the other modules.
"""
import math
import os
import re
import sys

from ftb.registry import ASSETS, P

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "layout"))
import cppactor as CA  # noqa: E402


def E(asset, cls, parts, anchor=None, orient="anchor", offset=(0, 0, 0), labels=None, construct=("OnConstruction",),
      hidden=False, stretch=None, extra=None):
    """asset <- parts of `cls` (regexes over component names, '{0}' = the anchor's regex group).
    anchor: component regex whose frame is the mesh pivot (None = actor root); orient 'parent' keeps the parent's
    rotation (the mesh already contains the part's own tilt); offset/stretch: values or callables of the anchor."""
    return dict(asset=asset, src=[(cls, anchor)] + list(extra or []), parts=parts, orient=orient, offset=offset,
                labels=labels, construct=construct, hidden=hidden, stretch=stretch)


ALL = [r".*"]
MAP = [
    # ---------------------------------------------------------------- Production/FTStations.cpp
    E("SM_Station_Lighthouse", "AFTLighthouse", ["Base", r"Seg\d", "Door", "Window", "Gallery", "Railing", "Roof", "Tip", "SwitchBox", "SwitchPlate"]),
    E("SM_Station_LighthouseLamp", "AFTLighthouse", ["Lamp", "LanternGlass"], anchor="Lamp"),
    E("SM_Station_LighthouseLever", "AFTLighthouse", ["Lever"], anchor="Lever"),
    E("SM_Station_StageLight_Base", "AFTStageLight", [r"Leg\d", "Collar"]),
    E("SM_Station_StageLight_Pole", "AFTStageLight", ["Pole"], anchor="Pole", offset=lambda c: (0, 0, -c.scale.z * 50),
      stretch=lambda c: (1, 1, c.scale.z)),
    E("SM_Station_StageLight_Head", "AFTStageLight", ["Yoke", "Body", "Rim", "Door(Top|Bottom|Left|Right)"], anchor="Head"),
    E("SM_Station_StageLight_Lens", "AFTStageLight", ["LensPart"], anchor="LensPart", orient="parent"),
    E("SM_Station_LightingBoard", "AFTLightingBoard", ["Desk", "Panel", "Trim"]),
    E("SM_Station_LightingBoard_Toggle", "AFTLightingBoard", [r"ToggleCap{0}"], anchor=r"ToggleCap(\d)"),
    E("SM_Station_Breaker", "AFTBreaker", ["Panel", r"Stripe\d", "Door"]),
    E("SM_Station_Breaker_Lever", "AFTBreaker", ["LeverArm", "LeverGrip"], anchor="LeverPivot"),
    E("SM_Station_EmergencyLight", "AFTEmergencyLight", ["Base"]),
    E("SM_Station_EmergencyDome", "AFTEmergencyLight", ["Dome"], anchor="Dome"),
    E("SM_Station_SoundConsole", "AFTSoundConsole", ["Desk", "Top", "Trim", "TitlePlate", r"Speaker\d", r"Woofer\d", r"Tweeter\d", "BoomStand", "BoomArm", "BoomMic"]),
    E("SM_Station_ConsoleButton", "AFTSoundConsole", [r"(Rig)?Button{0}"], anchor=r"Button(\d)", extra=[("AFTSharkRig", r"RigButton(3)")]),
    E("SM_Station_WindMachine", "AFTEffectMachine", [r"Part\d+"], labels=["WindMachine"]),
    E("SM_Station_WindMachine_Blades", "AFTEffectMachine", [r"Blade\d"], anchor="Spinner", labels=["WindMachine"]),
    E("SM_Station_RainMachine", "AFTEffectMachine", [r"Part\d+"], labels=["RainMachine"]),
    E("SM_Station_RainRig", "AFTEffectMachine", [r"RigPart\d"], anchor="Rig", labels=["RainMachine"]),
    E("SM_Station_SmokeMachine", "AFTEffectMachine", [r"Part\d+"], labels=["SmokeMachine"]),
    E("SM_Station_FoamCannon", "AFTEffectMachine", [r"Part\d+"], labels=["FoamCannon"]),
    E("SM_Station_FinGlider_Crank", "AFTFinGlider", ["CrankPost", "CrankWheel"]),
    E("SM_Station_FinGlider_Fin", "AFTFinGlider", ["FinBody", "FinEdge"], anchor="Fin"),
    E("SM_Station_FloodValve_Pipe", "AFTFloodController", ["ValvePipe"], anchor="ValvePipe", construct=("OnConstruction", "BeginPlay")),
    E("SM_Station_FloodValve_Wheel", "AFTFloodController", ["ValveWheel"], anchor="ValveWheel", construct=("OnConstruction", "BeginPlay")),
    # ---------------------------------------------------------------- Production/FTShark.cpp
    E("SM_Station_SharkRig_Rail", "AFTSharkRig", ["Rail"]),
    E("SM_Station_SharkRig_Trolley", "AFTSharkRig", ["Trolley"], anchor="Carriage"),
    E("SM_Station_SharkRig_Arm", "AFTSharkRig", ["Arm"], anchor="SharkRoot"),
    E("SM_Shark_Body", "AFTSharkRig", [r"Shark(Body|Belly|Snout|TailStalk|Tail|Dorsal|FinL|FinR|ToothU\d|EyeL|EyeR|PupilL|PupilR|BrowL|BrowR)"], anchor="SharkRoot"),
    E("SM_Shark_Jaw", "AFTSharkRig", ["SharkJawBone", "SharkMouth", r"SharkToothL\d"], anchor="SharkJaw"),
    E("SM_Station_SharkRig_Desk", "AFTSharkRig", ["Desk", "DeskTop", "KitPanel"], anchor="Station"),
    # ---------------------------------------------------------------- Production/FTFilmCamera.cpp
    E("SM_Station_FilmCamera_Track", "AFTFilmCamera", ["RailL", "RailR", r"Sleeper\d+"]),
    E("SM_Station_FilmCamera_Dolly", "AFTFilmCamera", ["Platform", "PlatformTrim", r"Wheel\d", "PushBar", "PushPostL", "PushPostR", "Pedestal", "PedestalRing", "MonitorArm", "MonitorBox"], anchor="Dolly"),
    E("SM_Station_FilmCamera_PanHead", "AFTFilmCamera", ["HeadBase"], anchor="PanHead"),
    E("SM_Station_FilmCamera_Body", "AFTFilmCamera", ["Body", "BodyStripe", "Badge", "LensBarrel", "LensRing", r"MatteBox\w*", "FrenchFlag", "FocusWheel", "FocusKnob", r"BodyVent\d", "Handle", "Viewfinder"], anchor="TiltHead"),
    E("SM_Station_FilmCamera_Reel", "AFTFilmCamera", ["Reel{0}", "Reel{0}Hub"], anchor="Reel(A|B)", orient="parent"),
    # ---------------------------------------------------------------- World/FTStudioObjects.cpp
    E("SM_Station_Projector", "AFTProjector", ["Stand", "Housing", "HousingTrim", "Barrel", "ArmA", "ArmB", r"SlotPeg\d", "LeverBase"]),
    E("SM_Station_Projector_Reel", "AFTProjector", ["Reel{0}", "Reel{0}Spoke"], anchor="ReelSpin(A|B)"),
    E("SM_Station_Projector_Lever", "AFTProjector", ["Lever"], anchor="Lever"),
    E("SM_Station_ProjectorPowerPanel", "AFTProjector", ["PowerBox", "PowerSwitchPart"], anchor="PowerPanel", construct=("OnConstruction", "BeginPlay")),
    E("SM_Prop_ScreenRoller", "AFTCinemaScreen", ["Housing", "HousingTrim", "CapL", "CapR"]),
    E("SM_Prop_ScreenBottomBar", "AFTCinemaScreen", ["BottomBar"], anchor="BottomBar"),
    E("SM_Prop_ScriptBook_Base", "AFTScriptBook", ["Back", "Pages", "PageEdge", r"Ribbon[ABC]"]),
    E("SM_Prop_ScriptBook_Cover", "AFTScriptBook", ["Cover", "Label"], anchor="CoverPivot"),
    E("SM_Prop_SceneBoard", "AFTSceneBoard", ["Frame", "Cork", "NoteA", "NoteB", "Paper"]),
    # ---------------------------------------------------------------- Props/FTProp.cpp
    E("SM_Prop_HeroHarpoon", "AFTProp_Harpoon", ALL),
    E("SM_Prop_LifeRing", "AFTProp_LifeRing", ALL),
    E("SM_Prop_PracticalLamp", "AFTProp_Lamp", [r"Leg\d", "Pole", "Collar", "Housing", "DoorTop"]),
    E("SM_Prop_PracticalLamp_Lens", "AFTProp_Lamp", ["Lens"], anchor="Lens", orient="parent"),
    E("SM_Prop_PropCrate", "AFTProp_Crate", ALL),
    E("SM_Prop_FinMarker", "AFTProp_FinMarker", ALL),
    E("SM_Prop_FilmReel", "AFTProp_Reel", ALL),
    # ---------------------------------------------------------------- Props/FTSetPieces.cpp
    E("SM_Prop_RescueBoat_Slipway", "AFTRescueBoat", ["SlipL", "SlipR", "Trolley", r"TrolleyWheel\d"]),
    E("SM_Prop_RescueBoat_Boat", "AFTRescueBoat", ["TubeL", "TubeR", "Bow", "Floor", "Seat", "Transom", "Motor", "MotorCap", "MotorLeg", "RopeL", "RopeR"], anchor="BoatRoot"),
    E("SM_Prop_LostAndFoundShelf", "AFTPropShelf", ALL),
    E("SM_Station_ReelTray", "AFTReelTray", ALL),
    E("SM_Wardrobe_CostumeRack", "AFTCostumeRack", ["PostL", "PostR", "Bar", "FootL", "FootR", "Header", r"H\dHook", r"H\dHanger"]),
    E("SM_Costume_Lifeguard", "AFTCostumeRack", ["H0A", "H0B", "H0C"], offset=(0, -180, 0)),
    E("SM_Costume_SharkSuit", "AFTCostumeRack", ["H1A", "H1B", "H1C"], offset=(0, -90, 0)),
    E("SM_Costume_Raincoat", "AFTCostumeRack", ["H2A", "H2B"]),
    E("SM_Costume_FoamKnight", "AFTCostumeRack", ["H3A", "H3B", "H3C"], offset=(0, 90, 0)),
    E("SM_Costume_WorkClothes", "AFTCostumeRack", ["H4A", "H4B"], offset=(0, 180, 0)),
    E("SM_Prop_StandIn_Base", "AFTStandIn", ["Base", "Brace", "Legs", "Face", "EyeL", "EyeR", "Smile", "ArmL", "ArmR"], anchor="Visual", hidden=True),
    E("SM_Prop_StandIn_Plain", "AFTStandIn", ["Body", "Hair"], anchor="Visual", hidden=True),
    E("SM_Prop_StandIn_Lifeguard", "AFTStandIn", [r"LG\w+"], anchor="Visual", hidden=True),
    E("SM_Prop_StandIn_Shark", "AFTStandIn", [r"SH\w+"], anchor="Visual", hidden=True),
    E("SM_Prop_StandIn_Raincoat", "AFTStandIn", [r"RC\w+"], anchor="Visual", hidden=True),
    E("SM_Prop_StandIn_Knight", "AFTStandIn", [r"FK\w+"], anchor="Visual", hidden=True),
    # ---------------------------------------------------------------- Career/FTShopItems.cpp
    E("SM_Lobby_StudioSupplyCounter", "AFTShopTerminal", ["Counter", "CounterTop", "CounterKick", r"CounterStripe\d", "Register", "RegisterTop", r"RegisterKey\d",
                                                          "BookStand", "BookL", "BookR", "BookCover", "TurntableBase", "TurntableRim", "ShelfBack", r"Shelf\d", r"Stock\d_\d", "Sign", r"SignNeon\w+"]),
    E("SM_Deco_DeliveryBay_Lobby", "AFTDeliveryBay", ALL, labels=["DeliveryBay_Lobby"], construct=("OnConstruction", "BeginPlay")),
    E("SM_Deco_DeliveryBay_Stage4", "AFTDeliveryBay", ALL, labels=["DeliveryBay_Stage4"], construct=("OnConstruction", "BeginPlay")),
    E("SM_Wardrobe_AccessoryWall", "AFTAccessoryStand", ["Back", "FrameTop", "FrameL", "FrameR", "Shelf", "ShelfBody", "ShelfTrim", "Mirror", "MirrorFrame", r"Bulb\d"]),
    E("SM_Prop_AccessoryBust", "AFTAccessoryStand", ["Bust{0}", "BustNeck{0}"], anchor=r"HookAnchor(\d)", offset=(0, 0, -54)),
    # ---------------------------------------------------------------- World/FTCity.cpp
    E("SM_Cinema_Marquee", "AFTCinemaMarquee", ["Canopy", "CanopyTrim", r"CanopyRod-?\d+", "Marquee", "MarqueeBoard", "MarqueeTop"]),
    E("SM_Cinema_MarqueeBulb", "AFTCinemaMarquee", ["{0}"], anchor=r"(Bulb(?:Top|Low)\d+)", orient="parent"),
    E("SM_Cinema_BladeSign", "AFTCinemaMarquee", ["Blade", "BladeTrim"]),
    E("SM_Cinema_PosterCase", "AFTCinemaMarquee", ["PosterBox", "PosterGlow"]),
    E("SM_Cinema_SearchlightBase", "AFTCinemaMarquee", ["SearchBase{0}"], anchor=r"SearchBase(\d)", offset=(0, 0, -30)),
    E("SM_Cinema_SearchlightHead", "AFTCinemaMarquee", ["SearchDrum{0}", "SearchLens{0}"], anchor=r"SearchHead(\d)"),
    E("SM_Cinema_ChartBoard", "AFTBoxOfficeBoard", ALL),
    E("SM_Cinema_Seat", "AFTCinemaSeating", [r"SeatBases\[{0}\]", r"SeatBacks\[{0}\]"], anchor=r"SeatBases\[(\d+)\]", offset=(4, 0, -42)),
    E("SM_Cinema_SeatingTiers", "AFTCinemaSeating", [r"Tiers\[\d+\]"]),
    E("SM_Prop_FilmCase", "AFTFilmCase", ["Body", "Lid", "Stripe", "Handle", r"Corner-?\d+-?\d+"]),
]
for _n, _label in (("Stage4", "Door_Stage4"), ("Office", "Door_Office"), ("Wardrobe", "Door_Wardrobe"), ("Projection", "Door_Projection")):
    MAP.append(E("SM_Door_%s_Frame" % _n, "AFTDoor", ["FrameL", "FrameR", "FrameTop"], labels=[_label]))
    MAP.append(E("SM_Door_%s_Panel" % _n, "AFTDoor", ["Panel", "PanelStripe"], anchor="Panel", labels=[_label]))
MAP.append(E("SM_Door_KeycardReader", "AFTDoor", ["KeyPanel"], anchor="KeyPanel", labels=["Door_Stage4"]))

# deliberate deviations from the code silhouette (shown in the fit report)
NOTES = {
    "SM_Cinema_Seat": "adds the pedestal + armrest down to the tier top (the code's seat boxes float 36 cm above it)",
    "SM_Station_SharkRig_Rail": "adds two support legs down to the tank floor (the code rail floats at the water line)",
    "SM_Station_LightingBoard": "adds a gooseneck desk lamp above the panel",
    "SM_Station_Lighthouse": "switch box gets a lever guard plate on the side",
    "SM_Costume_Raincoat": "the raincoat gets its hood up over the hanger",
    # shell assets
    "SM_Tank_IslandRamp": "adds rope-railing posts along both sides of the ramp",
    "SM_City_RooftopBillboard": "adds support legs down to the roof and a service catwalk with lamps",
    "SM_Dealer_SignGantry": "checked against the board turned to span the pillars (code board rotated 90 degrees, see BLENDER_ASSETS.md)",
    "SM_Prop_PalmTree": "organic trunk curve and fronds instead of stacked spheres",
    "SM_Prop_Boulder": "organic rock instead of a rotated ellipsoid",
    "SM_City_ParkBench": "taller curved backrest with iron scroll ends",
    "SM_City_Signpost": "the boards end in arrow tips (the code boards are plain rectangles)",
    "SM_Prop_PottedPlant": "individually bent leaves reach a little wider/higher than the code's leaf ellipsoids",
    "SM_Lobby_Stage4Header": "film-reel medallions at both ends of the header board",
    "SM_Prop_SpareLight_Coral": "lamp head with lens rim and barn doors reaches 12 cm further forward",
    "SM_Prop_SpareLight_Teal": "lamp head with lens rim and barn doors reaches 12 cm further forward",
}

# unit-space ISM meshes (the code scales the instances at runtime): compare with the generated unit shapes
UNIT = {"SM_Cinema_AudienceBody": ("Capsule", "AFTCinemaSeating: Bodies ISM"), "SM_Cinema_AudienceHead": ("Ball", "AFTCinemaSeating: Heads ISM"),
        "SM_Cinema_AudienceHair": ("Ball", "AFTCinemaSeating: Hair ISM")}


# ============================================================================ evaluation

def _unit_rot(m):
    cols = []
    for j in range(3):
        n = math.sqrt(sum(m[i][j] ** 2 for i in range(3))) or 1.0
        cols.append([m[i][j] / n for i in range(3)])
    return [[cols[j][i] for j in range(3)] for i in range(3)]


def _rotator(r):
    pitch = math.degrees(math.asin(max(-1.0, min(1.0, r[2][0]))))
    yaw = math.degrees(math.atan2(r[1][0], r[0][0]))
    roll = math.degrees(math.atan2(-r[2][1], r[2][2]))
    return tuple(0.0 if abs(x) < 1e-6 else round(x, 4) for x in (pitch, yaw, roll))


def _frame(comp, orient, offset):
    """Rigid frame (4x4, actor space) of the mesh pivot."""
    if comp is None:
        m = CA.mat_xf()
    else:
        full = comp.matrix()
        rsrc = comp.parent.matrix() if (orient == "parent" and isinstance(comp.parent, CA.Comp)) else full
        r = _unit_rot(rsrc)
        m = [r[i] + [full[i][3]] for i in range(3)] + [[0, 0, 0, 1]]
    off = offset(comp) if callable(offset) else offset
    return CA.mat_mul(m, CA.mat_xf(off))


def _match(pattern, name, group=None):
    pat = pattern.format(re.escape(group)) if group is not None else pattern
    return re.fullmatch(pat, name) is not None


_RUNS = {}


def _instances(cls, labels, construct):
    spawns = [s for s in CA.spawns() if s["cls"] == cls and (labels is None or s["label"] in labels)]
    if not spawns:
        spawns = [{"cls": cls, "loc": None, "yaw": 0.0, "label": "(spawned at runtime)", "settings": {}}]
    out = []
    for s in spawns:
        key = (cls, s["label"], construct)
        if key not in _RUNS:
            it = CA.Interp(CA.source())
            _, comps = it.run_actor(cls, s["settings"], construct=construct)
            _RUNS[key] = comps
        out.append((s, _RUNS[key]))
    return out


def _anchors(comps, anchor):
    if anchor is None:
        return [(None, None)]
    out = []
    for c in comps:
        m = re.fullmatch(anchor, c.name)
        if m:
            out.append((c, m.group(1) if m.groups() else None))
    return out


def evaluate(entry):
    """-> (expected corners in the mesh's local space, placements, {(line, component)}, first anchor)"""
    corners, placements, refs = [], [], set()
    first_anchor = False
    for cls, anchor in entry["src"]:
        for spawn, comps in _instances(cls, entry["labels"], entry["construct"]):
            for comp, group in _anchors(comps, anchor):
                if comp is not None and not comp.visible and not entry["hidden"]:
                    continue
                if first_anchor is False:
                    first_anchor = comp
                frame = _frame(comp, entry["orient"], entry["offset"])
                stretch = tuple(entry["stretch"](comp)) if entry["stretch"] else (1.0, 1.0, 1.0)
                if not corners:
                    inv = CA.mat_inv_rigid_scaled(frame)
                    for c in comps:
                        if not c.is_part() or c.size is None or getattr(c, "unknown", False):
                            continue
                        if not c.visible and not entry["hidden"]:
                            continue
                        if not any(_match(p, c.name, group) for p in entry["parts"]):
                            continue
                        lo, hi = CA.shape_bounds(c.shape, CA.mat_mul(inv, c.matrix()))
                        for q in (lo, hi):
                            corners.append((q[0] / stretch[0], q[1] / stretch[1], q[2] / stretch[2]))
                        refs.add((c.line, c.name))
                if spawn.get("loc") is not None:
                    world = CA.mat_mul(CA.mat_xf(spawn["loc"].t(), (0, spawn["yaw"] or 0.0, 0)), frame)
                    loc = tuple(round(world[i][3], 3) for i in range(3))
                    note = spawn["label"] + (" / " + comp.name if comp is not None else "")
                    placements.append(P(loc, _rotator(_unit_rot(world)), stretch, note=note))
    return corners, placements, refs, (None if first_anchor is False else first_anchor)


def hookup(entry, comp):
    """How the mesh plugs into the actor (no C++ was changed; this is the recipe)."""
    parts = ", ".join(p.replace("\\", "").replace("{0}", "N") for p in entry["parts"]) if entry["parts"] != ALL else "all visual parts"
    if comp is None:
        return "Attach as a StaticMeshComponent to the actor root at identity; hide the code parts (%s)." % parts
    rot = comp.rot.t()
    if comp.is_part():
        txt = "Put the mesh on component '%s' and set its RelativeScale3D to (1, 1, 1) (ApplyShape scales it to Size/100)" % comp.name
        if entry["orient"] == "parent":
            txt += " and its RelativeRotation to (0, 0, 0) - the mesh already contains the code's (%g, %g, %g)" % rot
        elif any(abs(x) > 1e-6 for x in rot):
            txt += "; keep its rotation (%g, %g, %g)" % rot
        off = entry["offset"]
        if callable(off) or any(abs(x) > 1e-6 for x in off):
            txt += "; the pivot is offset from the component origin (see the placements)"
        return txt + "."
    return "Attach as a StaticMeshComponent to scene component '%s' at identity; hide the code parts (%s)." % (comp.name, parts)


def _aabb(pts):
    return ([min(p[i] for p in pts) for i in range(3)], [max(p[i] for p in pts) for i in range(3)])


def _same_placements(a, b, tol=2.0):
    if len(a) != len(b):
        return False
    key = lambda p: tuple(round(x) for x in p["loc"])  # noqa: E731
    for p, q in zip(sorted(a, key=key), sorted(b, key=key)):
        if any(abs(x - y) > tol for x, y in zip(p["loc"], q["loc"])):
            return False
        if any(abs(((x - y + 180) % 360) - 180) > 1.0 for x, y in zip(p["rot"], q["rot"])):
            return False
    return True


REPORT = {}
_BY_NAME = {s.name: s for s in ASSETS}
for _e in MAP:
    _spec = _BY_NAME.get(_e["asset"])
    if _spec is None:
        REPORT[_e["asset"]] = "no such asset"
        continue
    _corners, _placements, _refs, _anchor = evaluate(_e)
    if not _corners:
        REPORT[_e["asset"]] = "no code parts matched"
        continue
    _lo, _hi = _aabb(_corners)
    _spec.expect = (lambda lo, hi, n: (lambda: {"min": lo, "max": hi, "parts": n}))(_lo, _hi, len(_refs))
    _files = {os.path.basename(getattr(CA.source().method(_e["src"][0][0], _e["src"][0][0]), "file", "") or "")}
    _spec.fit_note = NOTES.get(_e["asset"], "")
    _spec.hookup = hookup(_e, _anchor)
    _spec.code_parts = "%s (%s): %s" % (_e["src"][0][0], ", ".join(sorted(f for f in _files if f)), ", ".join(n for _, n in sorted(_refs)))
    if _placements:
        if _spec.placements is None:
            _spec.placements = (lambda pl: (lambda: pl))(_placements)
            REPORT[_e["asset"]] = "placements computed from the code (%d)" % len(_placements)
        elif not _same_placements(_spec.placements(), _placements):
            REPORT[_e["asset"]] = "module placements replaced by the code's (%d)" % len(_placements)
            _spec.placements = (lambda pl: (lambda: pl))(_placements)

for _name, (_shape, _what) in UNIT.items():
    _spec = _BY_NAME.get(_name)
    if _spec is None:
        continue
    hx, hy, hz = CA.HALF.get(_shape, (50, 50, 50))
    _spec.expect = (lambda lo, hi: (lambda: {"min": lo, "max": hi, "parts": 1}))([-hx, -hy, -hz], [hx, hy, hz])
    _spec.code_parts = "%s (unit SM_FT_%s)" % (_what, _shape)

for _spec in ASSETS:
    if _spec.name in NOTES and not getattr(_spec, "fit_note", ""):
        _spec.fit_note = NOTES[_spec.name]
