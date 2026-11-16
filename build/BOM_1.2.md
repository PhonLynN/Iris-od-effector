# Iris R 1.2 — A 9-opamp 首选出料表（BOM，DSP-first）

> 版本：1.2（A 9-opamp，2×OPA4134 + U4）；MFB twin-T 3k（TONE 联动）；DSP 是唯一真值。
> A 方案以 `build/A_topology.md` 为准；本表已并入 U4/3k 与 preE SVF，删除旧 preE 高架 R14/R15/C9。
> 容差与核对：见 `build/A_topology.md`；历史归档在 `work/archive_2026-11-16/`。

## 1. 电阻（preE SVF / 3k twin-T / TONE / SHELF 关键电阻 0.1% 25ppm；其余 RN55D 1% 即可）

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| R1 | 1M | 1 | 输入 HPF/偏置（7.2 Hz） |
| R2 | 1M | 1 | U1A +in 偏置 |
| R3 | 10k | 1 | U1A 反馈，×2 |
| R4 | 5.1k | 1 | LOWCUT 下限 |
| R22 | 80.6k | 1 | preE SVF R1（0.1%） |
| R23 | 80.6k | 1 | preE SVF R2（0.1%） |
| R24 | 71.5k | 1 | preE SVF R3（0.1%） |
| R25 | 80.6k | 1 | preE SVF R4（0.1%） |
| R26 | 715k | 1 | preE SVF R5/Q（0.1%） |
| R27 | 1M | 1 | preE SVF R6/Q（0.1%） |
| Rair | 27k | 1 | AIR 高架（6 kHz） |
| Rair2 | 82k | 1 | AIR 修值（A） |
| Rg1 | 3.9k | 1 | U1B 同相输入电阻 |
| Rser | 10k | 1 | U1B 削波串联 |
| R17 | 10k | 1 | U1C 输入分压上臂 |
| R18 | 5.4k | 1 | U1C 输入分压下臂（div 0.35） |
| Rg2 | 10k | 1 | U1C 同相输入 |
| Rfb2 | 20k | 1 | U1C 反馈 |
| Rser2 | 10k | 1 | U1C 削波串联 |
| R21 | 47k | 1 | 湿声混音 |
| Rbass | 220 | 1 | BASS 低通电阻（220 Hz） |
| Rbass2 | 11.8k | 1 | BASS 注入 0.835（U1C+ 3.51k） |
| Rlp | 10k | 1 | POST LP 8.84 kHz |
| Rf | 20k | 1 | U3A 恢复 ×3 反馈 |
| Rg | 10k | 1 | U3A 恢复 ×3 地端 |
| R13 | 8.2k | 1 | TONE 高架串联（C4 10n，1.94 kHz；0.1%） |
| Rsw3 | 1M | 1 | SW3 公共端偏置 |
| Rsp | 8.25k | 1 | SHELF 并联（0.1%） |
| Rsh | 11.5k | 1 | SHELF 下臂（0.1%） |
| Rpad1 | 10k | 1 | 输出 pad 串联（0.30） |
| Rpad2 | 4.3k | 1 | 输出 pad 对地（0.30） |
| Rload | 10k | 1 | 输出负载/测试，DNP |
| R9 | 100 | 1 | 电源滤波 |
| RLED | 4.7k | 1 | LED 限流 |
| Rin | 10k | 1 | U4 MFB 输入（A） |
| R3K1 | 10k | 1 | twin-T R1（0.1%） |
| R3K2 | 10k | 1 | twin-T R2（0.1%） |
| R3K3 | 4.99k | 1 | twin-T R3（0.1%） |
| R3K4 | 10k | 1 | U4 反馈 Rf |
| R3K5 | 100k | 1 | twin-T 中心到 U4 输出（与 RV3B 组成深度/Q 反馈） |

## 2. 电容

| Ref | Value | Qty | 类型/型号 | 说明 |
|---|---|---:|---|---|
| C1 | 22n | 1 | WIMA FKP2 100V 1% | 输入耦合/7.2 Hz |
| C2 | 100n | 1 | WIMA FKP2 100V 1% | LOWCUT |
| C3 | 100n | 1 | WIMA FKP2 100V 1% | U1D→U1B 耦合 |
| C4 | 10n | 1 | WIMA FKP2 100V 1% | TONE 高架 1.94 kHz |
| C14 | 1n | 1 | WIMA FKP2 100V 1% | preE C1（1%） |
| C15 | 1n | 1 | WIMA FKP2 100V 1% | preE C2（1%） |
| C6 | 100u | 1 | Panasonic FC 25V | 电源 |
| C8 | 1u | 1 | WIMA MKS2 50V | VOLUME 耦合 |
| C13 | 1u | 1 | WIMA MKS2 50V | SW3 防 pop |
| C10 | 10u | 1 | Panasonic FC 25V | 泵电容 |
| C12 | 10u | 1 | Panasonic FC 25V | −9V 滤波 |
| C1L | 1.8n | 1 | WIMA FKP2 100V 1% | POST LP 8.84 kHz |
| Cs | 56n | 1 | WIMA MKP2 100V 2.5% | SHELF |
| Cair | 1n | 1 | WIMA FKP2 100V 1% | AIR 高架 6 kHz |
| Cbass | 3.3u | 1 | WIMA MKS2 50V 5% | BASS 低通；−3 dB ≈222 Hz |
| C16 | 100n | 1 | X7R 50V | U4 V+ 去耦 |
| C17 | 100n | 1 | X7R 50V | U4 V− 去耦 |
| C3K1 | 5.1n | 1 | WIMA FKP2 100V 1% | twin-T C1（1%） |
| C3K2 | 5.1n | 1 | WIMA FKP2 100V 1% | twin-T C2（1%） |
| C3K3 | 10n | 1 | WIMA FKP2 100V 1% | twin-T C3（1%） |

## 3. 二极管 / 指示灯

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| D1a, D1b, D2a, D2b, D3a, D3b, D4b, D4c | 1N4148 配对 | 8 | U1B/U1C 各 2×Si 对称削波；Vf 分选 |
| D9 | 1N4007 | 1 | 电源反接 |
| LED | 3mm 红 | 1 | 指示灯 |

## 4. 运放 / 电源

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| U1 | OPA4134PA | 1 | 四运放：U1A 输入、U1B 削波 1、U1C 削波 2、U1D preE 第一只 |
| U3 | OPA4134PA | 1 | 四运放：U3A 恢复+TONE、U3B SHELF、U3C preE 第二只、U3D VOLUME |
| U4 | OPA1641PA / OPA134PA | 1 | 单运放：post-pad MFB twin-T 3k（TONE 联动）；DIP-8/SOIC-8；±9V |
| U4-SOCKET | 8-pin DIP socket | 1 | U4 |
| U2 | MAX1044CPA | 1 | −9V 电荷泵 |

## 5. 电位器 / trim

| Ref | Value | Qty | 型号 | 说明 |
|---|---|---:|---|---|
| RV1 | B100k 双联 10% | 1 | Bourns PTV142-4420A-B104 | RV1A U1B 增益；RV1B SHELF 深度（联动） |
| RV2 | B50k 10% | 1 | Bourns PTV112-4420A-B503 | LOWCUT |
| RV3 | C100k 双联 10% | 1 | Bourns PTV142-4420A-C104 | RV3A TONE 1.8 kHz 高架；RV3B 3k 深度（TONE 联动） |
| RV4 | A100k 10% | 1 | Bourns PTV112-4420A-A104 | VOLUME（x²） |
| RV5 | B50k 10% | 1 | Bourns PTV112-4420A-B503 | BLEND 0…50% |
| RV1T | 2k trim | 1 | Bourns 3296W-1-202LF | 增益 taper 对中 |
| RV1S | 2k trim | 1 | Bourns 3296W-1-202LF | SHELF taper 对中 |

## 6. 接插件 / 结构

| Ref | Value | Qty | 说明 |
|---|---|---:|---|
| J1, J2 | Switchcraft 12B | 2 | 1/4" 封闭插孔 |
| SW1 | 3PDT | 1 | true bypass |
| SW3 | SPDT ON-ON | 1 | SHELF/FLAT；Mountain MTS-102 或 Taiway 100-SPDT；面板 4.8mm |
| DC1 | Lumberg 1614 02 | 1 | 2.1mm 中心正 |
| BOX | Hammond 125B | 1 | 压铸铝壳 |
| KNOB | 6.35mm | 5 | 旋钮 |
| PCB | — | 1 | 嘉立创/PCBWay |

## 7. 备注

- DSP 顺序：输入 ×2 → LOWCUT → preE 2k/+7 → AIR 6k/+0.35 → U1B 削波 → BASS 220/+0.85 注入 → U1C 削波（div 0.35）→ POST LP 8.84k → 恢复 ×3 → TONE 1.94k 一阶 → SHELF 300 → VOLUME → pad 0.30 → U4 MFB twin-T 3k（TONE 联动）→ J2。
- U1B/U1C 均为同相削波级；RV1 100k 双联：RV1A 在 U1B 反馈做增益，RV1B 做 SHELF 深度；Rg1 3.9k 输入；U1C Rfb2 20k / Rg2 10k。
- TONE 在 U3A 反馈里做 1.8 kHz 高架，−12…+5 dB；C4 10n / R13 8.2k（拐点 1.94k）。RV3 为双联 C100k：RV3A TONE，RV3B 与 R3K5 100k 组成 3k 深度反馈，TONE 越暗 3k 越深（0…−6 dB）。
- SW3 SHELF 位接 U3B 的 shelf 节点，FLAT 位接 U3A 输出；公共端经 C13 1u 到 U3B +in，Rsw3 1M 对地。
- 输出 pad：Rpad1 10k + Rpad2 4.3k = 0.30（−10.5 dB）；U3D 的 ±7.5 V 轨限幅是本级限幅器。U4 为 ±9 V，3k 在 U4 的 MFB twin-T 内、post-pad、post-U3D-limiter；3k 与 H 的 `peaking(3000,−6*(1−tone),1)` 对应。
- D4b+D4c 固定串联，Light 为 2×Si；没有 1N34A、JFET、SW2、British 支路。
- 9V 电源建议稳压低噪；±9V 影响 op-amp 限幅/headroom。
- 旧 preE 高架 R14 10k / R15 22k / C9 3.3n 已删除；A 的 preE 改用 R22–R27 + C14/C15 SVF，U3C 从 POST LP 移到 preE 第二只；POST LP 为无源 10k/1.8n。
