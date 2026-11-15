# Iris R rev11 — 采购/替代料矩阵

> 说明：这是 BOM 的采购补充，不是第二份 BOM。首选见 `Iris-od-effector/build/BOM_rev11.md`。
> 平台只列“能买到”的主渠道；实时库存/批次下单前复核。DIY 小批量不必强求三平台都有。

## 采购原则

- **淘宝**：小批量、发烧件、双联电位器、JFET/Ge、WIMA/RN55D 的主渠道。
- **京东**：标准件/正品（TL074、ICL7660、Panasonic FC、Switchcraft/Neutrik、Hammond）更稳，价格高。
- **拼多多**：普通被动件（1N4148、电阻、电容、LED、旋钮、3PDT、DC 座）便宜，假货/混批风险最高，到货必须抽测。
- **嘉立创/PCBWay**：PCB，不属于上述三平台。

## 关键件

| 件 | 首选 | 主渠道 | 可替代 | 风险/备注 |
|---|---|---|---|---|
| RV1 B100k 双联 | Alpha RV16AF 双联 | 淘宝 | Bourns PTV112-4420A-B104；BI 双联 | 不是每家都有；轴长/引脚间距按 PCB 选；20% 容差用 RV1T/RV1S 对中 |
| RV1T/RV1S 2k trim | Bourns 3296W-1-202LF | 淘宝/京东 | 3296W-202（无 LF）；多圈 2k | 10%；对中 gain/shelf 5 点 |
| RVbias 10k trim | Bourns 3296W-1-103LF | 淘宝/京东 | 3296W-103；10k 多圈 | 2N5457 偏置必留 |
| RV2 B50k / RV3 C100k / RV4 B100k / RV5 B100k | Alpha RV16 | 淘宝 | 任何 16mm 单联；C100k 少见但标准 | C100k 20% 是 8k 主误差；要更紧选 10% 或 TONE trim |
| Q1 2N5457 | 2N5457 TO-92 | 淘宝（NOS） | LSK170-B、J201（重新偏置） | 假货多；必须测 Idss/Vp；RVbias 可调 |
| D4a 1N34A | 1N34A Ge | 淘宝 | D9E、1N60（Ge） | 假货多；Vf/漏电抽测；与 D4b Si 配对 |
| D1–D3/D4b 1N4148 | 1N4148 | 淘宝/京东/拼多多 | 1N4148W（贴片，需转接） | 首选直插 |
| U1/U3 TL074 | TL074CN | 淘宝/京东 | TL074IN；ST/TI 正品 | 避免打磨片；U3 用于缓冲/LPF |
| U2 ICL7660 | ICL7660CPA | 淘宝/京东 | ICL7660SCPA；MAX1044 | 10u 泵电容靠近引脚 |
| R1–Rpad RN55D 1% | Vishay RN55D | 淘宝 | KOA MF1/4、Vishay MRS25 | 8.7k 用 8.66k（E96）或 1% E24 |
| C1/C2/C3/C4/C9/C1L/C2L/Cd | WIMA FKP2 100V | 淘宝 | WIMA MKP2、Epcos B32620、Panasonic ECW | 2.5%/5%；56n 用 MKP2 |
| C8/Cout 1u | WIMA MKS2 50V | 淘宝 | Panasonic ECW、EPCOS MKT | 5% |
| C6/C10/C12/Csag | Panasonic FC 25V | 淘宝/京东/拼多多 | Nichicon PW、Rubycon ZLH | 100u/10u/47u；低 ESR |
| J1/J2 | Switchcraft 12B | 淘宝/京东 | Neutrik NYS229、Rean | 封闭式 |
| SW1 3PDT | Taiway 3PDT | 淘宝 | 任何 3PDT true bypass | 脚距 |
| SW2 DPDT | Taiway DPDT | 淘宝 | 任何 DPDT | Light/British |
| DC1 | Lumberg 1614 02 | 淘宝 | Neutrik NEB/J、Switchcraft 712A | 2.1mm 中心正 |
| BOX | Hammond 125B | 淘宝/京东 | 任何 125B 压铸铝 | 喷涂/丝印 |
| KNOB | 6.35mm | 淘宝/拼多多 | 任何 6.35mm | 5 个 |
| PCB | — | 嘉立创/PCBWay | — | 不属于三平台 |

## 到货验收（DIY 最低）

1. **双联电位器**：两联同角度阻值差 <5%；5 点（0/25/50/75/100%）抽测。
2. **2N5457**：Idss/Vp 抽测；假货常见“重印”；偏置可调但 gm 不对会没增益。
3. **1N34A**：Vf 0.2–0.35 V、反向漏电；与 1N4148 混用前分选。
4. **RN55D**：万用表抽 5%；WIMA 抽 2.5%/5% 标称。
5. **TL074/ICL7660**：上电测 ±9V；TL074 输出不要顶轨。

## 误差提醒

- TONE C100k 20% → 8k 约 ±2.36 dB（最大单件）；要更紧选 10% C100k 或加 TONE trim。
- BLEND 20% → 300 Hz/1k 约 ∓0.9/0.6 dB。
- RV1T/RV1S 用于把 gain/shelf 5 点对中；RVbias 必留。
- Ge/Si 配对影响 H2/H3；2N5457 的 Idss 影响 THD 和 headroom。
