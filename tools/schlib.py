"""
schlib.py -- a tiny schematic DSL that renders clean SVG for guitar-pedal circuits.

Coordinates: SVG (y grows downward), grid unit = 10.  Every symbol is defined in
local coordinates with its two "main" pins on the local x axis (a at -25,
b at +25) and is then rotated 0/90/180/270 degrees about its centre.

Authoring example
-----------------
    from schlib import Sch

    def draw(s):
        R1 = s.r('R1', '10K', 120, 200)
        C1 = s.c('C1', '2.2u', 120, 120, rot=90)
        Q1 = s.npn('Q1', '2N3904', 300, 200)
        s.wire(C1.b, Q1.b)
        s.route(R1.b, Q1.c)
        s.gnd(R1.a)
        s.vcc(R1.b, '9V')
        s.port(60, 200, 'IN', align='end')

Pins: .a/.b (two terminal parts), .b/.c/.e (BJT), .g/.d/.s (JFET),
.inp/.inn/.out/.vp/.vn (opamp), .a/.b/.w (potentiometer), .p (power symbols).
`s.lint()` reports dangling pins / open wire ends / duplicate refs and
`s.bom()` groups the components into a bill of materials.
"""

from __future__ import annotations

import math
import os
from collections import defaultdict

INK = "#1b1b1b"
SANS = "'Inter','Helvetica Neue','Microsoft YaHei',Arial,'Noto Sans SC',sans-serif"
MONO = "'JetBrains Mono','SFMono-Regular',Consolas,'Noto Sans Mono',monospace"
PAD = 26


# --------------------------------------------------------------------------- #
# helpers
# --------------------------------------------------------------------------- #
def _r(v, n=2):
    s = f"{v:.{n}f}"
    if "." in s:
        s = s.rstrip("0").rstrip(".")
    return s


def _poly(pts):
    return "M" + "L".join(f"{_r(x)} {_r(y)}" for x, y in pts) + "Z"


def xml_esc(t):
    return (str(t).replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def est_width(txt, size, mono=False):
    if not txt:
        return 0
    w = 0.0
    for ch in txt:
        if ord(ch) > 0x2A00:      # CJK & full width
            w += 1.0
        elif mono:
            w += 0.6
        else:
            w += 0.55
    return w * size


def _on_seg(p, a, b, tol=0.4):
    if abs(a[0] - b[0]) < tol:
        return (abs(p[0] - a[0]) < tol
                and min(a[1], b[1]) - tol <= p[1] <= max(a[1], b[1]) + tol)
    if abs(a[1] - b[1]) < tol:
        return (abs(p[1] - a[1]) < tol
                and min(a[0], b[0]) - tol <= p[0] <= max(a[0], b[0]) + tol)
    return False


class Pin:
    """A terminal: global position plus the direction that leads away from body."""

    __slots__ = ("comp", "name", "x", "y", "dx", "dy")

    def __init__(self, comp, name, x, y, dx, dy):
        self.comp = comp
        self.name = name
        self.x = round(x, 3)
        self.y = round(y, 3)
        self.dx = round(dx, 3)
        self.dy = round(dy, 3)

    @property
    def pt(self):
        return (self.x, self.y)

    def out(self, dist=10):
        return (round(self.x + self.dx * dist, 3), round(self.y + self.dy * dist, 3))

    def __getitem__(self, i):
        return (self.x, self.y)[i]

    def __iter__(self):
        return iter((self.x, self.y))

    def __repr__(self):
        return f"<Pin {self.comp.ref}.{self.name} ({self.x},{self.y})>"


# --------------------------------------------------------------------------- #
# symbol library : returns (paths, texts, pins, bbox)
#   paths : [(d, cls)]  cls in sym | symf | symthin
#   texts : [(x, y, txt, cls, anchor, size)]
#   pins  : {name: ((u, v), (du, dv))}
# --------------------------------------------------------------------------- #
def sym_resistor():
    d = "M-25 0H-18L-15 6.5L-9 -6.5L-3 6.5L3 -6.5L9 6.5L15 -6.5L18 0H25"
    return [(d, "sym")], [], {"a": ((-25, 0), (-1, 0)), "b": ((25, 0), (1, 0))}, \
        (-25, -7, 25, 7)


def sym_cap():
    return ([("M-25 0H-6", "sym"), ("M6 0H25", "sym"),
             ("M-6 -11V11", "sym"), ("M6 -11V11", "sym")], [],
            {"a": ((-25, 0), (-1, 0)), "b": ((25, 0), (1, 0))}, (-25, -11, 25, 11))


def sym_cap_pol():
    return ([("M-25 0H-6", "sym"), ("M6 0H25", "sym"),
             ("M-6 -11V11", "sym"), ("M6 -11Q10.5 0 6 11", "sym"),
             ("M-16 -12H-9M-12.5 -15.5V-8.5", "symthin")], [],
            {"a": ((-25, 0), (-1, 0)), "b": ((25, 0), (1, 0))}, (-25, -16, 25, 11))


def sym_inductor():
    d = ("M-25 0H-18a4.5 4.5 0 0 1 9 0a4.5 4.5 0 0 1 9 0"
         "a4.5 4.5 0 0 1 9 0a4.5 4.5 0 0 1 9 0H25")
    return [(d, "sym")], [], {"a": ((-25, 0), (-1, 0)), "b": ((25, 0), (1, 0))}, \
        (-25, -9, 25, 3)


def sym_diode():
    return ([("M-25 0H-9", "sym"), ("M-9 -9L7 0L-9 9Z", "symf"),
             ("M7 -9V9", "sym"), ("M7 0H25", "sym")], [],
            {"a": ((-25, 0), (-1, 0)), "k": ((25, 0), (1, 0))}, (-25, -9, 25, 9))


def sym_zener():
    return ([("M-25 0H-9", "sym"), ("M-9 -9L7 0L-9 9Z", "symf"),
             ("M7 -9V9", "sym"), ("M7 -9L4.5 -13", "sym"), ("M7 9L9.5 13", "sym"),
             ("M7 0H25", "sym")], [],
            {"a": ((-25, 0), (-1, 0)), "k": ((25, 0), (1, 0))}, (-25, -13, 25, 13))


def sym_led():
    return ([("M-25 0H-9", "sym"), ("M-9 -9L7 0L-9 9Z", "symf"),
             ("M7 -9V9", "sym"), ("M7 0H25", "sym"),
             ("M-3 -12L3 -18", "symthin"), ("M3 -18L-1 -17M3 -18L2 -14", "symthin"),
             ("M6 -10L12 -16", "symthin"), ("M12 -16L8 -15M12 -16L11 -12", "symthin")],
            [], {"a": ((-25, 0), (-1, 0)), "k": ((25, 0), (1, 0))}, (-25, -20, 25, 9))


def _bjt(is_npn=True):
    paths = [
        ("M-17 0a17 17 0 1 0 34 0a17 17 0 1 0-34 0", "sym"),
        ("M-25 0H-7", "sym"),
        ("M-7 -9V9", "sym"),
        ("M-7 -4L12 -13V-25", "sym"),
        ("M-7 4L12 13V25", "sym"),
    ]
    p = (1.5, 7.6)
    d = (0.9, 0.44)
    perp = (-0.44, 0.9)
    if is_npn:
        tip = (p[0] + 5.5 * d[0], p[1] + 5.5 * d[1])
        b1 = (p[0] - 3.0 * d[0] + 3.4 * perp[0], p[1] - 3.0 * d[1] + 3.4 * perp[1])
        b2 = (p[0] - 3.0 * d[0] - 3.4 * perp[0], p[1] - 3.0 * d[1] - 3.4 * perp[1])
    else:
        tip = (p[0] - 5.5 * d[0], p[1] - 5.5 * d[1])
        b1 = (p[0] + 3.0 * d[0] + 3.4 * perp[0], p[1] + 3.0 * d[1] + 3.4 * perp[1])
        b2 = (p[0] + 3.0 * d[0] - 3.4 * perp[0], p[1] + 3.0 * d[1] - 3.4 * perp[1])
    paths.append((_poly([tip, b1, b2]), "symf"))
    pins = {"b": ((-25, 0), (-1, 0)), "c": ((12, -25), (0, -1)),
            "e": ((12, 25), (0, 1))}
    return paths, [], pins, (-25, -25, 29, 17)


def sym_npn():
    return _bjt(True)


def sym_pnp():
    return _bjt(False)


def sym_jfet():
    paths = [
        ("M-25 0H-6", "sym"),
        ("M-6 -11V11", "sym"),
        ("M-3 -11V-7M-3 -3V3M-3 7V11", "sym"),
        ("M-3 -8L12 -14V-25", "sym"),
        ("M-3 8L12 14V25", "sym"),
        ("M-8.5 -2.6L-3.2 0L-8.5 2.6", "symf"),
    ]
    pins = {"g": ((-25, 0), (-1, 0)), "d": ((12, -25), (0, -1)),
            "s": ((12, 25), (0, 1))}
    return paths, [], pins, (-25, -25, 29, 17)


def sym_opamp():
    paths = [
        ("M-38 -30L42 0L-38 30Z", "sym"),
        ("M-52 -16H-38", "sym"), ("M-52 16H-38", "sym"),
        ("M42 0H56", "sym"),
        ("M0 -15.75V-52", "sym"), ("M0 15.75V52", "sym"),
    ]
    texts = [(-43, 20, "+", "symtxt", "middle", 13),
             (-43, -13, "\u2212", "symtxt", "middle", 13)]
    pins = {"inn": ((-52, -16), (-1, 0)), "inp": ((-52, 16), (-1, 0)),
            "out": ((56, 0), (1, 0)), "vp": ((0, -52), (0, -1)),
            "vn": ((0, 52), (0, 1))}
    return paths, texts, pins, (-52, -52, 56, 52)


def sym_opamp_bare():
    """Op-amp without the power pins, for audio-path-only drawings."""
    paths = [
        ("M-38 -30L42 0L-38 30Z", "sym"),
        ("M-52 -16H-38", "sym"), ("M-52 16H-38", "sym"),
        ("M42 0H56", "sym"),
    ]
    texts = [(-43, 20, "+", "symtxt", "middle", 13),
             (-43, -13, "\u2212", "symtxt", "middle", 13)]
    pins = {"inn": ((-52, -16), (-1, 0)), "inp": ((-52, 16), (-1, 0)),
            "out": ((56, 0), (1, 0))}
    return paths, texts, pins, (-52, -30, 56, 30)


def sym_pot():
    zig = "M-25 0H-18L-15 6.5L-9 -6.5L-3 6.5L3 -6.5L9 6.5L15 -6.5L18 0H25"
    paths = [(zig, "sym"), ("M0 -25V-12", "sym"),
             ("M0 -7.5L-3.6 -14H3.6Z", "symf")]
    pins = {"a": ((-25, 0), (-1, 0)), "b": ((25, 0), (1, 0)),
            "w": ((0, -25), (0, -1))}
    return paths, [], pins, (-25, -25, 25, 7)


def sym_switch():
    paths = [("M-25 0H-17", "sym"), ("M17 0H25", "sym"),
             ("M-17 0a4.2 4.2 0 1 0 8.4 0a4.2 4.2 0 1 0-8.4 0", "symf"),
             ("M8.6 0a4.2 4.2 0 1 0 8.4 0a4.2 4.2 0 1 0-8.4 0", "symf"),
             ("M-9 -2L12 -13", "sym")]
    pins = {"a": ((-25, 0), (-1, 0)), "b": ((25, 0), (1, 0))}
    return paths, [], pins, (-25, -14, 25, 5)


def sym_gnd():
    return ([("M0 0V7", "sym"), ("M-12 7H12", "sym"),
             ("M-8 11H8", "sym"), ("M-4 15H4", "sym")], [],
            {"p": ((0, 0), (0, -1))}, (-12, 0, 12, 15))


def sym_vcc():
    return ([("M0 0V-9", "sym"), ("M-9 -9H9", "sym")], [],
            {"p": ((0, 0), (0, 1))}, (-9, -9, 9, 0))


def sym_port():
    return ([("M0 -5a5 5 0 1 0 0.1 0Z", "sym")], [],
            {"p": ((0, 0), (0, 0))}, (-5, -5, 5, 5))


SYMBOLS = {
    "r": sym_resistor, "c": sym_cap, "cp": sym_cap_pol, "l": sym_inductor,
    "d": sym_diode, "zener": sym_zener, "led": sym_led,
    "npn": sym_npn, "pnp": sym_pnp, "jfet": sym_jfet,
    "opamp": sym_opamp, "pot": sym_pot, "sw": sym_switch,
    "gnd": sym_gnd, "vcc": sym_vcc, "port": sym_port,
}

KIND_CLASS = {"r": "电阻", "c": "电容", "cp": "电解电容", "l": "电感",
              "d": "二极管", "zener": "稳压二极管", "led": "LED",
              "npn": "晶体管", "pnp": "晶体管", "jfet": "场效应管",
              "opamp": "运放", "pot": "电位器", "sw": "开关",
              "ic": "集成电路", "box": "模块"}

DEFAULT_LPOS = {"pot": "right", "opamp": "none", "ic": "none", "box": "none",
                "npn": "farleft", "pnp": "farleft", "jfet": "farleft"}


# --------------------------------------------------------------------------- #
class Comp:
    def __init__(self, sch, kind, ref, value, x, y, rot, paths, texts,
                 pins_local, bbox, lpos, ldx, ldy, note, fx=1, fy=1):
        self.sch = sch
        self.kind = kind
        self.ref = ref
        self.value = value
        self.x = round(x, 3)
        self.y = round(y, 3)
        self.rot = rot % 360
        self.paths = paths
        self.texts = texts
        self.bbox = bbox
        self.note = note
        self.fx, self.fy = fx, fy
        self.lpos = lpos or DEFAULT_LPOS.get(kind, "bottom")
        th = math.radians(self.rot)
        self._c, self._s = math.cos(th), math.sin(th)
        self.pins = {}
        for name, ((u, v), (du, dv)) in pins_local.items():
            gx, gy = self._g(u, v)
            gdx, gdy = self._gdir(du, dv)
            p = Pin(self, name, gx, gy, gdx, gdy)
            self.pins[name] = p
            setattr(self, name, p)
        self.ldx, self.ldy = ldx, ldy

    def _g(self, u, v):
        u, v = u * self.fx, v * self.fy
        return (self.x + u * self._c - v * self._s,
                self.y + u * self._s + v * self._c)

    def _gdir(self, du, dv):
        du, dv = du * self.fx, dv * self.fy
        return (du * self._c - dv * self._s, du * self._s + dv * self._c)

    def bbox_global(self):
        x0, y0, x1, y1 = self.bbox
        pts = []
        for (u, v) in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
            pts.append(self._g(u, v))
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        return (min(xs), min(ys), max(xs), max(ys))

    def kind_class(self):
        return KIND_CLASS.get(self.kind, "其他")

    def _label_xy(self):
        L = self.lpos
        if L == "none":
            return None
        vertical = self.rot in (90, 270)
        if vertical:
            if L == "left":
                return (self.x - 13, self.y - 3 + self.ldy, self.x - 13,
                        self.y + 10 + self.ldy, "end")
            return (self.x + 13, self.y - 3 + self.ldy, self.x + 13,
                    self.y + 10 + self.ldy, "start")
        if L == "top":
            return (self.x, self.y - 17 + self.ldy, self.x,
                    self.y - 31 + self.ldy, "middle")
        if L == "left":
            return (self.x - 13, self.y - 4 + self.ldy, self.x - 13,
                    self.y + 9 + self.ldy, "end")
        if L == "right":
            return (self.x + 13, self.y - 4 + self.ldy, self.x + 13,
                    self.y + 9 + self.ldy, "start")
        if L == "farleft":
            return (self.x - 30, self.y - 6 + self.ldy, self.x - 30,
                    self.y + 14 + self.ldy, "end")
        if L == "farright":
            return (self.x + 30, self.y - 6 + self.ldy, self.x + 30,
                    self.y + 14 + self.ldy, "start")
        # bottom (default)
        return (self.x, self.y - 15 + self.ldy, self.x,
                self.y + 19 + self.ldy, "middle")

    def svg(self):
        sc = ("" if (self.fx, self.fy) == (1, 1)
              else f" scale({self.fx} {self.fy})")
        out = [f'<g transform="translate({_r(self.x)} {_r(self.y)}) '
               f'rotate({self.rot}){sc}">']
        for (d, cls) in self.paths:
            out.append(f'<path class="{cls}" d="{d}"/>')
        for (tx, ty, txt, cls, anchor, size) in self.texts:
            out.append(f'<text class="t-{cls}" x="{_r(tx)}" y="{_r(ty)}" '
                       f'text-anchor="{anchor}" font-size="{_r(size)}">{xml_esc(txt)}</text>')
        out.append("</g>")
        lab = self._label_xy()
        if lab:
            rx, ry, vx, vy, anchor = lab
            out.append(f'<text class="t-ref" x="{_r(rx+self.ldx)}" y="{_r(ry)}" '
                       f'text-anchor="{anchor}" font-size="11">{xml_esc(self.ref)}</text>')
            if self.value:
                out.append(f'<text class="t-val" x="{_r(vx+self.ldx)}" y="{_r(vy)}" '
                           f'text-anchor="{anchor}" font-size="9.8">{xml_esc(self.value)}</text>')
        return "".join(out)


# --------------------------------------------------------------------------- #
class Sch:
    def __init__(self, title="", pad=PAD):
        self.comps = []
        self.wires = []
        self.labels = []
        self.dots = []
        self.pad = pad
        self.title = title

    # -- component factories ---------------------------------------------- #
    def _add(self, kind, ref, value, x, y, rot=0, lpos=None, ldx=0, ldy=0,
             note=None, fn=None, fx=1, fy=1):
        fn = fn or SYMBOLS[kind]
        paths, texts, pins_local, bbox = fn()
        c = Comp(self, kind, ref, value, x, y, rot, paths, texts,
                 pins_local, bbox, lpos, ldx, ldy, note, fx, fy)
        self.comps.append(c)
        return c

    def r(self, ref, value, x, y, rot=0, **kw):
        return self._add("r", ref, value, x, y, rot, **kw)

    def c(self, ref, value, x, y, rot=0, **kw):
        return self._add("c", ref, value, x, y, rot, **kw)

    def cp(self, ref, value, x, y, rot=0, **kw):
        return self._add("cp", ref, value, x, y, rot, **kw)

    def l(self, ref, value, x, y, rot=0, **kw):
        return self._add("l", ref, value, x, y, rot, **kw)

    def d(self, ref, value, x, y, rot=0, **kw):
        return self._add("d", ref, value, x, y, rot, **kw)

    def zener(self, ref, value, x, y, rot=0, **kw):
        return self._add("zener", ref, value, x, y, rot, **kw)

    def led(self, ref, value, x, y, rot=0, **kw):
        return self._add("led", ref, value, x, y, rot, **kw)

    def npn(self, ref, value, x, y, rot=0, **kw):
        return self._add("npn", ref, value, x, y, rot, **kw)

    def pnp(self, ref, value, x, y, rot=0, **kw):
        return self._add("pnp", ref, value, x, y, rot, **kw)

    def jfet(self, ref, value, x, y, rot=0, **kw):
        return self._add("jfet", ref, value, x, y, rot, **kw)

    def opamp(self, ref, value, x, y, rot=0, no_power=False, **kw):
        fn = sym_opamp_bare if no_power else sym_opamp
        return self._add("opamp", ref, value, x, y, rot, fn=fn, **kw)

    def pot(self, ref, value, x, y, rot=0, **kw):
        return self._add("pot", ref, value, x, y, rot, **kw)

    def sw(self, ref, value, x, y, rot=0, **kw):
        return self._add("sw", ref, value, x, y, rot, **kw)

    def gnd(self, p, rot=0):
        x, y = self._pt(p)
        return self._add("gnd", "", "", x, y, rot, lpos="none")

    def vcc(self, p, label="9V", rot=0):
        x, y = self._pt(p)
        c = self._add("vcc", "", "", x, y, rot, lpos="none")
        ly = y - 15 if rot % 360 == 0 else y + 24
        self.labels.append((x, ly, label, "middle", "vcc", 10.5))
        return c

    def port(self, x, y, name, align="start"):
        c = self._add("port", "", "", x, y, 0, lpos="none")
        if align == "start":
            self.labels.append((x + 11, y + 3.5, name, "start", "port", 10.5))
        elif align == "end":
            self.labels.append((x - 11, y + 3.5, name, "end", "port", 10.5))
        else:
            self.labels.append((x, y - 12, name, "middle", "port", 10.5))
        return c

    def ic(self, ref, value, x, y, w=80, h=90, left=(), right=(), rot=0):
        def fn():
            paths = [(f"M{-w/2} {-h/2}H{w/2}V{h/2}H{-w/2}Z", "sym")]
            texts = [(-w / 2 + 7, -h / 2 + 15, ref, "ref", "start", 11),
                     (-w / 2 + 7, -h / 2 + 28, value, "val", "start", 9)]
            pins = {}
            if left:
                for i, name in enumerate(left):
                    yy = -h / 2 + (h / (len(left) + 1)) * (i + 1)
                    pins[name] = ((-w / 2 - 14, yy), (-1, 0))
                    paths.append((f"M{-w/2-14} {_r(yy)}H{-w/2}", "sym"))
                    texts.append((-w / 2 + 5, yy + 3.5, name, "pintxt", "start", 8))
            if right:
                for i, name in enumerate(right):
                    yy = -h / 2 + (h / (len(right) + 1)) * (i + 1)
                    pins[name] = ((w / 2 + 14, yy), (1, 0))
                    paths.append((f"M{w/2} {_r(yy)}H{w/2+14}", "sym"))
                    texts.append((w / 2 - 5, yy + 3.5, name, "pintxt", "end", 8))
            return paths, texts, pins, (-w / 2 - 14, -h / 2, w / 2 + 14, h / 2)
        return self._add("ic", ref, value, x, y, rot, lpos="none", fn=fn)

    def box(self, ref, value, x, y, w=90, h=60, left=(), right=()):
        def fn():
            paths = [(f"M{-w/2} {-h/2}H{w/2}V{h/2}H{-w/2}Z", "sym")]
            texts = [(-w / 2 + 7, -h / 2 + 16, ref, "ref", "start", 11),
                     (-w / 2 + 7, -h / 2 + 30, value, "val", "start", 9)]
            pins = {}
            if left:
                for i, name in enumerate(left):
                    yy = -h / 2 + (h / (len(left) + 1)) * (i + 1)
                    pins[name] = ((-w / 2 - 12, yy), (-1, 0))
                    paths.append((f"M{-w/2-12} {_r(yy)}H{-w/2}", "sym"))
                    texts.append((-w / 2 + 6, yy + 3.5, name, "pintxt", "start", 8))
            if right:
                for i, name in enumerate(right):
                    yy = -h / 2 + (h / (len(right) + 1)) * (i + 1)
                    pins[name] = ((w / 2 + 12, yy), (1, 0))
                    paths.append((f"M{w/2} {_r(yy)}H{w/2+12}", "sym"))
                    texts.append((w / 2 - 6, yy + 3.5, name, "pintxt", "end", 8))
            return paths, texts, pins, (-w / 2 - 12, -h / 2, w / 2 + 12, h / 2)
        return self._add("box", ref, value, x, y, 0, lpos="none", fn=fn)

    # -- wiring ------------------------------------------------------------ #
    @staticmethod
    def _pt(p):
        if isinstance(p, Pin):
            return p.pt
        return (round(p[0], 3), round(p[1], 3))

    def wire(self, *pts):
        flat = [self._pt(p) for p in pts]
        self.wires.append(flat)
        return flat

    def route(self, a, b, stub=10, mode="auto"):
        """Orthogonal auto-route, exiting each pin along its own direction."""
        pa, da = (a.pt, (a.dx, a.dy)) if isinstance(a, Pin) else (self._pt(a), (0, 0))
        pb, db = (b.pt, (b.dx, b.dy)) if isinstance(b, Pin) else (self._pt(b), (0, 0))
        if abs(pa[0] - pb[0]) < 0.01 or abs(pa[1] - pb[1]) < 0.01:
            self.wire(pa, pb)
            return [pa, pb]
        pts = [pa]
        if da != (0, 0):
            pts.append((pa[0] + da[0] * stub, pa[1] + da[1] * stub))
        end = (pb[0] + db[0] * stub, pb[1] + db[1] * stub) if db != (0, 0) else pb
        c = pts[-1]
        dx, dy = end[0] - c[0], end[1] - c[1]
        mid = (end[0], c[1]) if (mode == "hv" or (mode == "auto" and abs(dx) >= abs(dy))) \
            else (c[0], end[1])
        pts += [mid, end, pb]
        out = []
        for p in pts:
            if not out or abs(p[0] - out[-1][0]) > 0.01 or abs(p[1] - out[-1][1]) > 0.01:
                out.append(p)
        self.wire(*out)
        return out

    def junction(self, *pts):
        for p in pts:
            self.dots.append(self._pt(p))

    def text(self, x, y, txt, size=10, anchor="middle", cls="note"):
        self.labels.append((x, y, txt, anchor, cls, size))

    def netlabel(self, p, name, align="start", dx=4, dy=0):
        x, y = self._pt(p)
        if align == "start":
            self.labels.append((x + dx, y + 3.5 + dy, name, "start", "net", 9.5))
        elif align == "end":
            self.labels.append((x - dx, y + 3.5 + dy, name, "end", "net", 9.5))
        else:
            self.labels.append((x, y - 7 + dy, name, "middle", "net", 9.5))

    # -- bounds / render --------------------------------------------------- #
    def _bounds(self):
        xs, ys = [], []
        for c in self.comps:
            b = c.bbox_global()
            xs += [b[0], b[2]]
            ys += [b[1], b[3]]
        for w in self.wires:
            for x, y in w:
                xs.append(x)
                ys.append(y)
        for (x, y, txt, anchor, cls, size) in self.labels:
            w = est_width(txt, size, mono=cls in ("val", "net", "vcc", "symtxt", "pintxt"))
            if anchor == "middle":
                xs += [x - w / 2, x + w / 2]
            elif anchor == "start":
                xs.append(x + w)
            else:
                xs.append(x - w)
            ys += [y - size * 0.85, y + size * 0.3]
        if not xs:
            return (0, 0, 100, 100)
        return (min(xs), min(ys), max(xs), max(ys))

    def svg_size(self, pad=None):
        pad = self.pad if pad is None else pad
        x0, y0, x1, y1 = self._bounds()
        vx = math.floor((x0 - pad) / 10) * 10
        vy = math.floor((y0 - pad) / 10) * 10
        vw = math.ceil((x1 + pad - vx) / 10) * 10
        vh = math.ceil((y1 + pad - vy) / 10) * 10
        return vw, vh

    def svg(self, pad=None):
        pad = self.pad if pad is None else pad
        x0, y0, x1, y1 = self._bounds()
        vx = math.floor((x0 - pad) / 10) * 10
        vy = math.floor((y0 - pad) / 10) * 10
        vw = math.ceil((x1 + pad - vx) / 10) * 10
        vh = math.ceil((y1 + pad - vy) / 10) * 10
        body = []
        for w in self.wires:
            d = "M" + "L".join(f"{_r(x)} {_r(y)}" for x, y in w)
            body.append(f'<path class="w" d="{d}"/>')
        for (x, y) in self._junctions():
            body.append(f'<circle class="j" cx="{_r(x)}" cy="{_r(y)}" r="2.7"/>')
        for c in self.comps:
            body.append(c.svg())
        for (x, y, txt, anchor, cls, size) in self.labels:
            fam = MONO if cls in ("val", "net", "vcc", "symtxt", "pintxt") else SANS
            body.append(f'<text class="t-{cls}" x="{_r(x)}" y="{_r(y)}" '
                        f'text-anchor="{anchor}" font-size="{_r(size)}" '
                        f'font-family="{fam}">{xml_esc(txt)}</text>')
        css = ("text{fill:" + INK + ";}"
               ".t-ref{font-weight:700;letter-spacing:.02em}"
               ".t-val{font-weight:500}"
               ".t-note{fill:#666}"
               ".t-net{fill:#0d6b58;font-weight:600}"
               ".t-vcc{fill:#9a3412;font-weight:700}"
               ".t-port{font-weight:700;letter-spacing:.05em}"
               ".t-symtxt{font-weight:600}"
               ".t-pintxt{fill:#777}"
               ".w{fill:none;stroke:" + INK + ";stroke-width:1.45;stroke-linecap:round;stroke-linejoin:round}"
               ".sym{fill:none;stroke:" + INK + ";stroke-width:1.7;stroke-linecap:round;stroke-linejoin:round}"
               ".symthin{fill:none;stroke:" + INK + ";stroke-width:1.2;stroke-linecap:round;stroke-linejoin:round}"
               ".symf{fill:" + INK + ";stroke:" + INK + ";stroke-width:1.1;stroke-linejoin:round}"
               ".j{fill:" + INK + "}")
        head = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{vx} {vy} {vw} {vh}" '
                f'width="{vw}" height="{vh}" role="img" '
                f'aria-label="{xml_esc(self.title or "schematic")}">')
        return head + "<style>" + css + "</style>" + "".join(body) + "</svg>"

    def render(self, path):
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            f.write(self.svg())
        return path

    # -- connectivity ------------------------------------------------------ #
    def _segments(self):
        segs = []
        for w in self.wires:
            for i in range(len(w) - 1):
                if w[i] != w[i + 1]:
                    segs.append((w[i], w[i + 1]))
        return segs

    def _junctions(self):
        cnt = defaultdict(int)
        for w in self.wires:
            for p in w:
                cnt[(round(p[0], 2), round(p[1], 2))] += 1
        for c in self.comps:
            for pin in c.pins.values():
                cnt[(round(pin.x, 2), round(pin.y, 2))] += 1
        keys = list(cnt.keys())
        for (p1, p2) in self._segments():
            for q in keys:
                qr = (round(p1[0], 2), round(p1[1], 2))
                if q in (qr, (round(p2[0], 2), round(p2[1], 2))):
                    continue
                if _on_seg(q, p1, p2):
                    cnt[q] += 1
        out = [p for p, n in cnt.items() if n >= 3]
        for d in self.dots:
            dd = (round(d[0], 2), round(d[1], 2))
            if dd not in out:
                out.append(dd)
        return out

    def nets(self):
        pts, idx = [], {}

        def pid(p):
            k = (round(p[0], 1), round(p[1], 1))
            if k not in idx:
                idx[k] = len(pts)
                pts.append(k)
            return idx[k]

        # every pin is a vertex in its own right (so pins tapping a wire count)
        for c in self.comps:
            for pin in c.pins.values():
                pid(pin.pt)
        segs = []
        for (a, b) in self._segments():
            segs.append((pid(a), pid(b)))
        allv = list(idx.values())
        extra, split = [], []
        for (a, b) in segs:
            on = [(i, pts[i]) for i in allv
                  if i not in (a, b) and _on_seg(pts[i], pts[a], pts[b])]
            if not on:
                continue
            split.append((a, b))

            def dist(item):
                p = item[1]
                return (p[0] - pts[a][0]) ** 2 + (p[1] - pts[a][1]) ** 2

            on.sort(key=dist)
            chain = [a] + [i for i, _ in on] + [b]
            for i in range(len(chain) - 1):
                extra.append((chain[i], chain[i + 1]))
        split_set = set(split)
        segs = extra + [s for s in segs if s not in split_set]
        parent = list(range(len(pts)))

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x

        for a, b in segs:
            ra, rb = find(a), find(b)
            if ra != rb:
                parent[rb] = ra
        groups = defaultdict(list)
        for i in range(len(pts)):
            groups[find(i)].append(i)
        pins_of = defaultdict(list)
        dangling = []
        for c in self.comps:
            for name, pin in c.pins.items():
                k = (round(pin.x, 1), round(pin.y, 1))
                if k in idx:
                    pins_of[find(idx[k])].append(f"{c.ref}.{name}")
                else:
                    nm = c.ref or ("<" + c.kind + ">")
                    dangling.append(f"{nm}.{name}")
        return {"groups": {k: [pts[i] for i in v] for k, v in groups.items()},
                "pins_of": pins_of, "dangling": dangling}

    # -- layout QA --------------------------------------------------------- #
    def label_boxes(self):
        """Approximate bounding boxes for every piece of text on the sheet."""
        boxes = []
        for c in self.comps:
            lab = c._label_xy()
            if not lab:
                continue
            rx, ry, vx, vy, anchor = lab
            items = [(c.ref, rx + c.ldx, ry, 11.0)]
            if c.value:
                items.append((c.value, vx + c.ldx, vy, 9.8))
            for txt, x, y, size in items:
                if not txt:
                    continue
                w = est_width(txt, size)
                if anchor == "start":
                    x0 = x
                elif anchor == "end":
                    x0 = x - w
                else:
                    x0 = x - w / 2
                boxes.append((x0, y - 0.80 * size, x0 + w, y + 0.24 * size,
                              txt, "comp"))
        for (x, y, txt, anchor, cls, size) in self.labels:
            w = est_width(txt, size)
            if anchor == "start":
                x0 = x
            elif anchor == "end":
                x0 = x - w
            else:
                x0 = x - w / 2
            boxes.append((x0, y - 0.80 * size, x0 + w, y + 0.24 * size,
                          txt, cls))
        return boxes

    def layout_lint(self, pad=0.6):
        msgs = []
        boxes = self.label_boxes()
        for i in range(len(boxes)):
            a = boxes[i]
            for j in range(i + 1, len(boxes)):
                b = boxes[j]
                ox = min(a[2], b[2]) - max(a[0], b[0])
                oy = min(a[3], b[3]) - max(a[1], b[1])
                if ox > pad and oy > pad:
                    msgs.append(f"TEXT OVERLAP: {a[4]!r} / {b[4]!r}")
        for (x0, y0, x1, y1, txt, cls) in boxes:
            for (a, b) in self._segments():
                if abs(a[0] - b[0]) < 0.4:                      # vertical
                    hit = (x0 - pad <= a[0] <= x1 + pad
                           and min(a[1], b[1]) < y1 and max(a[1], b[1]) > y0)
                else:                                            # horizontal
                    hit = (y0 - pad <= a[1] <= y1 + pad
                           and min(a[0], b[0]) < x1 and max(a[0], b[0]) > x0)
                if hit:
                    msgs.append(f"TEXT/WIRE: {txt!r} crosses wire "
                                f"({a[0]:.0f},{a[1]:.0f})-({b[0]:.0f},{b[1]:.0f})")
                    break
        b = self._bounds()
        for (x0, y0, x1, y1, txt, cls) in boxes:
            if x0 < b[0] - 2 or y0 < b[1] - 2 or x1 > b[2] + 2 or y1 > b[3] + 2:
                msgs.append(f"TEXT OUTSIDE CANVAS: {txt!r}")
        return msgs

    def lint(self, big=1800):
        msgs = []
        n = self.nets()
        for d in n["dangling"]:
            msgs.append(f"DANGLING PIN {d}")
        # open wire ends: degree 1 and no net label and no pin
        pinpts = set()
        for c in self.comps:
            for pin in c.pins.values():
                pinpts.add((round(pin.x, 1), round(pin.y, 1)))
        labpts = {(round(x, 1), round(y, 1)) for (x, y, t, a, cl, s) in self.labels
                  if cl == "net"}
        deg = defaultdict(int)
        for w in self.wires:
            for p in w:
                deg[(round(p[0], 1), round(p[1], 1))] += 1
        for (p1, p2) in self._segments():
            for q in list(deg.keys()):
                if q != (round(p1[0], 1), round(p1[1], 1)) and \
                   q != (round(p2[0], 1), round(p2[1], 1)) and _on_seg(q, p1, p2):
                    deg[q] += 2
        for w in self.wires:
            for p in (w[0], w[-1]):
                k = (round(p[0], 1), round(p[1], 1))
                if deg[k] < 2 and k not in pinpts and k not in labpts:
                    msgs.append(f"OPEN WIRE END at {k}")
        seen = set()
        for c in self.comps:
            if c.ref and c.ref in seen:
                msgs.append(f"DUPLICATE REF {c.ref}")
            seen.add(c.ref)
        # wire-end near-miss check: an end that stops 5-30 units short of a pin
        # is almost always a typo, so it is worth reporting.
        for w in self.wires:
            for p in (w[0], w[-1]):
                k = (round(p[0], 1), round(p[1], 1))
                if deg[k] >= 2 or k in pinpts or k in labpts:
                    continue
                near = [q for q in pinpts
                        if 0 < abs(q[0] - k[0]) + abs(q[1] - k[1]) <= 30]
                if near:
                    msgs.append(f"WIRE END {k} not connected (nearest pin {near[0]})")
        seen = set()
        for c in self.comps:
            if c.ref and c.ref in seen:
                msgs.append(f"DUPLICATE REF {c.ref}")
            seen.add(c.ref)
        b = self._bounds()
        if b[2] - b[0] > big or b[3] - b[1] > 1200:
            msgs.append(f"LARGE CANVAS {b[2]-b[0]:.0f} x {b[3]-b[1]:.0f}")
        return msgs

    def bom(self):
        groups = {}
        order = []
        for c in self.comps:
            if c.kind in ("gnd", "vcc", "port", "box"):
                continue
            key = (c.kind, c.value, c.note or "")
            if key not in groups:
                groups[key] = []
                order.append(key)
            groups[key].append(c.ref)
        out = []
        for key in order:
            kind, value, note = key
            out.append({"cat": KIND_CLASS.get(kind, "其他"), "value": value,
                        "qty": len(groups[key]), "refs": groups[key], "note": note})
        return out


# --------------------------------------------------------------------------- #
if __name__ == "__main__":
    s = Sch("schlib smoke test")
    IN = s.port(40, 160, "IN", align="end")
    C1 = s.c("C1", "2.2u", 90, 160)
    R1 = s.r("R1", "33k", 160, 160, lpos="top")
    Q1 = s.npn("Q1", "2N3904", 240, 160)
    Q2 = s.npn("Q2", "2N3904", 380, 160)
    R3 = s.r("R3", "470", 300, 100)
    s.wire((40, 160), C1.a)
    s.wire(C1.b, R1.a)
    s.wire(R1.b, Q1.b)
    s.wire(Q1.c, (240, 90), R3.a) if False else None
    s.route(Q1.c, R3.a)
    s.route(R3.b, Q2.b)
    s.gnd(Q2.e)
    s.vcc(Q1.c, "9V")
    print("LINT:", s.lint())
    print("BOM:", s.bom())
    s.render("build/_smoke.svg")
    print("wrote build/_smoke.svg")
