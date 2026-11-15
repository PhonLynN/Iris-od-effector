# Iris R rev11-opt — 误差优化首选出料表（BOM）

> 版本：rev11-opt（误差优化版，Light / British 双模式；基线见 `BOM_rev11.md`）
> 说明：本表为**误差优化版**第一选择；替代料见 `Sourcing_rev11_opt.md`。
> 平台：淘宝 = 首选；京东 = 标准件/正品；拼多多 = 便宜被动件。实时库存下单前复核。

## 1. 电阻（Vishay RN55E，1/4W，0.1%，25ppm；非关键可用 RN55D 1%）

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| R1 | 1M | 1 | 输入偏置 |
| R2 | 1M | 1 | U1A +in 偏置 |
| R3 | 10k | 1 | U1A 反馈 |
| R4 | 5.1k | 1 | 低切下限 |
| R14 | 10k | 1 | preE 反馈 |
| R15 | 22k | 1 | preE 低频增益 |
| Rg1 | 3.3k | 1 | U1B 增益 |
| R17 | 10k | 1 | 级间上臂 |
| R18 | 5.1k | 1 | 级间下臂 |
| Rg2 | 3.3k | 1 | U1C 增益 |
| Rfb2 | 20k | 1 | U1C 反馈 |
| R21 | 47k | 1 | 湿声混音 |
| Rsh | 12k | 1 | shelf 下臂 |
| R1L | 20k | 1 | 5.4k LPF 上臂 |
| R2L | 20k | 1 | 5.4k LPF 下臂 |
| Rload | 10k | 1 | 输出负载/测试 |
| R9 | 100 | 1 | 电源滤波 |
| Rjs | 430 | 1 | JFET 源极 |
| Rsag | 1k | 1 | JFET 电源 sag |
| Rd | 1.8k | 1 | JFET 漏极 |
| Rts | 10k | 1 | taper 上串联 |
| Rbs | 10k | 1 | taper 下并联 |
| Rg | 4.7k | 1 | taper 下臂 |
| Rsp | 8.7k | 1 | shelf 并联 |
| Rfb1 | 10k | 1 | U1B 反馈串联 |
| RLED | 4.7k | 1 | LED 限流 |
| Rpad1 | 10k | 1 | 输出 pad 串联 |
| Rpad2 | 4.7k | 1 | 输出 pad 对地 |

首选型号：`RN55E` 0.1%（如 `RN55E1002BRE6` = 10k）；非关键路径可用 `RN55D` 1%。8.7k 用 0.1% E96 8.66k，或 1% E24 8.7k。

## 2. 电容

| Ref | Value | Qty | 类型/型号 | 说明 |
|---|---|---:|---|---|
| C1 | 22n | 1 | WIMA FKP2 100V 1% | 输入耦合 |
| C2 | 100n | 1 | WIMA FKP2 100V 1% | 低切 |
| C3 | 100n | 1 | WIMA FKP2 100V 1% | U1D→U1B 耦合 |
| C4 | 4.7n | 1 | WIMA FKP2 100V 1% | TONE |
| C6 | 100u | 1 | Panasonic FC 25V 10% 精选 | 电源 |
| C8 | 1u | 1 | WIMA MKS2 50V 2.5% | 音量耦合 |
| C9 | 100p | 1 | WIMA FKP2 100V 1% | preE 带限 |
| C10 | 10u | 1 | Panasonic FC 25V 10% 精选 | 泵电容 |
| C12 | 10u | 1 | Panasonic FC 25V 10% 精选 | -9V 滤波 |
| C1L | 2.2n | 1 | WIMA FKP2 100V 1% | LPF |
| C2L | 1n | 1 | WIMA FKP2 100V 1% | LPF |
| Cs | 56n | 1 | WIMA MKP2 100V 2.5% | shelf |
| Cin | 22n | 1 | WIMA FKP2 100V 1% | JFET 输入 |
| Cout | 1u | 1 | WIMA MKS2 50V 2.5% | JFET 输出 |
| Cd | 2.2n | 1 | WIMA FKP2 100V 1% | JFET 漏极低通 |
| Csag | 47u | 1 | Panasonic FC 25V 10% 精选 | JFET sag |

## 3. 二极管 / 晶体管

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| D1a, D1b, D2a, D2b, D3a, D3b, D4b | 1N4148 配对 | 7 | 削波/反馈；Vf 分选 |
| D4a | 1N34A 精选 | 1 | Ge 非对称；Vf/漏电分选 |
| D9 | 1N4007 | 1 | 电源反接 |
| LED | 3mm 红 | 1 | 指示灯 |
| Q1 | LSK170-B | 1 | 低噪声 JFET；Idss 分档，RVbias 可调 |

## 4. 运放 / 电源

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| U1 | OPA4134PA | 1 | 四运放，DIP-14；低噪声/低失真 |
| U3 | OPA4134PA | 1 | 四运放，DIP-14 |
| U2 | MAX1044CPA | 1 | -9V 电荷泵；低噪声，pin1 BOOST |

## 5. 电位器 / trim

| Ref | Value | Qty | 型号 | 说明 |
|---|---|---:|---|---|
| RV1 | B100k 双联 10% | 1 | Bourns PTV142-4420A-B104 | gain + shelf |
| RV2 | B50k 10% | 1 | Bourns PTV112-4420A-B503 | LOWCUT |
| RV3 | C100k 10% | 1 | Bourns PTV112-4420A-C104 | TONE |
| RV4 | B100k 10% | 1 | Bourns PTV112-4420A-B104 | VOLUME |
| RV5 | B100k 10% | 1 | Bourns PTV112-4420A-B104 | BLEND |
| RV1T | 2k trim | 1 | Bourns 3296W-1-202LF | gain taper 对中 |
| RV1S | 2k trim | 1 | Bourns 3296W-1-202LF | shelf taper 对中 |
| RVbias | 10k trim | 1 | Bourns 3296W-1-103LF | JFET 偏置 |
| RV3T | 10k trim | 1 | Bourns 3296W-1-103LF | TONE 10%→1% 对中 |
| RV5T | 10k trim | 1 | Bourns 3296W-1-103LF | BLEND 10%→1% 对中 |

## 6. 接插件 / 结构

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| J1, J2 | Switchcraft 12B | 2 | 1/4" 封闭插孔 |
| SW1 | 3PDT | 1 | true bypass |
| SW2 | DPDT | 1 | Light/British 模式 |
| DC1 | Lumberg 1614 02 | 1 | 2.1mm 中心正 |
| BOX | Hammond 125B | 1 | 压铸铝壳 |
| KNOB | 6.35mm | 5 | 旋钮 |
| PCB | — | 1 | 嘉立创/PCBWay |

## 7. 备注

- 输出 pad：`Rpad1 10k` 串联、`Rpad2 4.7k` 对地，约 -9.9 dB；与 DSP 的 `kOutTrim` 对应。
- Q1 改为 LSK170-B（Idss 分档更紧）；2N5457 仅作替代，假货多，必须测 Idss/Vp。
- 1N34A 仍写首选出料，但必须 Vf/漏电分选；替代 D9E / 1N60 也要分选。
- `RV1` 用 Bourns PTV142 10% 双联；轴长/引脚间距按 PCB 选。若只能买 Alpha RV16AF 20%，用 RV1T/RV1S 对中。
- TONE/BLEND 由 20% 换 10% 后，再加 RV3T/RV5T 把关键点对到 ~1%；这是 8k/300Hz 误差的主修复。
- 9V 电源建议用稳压/低噪适配器；9V 波动会直接改 JFET 偏置和 THD。
