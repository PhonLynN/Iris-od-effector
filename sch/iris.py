# -*- coding: utf-8 -*-
"""Iris R v1.8 —— 透明过载：输入×2 → 无源低切 → 2kHz 预提升
   → 双级对称削波（2×1N4148/侧 + 4.7k 软膝）→ 干湿混合 → TONE 高切 → VOLUME
   电源：9V 单入，ICL7660 电荷泵出 ±9V（运放 ±7V 摆幅）"""

from tools.schlib import Sch


def clipper(s, ux, uy, tag, rg_ref, rv_ref, rser_ref, d_refs):
    """一级反馈削波：反馈网络在上、二极管桥在下，全部正交走线。"""
    xin_in = ux - 52
    xc = ux - 100            # -in 竖轨（Rg、反馈腿、二极管桥共用）
    xout = ux + 56           # 输出竖轨
    ytop = uy - 115          # 反馈腿
    yr1 = uy + 120           # 顺向二极管
    yr2 = uy + 155           # 反向二极管

    # -in 引出 → 竖轨
    s.wire((xin_in, uy - 16), (xc, uy - 16))
    s.junction((xc, uy - 16))

    # 竖轨：反馈腿 → 二极管桥 → Rg
    s.wire((xc, ytop), (xc, yr2))
    s.junction((xc, yr1))
    s.junction((xc, yr2))

    # Rg 到地
    rg = s.r(rg_ref, "5.1K", xc, yr2 + 45, rot=90, lpos="left")
    s.wire((xc, yr2), rg.a)
    s.gnd(rg.b)

    # 反馈腿：RV(变阻器) + Rser → 输出竖轨
    s.wire((xc, ytop), (xc + 15, ytop))
    rv = s.pot(rv_ref, "100K  LIN", xc + 40, ytop, rot=0, lpos="none")
    s.text(xc + 40, ytop - 44, rv_ref, size=9.5, anchor="middle", cls="ref")
    s.text(xc + 40, ytop - 32, "100K LIN", size=9, anchor="middle")
    s.wire(rv.w, (xc + 40, ytop + 25), (xc + 65, ytop + 25), (xc + 65, ytop))
    rs = s.r(rser_ref, "4.7K", xc + 100, ytop, lpos="top")
    s.wire(rv.b, rs.a)
    s.wire(rs.b, (xout, ytop))

    # 输出竖轨
    s.wire((xout, ytop), (xout, yr2))
    s.junction((xout, uy))
    s.junction((xout, yr1))
    s.junction((xout, yr2))

    # 二极管桥（顺向两只 / 反向两只，反并联）
    d1 = s.d(d_refs[0], "1N4148", xc + 52, yr1, lpos="top")
    d2 = s.d(d_refs[1], "1N4148", xc + 107, yr1, lpos="top")
    s.wire((xc, yr1), d1.a)
    s.wire(d1.k, d2.a)
    s.wire(d2.k, (xout, yr1))
    d3 = s.d(d_refs[2], "1N4148", xc + 107, yr2, rot=180, lpos="bottom")
    d4 = s.d(d_refs[3], "1N4148", xc + 52, yr2, rot=180, lpos="bottom")
    s.wire((xout, yr2), d3.a)
    s.wire(d3.k, d4.a)
    s.wire(d4.k, (xc, yr2))

    # 级名
    s.text(xc + 4, ytop - 76, "U1%s  削波级 %s" % ("B" if tag == "1" else "C", tag),
           size=9.5, anchor="start")


def build():
    s = Sch()

    # ---------------- 标题 ----------------
    s.text(60, 56, "Iris R  v1.8  —  2026", size=17, anchor="start", cls="title")
    s.text(60, 78, "透明过载：输入 ×2 → 无源低切 → 2kHz +7dB 预提升 → 双级对称削波（1N4148 ×8）"
                   " → 干湿混合 → TONE 高切 → VOLUME", size=9.2, anchor="start", cls="sub")
    s.text(60, 104, "① 输入 ×2  ② LOWCUT 29–312Hz  ③ 预提升 2kHz  ④ 削波级 1  ⑤ 削波级 2  "
                    "⑥ BLEND 干湿  ⑦ TONE 高切  ⑧ VOLUME", size=8.4, anchor="start", cls="note")

    # ---------------- 电源：9V → ICL7660 → ±9V ----------------
    s.port(60, 190, "9V", align="end")
    d9 = s.d("D9", "1N4007", 120, 190, lpos="top")
    s.wire((60, 190), d9.a)
    r9 = s.r("R9", "100", 190, 190, lpos="top")
    s.wire(d9.k, r9.a)
    s.wire(r9.b, (250, 190))
    s.junction((250, 190))
    c6 = s.c("C6", "100u", 250, 240, rot=90, lpos="left")
    s.wire((250, 190), c6.a)
    s.gnd(c6.b)

    u2 = s.ic("U2", "ICL7660", 470, 200, w=130, h=96,
              left=("V+", "CP+", "CP-"), right=("VOUT",))
    s.wire((250, 190), u2.pins["V+"])
    px, py = u2.pins["CP+"]
    mx, my = u2.pins["CP-"]
    s.wire((px, py), (px - 40, py), (px - 40, py + 70))
    c10 = s.c("C10", "10u", px - 40, py + 95, rot=90, lpos="left")
    s.wire((px - 40, py + 70), c10.a)
    s.wire(c10.b, (px - 40, my + 130), (px - 30, my + 130), (px - 30, my))
    s.wire((px - 30, my), (mx, my))
    vx, vy = u2.pins["VOUT"]
    s.wire((vx, vy), (vx + 120, vy))
    s.vcc((vx + 120, vy), "-9V")
    c12 = s.c("C12", "10u", vx + 120, vy + 50, rot=90, lpos="right")
    s.wire((vx + 120, vy), c12.a)
    s.gnd(c12.b)
    s.text(60, 704, "U2 ICL7660：9V 单入 → ±9V，运放摆幅 ±7V", size=8.4, anchor="start", cls="note")

    # ---------------- ① 输入 ×2（U1A） ----------------
    y = 470
    s.port(60, y, "IN", align="end")
    c1 = s.c("C1", "22n", 120, y, lpos="bottom")
    s.wire((60, y), c1.a)
    s.wire(c1.b, (175, y))
    s.junction((175, y))
    r1 = s.r("R1", "1M", 175, 520, rot=90, lpos="left")
    s.wire((175, y), r1.a)
    s.gnd(r1.b)

    u1a = s.opamp("U1A", "TL074", 300, y)
    s.wire(u1a.vp, (300, 402))
    s.vcc((300, 402), "V+")
    s.port(300, y + 72, "-9V", align="end")
    s.wire(u1a.vn, (300, y + 72))
    s.wire((175, y), (200, y), (200, y + 16), u1a.inp)
    s.text(240, 596, "U1A  输入 ×2", size=9.3, anchor="start")

    # 反馈 R3：输出 → -in
    s.wire(u1a.out, (365, y))
    s.wire((365, y), (365, y - 115))
    r3 = s.r("R3", "10K", 310, y - 115, lpos="top")
    s.wire((365, y - 115), r3.b)
    s.wire(r3.a, (255, y - 115), (255, y - 16), (248, y - 16))
    s.wire((248, y - 16), u1a.inn)

    # ---------------- ② 无源可变低切 ----------------
    s.wire((365, y), c2a := (400, y))
    c2 = s.c("C2", "100n", 425, y, lpos="top")
    s.wire((400, y), c2.a)
    s.wire(c2.b, (475, y))
    s.junction((475, y))
    rv2 = s.pot("RV2", "50K  LIN", 475, 520, rot=90, lpos="left")
    s.wire((475, y), rv2.a)
    s.wire(rv2.w, (500, 520), (500, 555), (475, 555), rv2.b)
    r4 = s.r("R4", "5.1K", 475, 600, rot=90, lpos="left")
    s.wire(rv2.b, r4.a)
    s.gnd(r4.b)
    s.text(340, 660, "LOWCUT = 1/(2π·(RV2+R4)·C2) = 312…29 Hz（常开）", size=8.4, anchor="start", cls="note")

    # ---------------- ③ 2kHz +7dB 预提升（U1D） ----------------
    u1d = s.opamp("U1D", "TL074", 580, y)
    s.wire(u1d.vp, (580, 402))
    s.vcc((580, 402), "V+")
    s.port(580, y + 72, "-9V", align="end")
    s.wire(u1d.vn, (580, y + 72))
    s.wire((475, y), (500, y), (500, y + 16), u1d.inp)
    s.text(520, 596, "U1D  预提升", size=9.3, anchor="start")

    # 反馈：R14 ∥ (R15 + C9)
    s.wire(u1d.inn, (520, y - 16))
    s.wire((520, y - 16), (520, y - 185))
    s.junction((520, y - 115))
    s.wire((520, y - 115), (545, y - 115))
    r14 = s.r("R14", "10K", 570, y - 115, lpos="top")
    s.wire(r14.b, (665, y - 115), (665, y))
    s.wire((520, y - 185), (545, y - 185))
    r15 = s.r("R15", "8.2K", 570, y - 185, lpos="top")
    c9 = s.c("C9", "10n", 625, y - 185, lpos="top")
    s.wire(r15.b, c9.a)
    s.wire(c9.b, (665, y - 185), (665, y - 115))
    s.junction((665, y - 115))
    s.wire(u1d.out, (665, y))
    s.junction((665, y))
    s.text(380, y - 212, "2kHz +7dB 预提升", size=8.4, anchor="start", cls="note")

    # ---------------- ④ 削波级 1（U1B） ----------------
    s.wire((665, y), (700, y))
    c3 = s.c("C3", "100n", 725, y, lpos="top")
    s.wire((700, y), c3.a)
    s.wire(c3.b, (775, y))
    s.junction((775, y))
    u1b = s.opamp("U1B", "TL074", 860, y)
    s.wire(u1b.vp, (860, 402))
    s.vcc((860, 402), "V+")
    s.wire((775, y), (800, y), (800, y + 16), u1b.inp)
    clipper(s, 860, y, "1", "Rg1", "RV1", "R16", ["D1", "D2", "D3", "D4"])
    s.port(860, y + 72, "-9V", align="end")
    s.wire(u1b.vn, (860, y + 72))

    # ---------------- 级间 −6dB ----------------
    s.wire(u1b.out, (950, y))
    r17 = s.r("R17", "10K", 975, y, lpos="top")
    s.wire((950, y), r17.a)
    s.wire(r17.b, (1030, y))
    s.junction((1030, y))
    r18 = s.r("R18", "10K", 1030, 525, rot=90, lpos="left")
    s.wire((1030, y), r18.a)
    s.gnd(r18.b)

    # ---------------- ⑤ 削波级 2（U1C） ----------------
    u1c = s.opamp("U1C", "TL074", 1120, y)
    s.wire(u1c.vp, (1120, 402))
    s.vcc((1120, 402), "V+")
    s.wire((1030, y), (1060, y), (1060, y + 16), u1c.inp)
    clipper(s, 1120, y, "2", "Rg2", "RV6", "R20", ["D5", "D6", "D7", "D8"])
    s.port(1120, y + 72, "-9V", align="end")
    s.wire(u1c.vn, (1120, y + 72))

    # ---------------- ⑥ 干湿混合（BLEND） ----------------
    s.wire(u1c.out, (1210, y))
    r21 = s.r("R21", "22K", 1235, y, lpos="top")
    s.wire((1210, y), r21.a)
    s.wire(r21.b, (1300, y))
    s.junction((1300, y))
    # 干声：取自预提升输出
    s.junction((775, y))
    s.wire((775, y), (775, 720), (1130, 720))
    rv5 = s.pot("RV5", "100K  LIN", 1155, 720, rot=0, lpos="none")
    s.text(1192, 700, "RV5", size=9.5, anchor="start", cls="ref")
    s.text(1192, 712, "100K LIN", size=9, anchor="start")
    s.wire((1130, 720), rv5.a)
    s.wire(rv5.w, (1155, 745), (1180, 745), (1180, 720))
    s.wire(rv5.b, (1300, 720), (1300, y))
    s.junction((1300, y))
    s.text(1010, 762, "BLEND：干声经 RV5 回混，越大越干", size=8.4, anchor="start", cls="note")

    # ---------------- ⑦ TONE 高切 ----------------
    rv3 = s.pot("RV3", "100K  LOG", 1300, 545, rot=90, lpos="left")
    s.wire((1300, y), rv3.a)
    s.wire(rv3.w, (1325, 545), (1325, 505), (1300, 505))
    c4 = s.c("C4", "4.7n", 1300, 615, rot=90, lpos="right")
    s.wire(rv3.b, c4.a)
    s.gnd(c4.b)
    s.text(1330, 600, "TONE", size=8.4, anchor="start", cls="note")

    # ---------------- ⑧ VOLUME 与输出 ----------------
    c8 = s.c("C8", "1u", 1350, y, lpos="top")
    s.wire((1300, y), c8.a)
    s.wire(c8.b, (1400, y))
    rv4 = s.pot("RV4", "100K  LOG", 1435, y, rot=0, lpos="none")
    s.text(1435, y - 50, "RV4", size=9.5, anchor="middle", cls="ref")
    s.text(1435, y - 38, "100K LOG", size=9, anchor="middle")
    s.wire((1400, y), rv4.a)
    s.wire(rv4.b, (1480, y))
    s.gnd((1480, y))
    s.wire(rv4.w, (1435, y + 25), (1435, y + 60), (1520, y + 60))
    s.port(1520, y + 60, "OUT", align="start")

    # ---------------- 图注 ----------------
    s.text(60, y + 300,
           "总增益 = 2（输入）× 2（预提升低频段）× (1 + RV1/5.1K) × 0.5（级间）× 2（削波级 2）"
           " ≈ 4…41×；削波阈值 ±1.3V（每侧 2×0.65V），串 4.7K 使膝变圆；"
           "电源 ±9V，运放摆幅 ±7V", size=8.6, anchor="start", cls="note")

    return s


if __name__ == "__main__":
    s = build()
    print("LINT:", *s.lint(), sep="\n  ")
    print("LAYOUT:", *s.layout_lint(), sep="\n  ")
    print("BOM:")
    for row in s.bom():
        print("  ", row)
    print("wrote", s.render("assets/svg/iris.svg"))

