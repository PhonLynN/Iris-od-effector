# Iris R rev11 — 首选出料表（BOM）

> 版本：rev11（硬件基线，Light / British 双模式）
> 说明：本表只列**第一选择**，不含替代料。替代料见 `Sourcing_rev11.md`。
> 平台：淘宝 = 首选；京东 = 标准件/正品；拼多多 = 便宜被动件。实时库存下单前复核。

## 1. 电阻（Vishay RN55D，1/4W，1%，50ppm）

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

首选型号：`RN55D` 系列（如 `RN55D1002FRE6` = 10k）。8.7k 可用 1% E24；若只买 E96，用 8.66k。

## 2. 电容

| Ref | Value | Qty | 类型/型号 | 说明 |
|---|---|---:|---|---|
| C1 | 22n | 1 | WIMA FKP2 100V 2.5% | 输入耦合 |
| C2 | 100n | 1 | WIMA FKP2 100V 5% | 低切 |
| C3 | 100n | 1 | WIMA FKP2 100V 5% | U1D→U1B 耦合 |
| C4 | 4.7n | 1 | WIMA FKP2 100V 2.5% | TONE |
| C6 | 100u | 1 | Panasonic FC 25V | 电源 |
| C8 | 1u | 1 | WIMA MKS2 50V 5% | 音量耦合 |
| C9 | 100p | 1 | WIMA FKP2 100V 2.5% | preE 带限 |
| C10 | 10u | 1 | Panasonic FC 25V | 7660 泵电容 |
| C12 | 10u | 1 | Panasonic FC 25V | -9V 滤波 |
| C1L | 2.2n | 1 | WIMA FKP2 100V 2.5% | LPF |
| C2L | 1n | 1 | WIMA FKP2 100V 2.5% | LPF |
| Cs | 56n | 1 | WIMA MKP2 100V 5% | shelf |
| Cin | 22n | 1 | WIMA FKP2 100V 5% | JFET 输入 |
| Cout | 1u | 1 | WIMA MKS2 50V 5% | JFET 输出 |
| Cd | 2.2n | 1 | WIMA FKP2 100V 2.5% | JFET 漏极低通 |
| Csag | 47u | 1 | Panasonic FC 25V | JFET sag |

## 3. 二极管 / 晶体管

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| D1a, D1b, D2a, D2b, D3a, D3b, D4b | 1N4148 | 7 | 削波/反馈 |
| D4a | 1N34A | 1 | Ge 非对称 |
| D9 | 1N4007 | 1 | 电源反接 |
| LED | 3mm 红 | 1 | 指示灯 |
| Q1 | 2N5457 | 1 | JFET mu-amp |

## 4. 运放 / 电源

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| U1 | TL074CN | 1 | 输入/两级削波 |
| U3 | TL074CN | 1 | 缓冲/shelf/LPF/输出 |
| U2 | ICL7660CPA | 1 | -9V 电荷泵 |

## 5. 电位器 / trim

| Ref | Value | Qty | 型号 | 说明 |
|---|---|---:|---|---|
| RV1 | B100k 双联 | 1 | Alpha RV16AF 双联 | gain + shelf |
| RV2 | B50k | 1 | Alpha RV16 | LOWCUT |
| RV3 | C100k | 1 | Alpha RV16 | TONE |
| RV4 | B100k | 1 | Alpha RV16 | VOLUME |
| RV5 | B100k | 1 | Alpha RV16 | BLEND |
| RV1T | 2k trim | 1 | Bourns 3296W-1-202LF | gain taper 对中 |
| RV1S | 2k trim | 1 | Bourns 3296W-1-202LF | shelf taper 对中 |
| RVbias | 10k trim | 1 | Bourns 3296W-1-103LF | JFET 偏置 |

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
- 2N5457 假货多；首选出料仍写 2N5457，但建议按替代表准备 LSK170-B / J201。
- 1N34A 假货多；首选出料仍写 1N34A，但建议按替代表准备 D9E / 1N60。
- `RV1` 双联的轴长/引脚间距按实际 PCB 选；Alpha RV16AF 双联不是每个店都有。
