# -*- coding: utf-8 -*-
"""Validate one schematic script: electrical lint + layout lint + BOM + svg.

usage:  python tools/check.py sch/fuzz_face.py [--no-render]

Exit code 1 when there are hard problems (lint / layout), so a build loop can
short-circuit.  Layout warnings are printed last because they are the ones that
usually need eyeballing.
"""
import importlib.util
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT)

CAT = {"电阻", "电容", "电解电容", "电感", "二极管", "稳压二极管", "LED",
       "晶体管", "场效应管", "运放", "电位器", "开关", "其他"}


def load(path):
    spec = importlib.util.spec_from_file_location("sch_mod", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    path = argv[1]
    render = "--no-render" not in argv
    stem = os.path.splitext(os.path.basename(path))[0]
    mod = load(path)
    s = mod.build()

    hard = []
    hard += s.lint()
    soft = s.layout_lint()
    try:                       # text drawn on top of another symbol body
        sys.path.insert(0, os.path.join(ROOT, "tools"))
        import audit_symbols                                   # noqa: PLC0415
        soft += ["TEXT/SYMBOL: " + m for m in audit_symbols.audit(s)]
    except Exception:                                          # noqa: BLE001
        pass

    print(f"== {stem} ==")
    print(f"components: {len(s.comps)}   wires: {len(s.wires)}   "
          f"nets: {len(s.nets()['groups'])}")
    if hard:
        print("LINT:")
        for m in hard:
            print("  !", m)
    else:
        print("LINT: clean")
    if soft:
        print("LAYOUT:")
        for m in soft:
            print("  ~", m)
    else:
        print("LAYOUT: clean")

    rows = s.bom()
    print(f"BOM rows: {len(rows)}")
    for r in rows:
        print(f"  {r['cat']:<6} {r['value']:<14} x{r['qty']}  "
              f"{','.join(r['refs'])}")

    if render:
        out = os.path.join(ROOT, "assets", "svg", stem + ".svg")
        os.makedirs(os.path.dirname(out), exist_ok=True)
        s.render(out)
        w, h = s.svg_size() if hasattr(s, "svg_size") else (0, 0)
        print(f"wrote {os.path.relpath(out, ROOT)}  {w}x{h}")
    return 1 if hard else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
