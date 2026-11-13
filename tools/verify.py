# -*- coding: utf-8 -*-
"""Cross-check every pedal: JSON schema, BOM <-> schematic, SVG presence."""
import glob
import importlib.util
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT)

TAGS = {
    "Blues", "Blues Rock", "Classic Rock", "Hard Rock", "Heavy Metal",
    "Thrash Metal", "Doom / Sludge", "Stoner Rock", "Psychedelic Rock",
    "Progressive Rock", "Rockabilly", "Garage Rock", "Punk", "Grunge",
    "Shoegaze", "Indie Rock", "Alternative", "Funk", "Soul / R&B", "Country",
    "Jazz", "Surf / Instrumental", "Ambient / Post-rock", "Modern Metal",
}
KINDS = {"电阻", "电容", "电解电容", "薄膜电容", "晶体管", "场效应管", "运放",
         "集成电路", "二极管", "稳压二极管", "LED", "电位器", "开关", "接插件",
         "电感"}
REQ = ["slug", "name_zh", "name_en", "brand", "year", "category", "tagline",
       "style_tags", "sound_zh", "circuit", "bom_meta_zh", "bom", "usage_zh",
       "users", "sources"]
CATS = {"clean", "od", "dist", "fuzz", "comp"}


def load_sch(slug):
    cands = [os.path.join(ROOT, "sch", slug.replace("-", "_") + ".py"),
             os.path.join(ROOT, "sch", slug + ".py")]
    for p in cands:
        if os.path.exists(p):
            spec = importlib.util.spec_from_file_location("m_" + slug.replace("-", "_"), p)
            m = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(m)
            return p, m.build()
    return None, None


def main():
    errs, warns = [], []
    files = sorted(glob.glob(os.path.join(ROOT, "data", "pedals", "*.json")))
    tot_bom = 0
    for f in files:
        base = os.path.basename(f)
        try:
            d = json.load(open(f, encoding="utf-8"))
        except Exception as e:                          # noqa: BLE001
            errs.append(f"{base}: JSON 解析失败 {e}")
            continue
        slug = d.get("slug", "?")
        for k in REQ:
            if k not in d or d[k] in (None, "", []):
                errs.append(f"{slug}: 缺字段 {k}")
        if d.get("category") not in CATS:
            errs.append(f"{slug}: category 非法 {d.get('category')!r}")
        if not isinstance(d.get("year"), int):
            warns.append(f"{slug}: year 不是整数 ({d.get('year')!r})")
        if str(d.get("order", "")).strip() and not isinstance(d.get("order"), int) \
                and d.get("order") is not None:
            warns.append(f"{slug}: order 不是整数 ({d.get('order')!r})")
        if base != slug + ".json":
            errs.append(f"{slug}: 文件名应为 {slug}.json")
        for t in d.get("style_tags", []):
            if t.get("name") not in TAGS:
                errs.append(f"{slug}: 非受控标签 {t.get('name')!r}")
            if not (1 <= int(t.get("fit", 0)) <= 5):
                errs.append(f"{slug}: fit 越界 {t.get('fit')!r}")
        tl = len(d.get("tagline", ""))
        if tl > 44:
            warns.append(f"{slug}: tagline {tl} 字（建议 ≤40）")
        for r in d.get("bom", []):
            if r.get("kind") not in KINDS:
                errs.append(f"{slug}/{r.get('ref')}: kind {r.get('kind')!r} 不在词表")
            if not r.get("part"):
                errs.append(f"{slug}/{r.get('ref')}: 缺具体型号 part")
            if r.get("kind") == "电阻":
                continue
            p = (r.get("part") or "").lower()
            generic = ("固定型号", "通用", "根据", "若干", "tbd", "unknown")
            if any(g in p for g in generic) or len(p) < 6:
                warns.append(f"{slug}/{r.get('ref')}: 型号过泛 {r.get('part')!r}")
        tot_bom += len(d.get("bom", []))

        # schematic <-> BOM
        sp, s = load_sch(slug)
        if s is None:
            errs.append(f"{slug}: 找不到 sch/{slug.replace('-', '_')}.py")
            continue
        try:
            sbom = s.bom()
        except Exception as e:                          # noqa: BLE001
            errs.append(f"{slug}: 原理图 BOM 失败 {e}")
            continue
        refs_sch = set()
        for row in sbom:
            refs_sch.update(row.get("refs", []))
        refs_json = {r.get("ref") for r in d.get("bom", [])}
        only_s = sorted(refs_sch - refs_json)
        only_j = sorted(refs_json - refs_sch)

        def panel(ref):        # panel hardware not drawn on the schematic
            return ref.split("_")[0].rstrip("0123456789") in ("J", "SW", "BAT",
                                                             "LED", "DC", "X")
        only_j_elec = [r for r in only_j if not panel(r)]
        if only_s:
            errs.append(f"{slug}: 原理图有而 JSON 缺 {only_s}")
        if only_j_elec:
            errs.append(f"{slug}: JSON 有而原理图缺 {only_j_elec}")
        svg = os.path.join(ROOT, "assets", "svg", slug.replace("-", "_") + ".svg")
        if not os.path.exists(svg):
            svg = os.path.join(ROOT, "assets", "svg", slug + ".svg")
        if not os.path.exists(svg):
            errs.append(f"{slug}: 缺 assets/svg/<slug>.svg")
        else:
            size = os.path.getsize(svg)
            if size < 2500:
                warns.append(f"{slug}: SVG 只有 {size}B，可能没画完")
        # lint status
        try:
            l = s.lint()
            l2 = s.layout_lint()
        except Exception as e:                          # noqa: BLE001
            errs.append(f"{slug}: lint 失败 {e}")
            continue
        if l:
            errs.append(f"{slug}: LINT {l[:3]}")
        if l2:
            errs.append(f"{slug}: LAYOUT {l2[:3]}")
        try:
            sys.path.insert(0, os.path.join(ROOT, "tools"))
            import audit_symbols                                   # noqa: PLC0415
            for m in audit_symbols.audit(s):
                warns.append(f"{slug}: 文字压元件 {m}")
        except Exception:                                          # noqa: BLE001
            pass

    print(f"pedals: {len(files)}   bom rows: {tot_bom}")
    for w in warns:
        print("WARN  " + w)
    for e in errs:
        print("ERROR " + e)
    print(f"\n{'FAIL' if errs else 'OK'}  ({len(errs)} errors, {len(warns)} warnings)")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
