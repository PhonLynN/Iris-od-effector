# -*- coding: utf-8 -*-
"""Audit: a component's own ref/value text sitting on top of its own symbol body.

audit_symbols.py 只查「A 的文字压 B 的符号」，这里补查「自己的文字压自己的符号」，
例如 2SC1815 的值标签整个落在晶体管圆圈里。
IC/BOX/OPAMP 不算（体内写型号是原理图惯例）。
"""
import glob
import importlib.util
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import schlib                                                   # noqa: E402

SHRINK = 9.0
SKIP = ("ic", "box", "opamp")


def load(path):
    name = "s_" + os.path.basename(path)[:-3]
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m.build()


def box_of(x, y, txt, size, anchor):
    w = schlib.est_width(txt, size)
    if anchor == "start":
        x0 = x
    elif anchor == "end":
        x0 = x - w
    else:
        x0 = x - w / 2
    return (x0, y - 0.80 * size, x0 + w, y + 0.24 * size)


def audit(s):
    msg = []
    for c in s.comps:
        if c.kind in SKIP:
            continue
        try:
            b = c.bbox_global()
        except Exception:                                   # noqa: BLE001
            continue
        bx = (b[0] + SHRINK, b[1] + SHRINK, b[2] - SHRINK, b[3] - SHRINK)
        lab = c._label_xy()
        if not lab:
            continue
        rx, ry, vx, vy, anchor = lab
        for txt, tx, ty, size in ((c.ref, rx + c.ldx, ry, 11.0),
                                  (c.value or "", vx + c.ldx, vy, 9.8)):
            if not txt:
                continue
            t = box_of(tx, ty, txt, size, anchor)
            ox = min(t[2], bx[2]) - max(t[0], bx[0])
            oy = min(t[3], bx[3]) - max(t[1], bx[1])
            if ox > 1.0 and oy > 1.0:
                msg.append(f"{txt!r} inside {c.ref}")
    return msg


def main():
    files = sys.argv[1:] or sorted(glob.glob(os.path.join(ROOT, "sch", "*.py")))
    total = 0
    for f in files:
        try:
            s = load(f)
        except Exception as e:                              # noqa: BLE001
            print(f"{os.path.basename(f)}: BUILD FAIL {e}")
            continue
        msg = audit(s)
        total += len(msg)
        flag = "OK  " if not msg else "HIT "
        print(f"{flag}{os.path.basename(f):24s} {len(msg):2d}  "
              + "; ".join(msg[:8]) + (" ..." if len(msg) > 8 else ""))
    print(f"\ntotal self overlaps: {total}")


if __name__ == "__main__":
    main()
