# Iris R rev11-opt — 采购/替代料矩阵（误差优化版）

> 首选见 `Iris-od-effector/build/BOM_rev11_opt.md`；基线见 `BOM_rev11.md`。
> 目标：把 8 k 的 TONE 20%、300/1 k 的 BLEND 20%、2.5/5% 电容、1% 电阻、JFET/Ge 离散性压下来。
> 平台：淘宝=发烧件/小批量主渠道；京东=标准件正品；拼多多=普通被动件。实时库存下单前复核。

## 关键替换

| 件 | 优化后首选 | 主渠道 | 可替代 | 风险/备注 |
|---|---|---|---|---|
| RV1 B100k 双联 | Bourns PTV142-4420A-B104（10%） | 淘宝 | Alpha RV16AF 20% + RV1T/RV1S 对中；BI 双联 | PTV142 14mm 双联不是每家都有；轴长/脚距按 PCB |
| RV3 C100k | Bourns PTV112-4420A-C104（10%） | 淘宝 | Alpha RV16 C100k 20% + RV3T；10% C100k 单联 | 8 k 主误差；10% 后 ±1.2 dB，RV3T 对到 ~1% |
| RV5 B100k | Bourns PTV112-4420A-B104（10%） | 淘宝 | Alpha RV16 B100k 20% + RV5T | 300/1 k 主误差；10% 后约减半 |
| RV2 B50k / RV4 B100k | Bourns PTV112-4420A-B503 / B104（10%） | 淘宝 | Alpha RV16 20% | 音量/低切；10% 更稳 |
| RV1T/RV1S 2k trim | Bourns 3296W-1-202LF（10%） | 淘宝/京东 | 3296W-202；多圈 2k | 对中 gain/shelf 5 点；必留 |
| RV3T/RV5T 10k trim | Bourns 3296W-1-103LF（10%） | 淘宝/京东 | 3296W-103；多圈 10k | **新增**：把 TONE/BLEND 从 10% 对到 ~1% |
| RVbias 10k trim | Bourns 3296W-1-103LF（10%） | 淘宝/京东 | 3296W-103 | JFET 偏置必留 |
| Q1 JFET | LSK170-B（TO-92，Idss 分档） | 淘宝 | 2N5457（测 Idss/Vp）；J201（重偏置） | LSK170-B 更紧；pinout 按实际 |
| D4a Ge | 1N34A 精选（Vf/漏电） | 淘宝 | D9E、1N60（也要分选） | 假货多；Vf 0.2–0.35 V 抽测 |
| D1–D3/D4b Si | 1N4148 配对 | 淘宝/京东/拼多多 | BAV21（Vf 略高） | Vf 分选，Ge/Si 配对 |
| U1/U3 | OPA4134PA（DIP-14） | 淘宝 | TL074CN（原设计）；LME49740 | OPA4134 低噪声/低失真；±9 V 可用 |
| U2 | MAX1044CPA | 淘宝/京东 | ICL7660CPA；LTC1044 | pin1 BOOST；10 u 泵电容靠近 |
| R1–Rpad | Vishay RN55E 0.1% | 淘宝 | RN55D 1%；KOA MF1/4 0.1% | 关键路径 0.1%；非关键可 1% |
| C1/C2/C3/C4/C9/C1L/C2L/Cd/Cin | WIMA FKP2 100V 1% | 淘宝 | MKP2 1%；Amtrans AMCH 1%；COG 1%（小值） | 1% 需指定/抽测；2.5% 是普通货 |
| C8/Cout 1u | WIMA MKS2 50V 2.5% | 淘宝 | Panasonic ECW 2.5% | 耦合，低端 |
| Cs 56n | WIMA MKP2 100V 2.5% | 淘宝 | FKP2 56n 2.5% | shelf |
| C6/C10/C12/Csag | Panasonic FC 25V 10% 精选 | 淘宝/京东/拼多多 | Nichicon UFG 10% 精选 | 20% 普通货要抽测；Csag 影响 sag |
| J1/J2 | Switchcraft 12B | 淘宝/京东 | Neutrik NYS229、Rean | 封闭式 |
| SW1 3PDT | Taiway 3PDT | 淘宝 | 任何 3PDT true bypass | 脚距 |
| SW2 DPDT | Taiway DPDT | 淘宝 | 任何 DPDT | Light/British |
| DC1 | Lumberg 1614 02 | 淘宝 | Neutrik NEB/J、Switchcraft 712A | 2.1 mm 中心正 |
| BOX | Hammond 125B | 淘宝/京东 | 任何 125B 压铸铝 | 喷涂/丝印 |
| KNOB | 6.35 mm | 淘宝/拼多多 | 任何 6.35 mm | 5 个 |
| PCB | — | 嘉立创/PCBWay | — | 不属于三平台 |
| 9V 电源 | 稳压/低噪 9V 适配器 | 淘宝/京东 | 电池（注意 sag） | 9 V 波动直接改 JFET 偏置/THD |

## 到货验收（优化版加严）

1. **双联电位器**：两联同角度阻值差 <3%；5 点（0/25/50/75/100%）抽测。
2. **TONE/BLEND 电位器**：10% 到货抽 5 点；装板后用 RV3T/RV5T 把关键点对到 ~1%。
3. **LSK170-B**：Idss 分档抽测；RVbias 调到目标 Id；2N5457 替代必须测 Idss/Vp。
4. **1N34A**：Vf 0.2–0.35 V、反向漏电；与 1N4148 配对。
5. **0.1% 电阻**：万用表 4.5 位抽 5 点；1% 非关键可放宽。
6. **1% 电容**：指定 1% 或到货抽测；普通 2.5% 不能当 1% 用。
7. **OPA4134/MAX1044**：上电测 ±9 V；OPA4134 输出不要顶轨；MAX1044 pin1 处理正确。

## 误差提醒

- TONE 20% → 8 k ±2.36 dB；10% → 约 ±1.2 dB；再加 RV3T 对中可到 ~±0.3–0.6 dB（以实测为准）。
- BLEND 20% → 300 Hz ∓0.91 dB；10% → 约减半；RV5T 对中。
- RV1 双联 20% → 10% 改善 5 点；RV1T/RV1S 只对中，不修两联跟踪。
- JFET/Ge/Si/9 V 是非线性误差，线性 Monte Carlo 不覆盖；靠选件、配对、偏置 trim 和稳压电源。
- 1% 电容、0.1% 电阻是“把剩余小项压下去”；主项仍是 TONE/BLEND 电位器和 JFET/Ge 离散性。
