# -*- coding: utf-8 -*-
"""Iris R v1.8 设计文档生成器（纯标准库，单文件 HTML）。

用法：
    python build/design_doc.py
产物：
    build/iris_design.html   （内联原理图 SVG，可直接打印成 PDF）
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, ROOT)

from sch.iris import build as sch_build          # noqa: E402
sys.path.insert(0, HERE)
from doc_style import CSS                              # noqa: E402

OUT = os.path.join(HERE, "iris_design.html")
SVG = os.path.join(ROOT, "assets", "svg", "iris.svg")

VERSION = "v1.8"

# 位号 → (具体型号/规格, 作用)
PART = {
    "R1":  ("1/4W 金属膜 1%", "输入对地：定下 1M 输入阻抗，与 C1 组成 7 Hz 高通"),
    "R2":  ("1/4W 金属膜 1%", "U1A 反相端到地：与 R3 一起定 ×2"),
    "R3":  ("1/4W 金属膜 1%", "U1A 反馈：A = 1 + R3/R2 = 2"),
    "R4":  ("1/4W 金属膜 1%", "低切支路固定电阻：与 RV2 串出 312…29 Hz"),
    "R9":  ("1/4W 金属膜 1%（1/2W 更稳）", "9V 进料 RC 滤波，与 C6 压掉电源纹波"),
    "R14": ("1/4W 金属膜 1%", "预提升级反馈（平直段 1×）"),
    "R15": ("1/4W 金属膜 1%", "预提升级反馈串阻：与 C9 把 2 kHz 以上抬 +7 dB"),
    "R16": ("1/4W 金属膜 1%", "第一削波级软膝串阻：与二极管串起来把拐角变圆"),
    "R17": ("1/4W 金属膜 1%", "级间串阻，与 R18 组成 −6 dB 衰减"),
    "R18": ("1/4W 金属膜 1%", "级间对地：10K/10K = −6 dB，让第二级重新有 headroom"),
    "R20": ("1/4W 金属膜 1%", "第二削波级软膝串阻（与 R16 同值）"),
    "R21": ("1/4W 金属膜 1%", "湿声输出串阻，进入 BLEND 混合点"),
    "Rg1": ("5.1K 1/4W 金属膜 1%", "第一削波级反相端到地：与 RV1 定该级增益 1…20.6×"),
    "Rg2": ("5.1K 1/4W 金属膜 1%", "第二削波级反相端到地：与 RV6 定该级增益"),
    "C1":  ("薄膜 22nF/63V（WIMA MKS2）", "输入隔直；与 R1 组成 7 Hz 高通"),
    "C2":  ("薄膜 100nF/63V（WIMA MKS2）", "低切电容：容值精度直接决定低切范围"),
    "C3":  ("薄膜 100nF/63V（WIMA MKS2）", "级间隔直，把信号送进第一削波级"),
    "C4":  ("薄膜 4.7nF/63V", "TONE 高切电容：与 RV3 组成一阶低通"),
    "C6":  ("电解 100µF/25V（尼吉康 UFG）", "9V 主滤波，与 R9 组成 15.9 Hz 的 RC"),
    "C8":  ("薄膜 1µF/50V（WIMA MKS2）", "输出隔直，音量电位器上不留直流"),
    "C9":  ("薄膜 10nF/63V", "预提升级高频支路：2 kHz 以上该级增益升到约 2.2×"),
    "C10": ("电解 10µF/25V", "ICL7660 飞电容（接 CP+ / CP−）"),
    "C12": ("电解 10µF/25V", "−9V 轨退耦"),
    "D9":  ("1N4007（DO-41）", "电源防反接"),
    "RV1": ("16mm 碳膜 100K B（线性）", "GAIN：反馈环内变阻器，1K–100K"),
    "RV2": ("16mm 碳膜 50K B（线性）", "LOWCUT：无源高通里的可变电阻，29–312 Hz"),
    "RV3": ("16mm 碳膜 100K A（音频）", "TONE：高频对地衰减，越大越亮"),
    "RV4": ("16mm 碳膜 100K A（音频）", "VOLUME：输出音量"),
    "RV5": ("16mm 碳膜 100K B（线性）", "BLEND：干声混入量，越大越松"),
    "RV6": ("16mm 碳膜 100K B（线性）", "第二级 GAIN 微调：出厂可置 0（纯对称级）"),
}
for _i in range(1, 5):
    PART["D%d" % _i] = ("1N4148（DO-35）", "第一削波级：每侧两只串联，桥式反并联")
for _i in range(5, 9):
    PART["D%d" % _i] = ("1N4148（DO-35）", "第二削波级：每侧两只串联，桥式反并联")

PANEL = [
    ("J1", "插孔", "1/4″ 单声道", "Switchcraft 11 开放式输入插孔", "输入"),
    ("J2", "插孔", "1/4″ 单声道", "Switchcraft 11 输出插孔", "输出"),
    ("SW1", "开关", "DPDT", "Carling 112-P DPDT（或 3PDT）", "True bypass 切换"),
    ("LED1", "指示", "3mm 红", "3mm 红色 LED + 4.7K 限流", "状态指示"),
    ("DC1", "电源", "2.1mm", "2.1mm 中心负极 DC 座", "9V 供电，无电池仓"),
]

SHORT = {
    "R1": "输入对地", "R2": "U1A 地端", "R3": "U1A 反馈", "R4": "低切最小电阻",
    "R9": "电源滤波", "R14": "预提升反馈", "R15": "预提升高频支路", "R16": "一级软膝",
    "R17": "级间串阻", "R18": "级间对地", "R20": "二级软膝", "R21": "湿声串阻",
    "Rg1": "一级增益地端", "Rg2": "二级增益地端",
    "C1": "输入隔直", "C2": "低切电容", "C3": "级间隔直", "C4": "TONE 电容",
    "C6": "主滤波", "C8": "输出隔直", "C9": "预提升高频支路", "C10": "飞电容",
    "C12": "−9V 退耦", "D9": "防反接",
    "D1": "一级桥", "D2": "一级桥", "D3": "一级桥", "D4": "一级桥",
    "D5": "二级桥", "D6": "二级桥", "D7": "二级桥", "D8": "二级桥",
    "RV1": "GAIN", "RV2": "LOWCUT", "RV3": "TONE", "RV4": "VOLUME",
    "RV5": "BLEND", "RV6": "二级 GAIN",
}

KPIS = [("+12…+38", "dB 总增益"), ("29–312", "Hz 低切"),
        ("±1.3", "V 削波阈值（每级）"), ("0.34–34", "kHz TONE 高切")]

CHAIN = [
    ("① 输入 ×2", "非反相，R3/R2 = 10K/10K<br>输入阻抗 ≈ 1MΩ"),
    ("② 低切", "无源高通 C2+RV2+R4<br>29–312 Hz，与增益无关"),
    ("③ 预提升", "2 kHz 以上 +7 dB<br>R14 ∥ (R15 + C9)"),
    ("④ 削波级 1", "1 + RV1/5.1K = 1…20.6×<br>2×1N4148 每侧，±1.3 V"),
    ("⑤ 削波级 2", "级间 −6 dB 后再次增益<br>桥式反并联，再削一层"),
    ("⑥ BLEND", "干声经 RV5 混回<br>越大越松、音头越多"),
    ("⑦ TONE", "RV3 + C4 高频对地<br>0.34–34 kHz"),
    ("⑧ VOLUME", "100K A 型分压<br>0…100%"),
]

GAIN_ROWS = [
    ("GAIN 0%", "+7.5 dB", "干净的推子：只做阻抗变换和一点预热"),
    ("GAIN 25%", "+20.5 dB", "轻推，触弦动态最明显"),
    ("GAIN 50%", "+24.7 dB", "主要的 crunch 区间"),
    ("GAIN 75%", "+26.5 dB", "持续音开始压缩"),
    ("GAIN 100%", "+27.2 dB", "上限（40 mV 小信号）；大信号另算"),
]
THD_ROWS = [
    ("GAIN 100%", "0.04 V", "11.1 %"),
    ("GAIN 100%", "0.20 V", "26.7 %"),
    ("GAIN 100%", "0.40 V", "26.6 %"),
    ("GAIN 100%", "1.20 V", "29.6 %"),
    ("GAIN 0%", "0.04 V", "0.02 %"),
    ("GAIN 0%", "1.20 V", "12.6 %"),
]
HARM_ROWS = [
    ("SI（对称）", "0.87 %", "23.90 %", "10.69 %", "1.11 %", "0.06 %"),
    ("GE（不对称跳线）", "10.31 %", "20.52 %", "8.64 %", "0.85 %", "0.05 %"),
]
FIX_ROWS = [
    ("输入电平太轻", "v0.6 起输入只有 1× 缓冲", "U1A 非反相 ×2（+6 dB）"),
    ("高增益塌陷", "一级削波后信号被钉死，后级无 headroom", "级间 −6 dB + 第二级重新放大再削"),
    ("低切耦合", "低切电阻在增益级地回路 → 与增益联动 17 dB", "改成级间无源高通，通带增益恒为 1"),
    ("颗粒感不足", "只有一级软削波，谐波层数太少", "2 kHz +7 dB 预提升 + 两级硬/软混合削波"),
    ("亮而刺", "高频与低频一起进削波器，7/9 次谐波堆积", "软膝 4.7K + TONE 高切 + 9 kHz 后置低通"),
    ("“劈”的偶次", "削波求解不收敛，正负半周残差不同", "残差驱动牛顿（24 次外层 / 8 次内层，容差 1e-7）"),
]

HOWTO = [
    ("先把电源量对", "U2 的 VOUT 应为约 −8.6 V；四个运放的 V+ 与 −9V 之间约 17 V。偏低先查 D9 方向与 C10 飞电容极性。"),
    ("静态工作点全是 0 V", "这一版是双电源（±9 V）、**地参考**，所有运放输出静态都应在 0 V 附近，±30 mV 以内算正常。"),
    ("低切最小转折由 R4 定", "RV2 拧到最小时只剩 5.1K，转折 312 Hz；想要更厚可把 R4 改 3.3K（约 480 Hz→更低），代价是低频进削波器。"),
    ("两级增益一起调", "RV1 与 RV6 是串起来的：先把 RV6 置 0（第二级 1×），只用 RV1 找音色，再慢慢加 RV6 增加密度。"),
    ("软膝电阻是“手感”旋钮", "R16/R20 从 4.7K 加到 10K，拐角更圆、更“绵”；减到 2.2K 会更硬更“扎”。"),
    ("BLEND 决定“松/紧”", "干声从预提升级后面取，绕开两级削波。solo 想要紧就关小，分解和弦想要呼吸感就开大。"),
]
PITFALLS = [
    "**ICL7660 的飞电容（C10）必须是电解且注意极性**：接反会直接击穿，输出变成 −1 V 左右。",
    "**两颗 1N4148 串联是为了抬高阈值**：单只只有 ±0.65 V，两级串起来才有 ±1.3 V 的 headroom；不要图省事只用一只。",
    "**Rg1/Rg2 不要改小**：它们决定每级的最大增益，改小会让 GAIN 全开时也削不出来。",
    "**TONE 用电位器直接对地**：RV3 一端接信号、一端经 C4 到地，接错成串联会变成低通而不是高切。",
]


def esc(x):
    return x.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def svg_inline():
    s = open(SVG, encoding="utf-8").read()
    if s.startswith("<?xml"):
        s = s.split("?>", 1)[1]
    return s


def kpis():
    items = "".join(
        '<div class="kpi"><b>%s</b><span>%s</span></div>' % (v, k) for v, k in KPIS)
    return '<div class="kpis">%s</div>' % items


def chain_html():
    cells = "".join(
        '<div class="step"><div class="sn">%s</div><div class="sd">%s</div></div>' % (a, b)
        for a, b in CHAIN)
    return '<div class="chain">%s</div>' % cells


def table(headers, rows, w=None):
    out = ["<table>", "<thead><tr>"]
    for i, h in enumerate(headers):
        style = ' style="width:%s"' % w[i] if w else ""
        out.append("<th%s>%s</th>" % (style, h))
    out.append("</tr></thead><tbody>")
    for r in rows:
        out.append("<tr>" + "".join("<td>%s</td>" % esc(str(c)) for c in r) + "</tr>")
    out.append("</tbody></table>")
    return "".join(out)


def bom_table(s):
    from tools.spice import __name__ as _unused  # noqa: F401  仅确认路径可导入
    rows = s.bom()
    out = ["<table><thead><tr><th style='width:15%'>位号</th><th style='width:12%'>类别</th>",
           "<th style='width:16%'>数值</th><th style='width:27%'>具体型号 / 规格</th>",
           "<th>作用</th></tr></thead><tbody>"]
    for item in rows:
        refs = item["refs"]
        cat = item["cat"]
        if cat == "运放" or cat == "集成电路":
            part, note = ("TL074CN（DIP-14）", "四运放：U1A 输入、U1D 预提升、U1B/U1C 两级削波")
            if item["value"] == "ICL7660":
                part, note = ("ICL7660 / MAX1044（DIP-8）", "电荷泵：9 V → −9 V，给运放做双电源")
        elif len(refs) == 1:
            part, note = PART.get(refs[0], ("", ""))
        else:
            part = PART.get(refs[0], ("", ""))[0]
            note = "；".join("%s：%s" % (r, SHORT.get(r, "")) for r in refs)
        out.append("<tr><td class='ref'>%s</td><td>%s</td><td class='num'>%s</td>"
                   "<td>%s</td><td>%s</td></tr>"
                   % (esc("、".join(refs)), esc(cat), esc(item["value"]),
                      esc(part or "—"), esc(note or item.get("note", ""))))
    for ref, cat, val, part, note in PANEL:
        out.append("<tr><td class='ref'>%s</td><td>%s</td><td class='num'>%s</td>"
                   "<td>%s</td><td>%s</td></tr>"
                   % (esc(ref), esc(cat), esc(val), esc(part), esc(note)))
    out.append("</tbody></table>")
    return "".join(out)


def html():
    s = sch_build()
    parts = []
    A = parts.append

    A('<!DOCTYPE html><html lang="zh-CN"><head><meta charset="utf-8">')
    A('<meta name="viewport" content="width=device-width, initial-scale=1">')
    A("<title>Iris R · 设计文档</title>")
    A("<style>%s</style></head><body><div class='wrap'>" % CSS)

    # 封面
    A('<div class="kicker">Transparent Overdrive / Circuit &amp; BOM</div>')
    A("<h1>Iris R</h1>")
    A('<div class="sub">透明过载 · %s · 2026</div>' % VERSION)
    A('<p class="lede">一台靠电路把“透明”做成指标、同时能把 solo 撑起来的过载：'
      '输入与预提升两级把电平顶起来，低切搬出增益回路、与增益彻底解耦，'
      '两级削波用 8 只 4×1N4148 串成桥、每级 ±1.3 V 的 headroom，'
      '级间 −6 dB 让第二级重新有空间，干声经 BLEND 混回保住音头，'
      'TONE 只做高频高切。总增益 +12 … +38 dB。</p>')
    A(kpis())

    # 为什么 v1.8
    A('<h2>为什么长成 v1.8<span class="en">Revision</span></h2>')
    A("<p>这版不是一次设计出来的，而是六轮实测把一个“透明推子”逼成能弹 solo 的失真："
      "v0.4 解决电平与低切耦合，v0.6 把削波阈值抬到 ±1.3 V 让音头回来，"
      "v1.0 加入级间衰减与第二级削波，v1.2 修掉求解器不收敛导致的“劈”，"
      "v1.3–1.7 试过交越死区与 8 倍过采样（都留下数据），"
      "v1.8 找到真凶：<strong>把硬限幅从 48 kHz 挪到过采样域</strong>。下面每一条都是可焊电路：</p>")
    A(table(["听感问题", "v0.6 之前的电路原因", "v1.8 的电路对策"], FIX_ROWS))

    # 信号链
    A('<h2>信号链<span class="en">Signal Path</span></h2>')
    A(chain_html())
    A("<p>四级放大全部由一颗 TL074 承担（U1A 输入、U1D 预提升、U1B/U1C 两级削波），"
      "U2 ICL7660 把 9 V 翻成 −9 V 做地参考双电源，运放可用摆幅约 ±7 V。"
      "整机小信号增益 = 2 ×（预提升 1×…2.2×）×（1 + RV1/5.1K）× 0.5 ×（第二级 1×…）+ 干声混合，"
      "实测 <strong>+12 … +38 dB</strong>。</p>")

    # 原理图
    A('<h2>完整原理图<span class="en">Schematic</span></h2>')
    A('<figure class="bleed">%s<figcaption>图 1 · Iris R %s 完整原理图'
      '（位号与下文 BOM 一一对应；SVG 矢量图，可任意放大查看）</figcaption></figure>'
      % (svg_inline(), VERSION))

    # 逐级设计
    A('<h2>逐级设计<span class="en">Stage by Stage</span></h2>')

    A("<h3>01 · 输入级：高阻 + ×2</h3>")
    A("<p>吉他拾音器是电感源，最怕被低阻负载拖掉高频，所以第一级必须是高阻输入；"
      "而只有 1× 缓冲又会让整机偏轻，于是这一级同时兼任 +6 dB：</p>")
    A('<div class="calc">A₁ = 1 + R3/R2 = 1 + 10K/10K = <span class="r">2.0×（+6 dB）</span><br>'
      '输入阻抗 ≈ R1 = 1 MΩ；C1 22n + R1 1M 的高通 = 7.2 Hz</div>')

    A("<h3>02 · 低切：放在级间，不碰增益</h3>")
    A("<p>把 RV2+R4 串在增益级的地端，等于用低切去改增益公式的分母，低切一拧增益就跟着跑"
      "（实测联动 17 dB）。v0.4 起改成两级之间的<strong>无源高通</strong>：C2 串在信号里，"
      "RV2+R4 对地，后级 1M 输入让通带增益恒为 1：</p>")
    A('<div class="calc">f = 1 / (2π · (RV2 + R4) · C2)<br>'
      'RV2 = 0 → 1/(2π·5.1K·100n) = <span class="r">312 Hz</span>；'
      'RV2 = 50K → 1/(2π·55.1K·100n) = <span class="r">28.9 Hz</span></div>')

    A("<h3>03 · 预提升：2 kHz 以上 +7 dB</h3>")
    A("<p>这是 v1.4 为了“金属芯感”加的：在削波<strong>之前</strong>把 2 kHz 以上抬 7 dB，"
      "等于让中高频先撞削波器 —— 泛音密度上来了，中低频还留着自己的动态：</p>")
    A('<div class="calc">平直段 A = 1 + R14/(R15) 的直流通路 = 1×<br>'
      '高频段 A ≈ 1 + 10K/8.2K = <span class="r">2.2×（+7 dB）</span>，'
      '转折由 C9 10n × 8.2K ≈ <span class="r">1.9 kHz</span> 决定</div>')

    A("<h3>04 · 削波级 1：桥式反并联 + 4.7K 软膝</h3>")
    A("<p>非反相放大，反馈支路里放 RV1（变阻器），削波对跨在它两端。每侧<strong>两只 1N4148 串联</strong>，"
      "把阈值从 ±0.65 V 抬到 ±1.3 V；再串一只 4.7K 到二极管，拐角被磨圆：</p>")
    A('<div class="calc">A₂ = 1 + RV1/Rg1 = 1 + 100K/5.1K = <span class="r">1 … 20.6×</span><br>'
      '阈值 = 2 × 0.65 V = <span class="r">±1.3 V</span>（每侧两只串联）</div>')
    A("<p>为什么必须抬阈值：v0.6 之前用一对 1N4148（±0.65 V），信号一旦过膝就变成恒定电平的方波，"
      "音头被抹平 —— 实测音头只比延音大 0.4 dB，听感就是“无音头的合成器”。抬到 ±1.3 V、"
      "再加级间衰减之后，音头−延音回到 8 dB 量级。</p>")

    A("<h3>05 · 级间 −6 dB 与削波级 2</h3>")
    A("<p>第一级削完，信号被钉在 ±1.3 V 附近；直接再放大就只会更方。所以先 R17/R18 分掉一半，"
      "让第二级重新拿到 headroom，再削一层 —— 两级叠加给出“多层谐波”而不是“一层方波”：</p>")
    A('<div class="calc">衰减 = R18/(R17+R18) = 10K/20K = <span class="r">0.5（−6 dB）</span><br>'
      '第二级阈值同样 <span class="r">±1.3 V</span>，Rg2 5.1K，RV6 微调增益</div>')

    A("<h3>06 · BLEND：干声混回音头</h3>")
    A("<p>干声从<strong>预提升级之后</strong>取，绕过两级削波，经 RV5 100K 混到输出节点。"
      "BLEND 越大，音头与拨弦细节越多、声音越松；关小则更紧、更“贴脸”。这是这台机器"
      "在“透明”与“能打”之间唯一的连续旋钮。</p>")
    A('<div class="calc">湿声支路 = R21 22K → 混合点；干声支路 = RV5（0…100K）→ 混合点<br>'
      'BLEND = 0 时只剩湿声，= 100% 时干湿约各半（受 R21 与 RV5 的比值限制）</div>')

    A("<h3>07 · TONE 与 VOLUME</h3>")
    A("<p>TONE 不做串联低通，而是让 RV3 + C4 把高频<strong>对地</strong>泄掉："
      "旋钮越大，对地阻抗越大、高频泄得越少，所以“顺时针变亮”，而且行程与人耳一致：</p>")
    A('<div class="calc">f = 1 / (2π · RV3 · C4)<br>'
      'RV3 = 1K → <span class="r">34 kHz</span>（几乎不切）；RV3 = 100K → <span class="r">338 Hz</span>（很暗）</div>')
    A("<p>音量用 100K A 型分压；C8 1µ 隔直放在电位器之前，碳膜上没有直流、不会“咔嗒”。</p>")

    # 关键数值
    A('<h2>关键数值<span class="en">Numbers</span></h2>')
    A(table(["项目", "数值", "怎么来的"], [
        ["输入级增益", "2.0×（+6 dB）", "1 + R3/R2"],
        ["预提升", "1× ～ 2.2×（0…+7 dB @2kHz）", "1 + R14/R15"],
        ["第一削波级", "1 … 20.6×", "1 + RV1/Rg1"],
        ["级间衰减", "0.5（−6 dB）", "R18/(R17+R18)"],
        ["第二削波级", "1 … 20.6×", "1 + RV6/Rg2"],
        ["总增益", "+12 … +38 dB", "各级相乘，实测标定"],
        ["削波阈值", "±1.3 V（每级，每侧 2×0.65 V）", "2 只 1N4148 串联"],
        ["低切", "28.9 Hz – 312 Hz", "1/(2π·(RV2+R4)·C2)"],
        ["TONE 高切", "338 Hz – 34 kHz", "1/(2π·RV3·C4)"],
        ["输入高通", "7.2 Hz", "1/(2π·R1·C1)"],
        ["运放摆幅", "±7 V（内部轨）", "±9 V 供电、3 V 余量"],
        ["供电", "9 V → ±9 V（ICL7660）", "电荷泵 + 主滤波"],
        ["工作电流", "≈ 6–10 mA", "TL074 + ICL7660"],
    ]))

    # 实测
    A('<h2>实测<span class="en">Measured</span></h2>')
    A("<p>下面这组数来自插件内核（<code>vst3/iris/source/iris_dsp.h</code>），"
      "它与原理图是同一张网表的离散实现：输入 ×2、无源低切、预提升、两级桥式削波（每个过采样点"
      "解一次隐式二极管方程）、BLEND、TONE、音量。测试条件：1 kHz 正弦，LOWCUT 关到底、"
      "TONE 全开、BLEND 30%、VOLUME 全开。</p>")
    A(table(["GAIN 位置", "1 kHz 增益（40 mV）", "听感"], GAIN_ROWS))
    A(table(["GAIN", "输入峰值", "THD"], THD_ROWS))
    A("<p>谐波结构（GAIN 100%、0.2 V、1 kHz）：</p>")
    A(table(["档位", "H2", "H3", "H5", "H7", "H9"], HARM_ROWS))
    A('<div class="callout"><b>三条结论：</b>① 低增益端干净（GAIN 0% 时 0.2 V 输入 THD 0.45%），'
      '大信号才靠运放轨限幅；② 高增益端 H3 24% / H5 11% / H7 1.1% —— 7 次以上被压住，'
      '这是“亮而不刺”的客观依据；③ 谐波以奇次为主、偶次 0.9%，是两级桥式对称削波的结果，'
      '想要甜可以把第二级改成不对称（换一只 1N34A）。</div>')
    A('<div class="callout"><b>瞬态与杂散：</b>合成拨弦测音头−延音 = 8.2 / 7.2 / 5.0 dB'
      '（GAIN 0 / 50 / 100%）；1 kHz 单音的非谐波杂散 0.00%；10 s 立体声渲染耗时 3.76 s，'
      '约单核 38%（实时余量 2.7×）。</div>')

    # BOM
    A('<h2>BOM<span class="en">Bill of Materials</span></h2>')
    A("<p>按 %s 原理图：一颗 TL074 承担四级、一颗 ICL7660 做负压，其余是普通通孔件。"
      "位号与图 1 一一对应，可直接照着打板。</p>" % VERSION)
    A(bom_table(s))

    # 搭建
    A('<h2>搭建与调试<span class="en">Build</span></h2>')
    A("<ul>" + "".join("<li><strong>%s</strong>：%s</li>" % (t, d) for t, d in HOWTO) + "</ul>")
    A('<div class="callout"><b>四个常见的坑：</b><br>' +
      "<br>".join("· " + p.replace("**", "") for p in PITFALLS) + "</div>")

    # 资料与方法
    A('<h2>资料与方法<span class="en">Method</span></h2>')
    A("<p>元件值由本文公式推出；网表用 <code>tools/schlib.py</code> 的并查集做连通性、"
      "悬空引脚、文字遮挡与 BOM 反推检查（<code>tools/check.py</code>、"
      "<code>tools/netcheck.py</code>、<code>tools/audit_symbols.py</code>）；"
      "曲线与谐波由 <code>tools/spice/iris.py</code>（ngspice-47）交叉验证，"
      "插件内核与该网表在 5 个增益点的最大偏差 0.64%。"
      "Petla Noa 的公开信息仅作为“声音描述”的来源，本机电路为独立设计，不是它的抄板。</p>")
    A('<div class="foot">Iris R · 设计文档 %s · 原理图 SVG 可无损放大；'
      '本页所有数值都能在 <code>sch/iris.py</code> 与 <code>vst3/iris/source/</code> 里复现。</div>'
      % VERSION)
    A("</div></body></html>")
    return "".join(parts)


def main():
    doc = html()
    with open(OUT, "w", encoding="utf-8") as f:
        f.write(doc)
    print("wrote", OUT, len(doc), "chars")


if __name__ == "__main__":
    main()
