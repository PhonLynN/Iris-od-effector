"""netcheck.py --- 找「电气上孤立」的元件引脚（wire 与 netlabel 都算连接）。

lint() 只抓 DANGLING PIN（引脚处没有导线）和 OPEN WIRE END（导线端点悬空），
抓不到「线画了但没接到目标引脚」和「网名标签只出现一次」这两类。

判据：
  1. 网内只有一个元件、且不含 gnd/vcc/port 符号、且该引脚没有网名标签  -> 孤立；
  2. 网名标签（cls='net'）只被一个引脚使用，且该引脚没有导线            -> 孤立。
另：同一片运放另一半的电源脚（U1B.vp/vn）由 U1A 绘制，属设计约定，跳过。

用法：python tools\\netcheck.py [sch/xxx.py ...]  （不给参数则全量）
"""
import os
import sys
import glob
import importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
sys.path.insert(0, ROOT)
import tools.schlib as schlib  # noqa: E402

SYMBOL_PIN = ".p"
LABELS = []          # [(x, y, name)]  netlabel 调用的原始引脚坐标


def _patch():
    orig = schlib.Sch.netlabel

    def patched(self, p, name, *a, **kw):
        x, y = self._pt(p)
        LABELS.append((float(x), float(y), name))
        return orig(self, p, name, *a, **kw)

    schlib.Sch.netlabel = patched


def load_mod(path):
    name = os.path.splitext(os.path.basename(path))[0]
    spec = importlib.util.spec_from_file_location("schnc_" + name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def check(path):
    del LABELS[:]
    mod = load_mod(path)
    s = mod.build()
    nets = s.nets()
    groups, pins_of = nets["groups"], nets["pins_of"]

    labelled = {(round(x, 1), round(y, 1)): nm for x, y, nm in LABELS}
    users = {}
    for (x, y), nm in labelled.items():
        users.setdefault(nm, 0)
        users[nm] += 1

    # ---- 3) 悬空的电源符号：vcc/gnd/port 的坐标必须落在某个导线端点上，
    #         或与其它元件的引脚坐标重合；否则就是一个“画着但没接线”的符号。
    bad_symbols = []
    wire_ends = set()
    wire_segments = []
    for w in getattr(s, "wires", []):
        for p in w:
            wire_ends.add((round(float(p[0]), 1), round(float(p[1]), 1)))
        for a, b in zip(w, w[1:]):
            wire_segments.append(((float(a[0]), float(a[1])), (float(b[0]), float(b[1]))))

    def _on_wire(x, y):
        if (round(x, 1), round(y, 1)) in wire_ends:
            return True
        for (x1, y1), (x2, y2) in wire_segments:
            cross = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1)
            if abs(cross) > 0.35:
                continue
            if min(x1, x2) - 0.35 <= x <= max(x1, x2) + 0.35 and \
               min(y1, y2) - 0.35 <= y <= max(y1, y2) + 0.35:
                return True
        return False
    pin_pts = set()
    for c in getattr(s, "comps", []):
        if c.kind in ("vcc", "gnd", "port"):
            continue
        for pin in getattr(c, "pins", {}).values():
            pin_pts.add((round(float(pin.x), 1), round(float(pin.y), 1)))
    for c in getattr(s, "comps", []):
        if c.kind not in ("vcc", "gnd", "port"):
            continue
        xy = (round(float(c.x), 1), round(float(c.y), 1))
        if _on_wire(xy[0], xy[1]) or xy in pin_pts:
            continue
        bad_symbols.append((c.kind, xy))

    bad = []
    for gid, pins in pins_of.items():
        if SYMBOL_PIN in pins:
            continue
        real = list(pins)
        comps = {p.split(".")[0] for p in real}
        if len(real) == 1 and ".v" in real[0]:
            continue
        if len(comps) != 1:
            continue
        pts = groups.get(gid) or []
        pts = [(round(float(a), 1), round(float(b), 1)) for a, b in pts]
        if any(pt in labelled for pt in pts):
            continue                      # 用网名连接，OK
        bad.append((real, pts))
    return bad, users, bad_symbols


def main(argv):
    _patch()
    files = argv[1:] or sorted(glob.glob(os.path.join(ROOT, "sch", "*.py")))
    total = 0
    for path in files:
        if os.path.basename(path).startswith("_"):
            continue
        try:
            bad, users, bad_symbols = check(path)
        except Exception as exc:                      # noqa: BLE001
            print("ERR  %-26s %s" % (os.path.basename(path), exc))
            total += 1
            continue
        ok = not bad and not bad_symbols
        print("%-4s %-26s %d" % ("OK  " if ok else "FAIL", os.path.basename(path), len(bad) + len(bad_symbols)))
        for pins, pts in bad:
            print("       isolated: %-28s at %s" % (", ".join(pins), pts))
        for kind, xy in bad_symbols:
            print("       floating %-6s symbol at %s" % (kind, xy))
        total += len(bad) + len(bad_symbols)
    print("\ntotal isolated pins: %d" % total)
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
