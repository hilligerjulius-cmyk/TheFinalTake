"""Dump the studio + city shell layout as JSON by compiling the ORIGINAL C++ builder bodies.

The level architecture is generated at runtime by AFTStudioShell / AFTCityShell (instanced
primitives). This script copies the Build*() / helper method bodies verbatim out of
FTStudioShell.cpp and FTCity.cpp (keeping their line numbers via #line), compiles them against
tiny Unreal type stubs (ue_stubs.h) and runs them. Every Add()/Text()/Point()/Spot() call is
recorded with its source line and the stack of helper ("kit") calls it came from, so the Blender
assets can be checked against - and mapped back to - the code that currently builds the map.

Usage:  python3 Tools/blender/layout/extract_layout.py [--out layout.json]
Needs a C++17 compiler (g++ or clang++). Nothing here touches the Unreal project itself.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
SRC = os.path.join(REPO, "Source", "The_Final_Take", "TheFinalTake")
STUDIO_CPP = os.path.join(SRC, "World", "FTStudioShell.cpp")
CITY_CPP = os.path.join(SRC, "World", "FTCity.cpp")
VISUALS_CPP = os.path.join(SRC, "Core", "FTVisuals.cpp")

KITS = {"Arrow", "Wall", "Stairs", "Poster", "Plant", "Crate", "FlightCase", "Cone", "Palm", "Truss", "Building", "StreetLamp"}
STUDIO_METHODS = ["Arrow", "Wall", "Stairs", "Poster", "Plant", "Crate", "FlightCase", "Cone", "Palm", "Truss",
                  "BuildExterior", "BuildLobby", "BuildOffice", "BuildWardrobe", "BuildStage", "BuildTankSet",
                  "BuildUpperLevel", "BuildWarehouse"]
CITY_METHODS = ["StreetLamp", "BuildBoulevard", "Building", "BuildBlocks", "BuildPlaza", "BuildCinema"]

GROUPS = [
    # name, shape, solid, notes
    ("CubeSolid", "Cube", True), ("CubeDeco", "Cube", False), ("BoxSolid", "Box", True), ("BoxDeco", "Box", False),
    ("CylSolid", "Cylinder", True), ("CylDeco", "Cylinder", False), ("SphereDeco", "Sphere", False),
    ("BallDeco", "Ball", False), ("ConeDeco", "Cone", False), ("PrismDeco", "Prism", False),
    ("RampSolid", "Ramp", True), ("TorusDeco", "Torus", False), ("CapsuleDeco", "Capsule", False),
    ("GlassSolid", "Cube", True), ("Blocker", "Cube", True), ("ShorelineDeco", "Shoreline", False),
]


def read(path):
    with open(path, encoding="utf-8-sig") as f:
        return f.read()


def match_brace(text, open_index):
    depth = 0
    i = open_index
    in_str = None
    while i < len(text):
        c = text[i]
        if in_str:
            if c == "\\":
                i += 2
                continue
            if c == in_str:
                in_str = None
        elif c in "\"'":
            in_str = c
        elif c == "/" and text[i:i + 2] == "//":
            i = text.index("\n", i)
            continue
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unbalanced braces")


def extract_method(text, cls, name, new_cls, relpath):
    m = re.search(r"^(void|int32)\s+%s::%s\(" % (cls, name), text, re.M)
    if not m:
        raise ValueError("method %s::%s not found" % (cls, name))
    body_open = text.index("{", m.end())
    body_close = match_brace(text, body_open)
    line = text.count("\n", 0, m.start()) + 1
    chunk = text[m.start():body_close + 1]
    target = name + "Impl" if name in KITS else name
    chunk = chunk.replace("%s::%s(" % (cls, name), "%s::%s(" % (new_cls, target), 1)
    return '#line %d "%s"\n%s\n' % (line, relpath, chunk)


def constants(text):
    """const FLinearColor lines + enum/Hash01 helpers from the file's anonymous namespace."""
    out = []
    m = re.search(r"^namespace\s*\{", text, re.M)
    text = text[m.end() - 1:match_brace(text, m.end() - 1) + 1]
    for line in text.splitlines():
        s = line.strip()
        if re.match(r"const FLinearColor \w+ = .*;$", s) or s.startswith("enum EZone"):
            out.append(s)
    m = re.search(r"^\s*float Hash01\(", text, re.M)
    if m:
        open_i = text.index("{", m.end())
        out.append(text[m.start():match_brace(text, open_i) + 1].strip())
    return "\n".join(out)


def palette(text):
    m = re.search(r"namespace FTColors\s*\{", text)
    body_close = match_brace(text, text.index("{", m.start()))
    block = text[m.end():body_close]
    lines = [l.strip() for l in block.splitlines() if l.strip().startswith("const FLinearColor")]
    return "namespace FTColors\n{\n" + "\n".join(lines) + "\n}\n"


RUNTIME = r'''
#include "ue_stubs.h"
#include <sstream>

std::string J(float V) { char B[64]; std::snprintf(B, sizeof(B), "%.4f", V); return B; }
std::string J(int V) { return std::to_string(V); }
std::string J(bool V) { return V ? "true" : "false"; }
std::string J(const FVector& V) { return "[" + J(V.X) + "," + J(V.Y) + "," + J(V.Z) + "]"; }
std::string J(const FRotator& V) { return "[" + J(V.Pitch) + "," + J(V.Yaw) + "," + J(V.Roll) + "]"; }
std::string J(const FLinearColor& V) { return "[" + J(V.R) + "," + J(V.G) + "," + J(V.B) + "," + J(V.A) + "]"; }
std::string J(const FColor& V) { return "[" + J((int)V.R) + "," + J((int)V.G) + "," + J((int)V.B) + "]"; }
std::string J(const std::string& V)
{
	std::string O = "\"";
	for (char C : V) { if (C == '"' || C == '\\') O += '\\'; O += C; }
	return O + "\"";
}
std::string J(const char* V) { return J(std::string(V)); }

std::string Shell::ContextJson() const
{
	std::string O = "[";
	for (size_t i = 0; i < Stack.size(); ++i) { O += (i ? "," : "") + Stack[i]; }
	return O + "]";
}

static void Push(Shell* S, const char* Kind, const char* File, int Line, const std::string& Payload)
{
	FRecord R;
	R.Kind = Kind; R.File = File; R.Line = Line; R.Context = S->ContextJson(); R.Payload = Payload;
	S->Records.push_back(R);
}

int32 Shell::AddAt(const char* File, int Line, int32 Group, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Emissive, const FRotator& Rot, float Gloss)
{
	Push(this, "prim", File, Line, "\"group\":" + J(Group) + ",\"center\":" + J(Center) + ",\"size\":" + J(Size) + ",\"rot\":" + J(Rot) + ",\"color\":" + J(Color) + ",\"emissive\":" + J(Emissive) + ",\"gloss\":" + J(Gloss));
	return (int32)Records.size() - 1;
}
void Shell::TextAt(const char* File, int Line, const FString& S, const FVector& Loc, float Yaw, float Size, const FColor& Color, float Pitch)
{
	Push(this, "text", File, Line, "\"text\":" + J(S) + ",\"loc\":" + J(Loc) + ",\"yaw\":" + J(Yaw) + ",\"pitch\":" + J(Pitch) + ",\"size\":" + J(Size) + ",\"color\":" + J(Color));
}
void Shell::PointAt(const char* File, int Line, const FVector& Loc, const FLinearColor& Color, float Intensity, float Radius, int32 Zone, bool bShadows)
{
	Push(this, "point", File, Line, "\"loc\":" + J(Loc) + ",\"color\":" + J(Color) + ",\"intensity\":" + J(Intensity) + ",\"radius\":" + J(Radius) + ",\"zone\":" + J(Zone) + ",\"shadows\":" + J(bShadows));
}
void Shell::SpotAt(const char* File, int Line, const FVector& Loc, const FRotator& Rot, const FLinearColor& Color, float Intensity, float Radius, float ConeAngle, int32 Zone)
{
	Push(this, "spot", File, Line, "\"loc\":" + J(Loc) + ",\"rot\":" + J(Rot) + ",\"color\":" + J(Color) + ",\"intensity\":" + J(Intensity) + ",\"radius\":" + J(Radius) + ",\"cone\":" + J(ConeAngle) + ",\"zone\":" + J(Zone));
}

static void Dump(FILE* F, const char* Name, Shell& S, bool bLast)
{
	std::fprintf(F, "\"%s\":[\n", Name);
	for (size_t i = 0; i < S.Records.size(); ++i)
	{
		const FRecord& R = S.Records[i];
		std::fprintf(F, "{\"id\":%d,\"kind\":\"%s\",\"file\":\"%s\",\"line\":%d,\"ctx\":%s,%s}%s\n", (int)i, R.Kind.c_str(), R.File.c_str(), R.Line, R.Context.c_str(), R.Payload.c_str(), i + 1 < S.Records.size() ? "," : "");
	}
	std::fprintf(F, "]%s\n", bLast ? "" : ",");
}

#define FT_RUN(S, M) (S).CurrentBuild = #M; (S).M();

int main(int argc, char** argv)
{
	Shell Studio;
	FT_RUN(Studio, BuildExterior) FT_RUN(Studio, BuildLobby) FT_RUN(Studio, BuildOffice) FT_RUN(Studio, BuildWardrobe)
	FT_RUN(Studio, BuildStage) FT_RUN(Studio, BuildTankSet) FT_RUN(Studio, BuildUpperLevel) FT_RUN(Studio, BuildWarehouse)
	CityShell City;
	FT_RUN(City, BuildBoulevard) FT_RUN(City, BuildBlocks) FT_RUN(City, BuildPlaza) FT_RUN(City, BuildCinema)
	FILE* F = std::fopen(argv[1], "w");
	std::fprintf(F, "{\n");
	Dump(F, "studio", Studio, false);
	Dump(F, "city", City, true);
	std::fprintf(F, "}\n");
	std::fclose(F);
	return 0;
}
'''


def build_sources(work):
    studio = read(STUDIO_CPP)
    city = read(CITY_CPP)
    pal = palette(read(VISUALS_CPP))
    rel_s = "Source/The_Final_Take/TheFinalTake/World/FTStudioShell.cpp"
    rel_c = "Source/The_Final_Take/TheFinalTake/World/FTCity.cpp"
    head = '#include "ue_stubs.h"\n' + pal + "using namespace FTColors;\n"
    studio_tu = head + "namespace {\n" + constants(studio) + "\n}\n" + "".join(
        extract_method(studio, "AFTStudioShell", n, "Shell", rel_s) for n in STUDIO_METHODS)
    city_tu = head + "namespace {\n" + constants(city) + "\n}\n" + "".join(
        extract_method(city, "AFTCityShell", n, "CityShell", rel_c) for n in CITY_METHODS)
    shutil.copy(os.path.join(HERE, "ue_stubs.h"), os.path.join(work, "ue_stubs.h"))
    files = {"studio_tu.cpp": studio_tu, "city_tu.cpp": city_tu, "runtime.cpp": RUNTIME}
    for name, content in files.items():
        with open(os.path.join(work, name), "w") as f:
            f.write(content)
    return [os.path.join(work, n) for n in files]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(REPO, "SourceArt", "Blender", "layout", "shell_layout.json"))
    ap.add_argument("--keep", action="store_true", help="keep the generated C++ for inspection")
    args = ap.parse_args()
    cxx = shutil.which("g++") or shutil.which("clang++")
    if not cxx:
        sys.exit("needs g++ or clang++")
    work = tempfile.mkdtemp(prefix="ft_layout_")
    try:
        sources = build_sources(work)
        exe = os.path.join(work, "ft_layout")
        subprocess.run([cxx, "-std=c++17", "-O1", "-w", "-o", exe] + sources, check=True, cwd=work)
        raw = os.path.join(work, "raw.json")
        subprocess.run([exe, raw], check=True)
        with open(raw) as f:
            data = json.load(f)
        data["groups"] = [{"index": i, "name": n, "shape": s, "solid": b} for i, (n, s, b) in enumerate(GROUPS)]
        data["source"] = {"studio": os.path.relpath(STUDIO_CPP, REPO), "city": os.path.relpath(CITY_CPP, REPO)}
        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        with open(args.out, "w") as f:
            json.dump(data, f, separators=(",", ":"))
        counts = {k: len(v) for k, v in data.items() if isinstance(v, list)}
        print("layout written:", args.out, counts)
    finally:
        if args.keep:
            print("kept", work)
        else:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
