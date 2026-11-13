// Iris R — DSP core (v0.5)
// 与原理图 v0.5 一一对应（全部是能焊出来的真实电路，DSP 只是同一张网表的离散实现）：
//   U1A 输入 ×2  →  无源可变高通 LOWCUT  →  U1B 增益 + 削波（每侧 2×1N4148 串联 + R14 4.7k，可选 Ge 并入）
//   →  U1C 恢复 ×2.6  →  TONE 串联可变低通  →  LEVEL
// 削波级每过采样点解一次隐式方程（牛顿法），不做"削波后加滤波"这类数字近似。
#pragma once
#ifndef Iris_DSP_H
#define Iris_DSP_H

#include "iris_params.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef _MSC_VER
#include <xmmintrin.h>
#endif

namespace iris {




// 反规范化数（denormal）会让一阶递推在收尾处慢几个数量级；打开 FTZ/DAZ 后当 0 处理。
inline void enableFlushToZero() {
#ifdef _MSC_VER
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
}

// v0.5 削波网络：每侧 2 只 1N4148 串联；GE 档再在正半周并入 1 只 1N34A（非对称）；
// 整串串 R14 = 4.7 kΩ → 拐点变圆、峰值可通过，输出随输入继续上升（软压缩，不再钉死）。
// ---- 箱体模拟（CAB-1）：HPF 85Hz → BODY 峰 → BITE 峰 → AIR 低通 -------------
struct Diode {
    double isSi = 2.52e-9;
    double nVtSi = 1.752 * 0.025852;   // ≈ 45.3 mV（单只）
    double isGe = 1.0e-7;
    double nVtGe = 0.0326;             // 拟合 1N34A: 1 mA @ 0.30 V
};

// 二极管网络自身（不含串联电阻）：ud = 网络两端电压
inline void diodeNet(double ud, const Diode& d, bool geOn, double& i, double& g, double nUp = 2.0, double nDown = 2.0) {
    constexpr double kMax = 30.0;
    const double nSide = (ud < 0.0) ? 3.0 : 4.0;  // 正 4 只 / 反 3 只 → 非对称削波（+2.6V / -1.95V），补偶次
    double a = ud / (nSide * d.nVtSi);        // v0.6：每侧 4×1N4148 串联（阈值 ±2.6V）
    if (a > kMax) a = kMax;
    if (a < -kMax) a = -kMax;
    const double e = std::exp(a), em = std::exp(-a);
    i = d.isSi * (e - em);                     // = 2·Is·sinh(ud / 4nVt)
    g = d.isSi * (e + em) / (nSide * d.nVtSi);
    if (geOn && ud > 0.0) {                    // GE 档：正半周多一只 1N34A
        double b = ud / d.nVtGe;
        if (b > kMax) b = kMax;
        const double eg = std::exp(b);
        i += d.isGe * (eg - 1.0);
        g += d.isGe * eg / d.nVtGe;
    }
}

inline long long solverBad = 0, solverWorse = 0;
inline void resetSolverStats() { solverBad = 0; solverWorse = 0; }

// 串上 Rser 后的整串：输入 u，解出网络电压 ud，返回电流与 di/du
inline void clipBranch(double u, const Diode& d, bool geOn, double rser, double& i, double& gi, double nUp = 2.0, double nDown = 2.0) {
    // 热启动：ud 初值取 u 的一半（导通早期几乎就是线性分压），4 次牛顿足够 1e-4 精度
    double ud = (u > 0.6 ? 0.6 : (u < -0.6 ? -0.6 : u * 0.5));
    double i1 = 0.0, g1 = 0.0;
    for (int it = 0; it < 8; ++it) {
        diodeNet(ud, d, geOn, i1, g1, nUp, nDown);
        const double f = i1 - (u - ud) / rser;
        const double fp = g1 + 1.0 / rser;
        double du = -f / fp;
        if (du > 0.25) du = 0.25;
        if (du < -0.25) du = -0.25;
        ud += du;
        if (std::fabs(du) < 1e-7) break;
    }
    diodeNet(ud, d, geOn, i, g1, nUp, nDown);
    {
        const double fres = std::fabs(i - (u - ud) / rser);
        if (!(fres < 1e-4 * (1.0 + std::fabs(u)))) ++solverBad;
        if (!(fres < 1e-2 * (1.0 + std::fabs(u)))) ++solverWorse;
    }
    gi = g1 / (1.0 + rser * g1);
}

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline double tick(double x) {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void lowpass(double fs, double fc, double q) {
        const double w = 2.0 * kPi * fc / fs;
        const double cw = std::cos(w), sw = std::sin(w);
        const double alpha = sw / (2.0 * q);
        const double a0 = 1.0 + alpha;
        b0 = (1.0 - cw) * 0.5 / a0;
        b1 = (1.0 - cw) / a0;
        b2 = b0;
        a1 = -2.0 * cw / a0;
        a2 = (1.0 - alpha) / a0;
    }
    // v1.4 削波前"金属芯"预提升：2 kHz 峰值 +db（RBJ peaking，Q≈0.9）
    // 可焊实现 = U1D 一级同相峰值提升（运放 + 3 电阻 1 电容）
    void peaking(double fs, double fc, double gainDb, double q) {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * fc / fs;
        const double cw = std::cos(w), sw = std::sin(w);
        const double alpha = sw / (2.0 * q);
        const double a0 = 1.0 + alpha / A;
        b0 = (1.0 + alpha * A) / a0;
        b1 = (-2.0 * cw) / a0;
        b2 = (1.0 - alpha * A) / a0;
        a1 = (-2.0 * cw) / a0;
        a2 = (1.0 - alpha / A) / a0;
    }
};

// 4 阶 Butterworth（两级 biquad）——4x 过采样的抽取滤波
struct Decimator {   // v1.5：8 阶 Butterworth（4 级 biquad）——硬削波的 8x 混叠要靠陡的抽取滤波压住
    Biquad s1, s2, s3, s4;
    void design(double fs, double fc) {
        s1.lowpass(fs, fc, 0.50980);
        s2.lowpass(fs, fc, 0.60134);
        s3.lowpass(fs, fc, 0.89998);
        s4.lowpass(fs, fc, 2.56292);
    }
    inline double tick(double x) { return s4.tick(s3.tick(s2.tick(s1.tick(x)))); }
    void reset() { s1.reset(); s2.reset(); s3.reset(); s4.reset(); }
};

class IrisDsp {
public:
    static constexpr int kOS = 4;            // v1.6 回到 4x（8x 太吃 CPU）
    static constexpr double kInVolts = 4.0;  // 1.0 FS = 4V 峰值（足够容纳 ±3.15V 轨 + 抽取滤波过冲）
    static constexpr double kRail = 7.0;     // v1.0：18V 供电（电荷泵），运放轨 ±7.0V
    static constexpr double kSoftKnee = 2.95;   // 轨软削起点（±2.55V 开始压缩，±3.15V 渐近）    // 9V 单电源、4.5V 偏置下的运放输出摆幅

    // v1.3 GRIT：级间交越死区（可焊 = 两只 1N5817 反并联串在信号路径，Vf≈0.22V）
    static constexpr double kPreEHz = 2000.0;   // 削波前峰值提升频率（金属芯）
    static constexpr double kPreEDb = 7.0;      // 提升量 dB
    static constexpr double kOutLimit = 3.6;   // 输出软限幅门限（V，≈0.9 FS）
    static constexpr double kAirG = 0.35;        // 空气感补偿量（0.5 ≈ +3.5dB @HF）
    static constexpr double kGritLpHz = 8000.0;  // 颗粒输出的 OS 域低通（压掉 20k 以上的谐波，防混叠毛刺）
    static constexpr double kGritDz = 0.0;      // v1.7 关闭交越死区（TONE 开大时的毛刺来源）     // 死区半宽（V，内部 7V 标度）
    static constexpr double kGritDc = 0.016;    // 直流偏置（V）→ 正负半周不等，补偶次

    void prepare(double sampleRate) {
        fs_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
        osFs_ = fs_ * kOS;
        dt_ = 1.0 / osFs_;

        kC2_ = kC2 / dt_;          // 低切电容电流系数
        kBp_ = kC5 / dt_;          // 高频旁路电容电流系数
        aIn_ = 1.0 - std::exp(-dt_ / (1.0e6 * 22e-9));    // 输入 7.2Hz 高通
        aHp_ = 1.0 - std::exp(-dt_ / (22000.0 * 470e-9));
        aPost_ = 1.0 - std::exp(-dt_ * 2.0 * 3.14159265358979323846 * kPostLpHz); // 恢复级 72Hz 高通
        preLpG_ = 1.0 - std::exp(-dt_ / (1.0 / (2.0 * 3.141592653589793 * 3400.0)));
        bassG_ = 1.0 - std::exp(-dt_ / (1.0 / (2.0 * 3.141592653589793 * 220.0)));
        aAir_ = 1.0 - std::exp(-dt_ * 2.0 * 3.141592653589793 * 6000.0);
        aGrit_ = 1.0 - std::exp(-dt_ * 2.0 * 3.141592653589793 * kGritLpHz);

        for (int c = 0; c < 2; ++c) { dec_[c].design(osFs_, 0.48 * fs_); preE_[c].peaking(osFs_, kPreEHz, kPreEDb, 0.9); }
        aSm_ = 1.0 - std::exp(-dt_ / 0.008);              // 参数平滑 ~8ms
        reset();
    }

    void reset() {
        for (int c = 0; c < 2; ++c) {
            lpIn_[c] = 0.0;
            inPrev_[c] = 0.0;
            hpfPrev_[c] = 0.0;
            uPrev_[c] = 0.0;
            u2Prev_[c] = 0.0;
            vPrev_[c] = 0.0;
            bpPrev_[c] = 0.0;
            lpHp_[c] = 0.0;
        lpPost_[c] = 0.0;
        preLpZ_[c] = 0.0;
            lpTone_[c] = 0.0;
            lastIn_[c] = 0.0;
            dec_[c].reset();
        }
    }

    // 削波档位：false = SI（每侧 2×1N4148 对称），true = GE（正半周并入 1N34A，非对称）
    void setPreLp(bool on) { preLp_ = on; preLpZ_[0] = preLpZ_[1] = 0.0;
        bassLp_[0] = bassLp_[1] = 0.0; }
    void setClipGe(bool on) { clipGe_ = on; }

    // 亮音电容：跨在 RV1（反馈电阻）上的小电容，给高频更多增益 → 音头更突出
    void setBrightCap(double farads) { kCb_ = farads; }

    // 恢复级增益（出厂 2.6；调低可减少"二次撞轨"、换回动态）
    void setRecoveryGain(double g) { recGain_ = g; }

    // 干声混合（0 = 全削波；0.2 = 混入 20% 未削波的原始信号，找回音头）
    void setCleanBlend(double a) { blend_ = a; }
    void setBlendAmount(double b) { const double x = (b < 0.0 ? 0.0 : (b > 1.0 ? 1.0 : b)); blend_ = 0.5 * x; }
    // v1.1 TONE：2.2kHz 高架滤波（RBJ high-shelf），db 范围 -14..+6
    void updateToneShelf(double db, double fs) {
        const double A = std::pow(10.0, db / 40.0);
        const double w0 = 2.0 * 3.14159265358979323846 * kToneShelfHz / fs;
        const double cw = std::cos(w0), sw = std::sin(w0);
        const double alpha = sw / (2.0 * 0.7071067811865476);
        const double sq = std::sqrt(A);
        const double b0 = A * ((A + 1.0) + (A - 1.0) * cw + 2.0 * sq * alpha);
        const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw);
        const double b2 = A * ((A + 1.0) + (A - 1.0) * cw - 2.0 * sq * alpha);
        const double a0 = (A + 1.0) - (A - 1.0) * cw + 2.0 * sq * alpha;
        const double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw);
        const double a2 = (A + 1.0) - (A - 1.0) * cw - 2.0 * sq * alpha;
        for (int c = 0; c < 2; ++c) {
            toneShelf_[c].b0 = b0 / a0; toneShelf_[c].b1 = b1 / a0; toneShelf_[c].b2 = b2 / a0;
            toneShelf_[c].a1 = a1 / a0; toneShelf_[c].a2 = a2 / a0;
        }
        toneDb_ = db;
    }

    // 两级轻度削波实验（原型）：级间衰减 div + 第二级增益 a2（反馈 clipper）
    void setTwoStage(bool on, double div = 0.8, double a2 = 5.0) {
        twoStage_ = on;
        div_ = div;
        rf2_ = (a2 - 1.0) * rg2_;
    }

    // 调试只读访问（自检程序用）
    double dbgU() const { return dbgU_; }
    double dbgV() const { return dbgV_; }
    double dbgV3() const { return dbgV3_; }
    double dbgT1() const { return dbgT1_; }
    double dbgY() const { return dbgY_; }
    double dbgAcc() const { return dbgAcc_; }
    double dbgDec() const { return dbgDec_; }

    void setTargets(float gain, float lowcut, float tone, float level) {
        tgt_[0] = (float)uiValue(gain);
        tgt_[1] = (float)uiValue(lowcut);
        tgt_[2] = (float)uiValue(tone);
        tgt_[3] = (float)uiValue(level);
        if (!primed_) {
            for (int i = 0; i < 4; ++i) sm_[i] = tgt_[i];
            primed_ = true;
        }
    }

    // v0.7：模块开关与箱体参数
    void process(float** in, float** out, int n, int channels) {
        enableFlushToZero();
        const int ch = channels > 2 ? 2 : (channels < 1 ? 1 : channels);
        for (int s = 0; s < n; ++s) {
            for (int c = 0; c < ch; ++c) {
                double x = in[c] ? (double)in[c][s] : 0.0;
                if (!std::isfinite(x)) x = 0.0;
                out[c][s] = (float)processChannel(x, c);
            }
            if (ch == 1 && channels > 1)
                for (int c = 1; c < channels; ++c) out[c][s] = out[0][s];
        }
        sanitizeState();
    }

    double processMono(double x) {
        if (!std::isfinite(x)) x = 0.0;
        const double y = processChannel(x, 0);
        sanitizeState();
        return y;
    }

private:
    void sanitizeState() {
        bool bad = false;
        for (int c = 0; c < 2 && !bad; ++c) {
            bad = !std::isfinite(lpIn_[c]) || !std::isfinite(inPrev_[c]) ||
                  !std::isfinite(hpfPrev_[c]) || !std::isfinite(uPrev_[c]) ||
                  !std::isfinite(bpPrev_[c]) || !std::isfinite(lpHp_[c]) ||
                  !std::isfinite(lpTone_[c]) || !std::isfinite(lastIn_[c]) ||
                  !std::isfinite(dec_[c].s1.z1) || !std::isfinite(dec_[c].s2.z1);
        }
        if (bad) reset();
    }

    double processChannel(double x, int c) {
        for (int i = 0; i < 4; ++i) sm_[i] += aSm_ * (tgt_[i] - sm_[i]);

        const double rv1 = rv1FromUi(sm_[0]);
        const double rv2 = rv2FromUi(sm_[1]);
        const double rv3 = rv3FromUi(sm_[2]);
        const double lvl = volumeFromUi(sm_[3]);

        const double gHpf = 1.0 / (kRhpfMin + rv2);          // 低切支路电导
        const double toneDb = toneShelfDb(sm_[2]);
        if (std::fabs(toneDb - toneDb_) > 0.05) updateToneShelf(toneDb, osFs_);

        double acc = 0.0;
        const double prev = lastIn_[c];
        for (int k = 1; k <= kOS; ++k) {
            const double t = (double)k / (double)kOS;
            const double xo = prev + (x - prev) * t;
            acc = oneOsSample(xo, c, rv1, gHpf, lvl);
        }
        lastIn_[c] = x;
        const double dout = dec_[c].tick(acc);
        dbgAcc_ = acc;
        dbgDec_ = dout;
        double y = dout / kInVolts;
        if (y > 0.995) y = 0.995;          // 保险丝（软限幅已在 OS 域做过）
        if (y < -0.995) y = -0.995;
        return y;
    }

    inline double oneOsSample(double x, int c, double rv1, double gHpf, double lvl) {
        const double vin = x * kInVolts;

        // ---- 1) U1A：非反相 ×2（Rf=Rin=10k），7.2Hz 高通（C1 22n / R1 1M）
        lpIn_[c] += aIn_ * (vin - lpIn_[c]);
        double v1 = kA1_ * (vin - lpIn_[c]);
        v1 = clampRail(v1);

        // ---- 2) 无源可变高通：C2 串、(RV2+R4) 到地，另并 1M 偏置电阻
        //      kC*( (v1-v) - (v1p-vp) ) = v*(gHpf + gBias)
        const double num = kC2_ * (v1 - inPrev_[c] + hpfPrev_[c]);
        const double den = kC2_ + gHpf + kGBias_;
        double v = num / den;
        inPrev_[c] = v1;
        hpfPrev_[c] = v;
        v = clampRail(v);
        if (preLp_) { preLpZ_[c] += preLpG_ * (v - preLpZ_[c]); v = preLpZ_[c]; }
        v = preE_[c].tick(v);   // v1.4 2kHz +7dB：让削波器先咬住金属芯频段

        // ---- 2b) v1.6 空气感补偿：抽取滤波会让 12k 以上略掉，这里补回
        //          可焊实现 = 与主通路并联的小电容（0.01u 串 4.7k）
        airHp_[c] += aAir_ * (v - airHp_[c]);
        v = v + kAirG * (v - airHp_[c]);

        // ---- 3) U1B：增益级 + 反并削波 + 220p/10k 高频旁路
        //      u = vout - v；KCL: u/RV1 + Id(u) + (u-vc)/(R+dt/C) + v/Rg = 0
        const double gBp = 1.0 / (kR6bp + 1.0 / kBp_);
        double u = uPrev_[c];
        for (int it = 0; it < 24; ++it) {
            double id = 0.0, gd = 0.0;
            clipBranch(u, dio_, clipGe_, kRser, id, gd, kSi1, kSi1);
            // 亮音电容 Cb 跨在 RV1 上：i_bc = gBc*((u-v) - (uPrev-vPrev))
            const double gBc = kCb_ / dt_;
            const double iBc = gBc * ((u - v) - (uPrev_[c] - vPrev_[c]));
            const double f = u / rv1 + id + (u - bpPrev_[c]) * gBp + iBc - v / kRg;
            const double fp = 1.0 / rv1 + gd + gBp + gBc;
            double du = -f / fp;
            if (du > 0.1) du = 0.1;
            if (du < -0.2) du = -0.2;
            u += du;
            if (std::fabs(f) < 1e-7 + 1e-6 * std::fabs(v)) break;   // 残差收敛即停
            if (std::fabs(du) < 1e-9) break;
        }
        uPrev_[c] = u;
        vPrev_[c] = v;

        // ---- 3a0b) v1.3 GRIT：把交越死区放在**削波残差 u** 上（不是整段信号）
        //      可焊实现 = 两只 1N5817 反并联串在反馈支路里、与 1N4148 削波对串联：
        //      信号没到削波区时 u≈0（透明），一到削波区，u 每穿过零点就被卡出一个台阶 → 颗粒。
        if (kGritDz > 1e-9) {
            const double uLin = u;
            const double xr = u + kGritDc;
            const double ax = std::fabs(xr);
            const double sg = (xr >= 0.0) ? 1.0 : -1.0;
            const double y = (ax < 1e-9) ? 0.0 : (xr - kGritDz * std::tanh(ax / kGritDz) * sg);
            const double ab = std::fabs(kGritDc);
            const double base = (ab < 1e-9) ? 0.0
                              : (kGritDc - kGritDz * std::tanh(ab / kGritDz) * (kGritDc >= 0.0 ? 1.0 : -1.0));
            const double uGrit = y - base;
            // 门控：按"削波残差"大小决定是否启用颗粒 —— 残差小=还没真正削波 → 保持干净透明
            double g = (std::fabs(uLin) - 0.15) / 0.30;
            if (g < 0.0) g = 0.0; else if (g > 1.0) g = 1.0;
            const double uMix = uLin + g * (uGrit - uLin);
            gritLp_[c] += aGrit_ * (uMix - gritLp_[c]);
            u = gritLp_[c];
            dbgGrit_ = u;
        }
        // 旁路支路电容电压更新（与 u 同步）
        const double iBp = (u - bpPrev_[c]) * gBp;
        bpPrev_[c] += iBp / kBp_;

        double v3 = v + u;
        dbgU_ = u; dbgV_ = v;

        // ---- 3a) clean low-end blend: low-passed U1A output (before HPF) mixed back ----
        bassLp_[c] += bassG_ * (v1 - bassLp_[c]);
        v3 = clampRail(v3 + kBlend * bassLp_[c]);
        dbgV3_ = v3;

        // ---- 3b) 第二级轻度削波（U1D，实验原型）
        if (twoStage_) {
            const double v2 = div_ * v3;
            double u2 = u2Prev_[c];
            for (int it = 0; it < 24; ++it) {
                double id2 = 0.0, gd2 = 0.0;
                clipBranch(u2, dio_, clipGe_, kRser2, id2, gd2, kSi2Up, kSi2Dn);
                const double f2 = u2 / rf2_ + id2 - v2 / rg2_;
                const double fp2 = 1.0 / rf2_ + gd2;
                double du2 = -f2 / fp2;
                if (du2 > 0.1) du2 = 0.1;
                if (du2 < -0.2) du2 = -0.2;
                u2 += du2;
                if (std::fabs(f2) < 1e-7 + 1e-6 * std::fabs(v2)) break;
                if (std::fabs(du2) < 1e-11) break;
            }
            u2Prev_[c] = u2;
            v3 = clampRail(v2 + u2);
        }

        // ---- 3c) 削波后固定低通 8.5kHz
        lpPost_[c] += aPost_ * (v3 - lpPost_[c]);
        v3 = lpPost_[c];

        // ---- 4) U1C：恢复级 ×1.5 + 15Hz 高通（C3 470n / R7 22k）
        lpHp_[c] += aHp_ * (v3 - lpHp_[c]);
        double t1 = clampRail(recGain_ * (v3 - lpHp_[c]));   // U1C 输出，经过轨饱和   // U1C 输出同样受 ±3.15V 轨限制
        if (blend_ > 0.0) {
            // 干声混合：把削波前的信号（×0.5 归一）混进来，救回拨弦音头
            const double clean = recGain_ * v;
            t1 = (1.0 - blend_) * t1 + blend_ * clean;
        }

        // ---- 5) TONE：串联可变低通（R13 + RV3 串 C4）
        lpTone_[c] = toneShelf_[c].tick(t1);
        // v1.8：OS 域软限幅——不许输出在 48kHz 域硬削（那会产生混叠=爆音/毛刺）
        {
            const double lim = kOutLimit;
            lpTone_[c] = lim * std::tanh(lpTone_[c] / lim);
        }
        dbgT1_ = t1;

        // ---- 6) LEVEL（最后的 ±kInVolts 只做安全限幅，正常信号碰不到）
        //      注意：这里是"伏特"域，1.0 不是满刻度；满刻度是 kInVolts=4V
        const double y = lpTone_[c] * lvl * (kInVolts / kRail);   // 内部满轨 7.0V -> FS(4V)
        dbgY_ = y;
        const double lim = kInVolts * 0.999;
        return y > lim ? lim : (y < -lim ? -lim : y);
    }

    static inline double clampRail(double v) {
        return v > kRail ? kRail : (v < -kRail ? -kRail : v);
    }

    // 功率级 / 运放轨的软饱和：|x| 超过 kSoftKnee 后按 tanh 压向 kRail
    static inline double softRail(double x) {
        const double a = x < 0.0 ? -x : x;
        if (a <= kSoftKnee) return x;
        const double room = kRail - kSoftKnee;
        const double y = kSoftKnee + room * std::tanh((a - kSoftKnee) / room);
        return x < 0.0 ? -y : y;
    }

    double fs_ = 48000.0, osFs_ = 192000.0, dt_ = 1.0 / 192000.0;
    double kC2_ = 0.0, kBp_ = 0.0, aIn_ = 0.0, aHp_ = 0.0, aSm_ = 0.0, aPost_ = 0.0;
    double lpPost_[2] = {0, 0};
    Biquad toneShelf_[2];
    double toneDb_ = -99.0;
    Diode dio_;
    bool clipGe_ = false;
    double dbgU_ = 0.0, dbgV_ = 0.0, dbgV3_ = 0.0, dbgT1_ = 0.0, dbgY_ = 0.0;
    double dbgGrit_ = 0.0;
    double dbgAcc_ = 0.0, dbgDec_ = 0.0;
    double kCb_ = 0.0;                 // 亮音电容（法拉），0 = 无
    double recGain_ = kRecoveryGain;   // 恢复级增益（v1.0：1.5）
    double blend_ = 0.30;              // clean 支路混合比例（v1.0）
    bool twoStage_ = true;             // v1.0：两级削波（级间 -12dB + 第二级 x2）
    bool preLp_ = false;          // 3.4 kHz one-pole BEFORE clipper (RAT-style HF softening)
    double bassLp_[2] = {0.0, 0.0};
    double bassG_ = 0.0;
    static constexpr double kBlend = 0.85;   // clean low-end blend (0..1)
    double preLpG_ = 0.0;
    double preLpZ_[2] = {0.0, 0.0};
    double gritLp_[2] = {0.0, 0.0};
    Biquad preE_[2];
    double airHp_[2] = {0.0, 0.0};
    double aAir_ = 0.0;
    double aGrit_ = 0.0;
    double div_ = 0.35, rf2_ = 20e3, rg2_ = 10e3;   // 级间 -12dB、第二级 x2
    static constexpr double kRser2 = 10e3;         // 第二级削波串阻（软膝）
    static constexpr double kSi1 = 2.0;            // 一级每侧 2 只（±1.3V，对称）
    static constexpr double kSi2Up = 2.0;          // 二级正半周 2 只（±1.3V，对称）
    static constexpr double kSi2Dn = 2.0;          // 二级负半周 2 只（±1.3V，对称）
    double u2Prev_[2] = {0, 0};
    double vPrev_[2] = {0, 0};         // 上一采样的 v（亮音电容用）
    Decimator dec_[2];

    double lpIn_[2] = {0, 0};
    double inPrev_[2] = {0, 0};
    double hpfPrev_[2] = {0, 0};
    double uPrev_[2] = {0, 0};
    double bpPrev_[2] = {0, 0};
    double lpHp_[2] = {0, 0};
    double lpTone_[2] = {0, 0};
    double lastIn_[2] = {0, 0};

    float tgt_[4] = {0.5f, 0.25f, 1.0f, 0.8f};
    double sm_[4] = {0.5, 0.25, 1.0, 0.8};
    bool primed_ = false;

    static constexpr double kA1_ = 1.0 + kRf1 / kRin1;   // 2.0
    static constexpr double kA2_ = 1.0 + kRf2 / kRin2;   // 2.6（v0.5 恢复级）
    static constexpr double kRser = 4700.0;              // R14：削波串的串联电阻（v0.5）
    static constexpr double kGBias_ = 1.0 / 1.0e6;       // 1M 偏置电阻
};

} // namespace iris
#endif // Iris_DSP_H
