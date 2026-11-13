// Iris R — parameter mapping (shared by DSP and UI), v1.0
// 数值直接对应原理图 v1.0：
//   输入 ×2 (Rin=Rf=10k) → 无源低切 C2 100n + (RV2 50k + R4 9.1k)
//   → 增益级 Rg 5.1k + RV1 100k LIN + 反馈削波（每侧 4/3×1N4148 + 4.7k 串阻）
//   → 级间 -12dB → 第二级 ×2 + 非对称削波 → 后置低通 8.5kHz
//   → 与 clean 支路 MIX（BLEND）→ TONE（R13 6.8k + RV3 100k C + C4 1.5n）
//   → 恢复 ×1.5 → VOLUME 100k LOG
#pragma once
#ifndef Iris_PARAMS_H
#define Iris_PARAMS_H

#include <algorithm>
#include <cmath>

namespace iris {

enum ParamId : int {
    kGain = 0,    // RV1：增益级
    kLowcut = 1,  // RV2：级间无源低切
    kTone = 2,    // RV3：削波后音调（只切）
    kBlend = 3,   // MIX：clean 支路比例
    kVolume = 4,  // RV4：输出音量
    kNumParams = 5
};

inline constexpr double kPi = 3.14159265358979323846;

// ---- 原理图元件值 -----------------------------------------------------------
inline constexpr double kRin1 = 10e3;       // U1A 输入级 Rin
inline constexpr double kRf1 = 10e3;        // U1A 输入级 Rf  -> ×2
inline constexpr double kRhpfMin = 9100.0;  // R4：低切最小电阻（175Hz 下限）
inline constexpr double kC2 = 100e-9;       // C2：低切电容
inline constexpr double kRv2Max = 50e3;     // RV2
inline constexpr double kRg = 5100.0;       // R5：增益级地端电阻
inline constexpr double kRv1Max = 100e3;    // RV1：1..20.6×
inline constexpr double kR6bp = 10e3;       // 高频旁路串联电阻（v1.0 取消 220p，保留占位）
inline constexpr double kC5 = 1.0e-9;       // 高频旁路电容 1n（与 R6 组成 ~16kHz，几乎不作用）
inline constexpr double kRser = 10e3;       // R14：第一级削波串阻（v1.2 软膝）
inline constexpr double kRin2 = 10e3;       // U1C 恢复级 Rin
inline constexpr double kRf2 = 5e3;         // U1C 恢复级 Rf -> ×1.5
inline constexpr double kR13 = 6800.0;      // R13：音调支路（v1.1 起改为高架滤波）
inline constexpr double kC4 = 1.5e-9;       // C4：音调电容
inline constexpr double kRv3Max = 100e3;    // RV3
inline constexpr double kPostLpHz = 9000.0; // 削波后固定低通（让 TONE 管脆度）

inline constexpr double kInputGain = 1.0 + kRf1 / kRin1;         // 2.0
inline constexpr double kStage2Gain = 2.0;                       // 第二级 ×2
inline constexpr double kRecoveryGain = 1.0 + kRf2 / kRin2;      // 1.5
// 整机 = 2.0 × (1..20.6) × 2.0 × 1.5 = 6 .. 123.6 → +15.6 .. +41.8 dB
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
inline double gainDb(double g, double l = 0.0) { return 20.0 * std::log10(gainLinear(g, l)); }
inline double lowcutHz(double l) { return 1.0 / (2.0 * kPi * (kRhpfMin + rv2FromUi(l)) * kC2); }
inline double toneHz(double t) { return 1.0 / (2.0 * kPi * (kR13 + rv3FromUi(t)) * kC4); }
// v1.1：TONE = 2.2kHz 高架（high shelf），从 -14dB（暖/散）到 +6dB（硬/脆）
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
    };
    return m[i < 0 ? 0 : (i >= kNumParams ? kNumParams - 1 : i)];
}

} // namespace iris
#endif // Iris_PARAMS_H
