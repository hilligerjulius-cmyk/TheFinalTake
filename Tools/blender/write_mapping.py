"""Write SourceArt/Blender/ASSET_MAPPING.md from asset_manifest.json: which Blender mesh replaces which
code-built object, where it goes, and how it hooks up. Plain Python (no Blender):  python3 write_mapping.py"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
ART = os.path.join(REPO, "SourceArt", "Blender")


def esc(s):
    return str(s).replace("|", "\\|").replace("\n", " ")


def placements(e):
    pl = e.get("placements") or []
    if not pl:
        return "zur Laufzeit / per Code"
    p = pl[0]
    loc = "(%s)" % ", ".join("%g" % round(x, 1) for x in p["loc"])
    rot = p["rot"]
    s = loc + ("" if not any(abs(x) > 1e-6 for x in rot) else " rot (%s)" % ", ".join("%g" % round(x, 1) for x in rot))
    if p["scale"] != [1.0, 1.0, 1.0]:
        s += " scale (%s)" % ", ".join("%g" % round(x, 3) for x in p["scale"])
    return s + (" +%d weitere" % (len(pl) - 1) if len(pl) > 1 else "")


def main():
    with open(os.path.join(ART, "asset_manifest.json"), encoding="utf-8") as f:
        assets = json.load(f)["assets"]
    by_folder = {}
    for e in assets:
        by_folder.setdefault(e["folder"], []).append(e)
    tris = sum(e["tris"] for e in assets)
    fits = [e["fit"]["status"] for e in assets if e.get("fit")]
    lines = [
        "# Zuordnung: Blender-Modell → ersetztes Code-Objekt",
        "",
        "Automatisch erzeugt aus `asset_manifest.json` (`python3 Tools/blender/write_mapping.py`). %d Meshes, %d Dreiecke gesamt; "
        "Maßprüfung: %d ok, %d close, %d check, %d ohne Code-Geometrie (Fahrzeuge, Schriftzüge)." % (
            len(assets), tris, fits.count("ok"), fits.count("close"), fits.count("check"), len(assets) - len(fits)),
        "",
        "Spalten: **Ersetzt** = die Code-Stelle (Datei/Funktion bzw. Actor-Komponenten); **Pivot** = Ursprung des Meshes; "
        "**Platzierung** = erste Welt-Platzierung im Level (Unreal-Koordinaten, cm; alle stehen im Manifest); "
        "**Fit** = Abweichung der Bounding-Box von den ersetzten Code-Teilen (Details: `fit_report.md`).",
        "",
    ]
    for folder in sorted(by_folder):
        lines += ["## %s" % folder, "", "| Mesh | Ersetzt (Code) | Pivot | Platzierung | Tris | Fit |", "|---|---|---|---|---|---|"]
        for e in sorted(by_folder[folder], key=lambda x: x["name"]):
            fit = e.get("fit")
            fit_s = "%s (%.0f cm)" % (fit["status"], fit["dev_cm"]) if fit else "–"
            rep = "; ".join(e["replaces"])
            if e.get("hookup"):
                rep += " — *Einbau:* " + e["hookup"]
            elif e.get("integration"):
                rep += " — *Einbau:* " + e["integration"]
            lines.append("| `%s` | %s | %s | %s | %d | %s |" % (e["name"], esc(rep), esc(e["pivot"]), esc(placements(e)), e["tris"], fit_s))
        lines.append("")
    with open(os.path.join(ART, "ASSET_MAPPING.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("wrote %d rows" % len(assets))


if __name__ == "__main__":
    main()
