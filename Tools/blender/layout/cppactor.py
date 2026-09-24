"""Tiny interpreter for the C++ subset the actor constructors use.

Runs an actor class' constructor chain (and OnConstruction) straight from the game source and records the
component tree: every FTVis::MakePart/SpawnPart (shape, size, relative transform) and every scene component
(CreateDefaultSubobject + SetupAttachment + SetRelative*). It also runs the map builder so each spawned actor
gets its per-instance settings (Spawn<T>(W, Loc, Yaw, Label, [](T* A) { A->X = ...; })).

It evaluates what it can: literals, FVector/FRotator maths, locals, loops, if/switch, lambdas and member
functions of the class. Anything else (lights, particles, materials, runtime-only calls) evaluates to Unknown and
is ignored. Pure Python, no Unreal or Blender needed:  python3 cppactor.py AFTStageLight
"""
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
SRC = os.path.join(REPO, "Source", "The_Final_Take", "TheFinalTake")

# unit half extents of the generated shapes (FTContentCommandlet: 100 cm meshes; torus R35/r15, capsule r25)
HALF = {"Torus": (50, 50, 15), "Capsule": (25, 25, 50), "Plane": (50, 50, 0), "WaterGrid": (50, 50, 0)}


# ============================================================================ values

class _Unknown:
    def __repr__(self):
        return "?"

    def __bool__(self):
        raise UnknownBranch()


U = _Unknown()


class UnknownBranch(Exception):
    pass


class V:
    __slots__ = ("x", "y", "z")

    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x, self.y, self.z = float(x), float(y), float(z)

    def t(self):
        return (self.x, self.y, self.z)

    def __repr__(self):
        return "V(%g, %g, %g)" % self.t()


class R:
    __slots__ = ("p", "y", "r")

    def __init__(self, p=0.0, y=0.0, r=0.0):
        self.p, self.y, self.r = float(p), float(y), float(r)

    def t(self):
        return (self.p, self.y, self.r)

    def __repr__(self):
        return "R(%g, %g, %g)" % self.t()


def rot_matrix(p, y, r):
    """FRotationMatrix, column-vector convention (v_world = M v_local)."""
    d = math.pi / 180.0
    sp, cp = math.sin(p * d), math.cos(p * d)
    sy, cy = math.sin(y * d), math.cos(y * d)
    sr, cr = math.sin(r * d), math.cos(r * d)
    return [[cp * cy, sr * sp * cy - cr * sy, -(cr * sp * cy + sr * sy)],
            [cp * sy, sr * sp * sy + cr * cy, cy * sr - cr * sp * sy],
            [sp, -sr * cp, cr * cp]]


def mat_xf(loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    m = rot_matrix(*rot)
    return [[m[i][0] * scale[0], m[i][1] * scale[1], m[i][2] * scale[2], loc[i]] for i in range(3)] + [[0, 0, 0, 1]]


def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def mat_pt(m, p):
    return tuple(m[i][0] * p[0] + m[i][1] * p[1] + m[i][2] * p[2] + m[i][3] for i in range(3))


def mat_inv_rigid_scaled(m):
    """General 4x4 inverse (affine)."""
    a = [row[:3] for row in m[:3]]
    det = (a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
           + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]))
    inv = [[(a[(j + 1) % 3][(i + 1) % 3] * a[(j + 2) % 3][(i + 2) % 3] - a[(j + 1) % 3][(i + 2) % 3] * a[(j + 2) % 3][(i + 1) % 3]) / det
            for j in range(3)] for i in range(3)]
    t = [-(inv[i][0] * m[0][3] + inv[i][1] * m[1][3] + inv[i][2] * m[2][3]) for i in range(3)]
    return [inv[i] + [t[i]] for i in range(3)] + [[0, 0, 0, 1]]


def rotate(rot, v):
    m = rot_matrix(*rot.t())
    return V(*(m[i][0] * v.x + m[i][1] * v.y + m[i][2] * v.z for i in range(3)))


def shape_bounds(shape, m):
    """Exact-ish AABB (lo, hi) of a generated unit shape under the affine 3x4/4x4 matrix m (unit mesh -> target).
    Round shapes use their analytic extents instead of box corners (a rotated sphere is not its rotated box)."""
    t = [m[i][3] for i in range(3)]
    ext = []
    for i in range(3):
        a, b, c = m[i][0], m[i][1], m[i][2]
        disc = math.sqrt(a * a + b * b)
        if shape in ("Sphere", "Ball"):
            e = 50.0 * math.sqrt(a * a + b * b + c * c)
        elif shape in ("Cylinder", "Cone"):
            e = 50.0 * disc + 50.0 * abs(c)
        elif shape == "Capsule":
            e = 25.0 * abs(c) + 25.0 * math.sqrt(a * a + b * b + c * c)
        elif shape == "Torus":
            e = 50.0 * disc + 15.0 * abs(c)
        elif shape in ("Plane", "WaterGrid"):
            e = 50.0 * (abs(a) + abs(b))
        else:  # Cube, Box, Prism, Ramp, CrewTorso, Shoreline ...
            e = 50.0 * (abs(a) + abs(b) + abs(c))
        ext.append(e)
    return [t[i] - ext[i] for i in range(3)], [t[i] + ext[i] for i in range(3)]


class Comp:
    """A USceneComponent / UStaticMeshComponent created by the constructor."""

    def __init__(self, kind, name, line, site):
        self.kind, self.name, self.line, self.site = kind, name, line, site
        self.parent = None
        self.shape = None
        self.size = None
        self.loc, self.rot, self.scale = V(), R(), V(1, 1, 1)
        self.visible = True

    def __repr__(self):
        return "<%s %s>" % (self.kind, self.name)

    def is_part(self):
        return self.shape is not None

    def matrix(self):
        """Transform into the actor's space (the actor root)."""
        m = mat_xf(self.loc.t(), self.rot.t(), self.scale.t())
        return mat_mul(self.parent.matrix(), m) if isinstance(self.parent, Comp) else m

    def chain(self):
        c, out = self, []
        while isinstance(c, Comp):
            out.append(c)
            c = c.parent
        return out

    def bounds(self, space=None):
        """Shape-aware AABB (lo, hi) in actor space or relative to component `space`."""
        m = self.matrix()
        if space is not None:
            m = mat_mul(mat_inv_rigid_scaled(space.matrix()), m)
        return shape_bounds(self.shape, m)

    def corners(self, space=None):
        """8 corners of the part's shape; in actor space or relative to component `space`."""
        hx, hy, hz = HALF.get(self.shape, (50, 50, 50))
        m = self.matrix()
        if space is not None:
            m = mat_mul(mat_inv_rigid_scaled(space.matrix()), m)
        return [mat_pt(m, (sx * hx, sy * hy, sz * hz)) for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)]


class Obj:
    """An actor instance (members) or a spawn-setup proxy."""

    def __init__(self, cls):
        self.cls = cls
        self.members = {}


class Lambda:
    def __init__(self, params, body, scope):
        self.params, self.body, self.scope = params, body, scope


class Method:
    def __init__(self, cls, name, params, body):
        self.cls, self.name, self.params, self.body = cls, name, params, body


# ============================================================================ tokenizer

TOKEN = re.compile(r"""
    (?P<ws>[ \t\r\f\v]+|\n)
  | (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<pp>\#[^\n]*)
  | (?P<str>(?:u8|L|u|U)?"(?:\\.|[^"\\])*")
  | (?P<chr>'(?:\\.|[^'\\])')
  | (?P<num>0[xX][0-9A-Fa-f]+[uUlL]*|(?:\d+\.\d*|\.\d+|\d+)(?:[eE][+-]?\d+)?[fFuUlL]*)
  | (?P<id>[A-Za-z_]\w*)
  | (?P<op>::|->|\+\+|--|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|/=|%=|<<|[-+*/%<>=!&|^~?:;,.(){}\[\]])
""", re.S | re.X)


class Tok:
    __slots__ = ("kind", "text", "line")

    def __init__(self, kind, text, line):
        self.kind, self.text, self.line = kind, text, line

    def __repr__(self):
        return "%s@%d" % (self.text, self.line)


def tokenize(text):
    out, line, pos = [], 1, 0
    text = text.lstrip("﻿")
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m:
            raise SyntaxError("bad char %r at line %d" % (text[pos], line))
        kind = m.lastgroup
        s = m.group()
        if kind not in ("ws", "comment", "pp"):
            out.append(Tok(kind, s, line))
        line += s.count("\n")
        pos = m.end()
    return out


# ============================================================================ parser (tokens -> AST)

class ParseError(Exception):
    pass


TEMPLATE_FUNCS = {"CreateDefaultSubobject", "NewObject", "Cast", "CastChecked", "static_cast", "reinterpret_cast",
                  "const_cast", "Spawn", "GetGameState", "SpawnActor", "SpawnActorDeferred", "MakeShared",
                  "TArray", "TInlineComponentArray", "GetComponents", "FindComponentByClass", "LoadObject"}
TYPE_WORDS = {"const", "static", "constexpr", "unsigned", "signed", "volatile", "mutable", "inline"}
BIN_PREC = [("||",), ("&&",), ("|",), ("^",), ("&",), ("==", "!="), ("<", ">", "<=", ">="), ("<<",), ("+", "-"), ("*", "/", "%")]


class Parser:
    def __init__(self, toks, i=0, end=None):
        self.t = toks
        self.i = i
        self.end = len(toks) if end is None else end

    # ------------------------------------------------------------ helpers
    def peek(self, k=0):
        j = self.i + k
        return self.t[j] if j < self.end else Tok("eof", "", self.t[-1].line if self.t else 0)

    def next(self):
        tok = self.peek()
        self.i += 1
        return tok

    def accept(self, text):
        if self.peek().text == text and self.peek().kind in ("op", "id"):
            self.i += 1
            return True
        return False

    def expect(self, text):
        if not self.accept(text):
            raise ParseError("expected %r got %r at line %d" % (text, self.peek().text, self.peek().line))

    def match_close(self, i, open_, close):
        depth = 0
        for j in range(i, self.end):
            s = self.t[j].text
            if self.t[j].kind == "op":
                if s == open_:
                    depth += 1
                elif s == close:
                    depth -= 1
                    if depth == 0:
                        return j
        raise ParseError("unbalanced %s at line %d" % (open_, self.t[i].line))

    def skip_statement(self):
        depth = 0
        while self.i < self.end:
            s = self.peek().text
            if s in "({[":
                depth += 1
            elif s in ")}]":
                if depth == 0:
                    return
                depth -= 1
                if depth == 0 and s == "}" and self.peek(1).text != ";" and self.peek(1).text != ")":
                    self.i += 1
                    return
            elif s == ";" and depth == 0:
                self.i += 1
                return
            self.i += 1

    def template_args_end(self, i):
        """If t[i] is '<' opening template arguments, return the index of the matching '>' (else None)."""
        depth = 0
        for j in range(i, min(self.end, i + 40)):
            tok = self.t[j]
            if tok.text == "<":
                depth += 1
            elif tok.text == ">":
                depth -= 1
                if depth == 0:
                    return j
            elif tok.text == ">>":
                return None
            elif tok.kind == "op" and tok.text not in ("::", "*", "&", ",", "<", ">"):
                return None
        return None

    # ------------------------------------------------------------ statements
    def block_body(self):
        """Parse statements until the end (for a function body range)."""
        stmts = []
        while self.i < self.end:
            stmts.append(self.statement())
        return ("block", stmts)

    def statement(self):
        tok = self.peek()
        line = tok.line
        start = self.i
        try:
            return self._statement()
        except (ParseError, IndexError) as e:
            self.i = start
            self.skip_statement()
            if self.i == start:
                self.i += 1
            return ("skip", line, str(e))

    def _statement(self):
        tok = self.peek()
        s = tok.text
        if s == "{":
            j = self.match_close(self.i, "{", "}")
            sub = Parser(self.t, self.i + 1, j)
            body = sub.block_body()
            self.i = j + 1
            return body
        if s == ";":
            self.i += 1
            return ("block", [])
        if tok.kind == "id":
            if s == "if":
                self.next()
                self.accept("constexpr")
                self.expect("(")
                j = self.match_close(self.i - 1, "(", ")")
                cond = self.cond_or_decl(self.i, j)
                self.i = j + 1
                then = self.statement()
                other = None
                if self.accept("else"):
                    other = self.statement()
                return ("if", cond, then, other, tok.line)
            if s == "for":
                self.next()
                self.expect("(")
                j = self.match_close(self.i - 1, "(", ")")
                # range-for: find a ':' at depth 0 (not '::')
                depth, colon = 0, None
                for k in range(self.i, j):
                    x = self.t[k].text
                    if x in "([{":
                        depth += 1
                    elif x in ")]}":
                        depth -= 1
                    elif x == ":" and depth == 0:
                        colon = k
                        break
                if colon is not None:
                    name = self.t[colon - 1].text
                    expr = Parser(self.t, colon + 1, j).expression()
                    self.i = j + 1
                    body = self.statement()
                    return ("forrange", name, expr, body, tok.line)
                semis = [k for k in range(self.i, j) if self.t[k].text == ";"]
                init = Parser(self.t, self.i, semis[0] + 1).statement() if semis[0] > self.i else ("block", [])
                cond = Parser(self.t, semis[0] + 1, semis[1]).expression() if semis[1] > semis[0] + 1 else ("num", 1.0)
                step = Parser(self.t, semis[1] + 1, j).expression() if j > semis[1] + 1 else ("num", 0.0)
                self.i = j + 1
                body = self.statement()
                return ("for", init, cond, step, body, tok.line)
            if s == "while":
                self.next()
                self.expect("(")
                j = self.match_close(self.i - 1, "(", ")")
                cond = Parser(self.t, self.i, j).expression()
                self.i = j + 1
                return ("while", cond, self.statement(), tok.line)
            if s == "do":
                self.next()
                body = self.statement()
                self.expect("while")
                self.expect("(")
                j = self.match_close(self.i - 1, "(", ")")
                cond = Parser(self.t, self.i, j).expression()
                self.i = j + 1
                self.accept(";")
                return ("dowhile", body, cond, tok.line)
            if s == "switch":
                self.next()
                self.expect("(")
                j = self.match_close(self.i - 1, "(", ")")
                expr = Parser(self.t, self.i, j).expression()
                self.i = j + 1
                if self.peek().text != "{":
                    raise ParseError("switch without block")
                k = self.match_close(self.i, "{", "}")
                sub = Parser(self.t, self.i + 1, k)
                items = []
                while sub.i < sub.end:
                    if sub.peek().text == "case":
                        sub.next()
                        colon = sub.i
                        while sub.t[colon].text != ":" or (colon + 1 < sub.end and sub.t[colon + 1].text == ":"):
                            colon += 2 if sub.t[colon].text == "::" else 1
                        items.append(("case", Parser(self.t, sub.i, colon).expression()))
                        sub.i = colon + 1
                    elif sub.peek().text == "default" and sub.peek(1).text == ":":
                        sub.i += 2
                        items.append(("default",))
                    else:
                        items.append(sub.statement())
                self.i = k + 1
                return ("switch", expr, items, tok.line)
            if s == "return":
                self.next()
                if self.accept(";"):
                    return ("return", None)
                e = self.expression()
                self.accept(";")
                return ("return", e)
            if s in ("break", "continue"):
                self.next()
                self.accept(";")
                return (s,)
            if s in ("using", "typedef", "static_assert", "check", "ensure", "checkf", "UE_LOG", "DOREPLIFETIME", "DOREPLIFETIME_CONDITION"):
                self.skip_statement()
                return ("block", [])
            decl = self.try_declaration()
            if decl is not None:
                return decl
        e = self.expression()
        if not self.accept(";"):
            if self.i < self.end:
                raise ParseError("expected ; after expression at line %d (got %r)" % (tok.line, self.peek().text))
        return ("expr", e, tok.line)

    def cond_or_decl(self, a, b):
        sub = Parser(self.t, a, b)
        d = sub.try_declaration(end_ok=True)
        if d is not None:
            return ("condecl", d)
        return sub.expression()

    def parse_type(self):
        """Consume a type; returns its text or None (restoring the position)."""
        start = self.i
        words = []
        while self.peek().kind == "id" and self.peek().text in TYPE_WORDS:
            self.next()
        if self.peek().kind != "id":
            self.i = start
            return None
        if self.peek().text in ("typename", "struct", "class", "enum"):
            self.next()
        words.append(self.next().text)
        while self.peek().text == "::" and self.peek(1).kind == "id":
            self.next()
            words.append(self.next().text)
        if self.peek().text == "<":
            j = self.template_args_end(self.i)
            if j is None:
                self.i = start
                return None
            self.i = j + 1
        while self.peek().kind == "id" and self.peek().text in TYPE_WORDS:
            self.next()
        while self.peek().text in ("*", "&", "&&"):
            self.next()
            while self.peek().kind == "id" and self.peek().text == "const":
                self.next()
        return "::".join(words)

    def try_declaration(self, end_ok=False):
        start = self.i
        ty = self.parse_type()
        if ty is None or ty in ("return", "delete", "new", "throw", "case", "goto", "else"):
            self.i = start
            return None
        if self.peek().kind != "id" or self.peek(1).text not in ("=", ";", "(", "{", "[", ",", ")") and not end_ok:
            self.i = start
            return None
        if self.peek().kind != "id":
            self.i = start
            return None
        decls = []
        while True:
            name = self.next().text
            line = self.peek().line
            is_array = False
            if self.accept("["):
                is_array = True
                j = self.match_close(self.i - 1, "[", "]")
                self.i = j + 1
            init = None
            if self.accept("="):
                init = self.initializer()
            elif self.peek().text == "(":
                j = self.match_close(self.i, "(", ")")
                args = Parser(self.t, self.i + 1, j).arg_list()
                self.i = j + 1
                init = ("call", ("name", ty), args, line)
            elif self.peek().text == "{":
                init = self.initializer()
                if not is_array:
                    init = ("call", ("name", ty), init[1], line)
            decls.append((ty, name, init, is_array))
            if self.accept(","):
                continue
            break
        if not self.accept(";") and not end_ok:
            if self.i < self.end:
                raise ParseError("bad declaration at line %d" % self.peek().line)
        return ("decl", decls)

    def initializer(self):
        if self.peek().text == "{":
            j = self.match_close(self.i, "{", "}")
            items = Parser(self.t, self.i + 1, j).arg_list(allow_trailing=True)
            self.i = j + 1
            return ("list", items)
        return self.assignment()

    def arg_list(self, allow_trailing=False):
        items = []
        while self.i < self.end:
            if self.peek().text == "{":
                items.append(self.initializer())
            else:
                items.append(self.assignment())
            if not self.accept(","):
                break
            if allow_trailing and self.i >= self.end:
                break
        if self.i < self.end:
            raise ParseError("junk in argument list at line %d: %r" % (self.peek().line, self.peek().text))
        return items

    # ------------------------------------------------------------ expressions
    def expression(self):
        e = self.assignment()
        while self.accept(","):
            e = ("comma", e, self.assignment())
        return e

    def assignment(self):
        lhs = self.ternary()
        tok = self.peek()
        if tok.kind == "op" and tok.text in ("=", "+=", "-=", "*=", "/=", "%="):
            self.next()
            rhs = self.initializer() if self.peek().text == "{" else self.assignment()
            return ("assign", tok.text, lhs, rhs)
        return lhs

    def ternary(self):
        c = self.binary(0)
        if self.accept("?"):
            a = self.assignment()
            self.expect(":")
            b = self.assignment()
            return ("tern", c, a, b)
        return c

    def binary(self, level):
        if level >= len(BIN_PREC):
            return self.unary()
        e = self.binary(level + 1)
        while self.peek().kind == "op" and self.peek().text in BIN_PREC[level]:
            op = self.next().text
            e = ("bin", op, e, self.binary(level + 1))
        return e

    def unary(self):
        tok = self.peek()
        if tok.kind == "op" and tok.text in ("-", "+", "!", "~", "*", "&", "++", "--"):
            self.next()
            return ("unary", tok.text, self.unary())
        if tok.kind == "id" and tok.text in ("new",):
            raise ParseError("new")
        if tok.text == "(":
            # C-style cast: (float)x / (int32)x
            if self.peek(1).kind == "id" and self.peek(2).text == ")" and self.peek(1).text in ("float", "double", "int32", "int", "uint8", "uint32", "bool", "void"):
                self.i += 3
                return ("cast", "num", self.unary())
        return self.postfix(self.primary())

    def postfix(self, e):
        while True:
            tok = self.peek()
            if tok.text == "(":
                j = self.match_close(self.i, "(", ")")
                args = Parser(self.t, self.i + 1, j).arg_list() if j > self.i + 1 else []
                self.i = j + 1
                e = ("call", e, args, tok.line)
            elif tok.text == "[":
                j = self.match_close(self.i, "[", "]")
                idx = Parser(self.t, self.i + 1, j).expression()
                self.i = j + 1
                e = ("index", e, idx)
            elif tok.text in (".", "->"):
                self.next()
                name = self.next().text
                if self.peek().text == "<":
                    j = self.template_args_end(self.i)
                    if j is not None and j + 1 < self.end and self.t[j + 1].text == "(":
                        self.i = j + 1
                e = ("member", e, name)
            elif tok.text in ("++", "--") and tok.kind == "op":
                self.next()
                e = ("postfix", tok.text, e)
            else:
                return e

    def primary(self):
        tok = self.next()
        if tok.kind == "num":
            s = tok.text.rstrip("fFuUlL")
            if s.lower().startswith("0x"):
                return ("num", float(int(s, 16)))
            return ("num", float(s))
        if tok.kind == "str":
            return ("str", bytes(tok.text[tok.text.index('"') + 1:-1], "utf-8").decode("unicode_escape", errors="ignore"))
        if tok.kind == "chr":
            return ("str", tok.text[1:-1])
        if tok.text == "(":
            e = self.expression()
            self.expect(")")
            return e
        if tok.text == "{":
            self.i -= 1
            return self.initializer()
        if tok.text == "[":
            return self.lambda_expr()
        if tok.text == "::":
            tok = self.next()
        if tok.kind == "id":
            name = tok.text
            while self.peek().text == "::" and self.peek(1).kind == "id":
                self.next()
                name += "::" + self.next().text
            if self.peek().text == "<":
                base = name.split("::")[-1]
                j = self.template_args_end(self.i)
                if j is not None and (base in TEMPLATE_FUNCS or (j + 1 < self.end and self.t[j + 1].text in ("(", "{", "::"))):
                    targs = [t.text for t in self.t[self.i + 1:j] if t.kind == "id"]
                    self.i = j + 1
                    if self.peek().text == "::" and self.peek(1).kind == "id":
                        self.next()
                        name += "::" + self.next().text
                    return ("tname", name, targs, tok.line)
            return ("name", name, tok.line)
        raise ParseError("unexpected %r at line %d" % (tok.text, tok.line))

    def lambda_expr(self):
        j = self.match_close(self.i - 1, "[", "]")
        self.i = j + 1
        params = []
        if self.peek().text == "(":
            k = self.match_close(self.i, "(", ")")
            params = parse_params(self.t, self.i + 1, k)
            self.i = k + 1
        while self.peek().text != "{":
            self.next()
        k = self.match_close(self.i, "{", "}")
        body = Parser(self.t, self.i + 1, k).block_body()
        self.i = k + 1
        return ("lambda", params, body)


def parse_params(toks, a, b):
    """Parameter list -> [(type, name, default_ast)]."""
    params = []
    depth, cur = 0, []
    items = []
    for k in range(a, b):
        s = toks[k].text
        if s in "(<{[":
            depth += 1
        elif s in ")>}]":
            depth -= 1
        if s == "," and depth == 0:
            items.append(cur)
            cur = []
        else:
            cur.append(k)
    if cur:
        items.append(cur)
    for idx in items:
        eq = next((k for k in idx if toks[k].text == "="), None)
        head = [k for k in idx if eq is None or k < eq]
        ids = [k for k in head if toks[k].kind == "id"]
        if not ids:
            continue
        name = toks[ids[-1]].text
        ty = " ".join(toks[k].text for k in head[:-1])
        default = Parser(toks, eq + 1, idx[-1] + 1).expression() if eq is not None else None
        params.append((ty, name, default))
    return params


# ============================================================================ source index

class Source:
    """All .cpp/.h of the module: class methods and member defaults."""

    def __init__(self, root=SRC):
        self.root = root
        self.methods = {}   # (cls, name) -> Method
        self.bases = {}     # cls -> base cls
        self.defaults = {}  # cls -> [(name, ast)]
        self.globals = {}   # file -> [(type, name, init_ast, is_array)] at file/namespace scope
        self.funcs = {}     # (file, name) -> free function
        self.files = {}
        for dirpath, _, files in os.walk(root):
            for f in files:
                if f.endswith((".cpp", ".h")):
                    self.index(os.path.join(dirpath, f))

    def index(self, path):
        with open(path, encoding="utf-8-sig", errors="replace") as fh:
            text = fh.read()
        try:
            toks = tokenize(text)
        except SyntaxError:
            return
        rel = os.path.relpath(path, self.root).replace("\\", "/")
        self.files[rel] = toks
        if path.endswith(".cpp"):
            self.scan_top(rel, toks)
        n = len(toks)
        i = 0
        while i < n:
            t = toks[i]
            # class Foo : public Bar {  (headers)
            if t.text in ("class", "struct") and t.kind == "id" and path.endswith(".h"):
                j = i + 1
                while j < n and toks[j].kind == "id" and (toks[j].text.endswith("_API") or toks[j].text in ("final",)):
                    j += 1
                if j < n and toks[j].kind == "id":
                    cls = toks[j].text
                    k = j + 1
                    base = None
                    if k < n and toks[k].text == ":":
                        while k < n and toks[k].text not in ("{", ";"):
                            if toks[k].kind == "id" and toks[k].text not in ("public", "protected", "private", "virtual", "final"):
                                base = toks[k].text
                                break
                            k += 1
                    while k < n and toks[k].text not in ("{", ";"):
                        k += 1
                    if k < n and toks[k].text == "{":
                        end = Parser(toks).match_close(k, "{", "}")
                        if base:
                            self.bases[cls] = base
                        self.defaults[cls] = self.member_defaults(toks, k + 1, end)
                        i = k + 1
                        continue
            # Cls::Name(params) [const] [: init] { body }
            if t.kind == "id" and i + 3 < n and toks[i + 1].text == "::" and toks[i + 2].kind == "id" and toks[i + 3].text == "(" \
                    and (i == 0 or toks[i - 1].text not in ("::", ".", "->", "(", ",", "=", "return")):
                cls, name = t.text, toks[i + 2].text
                if name.startswith("~"):
                    i += 1
                    continue
                try:
                    close = Parser(toks).match_close(i + 3, "(", ")")
                except ParseError:
                    i += 1
                    continue
                k = close + 1
                while k < n and toks[k].text in ("const", "override", "noexcept"):
                    k += 1
                if k < n and toks[k].text == ":":
                    # constructor initialiser list: skip to the body brace at depth 0
                    depth = 0
                    while k < n:
                        s = toks[k].text
                        if s in "([":
                            depth += 1
                        elif s in ")]":
                            depth -= 1
                        elif s == "{" and depth == 0 and toks[k - 1].text in (")", "}"):
                            break
                        elif s == "{":
                            depth += 1
                        elif s == "}":
                            depth -= 1
                        k += 1
                if k < n and toks[k].text == "{":
                    end = Parser(toks).match_close(k, "{", "}")
                    params = parse_params(toks, i + 4, close)
                    self.methods[(cls, name)] = (rel, toks, k + 1, end, params)
                    i = end + 1
                    continue
            i += 1

    def scan_top(self, rel, toks):
        """File/namespace scope of a .cpp: constants (const float X = ...;) and free functions."""
        n = len(toks)
        P = Parser(toks)
        i = 0
        while i < n:
            s = toks[i].text
            if s == "namespace":
                j = i + 1
                while j < n and toks[j].text not in ("{", ";"):
                    j += 1
                i = j + 1
                continue
            if s in ("}", ";"):
                i += 1
                continue
            if s == "{":
                i = P.match_close(i, "{", "}") + 1
                continue
            p = Parser(toks, i)
            ty = p.parse_type()
            if ty and p.peek().kind == "id" and p.peek(1).text == "(":
                name = p.peek().text
                try:
                    close = P.match_close(p.i + 1, "(", ")")
                except ParseError:
                    close = None
                if close is not None:
                    k = close + 1
                    while k < n and toks[k].text in ("const", "override", "noexcept"):
                        k += 1
                    if k < n and toks[k].text == "{":
                        end = P.match_close(k, "{", "}")
                        self.funcs[(rel, name)] = (rel, toks, k + 1, end, parse_params(toks, p.i + 2, close))
                        i = end + 1
                        continue
            elif ty and p.peek().kind == "id":
                p2 = Parser(toks, i)
                try:
                    d = p2.try_declaration()
                except ParseError:
                    d = None
                if d is not None:
                    self.globals.setdefault(rel, []).extend(d[1])
                    i = p2.i
                    continue
            j = i
            while j < n and toks[j].text not in (";", "{", "}"):
                if toks[j].text == "(":
                    try:
                        j = P.match_close(j, "(", ")")
                    except ParseError:
                        pass
                j += 1
            if j < n and toks[j].text == "{":
                try:
                    i = P.match_close(j, "{", "}") + 1
                except ParseError:
                    i = j + 1
            else:
                i = j + 1

    def free_function(self, name, rel=None):
        keys = [(rel, name)] + [k for k in self.funcs if k[1] == name and k[0] != rel]
        for k in keys:
            m = self.funcs.get(k)
            if m is None:
                continue
            if not isinstance(m, Method):
                frel, toks, a, b, params = m
                m = Method(None, name, params, Parser(toks, a, b).block_body())
                m.file = frel
                self.funcs[k] = m
            return m
        return None

    def member_defaults(self, toks, a, b):
        out = []
        k = a
        while k < b:
            s = toks[k].text
            if s == "{":
                # skip inline method bodies / nested types
                try:
                    k = Parser(toks, 0, b).match_close(k, "{", "}") + 1
                except ParseError:
                    k += 1
                continue
            # TArray<...> Name;  -> empty array
            if toks[k].text == "TArray" and k + 1 < b and toks[k + 1].text == "<":
                j = Parser(toks, 0, b).template_args_end(k + 1)
                if j is not None and j + 2 < b and toks[j + 1].kind == "id" and toks[j + 2].text == ";":
                    out.append((toks[j + 1].text, ("list", [])))
                    k = j + 3
                    continue
            # Type Name = expr ;
            if toks[k].kind == "id" and k + 2 < b and toks[k + 1].text == "=" and toks[k - 1].kind in ("id", "op") and toks[k - 1].text not in ("(", ",", "=", "return", "::", ".", "->"):
                e = k + 2
                d = 0
                while e < b and not (toks[e].text == ";" and d == 0):
                    if toks[e].text in "({[":
                        d += 1
                    elif toks[e].text in ")}]":
                        d -= 1
                    e += 1
                try:
                    out.append((toks[k].text, Parser(toks, k + 2, e).initializer()))
                except ParseError:
                    pass
                k = e + 1
                continue
            k += 1
        return out

    def method(self, cls, name):
        c = cls
        while c:
            m = self.methods.get((c, name))
            if m is not None:
                if not isinstance(m, Method):
                    rel, toks, a, b, params = m
                    m = Method(c, name, params, Parser(toks, a, b).block_body())
                    m.file = rel
                    self.methods[(c, name)] = m
                return m
            c = self.bases.get(c)
        return None

    def chain(self, cls):
        out = []
        c = cls
        while c:
            out.append(c)
            c = self.bases.get(c)
        return out[::-1]


# ============================================================================ interpreter

class Return(Exception):
    def __init__(self, value):
        self.value = value


class Break(Exception):
    pass


class Continue(Exception):
    pass


CONSTS = {
    "FVector::ZeroVector": lambda: V(), "FVector::OneVector": lambda: V(1, 1, 1), "FVector::UpVector": lambda: V(0, 0, 1),
    "FVector::ForwardVector": lambda: V(1, 0, 0), "FVector::RightVector": lambda: V(0, 1, 0),
    "FRotator::ZeroRotator": lambda: R(), "PI": lambda: math.pi, "UE_PI": lambda: math.pi, "true": lambda: True,
    "false": lambda: False, "nullptr": lambda: None, "INDEX_NONE": lambda: -1.0,
}


def num(x):
    return isinstance(x, (int, float)) and not isinstance(x, bool) or isinstance(x, bool)


class Interp:
    def __init__(self, source, trace=False):
        self.src = source
        self.trace = trace
        self.comps = []
        self.site = []
        self.depth = 0
        self.skipped = []
        self.spawns = []
        self.file_stack = []
        self._globals = {}

    def cur_file(self):
        return self.file_stack[-1] if self.file_stack else None

    def file_globals(self, rel):
        if rel is None:
            return {}
        g = self._globals.get(rel)
        if g is None:
            g = self._globals[rel] = {}
            for ty, name, init, is_array in self.src.globals.get(rel, []):
                try:
                    g[name] = self.eval(init, [g], None) if init is not None else self.default_value(ty, is_array)
                except (UnknownBranch, Return, Break, Continue):
                    g[name] = U
        return g

    # ------------------------------------------------------------ running actors
    def run_actor(self, cls, overrides=None, construct=("OnConstruction",)):
        """Constructor chain, then the spawn settings, then OnConstruction. Returns (actor Obj, comps)."""
        self.comps = []
        actor = Obj(cls)
        for c in self.src.chain(cls):
            for name, ast in self.src.defaults.get(c, []):
                try:
                    actor.members[name] = self.eval(ast, [{}], actor)
                except Exception:
                    actor.members[name] = U
        for c in self.src.chain(cls):
            m = self.src.methods.get((c, c))
            if m is not None:
                m = self.src.method(c, c)
                self.call_method(m, [], actor)
        for k, v in (overrides or {}).items():
            actor.members[k] = v
        for name in construct:
            m = self.src.method(cls, name)
            if m is not None:
                self.call_method(m, [None] * len(m.params), actor)
        return actor, list(self.comps)

    def run_function(self, cls, name):
        m = self.src.method(cls, name)
        actor = Obj(cls)
        self.call_method(m, [], actor)
        return actor

    def call_method(self, m, args, this):
        if self.depth > 40:
            return U
        scope = {}
        for idx, (ty, pname, default) in enumerate(m.params):
            if idx < len(args):
                scope[pname] = args[idx]
            elif default is not None:
                scope[pname] = self.eval(default, [scope], this)
            else:
                scope[pname] = U
        self.depth += 1
        self.file_stack.append(getattr(m, "file", None))
        try:
            self.exec(m.body, [scope], this)
        except Return as r:
            return r.value
        except (Break, Continue):
            pass
        finally:
            self.depth -= 1
            self.file_stack.pop()
        return None

    def call_lambda(self, lam, args, this, line):
        if self.depth > 40:
            return U
        scope = {}
        for idx, (ty, pname, default) in enumerate(lam.params):
            if idx < len(args):
                scope[pname] = args[idx]
            elif default is not None:
                scope[pname] = self.eval(default, lam.scope + [scope], this)
            else:
                scope[pname] = U
        self.depth += 1
        self.site.append(line)
        try:
            self.exec(lam.body, lam.scope + [scope], this)
        except Return as r:
            return r.value
        finally:
            self.depth -= 1
            self.site.pop()
        return None

    # ------------------------------------------------------------ statements
    def exec(self, st, scopes, this):
        kind = st[0]
        if kind == "block":
            scopes = scopes + [{}]
            for s in st[1]:
                self.exec(s, scopes, this)
        elif kind == "expr":
            try:
                self.eval(st[1], scopes, this)
            except UnknownBranch:
                self.skipped.append((st[2], "unknown condition"))
        elif kind == "decl":
            for ty, name, init, is_array in st[1]:
                if init is None:
                    val = self.default_value(ty, is_array)
                else:
                    try:
                        val = self.eval(init, scopes, this)
                    except UnknownBranch:
                        val = U
                    if ty in ("float", "double") and isinstance(val, bool):
                        val = float(val)
                    if ty in ("int32", "int", "uint8", "uint32", "int64") and num(val):
                        val = float(int(val))
                scopes[-1][name] = val
        elif kind == "if":
            cond = st[1]
            inner = scopes + [{}]
            if cond[0] == "condecl":
                self.exec(cond[1], inner, this)
                c = inner[-1][cond[1][1][-1][1]]
                c = c if c is not U else U
            else:
                try:
                    c = self.eval(cond, inner, this)
                except UnknownBranch:
                    c = U
            if c is U:
                self.skipped.append((st[4], "unknown if"))
                return
            if self.truthy(c):
                self.exec(st[2], inner, this)
            elif st[3] is not None:
                self.exec(st[3], inner, this)
        elif kind == "for":
            inner = scopes + [{}]
            self.exec(st[1], inner, this)
            for _ in range(2000):
                c = self.eval(st[2], inner, this)
                if c is U:
                    self.skipped.append((st[5], "unknown loop bound"))
                    break
                if not self.truthy(c):
                    break
                try:
                    self.exec(st[4], inner, this)
                except Break:
                    break
                except Continue:
                    pass
                self.eval(st[3], inner, this)
        elif kind == "forrange":
            seq = self.eval(st[2], scopes, this)
            if not isinstance(seq, list):
                self.skipped.append((st[4], "unknown range"))
                return
            for item in list(seq):
                inner = scopes + [{st[1]: item}]
                try:
                    self.exec(st[3], inner, this)
                except Break:
                    break
                except Continue:
                    pass
        elif kind == "while":
            for _ in range(2000):
                c = self.eval(st[1], scopes, this)
                if c is U or not self.truthy(c):
                    break
                try:
                    self.exec(st[2], scopes + [{}], this)
                except Break:
                    break
                except Continue:
                    pass
        elif kind == "dowhile":
            for _ in range(2000):
                try:
                    self.exec(st[1], scopes + [{}], this)
                except Break:
                    break
                except Continue:
                    pass
                c = self.eval(st[2], scopes, this)
                if c is U or not self.truthy(c):
                    break
        elif kind == "switch":
            v = self.eval(st[1], scopes, this)
            if v is U:
                self.skipped.append((st[3], "unknown switch"))
                return
            items = st[2]
            start = None
            for k, it in enumerate(items):
                if it[0] == "case" and self.eq(self.eval(it[1], scopes, this), v):
                    start = k
                    break
            if start is None:
                start = next((k for k, it in enumerate(items) if it[0] == "default"), None)
            if start is None:
                return
            inner = scopes + [{}]
            try:
                for it in items[start:]:
                    if it[0] in ("case", "default"):
                        continue
                    self.exec(it, inner, this)
            except Break:
                pass
        elif kind == "return":
            raise Return(self.eval(st[1], scopes, this) if st[1] is not None else None)
        elif kind == "break":
            raise Break()
        elif kind == "continue":
            raise Continue()
        elif kind == "skip":
            self.skipped.append((st[1], "parse: " + st[2]))

    def default_value(self, ty, is_array):
        if is_array or ty.startswith("TArray"):
            return []
        base = ty.split("::")[-1]
        if base in ("FVector", "FVector3f"):
            return V()
        if base == "FRotator":
            return R()
        if base in ("float", "double", "int32", "int", "uint8"):
            return 0.0
        if base == "bool":
            return False
        if base in self.src.defaults:
            return self.make_struct(base)
        return U

    def make_struct(self, cls):
        """USTRUCT value with its member defaults (FFTItemPart, FFTShopItemDef, ...)."""
        o = Obj(cls)
        for c in self.src.chain(cls):
            for name, ast in self.src.defaults.get(c, []):
                try:
                    o.members[name] = self.eval(ast, [{}], None)
                except (UnknownBranch, Return, Break, Continue):
                    o.members[name] = U
        return o

    # ------------------------------------------------------------ expressions
    def truthy(self, v):
        if v is U:
            raise UnknownBranch()
        if isinstance(v, (V, R)):
            return True
        return bool(v)

    def eq(self, a, b):
        if a is U or b is U:
            return U
        if isinstance(a, str) and isinstance(b, str):
            return a.split("::")[-1] == b.split("::")[-1]
        if isinstance(a, V) and isinstance(b, V):
            return a.t() == b.t()
        return a == b

    def lookup(self, name, scopes, this):
        for s in reversed(scopes):
            if name in s:
                return s[name]
        if this is not None and name in this.members:
            return this.members[name]
        g = self.file_globals(self.cur_file())
        if name in g:
            return g[name]
        if "::" in name:
            cls, member = name.rsplit("::", 1)
            for dname, ast in self.src.defaults.get(cls, []):
                if dname == member:
                    return self.eval(ast, [{}], None)
        if name in CONSTS:
            return CONSTS[name]()
        if "::" in name:
            head, last = name.rsplit("::", 1)
            if head.startswith("E") or head in ("ECollisionEnabled", "EHTA", "EVRTA"):
                return name
        return U

    def assign(self, target, value, scopes, this):
        if target[0] == "name":
            name = target[1]
            for s in reversed(scopes):
                if name in s:
                    s[name] = value
                    return
            if this is not None:
                this.members[name] = value
        elif target[0] == "member":
            obj = self.eval(target[1], scopes, this)
            if isinstance(obj, Obj):
                obj.members[target[2]] = value
            elif isinstance(obj, V) and target[2] in ("X", "Y", "Z") and num(value):
                setattr(obj, target[2].lower(), float(value))
            elif isinstance(obj, R) and target[2] in ("Pitch", "Yaw", "Roll") and num(value):
                setattr(obj, {"Pitch": "p", "Yaw": "y", "Roll": "r"}[target[2]], float(value))
        elif target[0] == "index":
            obj = self.eval(target[1], scopes, this)
            idx = self.eval(target[2], scopes, this)
            if isinstance(obj, list) and num(idx) and 0 <= int(idx) < len(obj):
                obj[int(idx)] = value

    def eval(self, e, scopes, this):
        k = e[0]
        if k == "num":
            return e[1]
        if k == "str":
            return e[1]
        if k == "name":
            if e[1] == "this":
                return this
            return self.lookup(e[1], scopes, this)
        if k == "tname":
            return ("template", e[1], e[2])
        if k == "list":
            return [self.eval(x, scopes, this) for x in e[1]]
        if k == "lambda":
            return Lambda(e[1], e[2], list(scopes))
        if k == "comma":
            self.eval(e[1], scopes, this)
            return self.eval(e[2], scopes, this)
        if k == "cast":
            return self.eval(e[2], scopes, this)
        if k == "assign":
            op, target, rhs = e[1], e[2], e[3]
            val = self.eval(rhs, scopes, this)
            if op != "=":
                cur = self.eval(target, scopes, this)
                val = self.arith(op[0], cur, val)
            self.assign(target, val, scopes, this)
            return val
        if k == "tern":
            c = self.eval(e[1], scopes, this)
            if c is U:
                return U
            return self.eval(e[2] if self.truthy(c) else e[3], scopes, this)
        if k == "bin":
            op = e[1]
            a = self.eval(e[2], scopes, this)
            if op == "&&":
                if a is not U and not self.truthy(a):
                    return False
                b = self.eval(e[3], scopes, this)
                return U if (a is U or b is U) else bool(self.truthy(b))
            if op == "||":
                if a is not U and self.truthy(a):
                    return True
                b = self.eval(e[3], scopes, this)
                return U if (a is U or b is U) else bool(self.truthy(b))
            b = self.eval(e[3], scopes, this)
            return self.arith(op, a, b)
        if k == "unary":
            op = e[1]
            if op in ("++", "--"):
                cur = self.eval(e[2], scopes, this)
                val = self.arith(op[0], cur, 1.0)
                self.assign(e[2], val, scopes, this)
                return val
            v = self.eval(e[2], scopes, this)
            if op in ("*", "&", "+"):
                return v
            if v is U:
                return U
            if op == "-":
                if isinstance(v, V):
                    return V(-v.x, -v.y, -v.z)
                if isinstance(v, R):
                    return R(-v.p, -v.y, -v.r)
                return -v if num(v) else U
            if op == "!":
                return not self.truthy(v)
            return U
        if k == "postfix":
            cur = self.eval(e[2], scopes, this)
            self.assign(e[2], self.arith(e[1][0], cur, 1.0), scopes, this)
            return cur
        if k == "index":
            obj = self.eval(e[1], scopes, this)
            idx = self.eval(e[2], scopes, this)
            if isinstance(obj, list) and num(idx):
                i = int(idx)
                return obj[i] if 0 <= i < len(obj) else U
            if isinstance(obj, V) and num(idx):
                return obj.t()[int(idx)]
            return U
        if k == "member":
            obj = self.eval(e[1], scopes, this)
            name = e[2]
            if isinstance(obj, V):
                return {"X": obj.x, "Y": obj.y, "Z": obj.z}.get(name, U)
            if isinstance(obj, R):
                return {"Pitch": obj.p, "Yaw": obj.y, "Roll": obj.r}.get(name, U)
            if isinstance(obj, Obj):
                return obj.members.get(name, U)
            return U
        if k == "call":
            return self.call(e, scopes, this)
        return U

    def arith(self, op, a, b):
        if a is U or b is U or a is None or b is None:
            return U
        try:
            if isinstance(a, V) or isinstance(b, V):
                if op in ("==", "!="):
                    r = isinstance(a, V) and isinstance(b, V) and a.t() == b.t()
                    return r if op == "==" else not r
                if isinstance(a, V) and isinstance(b, V):
                    f = {"+": lambda x, y: x + y, "-": lambda x, y: x - y, "*": lambda x, y: x * y, "/": lambda x, y: x / y}[op]
                    return V(f(a.x, b.x), f(a.y, b.y), f(a.z, b.z))
                if isinstance(a, V) and num(b):
                    f = {"*": lambda x: x * b, "/": lambda x: x / b, "+": lambda x: x + b, "-": lambda x: x - b}[op]
                    return V(f(a.x), f(a.y), f(a.z))
                if num(a) and isinstance(b, V) and op == "*":
                    return V(a * b.x, a * b.y, a * b.z)
                return U
            if isinstance(a, R) or isinstance(b, R):
                if isinstance(a, R) and isinstance(b, R):
                    if op == "+":
                        return R(a.p + b.p, a.y + b.y, a.r + b.r)
                    if op == "-":
                        return R(a.p - b.p, a.y - b.y, a.r - b.r)
                if isinstance(a, R) and num(b) and op == "*":
                    return R(a.p * b, a.y * b, a.r * b)
                return U
            if isinstance(a, str) or isinstance(b, str):
                if op == "+":
                    return str(a) + str(b)
                if op in ("==", "!="):
                    r = self.eq(a, b)
                    return r if op == "==" else (U if r is U else not r)
                return U
            if not (num(a) and num(b)):
                if op in ("==", "!="):
                    r = a is b
                    return r if op == "==" else not r
                return U
            if op == "+":
                return a + b
            if op == "-":
                return a - b
            if op == "*":
                return a * b
            if op == "/":
                return a / b if b else U
            if op == "%":
                return float(int(a) % int(b)) if b else U
            if op == "<":
                return a < b
            if op == ">":
                return a > b
            if op == "<=":
                return a <= b
            if op == ">=":
                return a >= b
            if op == "==":
                return a == b
            if op == "!=":
                return a != b
            if op == "&":
                return float(int(a) & int(b))
            if op == "|":
                return float(int(a) | int(b))
            if op == "^":
                return float(int(a) ^ int(b))
            if op == "<<":
                return float(int(a) << int(b))
        except (TypeError, ValueError, ZeroDivisionError, KeyError, OverflowError):
            return U
        return U

    # ------------------------------------------------------------ calls
    def call(self, e, scopes, this):
        callee, args_ast, line = e[1], e[2], e[3]

        def args():
            return [self.eval(a, scopes, this) for a in args_ast]

        if callee[0] == "member":
            obj = self.eval(callee[1], scopes, this)
            return self.method_call(obj, callee[2], args(), scopes, this, line, callee)
        if callee[0] == "tname":
            name, targs = callee[1], callee[2]
            base = name.split("::")[-1]
            a = args()
            if base == "CreateDefaultSubobject" or base == "NewObject":
                cname = a[0] if a and isinstance(a[0], str) else "%s_%d" % (targs[0] if targs else "Comp", len(self.comps))
                c = Comp(targs[0] if targs else "Component", cname, line, list(self.site))
                self.comps.append(c)
                return c
            if base in ("static_cast", "Cast", "CastChecked"):
                return a[0] if a else U
            if base == "Spawn" and targs:
                return self.spawn(targs[0], a, line)
            if base in ("TArray", "TInlineComponentArray"):
                return list(a[0]) if a and isinstance(a[0], list) else []
            return U
        if callee[0] != "name":
            fn = self.eval(callee, scopes, this)
            if isinstance(fn, Lambda):
                return self.call_lambda(fn, args(), this, line)
            return U
        name = callee[1]
        # local lambdas
        fn = None
        for s in reversed(scopes):
            if name in s:
                fn = s[name]
                break
        if isinstance(fn, Lambda):
            return self.call_lambda(fn, args(), this, line)
        base = name.split("::")[-1]
        if name in ("FVector", "FVector3f", "FVector3d", "UE::Math::TVector"):
            a = args()
            if len(a) == 0:
                return V()
            if len(a) == 1:
                if isinstance(a[0], V):
                    return V(*a[0].t())
                return V(a[0], a[0], a[0]) if num(a[0]) else U
            if len(a) >= 3 and all(num(x) for x in a[:3]):
                return V(*a[:3])
            if len(a) == 2 and all(num(x) for x in a):
                return V(a[0], a[1], 0.0)
            return U
        if name in ("FVector2D", "FVector2f", "FIntPoint"):
            a = args()
            if len(a) == 1 and num(a[0]):
                return V(a[0], a[0], 0.0)
            if len(a) >= 2 and all(num(x) for x in a[:2]):
                return V(a[0], a[1], 0.0)
            return V() if not a else U
        if name == "FRotator":
            a = args()
            if len(a) == 0:
                return R()
            if len(a) == 1 and num(a[0]):
                return R(a[0], a[0], a[0])
            if len(a) >= 3 and all(num(x) for x in a[:3]):
                return R(*a[:3])
            return U
        if name in ("MoveTemp", "MoveTempIfPossible", "CopyTemp"):
            a = args()
            return a[0] if a else U
        if name in ("TEXT", "FName", "FString"):
            a = args()
            return a[0] if a else ""
        if name == "FString::Printf":
            a = args()
            if not a or not isinstance(a[0], str) or any(x is U for x in a[1:]):
                return U
            fmt = re.sub(r"%(\d*)d", lambda m: "%" + m.group(1) + "d", a[0].replace("%s", "%s"))
            try:
                vals = [int(x) if num(x) and "%" in fmt else x for x in a[1:]]
                return fmt % tuple(vals)
            except (TypeError, ValueError):
                return U
        if name.startswith("FMath::") or name in ("FMath",):
            return self.fmath(base, args())
        if name in ("FTVis::MakePart", "FTVis::SpawnPart"):
            a = args()
            if name.endswith("MakePart"):
                owner, parent, cname, shape, loc, size = (a + [U] * 6)[:6]
                rest = a[6:]
            else:
                owner, parent, shape, loc, size = (a + [U] * 5)[:5]
                cname = "SpawnPart_%d" % len(self.comps)
                rest = a[5:]
            rot = rest[1] if len(rest) > 1 else R()
            c = Comp("UStaticMeshComponent", cname if isinstance(cname, str) else "Part_%d" % len(self.comps), line, list(self.site))
            c.parent = parent if isinstance(parent, Comp) else None
            c.shape = shape.split("::")[-1] if isinstance(shape, str) else None
            ok = isinstance(loc, V) and isinstance(size, V) and isinstance(rot, R) and c.shape is not None
            if ok:
                c.size = size
                c.loc, c.rot, c.scale = V(*loc.t()), R(*rot.t()), V(size.x / 100, size.y / 100, size.z / 100)
            else:
                c.shape = c.shape or "Unknown"
                c.unknown = True
            self.comps.append(c)
            return c
        if name == "FTVis::ApplyShape":
            a = args()
            if len(a) >= 3 and isinstance(a[0], Comp):
                c = a[0]
                if isinstance(a[1], str) and isinstance(a[2], V):
                    c.shape = a[1].split("::")[-1]
                    c.size = a[2]
                    c.scale = V(a[2].x / 100, a[2].y / 100, a[2].z / 100)
                    c.unknown = False
                else:
                    c.unknown = True
            return None
        if name == "FTVis::GetMesh":
            a = args()
            return ("mesh", a[0].split("::")[-1]) if a and isinstance(a[0], str) else U
        if name == "FTransform":
            a = args()
            rot = next((x for x in a if isinstance(x, R)), R())
            vs = [x for x in a if isinstance(x, V)]
            if any(x is U for x in a) or not vs:
                return U
            return ("xf", rot, vs[0], vs[1] if len(vs) > 1 else V(1, 1, 1))
        if name.startswith("FTVis::") or name.startswith("FTColors::") or name in ("Hex", "FLinearColor", "FColor", "LOCTEXT", "NSLOCTEXT", "FText::FromString", "FText::GetEmpty"):
            args()
            return U
        if name.startswith("Super::"):
            m = self.src.method(self.src.bases.get(this.cls) if this else None, base) if this else None
            if m is not None and base not in ("OnConstruction", "BeginPlay", "Tick"):
                return self.call_method(m, args(), this)
            if m is not None and base == "OnConstruction":
                return self.call_method(m, args(), this)
            return None
        # member function of the actor class, static class function, free function of the file
        if this is not None and "::" not in name:
            m = self.src.method(this.cls, name)
            if m is not None:
                return self.call_method(m, args(), this)
        if "::" in name:
            m = self.src.method(name.rsplit("::", 1)[0], base)
            if m is not None:
                return self.call_method(m, args(), None)
        else:
            m = self.src.free_function(name, self.cur_file())
            if m is not None:
                self.site.append(line)
                try:
                    return self.call_method(m, args(), this)
                finally:
                    self.site.pop()
        # unknown call: evaluate the arguments for their side effects (Next++ etc.)
        try:
            args()
        except UnknownBranch:
            pass
        return U

    def fmath(self, fn, a):
        if any(x is U for x in a):
            return U
        try:
            if fn in ("Sin", "Cos", "Tan", "Sqrt", "Abs", "Exp", "Loge", "Atan", "Asin", "Acos"):
                return {"Sin": math.sin, "Cos": math.cos, "Tan": math.tan, "Sqrt": math.sqrt, "Abs": abs, "Exp": math.exp,
                        "Loge": math.log, "Atan": math.atan, "Asin": math.asin, "Acos": math.acos}[fn](a[0])
            if fn == "Atan2":
                return math.atan2(a[0], a[1])
            if fn == "DegreesToRadians":
                return math.radians(a[0])
            if fn == "RadiansToDegrees":
                return math.degrees(a[0])
            if fn in ("Max", "Min"):
                return (max if fn == "Max" else min)(a[0], a[1])
            if fn == "Clamp":
                return min(max(a[0], a[1]), a[2])
            if fn == "Lerp":
                if isinstance(a[0], V):
                    return V(*(x + (y - x) * a[2] for x, y in zip(a[0].t(), a[1].t())))
                return a[0] + (a[1] - a[0]) * a[2]
            if fn in ("FloorToInt", "FloorToFloat", "Floor"):
                return float(math.floor(a[0]))
            if fn in ("CeilToInt", "CeilToFloat"):
                return float(math.ceil(a[0]))
            if fn in ("RoundToInt", "RoundToFloat"):
                return float(math.floor(a[0] + 0.5))
            if fn == "TruncToInt":
                return float(int(a[0]))
            if fn == "Square":
                return a[0] * a[0]
            if fn == "Pow":
                return a[0] ** a[1]
            if fn == "Fmod":
                return math.fmod(a[0], a[1])
            if fn == "Sign":
                return float((a[0] > 0) - (a[0] < 0))
            if fn in ("IsNearlyEqual",):
                return abs(a[0] - a[1]) < (a[2] if len(a) > 2 else 1e-4)
            if fn == "IsNearlyZero":
                return abs(a[0]) < (a[1] if len(a) > 1 else 1e-4)
        except (TypeError, ValueError, IndexError, AttributeError):
            return U
        return U  # FRand etc.

    def method_call(self, obj, name, a, scopes, this, line, callee):
        if name in ("CreateDefaultSubobject", "CreateOptionalDefaultSubobject"):
            cname = a[0] if a and isinstance(a[0], str) else "Component_%d" % len(self.comps)
            c = Comp("Component", cname, line, list(self.site))
            self.comps.append(c)
            return c
        if isinstance(obj, Comp):
            if name == "SetupAttachment" or name == "AttachToComponent":
                if a and isinstance(a[0], Comp):
                    obj.parent = a[0]
            elif name == "SetRelativeLocation" and a and isinstance(a[0], V):
                obj.loc = V(*a[0].t())
            elif name == "SetRelativeRotation" and a and isinstance(a[0], R):
                obj.rot = R(*a[0].t())
            elif name == "SetRelativeLocationAndRotation" and len(a) >= 2:
                if isinstance(a[0], V):
                    obj.loc = V(*a[0].t())
                if isinstance(a[1], R):
                    obj.rot = R(*a[1].t())
            elif name == "SetRelativeScale3D" and a and isinstance(a[0], V):
                obj.scale = V(*a[0].t())
            elif name == "SetVisibility" and a:
                obj.visible = a[0] if isinstance(a[0], bool) else obj.visible
            elif name == "SetHiddenInGame" and a:
                obj.visible = (not a[0]) if isinstance(a[0], bool) else obj.visible
            elif name == "SetStaticMesh" and a and isinstance(a[0], tuple) and a[0][0] == "mesh":
                obj.ism_shape = a[0][1]
            elif name == "AddInstance" and a and isinstance(a[0], tuple) and a[0][0] == "xf":
                _, rot, loc, scale = a[0]
                idx = sum(1 for c in self.comps if c.parent is obj and c.kind == "Instance")
                c = Comp("Instance", "%s[%d]" % (obj.name, idx), line, list(self.site))
                c.parent = obj
                c.shape = getattr(obj, "ism_shape", None) or "Box"
                c.loc, c.rot, c.scale = V(*loc.t()), R(*rot.t()), V(*scale.t())
                c.size = V(scale.x * 100, scale.y * 100, scale.z * 100)
                self.comps.append(c)
                return float(idx)
            elif name == "ClearInstances":
                self.comps = [c for c in self.comps if not (c.parent is obj and c.kind == "Instance")]
            elif name == "Get":
                return obj
            elif name == "GetRelativeLocation":
                return V(*obj.loc.t())
            elif name == "GetRelativeRotation":
                return R(*obj.rot.t())
            elif name == "GetRelativeScale3D":
                return V(*obj.scale.t())
            return U
        if isinstance(obj, list):
            if name in ("Add", "Emplace", "Push"):
                obj.append(a[0] if a else U)
                return float(len(obj) - 1)
            if name == "Num":
                return float(len(obj))
            if name in ("Reset", "Empty"):
                del obj[:]
                return None
            if name == "IsValidIndex":
                return num(a[0]) and 0 <= int(a[0]) < len(obj)
            if name == "Last":
                return obj[-1] if obj else U
            return U
        if isinstance(obj, V):
            if name in ("GetSafeNormal", "GetUnsafeNormal"):
                n = math.sqrt(obj.x ** 2 + obj.y ** 2 + obj.z ** 2)
                return V(obj.x / n, obj.y / n, obj.z / n) if n > 1e-8 else V()
            if name == "Size":
                return math.sqrt(obj.x ** 2 + obj.y ** 2 + obj.z ** 2)
            if name == "Size2D":
                return math.sqrt(obj.x ** 2 + obj.y ** 2)
            if name == "Rotation":
                return R(math.degrees(math.atan2(obj.z, math.hypot(obj.x, obj.y))), math.degrees(math.atan2(obj.y, obj.x)), 0)
            if name == "GetMin":
                return min(obj.t())
            if name == "GetMax":
                return max(obj.t())
            return U
        if isinstance(obj, R):
            if name == "Vector":
                return rotate(obj, V(1, 0, 0))
            if name == "RotateVector" and a and isinstance(a[0], V):
                return rotate(obj, a[0])
            return U
        if isinstance(obj, Obj):
            m = self.src.method(obj.cls, name)
            if m is not None:
                return self.call_method(m, a, obj)
            return U
        return U

    # ------------------------------------------------------------ map builder
    def spawn(self, cls, a, line):
        loc = a[1] if len(a) > 1 and isinstance(a[1], V) else U
        yaw = a[2] if len(a) > 2 and num(a[2]) else U
        label = a[3] if len(a) > 3 and isinstance(a[3], str) else ""
        proxy = Obj(cls)
        if len(a) > 4 and isinstance(a[4], Lambda):
            saved = self.comps
            self.comps = []
            self.call_lambda(a[4], [proxy], None, line)
            self.comps = saved
        self.spawns.append({"cls": cls, "loc": loc, "yaw": yaw, "label": label, "settings": dict(proxy.members), "line": line})
        return proxy


# ============================================================================ convenience

_SOURCE = None


def source():
    global _SOURCE
    if _SOURCE is None:
        _SOURCE = Source()
    return _SOURCE


def spawns(function="FTBuildStudioMap"):
    """Actor instances placed by the map builder: [{cls, loc(V), yaw, label, settings{member: value}}]."""
    it = Interp(source())
    toks = source().files["Editor/FTMapBuilder.cpp"]
    # free function: find its body
    for i, t in enumerate(toks):
        if t.text == function and toks[i + 1].text == "(" and toks[i - 1].text == "bool":
            close = Parser(toks).match_close(i + 1, "(", ")")
            if toks[close + 1].text != "{":
                continue
            end = Parser(toks).match_close(close + 1, "{", "}")
            body = Parser(toks, close + 2, end).block_body()
            try:
                it.exec(body, [{"W": U}], None)
            except Return:
                pass
            break
    return it.spawns


def run(cls, settings=None):
    it = Interp(source())
    actor, comps = it.run_actor(cls, settings)
    return actor, comps, it.skipped


def find(comps, name):
    for c in comps:
        if c.name == name:
            return c
    return None


def parts_under(comps, anchor=None, exclude=(), only_visible=True):
    """Mesh parts attached (transitively) below `anchor` (None = whole actor), skipping sub-trees rooted at the
    components named in `exclude` (separate meshes)."""
    ex = set(exclude)
    out = []
    for c in comps:
        if not c.is_part() or getattr(c, "unknown", False) or c.size is None:
            continue
        if only_visible and not c.visible:
            continue
        chain = c.chain()
        names = [x.name for x in chain]
        if anchor is not None and anchor.name not in names:
            continue
        # sub-trees between the part and the anchor that are excluded
        upto = names[:names.index(anchor.name)] if anchor is not None else names
        if anchor is not None and c is anchor:
            upto = [c.name]
        if any(n in ex for n in upto) and not (anchor is not None and c is anchor and c.name not in ex):
            continue
        out.append(c)
    return out


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--spawns":
        for s in spawns():
            print(s["cls"], s["loc"], s["yaw"], s["label"], s["settings"])
        sys.exit(0)
    cls = sys.argv[1] if len(sys.argv) > 1 else "AFTStageLight"
    actor, comps, skipped = run(cls)
    for c in comps:
        par = c.parent.name if isinstance(c.parent, Comp) else "-"
        if c.is_part():
            print("%-22s <- %-14s %-9s loc%s rot%s size%s vis=%s L%d" % (c.name, par, c.shape, c.loc, c.rot, c.size, c.visible, c.line))
        else:
            print("%-22s <- %-14s [%s] loc%s rot%s" % (c.name, par, c.kind, c.loc, c.rot))
    for s in skipped:
        print("skipped", s)
