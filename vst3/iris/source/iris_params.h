// Iris R — parameter mapping (shared by DSP and UI), Light-only
// 数值对应 1.2 Light 原理图：
//   输入 ×2 (Rin=Rf=10k) → 无源低切 C2 100n + (RV2 50k + R4 5.1k)
//   → C3 100n 耦合 → 增益级 Rg1 5.1k + RV1 100k LIN + 反馈削波（每侧 2×1N4148）
//   → 级间 -12dB → 第二级 Rg2 10k/Rfb2 20k → D4b+D4c 2×Si → 后置低通 9kHz
//   → 与 clean 支路 MIX（BLEND）→ TONE 高架 → 300Hz 低架（SHELF）→ VOLUME
#pragma once
#ifndef Iris_PARAMS_H
#define Iris_PARAMS_H

#include <algorithm>
#include <cmath>

namespace iris {

enum ParamId : int {
    kGain = 0,    // RV1：增益级
    kLowcut = 1,  // RV2：级间无源低切
    kTone = 2,    // RV3：削波后音调（高架）
    kBlend = 3,   // MIX：clean 支路比例
    kVolume = 4,  // RV4：输出音量
    kShelf = 5,   // SW3：SHELF / FLAT
    kNumParams = 6
};

inline constexpr double kPi = 3.14159265358979323846;

// ---- 原理图元件值 -----------------------------------------------------------
inline constexpr double kRin1 = 10e3;       // U1A 输入级 Rin
inline constexpr double kRf1 = 10e3;        // U1A 输入级 Rf  -> ×2
inline constexpr double kRhpfMin = 5100.0;  // R4：低切最小电阻
inline constexpr double kC2 = 100e-9;       // C2：低切电容
inline constexpr double kRv2Max = 50e3;     // RV2
inline constexpr double kRg = 5100.0;       // Rg1：增益级地端电阻（Light）
inline constexpr double kRv1Max = 100e3;    // RV1：1..20.6×
inline constexpr double kR6bp = 10e3;       // 高频旁路串联电阻（占位）
inline constexpr double kC5 = 1.0e-9;       // 高频旁路电容 1n（与 R6 组成 ~16kHz，几乎不作用）
inline constexpr double kRser = 10e3;       // R14：第一级削波串阻
inline constexpr double kRin2 = 10e3;       // U1C 恢复级 Rin
inline constexpr double kRf2 = 20e3;        // U1C 恢复级 Rf（Rfb2）
inline constexpr double kR13 = 6800.0;      // TONE 支路串联（旧读数用）
inline constexpr double kC4 = 4.7e-9;       // C4：音调电容
inline constexpr double kRv3Max = 100e3;    // RV3
inline constexpr double kPostLpHz = 9000.0; // 削波后固定一阶低通
// ---- 联动低架（SW3） --------------------------------------------------------
inline constexpr double kLowShelfHz = 300.0;    // 随 gain 的低架
inline constexpr double kLowShelfMaxDb = 4.0;   // gain=1 时 -4 dB
inline constexpr double kMakeupLight = 1.0;     // 轻过载支路：不加 makeup

inline constexpr double kInputGain = 1.0 + kRf1 / kRin1;         // 2.0
inline constexpr double kStage2Gain = 1.0 + kRf2 / kRin2;        // 3.0
inline constexpr double kRecoveryGain = 1.0 + kRf2 / kRin2;      // 3.0
// 整机小信号 = 2.0 × (1..20.6) × 3.0；读数仍用实测表
inline constexpr double kFixedGain = kInputGain * kStage2Gain * kRecoveryGain;

// ---- UI(0..1) → 原理图阻值 --------------------------------------------------
inline double uiValue(double v) {
    if (!std::isfinite(v)) return 0.0;
    return std::min(1.0, std::max(0.0, v));
}
inline double rv1FromUi(double g) { return 1.0 + kRv1Max * uiValue(g); }
// LOWCUT 拧大 = 转折更高 = 切得更多 → RV2 变小（电位器反接使用）
inline double rv2FromUi(double l) { return 1.0 + kRv2Max * (1.0 - uiValue(l)); }
// TONE 用 C 型（反对数）：拧大 → 串联电阻变小 → 转折上移（更亮）
inline double rv3FromUi(double t) { return 1.0 + kRv3Max * std::pow(10.0, -2.0 * uiValue(t)); }
inline double blendFromUi(double b) { return uiValue(b) * 0.5; }   // 0..50% 干声
inline double volumeFromUi(double v) { const double x = uiValue(v); return x * x; }

// ---- 读数 -------------------------------------------------------------------
inline double gainLinear(double g, double /*lowcut unused*/ = 0.0) {
    return kFixedGain * (1.0 + rv1FromUi(g) / kRg);
}
// UI 读数用实测值（v1.0 实测 40 mV：0/25/50/75/100% = +7.5/+20.5/+24.7/+26.5/+27.2 dB）
inline double interpDb5(const double (&db)[5], double x) {
    const double u = x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x) * 4.0;
    const int i = (int)u;
    if (i >= 4) return db[4];
    return db[i] + (db[i + 1] - db[i]) * (u - (double)i);
}
inline double gainDb(double g, double /*l*/ = 0.0) {
    static const double kDb[5] = {7.5, 20.5, 24.7, 26.5, 27.2};
    return interpDb5(kDb, uiValue(g));
}
// 全局输出校准：避免 gain/volume 拉满时接近 0 dBFS
inline constexpr double kOutTrim = 0.30; // -10.5 dB
inline double lowcutHz(double l) { return 1.0 / (2.0 * kPi * (kRhpfMin + rv2FromUi(l)) * kC2); }
inline double toneHz(double t) { return 1.0 / (2.0 * kPi * (kR13 + rv3FromUi(t)) * kC4); }
// TONE = 1.8kHz 高架（high shelf），从 -12dB（暖/散）到 +5dB（硬/脆）
inline constexpr double kToneShelfHz = 1800.0;
inline double toneShelfDb(double t) { return -12.0 + 17.0 * uiValue(t); }
inline double blendPct(double b) { return blendFromUi(b) * 100.0; }
inline double volumeDb(double v) { return 20.0 * std::log10(std::max(1e-6, volumeFromUi(v))); }

// ---- 参数元信息 -------------------------------------------------------------
struct ParamMeta {
    const char* id;     // VST3 参数 id
    const char* name;   // 面板标签
    float def;          // 默认值（归一化）
    const char* unit;
};
inline const ParamMeta& meta(int i) {
    static const ParamMeta m[kNumParams] = {
        {"gain", "GAIN", 0.55f, "dB"},
        {"lowcut", "LOWCUT", 0.15f, "Hz"},
        {"tone", "TONE", 0.85f, "dB"},
        {"blend", "BLEND", 0.60f, "%"},
        {"volume", "VOLUME", 0.85f, "dB"},
        {"shelf", "SHELF", 1.0f, ""},
    };
    return m[i < 0 ? 0 : (i >= kNumParams ? kNumParams - 1 : i)];
}

} // namespace iris
#endif // Iris_PARAMS_H
