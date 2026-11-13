# -*- coding: utf-8 -*-
"""Audit: reported text boxes that sit on top of OTHER components' symbols."""
import glob
import importlib.util
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import schlib                                                   # noqa: E402

SHRINK = 3.0


def load(path):
    name = "m_" + os.path.basename(path)[:-3]
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m.build()


def audit(s):
    """Text of component A must not cover the symbol body of component B."""
    def box_of(x, y, txt, size, anchor):
        w = schlib.est_width(txt, size)
        if anchor == "start":
            x0 = x
        elif anchor == "end":
            x0 = x - w
        else:
            x0 = x - w / 2
        return (x0, y - 0.80 * size, x0 + w, y + 0.24 * size)

    syms = {}
    for c in s.comps:
        try:
            bx = c.bbox_global()
        except Exception:                                   # noqa: BLE001
            continue
        syms[id(c)] = (bx[0] + SHRINK, bx[1] + SHRINK,
                       bx[2] - SHRINK, bx[3] - SHRINK, c.ref, c.kind)
    msg = []

    def probe(box, txt, own_id):
        for oid, (a0, b0, a1, b1, ref, kind) in syms.items():
            if oid == own_id:
                continue
            if kind in ("ic", "box"):
                continue                       # labels inside an IC body are fine
            ox = min(box[2], a1) - max(box[0], a0)
            oy = min(box[3], b1) - max(box[1], b0)
            if ox > 1.0 and oy > 1.0:
                msg.append(f"{txt!r} over {ref}")

    for c in s.comps:
        lab = c._label_xy()
        if not lab:
            continue
        rx, ry, vx, vy, anchor = lab
        probe(box_of(rx + c.ldx, ry, c.ref, 11.0, anchor), c.ref, id(c))
        if c.value:
            probe(box_of(vx + c.ldx, vy, c.value, 9.8, anchor), c.value, id(c))
    for (x, y, txt, anchor, _cls, size) in s.labels:
        probe(box_of(x, y, txt, size, anchor), txt, None)
    return msg


def main():
    files = sys.argv[1:] or sorted(glob.glob(os.path.join(ROOT, "sch", "*.py")))
    total = 0
    for f in files:
        if os.path.basename(f) in ("schlib.py",):
            continue
        try:
            s = load(f)
        except Exception as e:                              # noqa: BLE001
            print(f"{os.path.basename(f)}: BUILD FAIL {e}")
            continue
        msg = audit(s)
        total += len(msg)
        flag = "OK  " if not msg else "HIT "
        print(f"{flag}{os.path.basename(f):24s} {len(msg):2d}  "
              + "; ".join(msg[:6]) + (" ..." if len(msg) > 6 else ""))
    print(f"\ntotal symbol overlaps: {total}")


if __name__ == "__main__":
    main()
