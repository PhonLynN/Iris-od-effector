# Iris R 1.2 — A 电路拓扑（DSP 等效，9 运放）

> **状态：拓扑基线，原理图暂缓。** DSP 是唯一真值；本文件描述 A 方案（9 opamps / 2×OPA4134 + 1×单运放）的电路拓扑、运放分配和已定元件。
>
> 旧 `design_spec.md` / 旧 BOM 中的 preE +10 dB、U1B/U1C 反相、U3C POST LP 等内容已过时；当前 BOM 见 `build/BOM_1.2.md`，历史文件在 `work/archive_2026-11-16/`。
>
> 标记：✅ 已定；⚠ 仅指需要现场微调（RV5/RV1S）或原理图未画，不是缺值。

## 1. 总信号链

```
J1 ──C1──► U1A ×2 ──► LOWCUT ──► preE SVF (U1D+U3C) + AIR ──► U1B clipper1
  │                         │                                      │
  │                         └── v1 (clean) ──► BASS LPF ──┐        │
  │                                                       ▼        ▼
  └──────────────────────────────► BLEND clean ──►  U1C clipper2 ──► POST LP
                                                                     │
                                      U3A ×3 + TONE ◄───────────────┘
                                            │
                                      U3B SHELF ◄── SW3
                                            │
                                      U3D VOLUME ──► PAD ──► U4 MFB TWIN-T 3k ──► J2
```

信号顺序与 DSP 一致：输入 ×2 → LOWCUT → preE → AIR → U1B 削波 → BASS 注入 → U1C 削波 → POST LP → 恢复 ×3 → TONE → SHELF → VOLUME → PAD → U4 post-pad MFB twin-T 3k（TONE 联动）→ J2。BLEND 取 v1 clean，不改变主链顺序。

## 2. 运放分配（2×OPA4134PA + 1×单运放，DIP-14/DIP-8）

| Unit | 功能 | 引脚（out / − / +） |
|---|---|---|
| U1A | 输入 ×2 | 1 / 2 / 3 |
| U1B | 削波 1（同相） | 7 / 6 / 5 |
| U1C | 削波 2（同相） | 8 / 9 / 10 |
| U1D | preE SVF（BP） | 14 / 13 / 12 |
| U3A | 恢复 ×3 + TONE | 1 / 2 / 3 |
| U3B | SHELF buffer | 7 / 6 / 5 |
| U3C | preE SVF（LP） | 8 / 9 / 10 |
| U3D | VOLUME buffer | 14 / 13 / 12 |
| U4 | post-pad MFB twin-T 3k（单运放） | 1 / 2 / 3；V+ 7 / V− 4 |
| 电源 | U1/U3: V+ = 4，V− = 11；U4: V+ 7，V− 4 | — |

## 3. 逐级拓扑

### 3.1 输入 U1A ×2 ✅

- J1 tip → C1 22n → U1A +（pin 3）。
- R1 1M：J1 tip → GND（输入负载/HPF）。
- R2 1M：U1A + → GND（偏置）。
- R3 10k：U1A out（pin 1）→ U1A −（pin 2）。
- **Rin 10k：U1A −（pin 2）→ GND**（×2 的下臂；旧 BOM 漏列，需补）。
- 增益 = 1 + R3/Rin = 2；HPF ≈ 1/(2π·22n·1M) = 7.2 Hz。
- 输出 AOUT。

### 3.2 LOWCUT ✅

- AOUT → C2 100n → 节点 LC。
- LC → RV2 B50k（接成变阻）→ R4 5.1k → GND。
- LC → preE 输入。
- fc = 1/(2π·(R4+RV2)·C2) = 312 Hz（RV2=0）… 28.8 Hz（RV2=50k）。

### 3.3 preE：U1D + U3C 二阶 SVF ✅

目标：2 kHz、+7 dB、Q=0.9。当前 E96 值：

| 元件 | 值 | 作用 |
|---|---:|---|
| R22 | 80.6k | LC → U1D −（输入） |
| R23 | 80.6k | U3C out（LP）→ U1D −（LP 输入） |
| R24 | 71.5k | U1D out（BP）→ U1D −（阻尼，与 C14 并联） |
| R25 | 80.6k | U1D out（BP）→ U3C − |
| C14 | 1n | U1D 积分电容（− 到 out） |
| C15 | 1n | U3C 积分电容（− 到 out） |
| R26 | 715k | BP → U1B + 求和 |
| R27 | 1M | U1B + 偏置 |

拓扑：

- U1D：反相积分器/求和；U1D +（pin 12）→ GND；U1D −（pin 13）接 R22、R23、R24、C14；U1D out（pin 14）= BP。
- U3C：反相积分器；U3C +（pin 10）→ GND；U3C −（pin 9）接 R25、C15；U3C out（pin 8）= LP。
- U1B + 为 preE 求和节点：LC 直通 + BP 经 R26 715k；R27 1M 偏置。
- 结果：f0 = 1974.6 Hz，Q = 0.887，A = +7.01 dB；与 DSP peaking 最大差 0.125 dB @1090 Hz。
- LC 直通到 U1B+ 的串联电阻 **15k**（与 R26 715k/R27 1M 一起保持 K=1.399）。

### 3.4 AIR：无源高架 ✅

DSP：`v += 0.35*(v − HPF6k(v))` → 6 kHz 高架，HF +2.6 dB。

**注意：旧 design_spec 的 `Cair` 串联 + `Rair` 对地是高通（切低频），不是 DSP 的高架。** 修正为无源高架：

- preE 输出 → C3 100n → 节点 AIR。
- Rair 27k：AIR → U1B +（串联臂）。
- Cair 1n：跨接在 Rair 两端（HF 旁路）。
- Rair2 82k：U1B + → GND（并联臂；新增）。
- R27 1M 仍是 U1B + 偏置。
- LF 增益 ≈ 82k∥1M / (27k + 82k∥1M) ≈ 0.737（−2.65 dB）；HF 增益 ≈ 1；相对高架 ≈ +2.65 dB，接近 DSP +2.6 dB。
- Rair2 = **82k**（E96）；U1B 的 +2.6 dB 补偿通过 Rg1 5.1k → **3.9k** 实现；C3 在 AIR 之前。

### 3.5 U1B 削波 1（同相）✅

- U1B +（pin 5）← AIR/U1B + 节点。
- U1B −（pin 6）→ Rg1 3.9k → GND。
- 反馈：U1B out（pin 7）→ RV1 B100k（1k 最小）→ Rser 10k → U1B −。
- 削波：D1a+D1b 与 D2a+D2b 反并联，各 2×1N4148，跨接在 U1B − 与 out 之间。
- **删除 Rfb1 10k**（DSP 没有；旧块式图/旧 BOM 的 Rfb1 不在 A 中）。
- 输出 pin 7 → R17 10k → INT；R18 5.4k：INT → GND（div 0.351）。

### 3.6 BASS：clean 低通注入 ✅

DSP：`v3 += 0.85*LPF220(v1)`，clean 取 U1A 输出 v1。

**注意：旧 design_spec 的 `Cbass` 串联 + `Rbass` 对地是高通；低通应交换为 R 串联、C 对地。**

- U1A out（v1）→ Rbass 220 → 节点 BASS。
- Cbass 3.3u：BASS → GND。
- Rbass2 11.8k：BASS → INT / U1C + 节点。
- Cbass 3.3u；实际 −3 dB ≈222 Hz（U1C+ 3.51k 负载）；Rbass2 11.8k 给 0.835 注入（DC）；已仿真。

### 3.7 U1C 削波 2（同相）✅

- U1C +（pin 10）← INT（wet 0.351 + BASS clean）。
- U1C −（pin 9）→ Rg2 10k → GND。
- 反馈：U1C out（pin 8）→ Rfb2 20k → Rser2 10k → U1C −。
- 削波：D3a+D3b 与 D4b+D4c 反并联，各 2×1N4148。
- 增益 = 1 + 20k/10k = 3；输出 COUT。

### 3.8 POST LP：无源一阶 ✅

- COUT → Rlp 10k → U3A +（pin 3）。
- C1L 1.8n：U3A + → GND。
- fc = 1/(2π·10k·1.8n) = 8.84 kHz；U3A + 高阻，无加载。
- U3C 不再做 POST LP；U3C 已用于 preE。

### 3.9 U3A 恢复 ×3 + TONE ✅

- U3A +（pin 3）← POST LP。
- U3A −（pin 2）→ Rf 20k → out（pin 1）；Rg 10k：− → GND；恢复增益 = 3。
- TONE：Baxandall 一阶高架，RV3 双联 C100k（RV3A）+ C4 10n + R13 8.2k（拐点 1.94k）。
  - RV3A 滑片 → U3A −（pin 2）。
  - RV3A 一端 → C4 10n → U3A out（pin 1）。
  - RV3A 另一端 → R13 8.2k → GND。
  - Rf 20k / Rg 10k 保持 ×3 基线。
- 目标：1.8 kHz 高架，−12…+5 dB，一阶；A 的 0.5 点 −3.5 dB。
- RV3A C taper；RV3B 3k 深度（0…−6 dB，TONE 越暗越深）；C4/R13 定稿 10n/8.2k（1.94k）；一阶 vs DSP Q0.707 在 3.3k 的最大形状误差 1.9 dB，已接受。

### 3.10 U3B SHELF buffer + 无源低架 ✅

- U3B +（pin 5）← C13 1u ← SW3 common；Rsw3 1M：C13/U3B+ → GND。
- U3B −（pin 6）→ out（pin 7）；单位缓冲。
- SHELF 网络：
  - SHIN（TONE out）→ (Rsp 8.25k + RV1S 2k) ∥ RV1B 100k → SHOUT。
  - Cs 56n：SHIN → SHOUT。
  - Rsh 11.5k：SHOUT → GND。
- SW3 SPDT ON-ON：SHELF 位接 SHOUT，FLAT 位接 SHIN；common → C13 → U3B+。
- RV1B 是 RV1 双联的第二联，随 gain 联动；RV1S 2k 调 shelf 深度。
- A 目标 −4.0·g；Rsp 8.25k / Rsh 11.5k 给 −4.0 dB。

### 3.11 U3D VOLUME + PAD ✅

- U3B out → C8 1u → RV4 A100k（上端 C8，下端 GND，滑片 → U3D + pin 12）。
- U3D −（pin 13）→ out（pin 14）；单位缓冲。
- out → Rpad1 10k → FX_OUT；Rpad2 4.3k：FX_OUT → GND。
- Rload 10k：FX_OUT → GND，**DNP**（测试负载；装上后 pad 从 0.301 变 0.231）。
- pad = 4.3/(10+4.3) = 0.301；U3D 的 ±7.5 V 轨限幅是本级限幅器（不是 DSP 的 3.6 V tanh）。
- FX_OUT 是 U3D 的 pad 输出；不再直接进 J2，而是经 Rin 10k 进 U4 的 MFB twin-T（U4 反相，+ pin 3 接地）。

### 3.12 U4 post-pad MFB twin-T 3k（TONE 联动）✅

- U4 单运放（OPA1641PA 或 OPA134PA，DIP-8/SOIC-8；V+ 7 / V− 4），±9 V。
- U4 反相 MFB twin-T：+（pin 3）接 GND；pad FX_OUT 经 Rin 10k 进 twin-T 输入；twin-T 输出接 U4 −（pin 2）；U4 输出 pin 1 → J2 tip。
- twin-T：R1/R2 10k，C1/C2 5.1n，R3 4.99k，C3 10n；f0≈3.12 kHz；Q 由 Rq/RV3B 的正反馈增强到 ≈1。
- 深度与 TONE 联动：RV3 改双联 C100k；RV3A 仍 TONE 1.8k 高架；RV3B 与 R3K5 组成 twin-T 中心到 U4 输出的反馈，TONE 越暗（RV3B 越小、反馈越强）→ 3k 越深，0…−6 dB。
- 3k 在 pad 后、U4 轨限幅前；post-volume/post-pad；与 H 的 `peaking(3000,−6*(1−tone),1)` 对应。
- U4 去耦：V+ 7 → 100n → GND；V− 4 → 100n → GND；8-pin socket。
- A 仍 9 opamps（2×OPA4134 + U4）。

### 3.13 BLEND ✅ 注入点已定

- wet：POST LP 输出（Rlp/C1L 节点）→ R21 47k → MIX（不取 COUT，避免 R21 旁路 POST LP）。
- clean：U1A out（v1）→ RV5 B50k → MIX。
- MIX = U3A +（pin 3）；wet 经 R21、clean 经 RV5 在 U3A+ 求和。
- 0…50% clean；RV5 50k。
- **已定**：MIX 是 U3A+ 的无源求和节点；wet 经 R21 47k，clean 经 RV5 B50k；RV5 为现场微调（0…50% clean），不预设固定值。BASS 与 BLEND 都取 v1 clean，但 BASS 在 U1C+ 注入，BLEND 在 U3A+ 注入，两者不冲突。

### 3.14 电源 ✅

- 9V → D9 1N4007 → R9 100 → V+。
- C6 100u：V+ → GND。
- U2 MAX1044CPA：V+（pin 8）、CP+（pin 2）、CP−（pin 3）、VOUT（pin 5）、GND（pin 4）。
- C10 10u：CP+ ↔ CP−（飞电容）。
- VOUT → −9V；C12 10u：−9V → GND。
- OPA4134：V+ = pin 4，V− = pin 11。

### 3.15 BYPASS / 接口 ✅

- SW1 3PDT true bypass：IN JACK / FX_IN / OUT JACK / FX_OUT / LED。
- J1、J2：Switchcraft 12B。
- LED 3mm 红 + RLED 4.7k。
- DC1：Lumberg 1614 02，2.1 mm 中心正。

## 4. 关键数值与差异

| Block | A 硬件 | DSP | 差异 |
|---|---|---|---|
| Input | ×2，7.2 Hz | ×2，7.2 Hz | 0 |
| LOWCUT | 28.8–312 Hz | 同 | 0 |
| preE | SVF 2k/+7/Q0.9 | 同 | 0.125 dB |
| AIR | 无源高架 +2.65 dB @6k | +0.35（+2.6 dB） | ≈0.05 dB（已仿真） |
| BASS | LPF 219 Hz +0.85 | 220 Hz +0.85 | ≈0.02 dB（已仿真） |
| U1B | 同相 2×Si | 同相 2×Si | 拓扑一致 |
| U1C | 同相 ×3，2×Si | 同相 ×3，2×Si | 拓扑一致 |
| POST LP | 8.84 kHz 一阶 | 9 kHz 一阶 | 0.16 kHz |
| TONE | 一阶 −12…+5 | 二阶 Q0.707 | 3.3k 最大 1.9 dB |
| SHELF | 一阶 −4.0g | 二阶 Q0.707 | ≈1 dB |
| VOLUME | A100k | v² | taper 不同 |
| PAD | 0.301 | 0.30 | 0.1% |
| 3k peaking | U4 MFB twin-T −6 dB @3k Q1，TONE 联动（post-pad） | H 同（R 无） | 与 H 的 peaking 对应；TONE 越暗越深 |
| 输出限幅 | U3D ±7.5 V 硬；U4 ±9 V；3k 在 U4 的 MFB twin-T 内 | 3.6 V tanh | +6.4 dB headroom；3k pre-U4-rail |

整链（gain 0.5 / lowcut 0.5 / tone 0.5 / shelf 0.5）：A @2k **1.73** vs DSP **2.38**（−0.65）；@8k **−9.08** vs **−9.09**（+0.01）；@100 **−5.15** vs **−4.57**（−0.58）。

## 5. 状态与下一步

1. ✅ **TONE 已定稿**：RV3 改双联 C100k（RV3A TONE 1.8k 高架；RV3B 3k 深度）+ C4 10n + R13 8.2k，拐点 1.94k；一阶 1.9 dB 形状误差已接受。
2. ✅ **AIR 已定稿**：Rair2 82k；U1B Rg1 3.9k 补 +2.6 dB。
3. ✅ **BASS 已算**：Rbass 220/Cbass 3.3u/Rbass2 11.8k；U1C+ 节点 10k/5.4k；DC 0.835，−3 dB ≈222 Hz。
4. ✅ **SHELF 已定稿**：Rsp 8.25k / Rsh 11.5k，−4.0 dB；RV1S 2k。
5. ✅ **BOM 已同步**：新增 Rin 10k、R22–R27、C14/C15、Rair2；删除 R14/R15/C9、Rfb1 10k；U3C 从 POST LP 改 preE；Rload 标 DNP；新增 U4、8-pin socket、100n 去耦、twin-T R/C、RV3 双联 C100k。
6. ✅ **U4 + 3k 已定**：U4 = 反相 MFB twin-T；twin-T R1/R2 10k、C1/C2 5.1n、R3 4.99k、C3 10n；Rf 10k；R3K5 100k 与 RV3B 组成深度/Q 反馈；TONE 越暗越深 0…−6 dB。
7. ✅ **H DSP 已装**：`peaking(3000,−6*(1−tone),1)` 放 `kOutTrim` 之后；已构建并安装 IrisH。
8. ⚠ **输出限幅是设计选择，不是缺值**：A 取 ±7.5 V 硬限幅（已定）；若以后要 DSP 3.6 V tanh，需另加钳位/齐纳（不是无源 EQ）。
9. ⏸ **原理图**：暂缓；本拓扑定稿后再画。
10. ⚙ **现场微调**：RV5 BLEND 0…50% clean；RV1S SHELF 深度；元件值 R21 47k/RV5 50k/RV1S 2k 已定，微调不改 BOM。

## 6. 参考

- `work/sim/preE_svf_design.py`、`work/sim/preE_svf_compare.png`
- `work/sim/dsp_hw_diff_A.py`、`work/sim/dsp_hw_diff_A_overall.png`、`work/sim/dsp_hw_diff_A_blocks.png`
- `work/sim/tone12_overall.png`（TONE −12 + SHELF −4.0 的整链）
- 旧文件：`work/archive_2026-11-16/`
