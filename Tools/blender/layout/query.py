"""Print layout records compactly: python3 query.py studio 426 547 [--ctx]"""
import json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
D = json.load(open(os.path.join(HERE, "..", "..", "..", "SourceArt", "Blender", "layout", "shell_layout.json")))
G = ["CubeS", "Cube", "BoxS", "Box", "CylS", "Cyl", "Sph", "Ball", "Cone", "Prism", "RampS", "Torus", "Caps", "Glass", "Block", "Shore"]
def hx(c):
    f = lambda x: int(round(255 * (x * 12.92 if x <= 0.0031308 else 1.055 * x ** (1 / 2.4) - 0.055)))
    return "%02X%02X%02X" % (f(c[0]), f(c[1]), f(c[2]))
area, a, b = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
for r in D[area]:
    lines = [r["line"]] + [c["line"] for c in r["ctx"]]
    top = r["ctx"][0]["line"] if r["ctx"] else r["line"]
    if not (a <= top <= b):
        continue
    if r["kind"] == "prim":
        g = lambda v: ",".join("%g" % round(x, 1) for x in v)
        rot = "" if not any(r["rot"]) else " rot(" + g(r["rot"]) + ")"
        e = " e%g" % r["emissive"] if r["emissive"] else ""
        print("%4d %-5s c(%s) s(%s)%s #%s%s%s" % (top, G[r["group"]], g(r["center"]), g(r["size"]), rot, hx(r["color"]), e, (" [" + r["ctx"][0]["kit"] + "]") if r["ctx"] else ""))
    else:
        print("%4d %s %s" % (top, r["kind"], {k: r[k] for k in r if k in ("text", "loc", "yaw", "size", "intensity")}))
