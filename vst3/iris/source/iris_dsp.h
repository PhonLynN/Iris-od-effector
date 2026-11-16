// Iris R — DSP core (v1.2, Light-only)
// Light: v1.0 的透明过载路径（保留原行为）。
// U1A ×2 -> 7.2 Hz HPF -> 低切 C2/(RV2+R4) -> preE 2 kHz +7 dB -> air
// -> U1B 2×Si(D1/D2) 削波 -> 低频 clean blend -> U1C 2×Si(D4b/D4c) 削波
// -> 9 kHz 一阶 LPF -> 恢复级 -> TONE 高架 -> 300 Hz 低架 -> VOLUME。
// 4x oversampling；decimator 每个 OS 采样都 tick 一次（rev9 修正）。
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

inline void enableFlushToZero() {
#ifdef _MSC_VER
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
}

struct Diode {
    double isSi = 2.52e-9;
    double nVtSi = 1.752 * 0.025852;
};

inline void diodeNet(double ud, const Diode& d, double& i, double& g) {
    constexpr double kMax = 30.0;
    const double nSide = (ud < 0.0) ? 3.0 : 4.0;
    double a = ud / (nSide * d.nVtSi);
    if (a > kMax) a = kMax;
    if (a < -kMax) a = -kMax;
    const double e = std::exp(a), em = std::exp(-a);
    i = d.isSi * (e - em);
    g = d.isSi * (e + em) / (nSide * d.nVtSi);
}

// 2×Si 串联支路（D1/D2 与 D4b/D4c 均为对称两串）
inline void clipBranch(double u, const Diode& d, double rser,
                       double& i, double& gi) {
    double ud = (u > 0.6 ? 0.6 : (u < -0.6 ? -0.6 : u * 0.5));
    double i1 = 0.0, g1 = 0.0;
    for (int it = 0; it < 12; ++it) {
        diodeNet(ud, d, i1, g1);
        const double f = i1 - (u - ud) / rser;
        const double fp = g1 + 1.0 / rser;
        double du = -f / fp;
        if (du > 0.2) du = 0.2;
        if (du < -0.2) du = -0.2;
        ud += du;
        if (std::fabs(du) < 1e-8) break;
    }
    diodeNet(ud, d, i, g1);
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
    void highShelf(double fs, double fc, double db, double q) {
        const double A = std::pow(10.0, db / 40.0);
        const double w = 2.0 * kPi * fc / fs;
        const double cw = std::cos(w), sw = std::sin(w);
        const double alpha = sw / (2.0 * q);
        const double sq = std::sqrt(A);
        const double a0 = (A + 1.0) - (A - 1.0) * cw + 2.0 * sq * alpha;
        b0 = A * ((A + 1.0) + (A - 1.0) * cw + 2.0 * sq * alpha) / a0;
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0;
        b2 = A * ((A + 1.0) + (A - 1.0) * cw - 2.0 * sq * alpha) / a0;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0;
        a2 = ((A + 1.0) - (A - 1.0) * cw - 2.0 * sq * alpha) / a0;
    }
    void lowShelf(double fs, double fc, double db, double q) {
        const double A = std::pow(10.0, db / 40.0);
        const double w = 2.0 * kPi * fc / fs;
        const double cw = std::cos(w), sw = std::sin(w);
        const double alpha = sw / (2.0 * q);
        const double sq = std::sqrt(A);
        const double a0 = (A + 1.0) + (A - 1.0) * cw + 2.0 * sq * alpha;
        b0 = A * ((A + 1.0) - (A - 1.0) * cw + 2.0 * sq * alpha) / a0;
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cw) / a0;
        b2 = A * ((A + 1.0) - (A - 1.0) * cw - 2.0 * sq * alpha) / a0;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cw) / a0;
        a2 = ((A + 1.0) + (A - 1.0) * cw - 2.0 * sq * alpha) / a0;
    }
};

struct Decimator {
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
    static constexpr int kOS = 4;
    static constexpr double kInVolts = 4.0;
    static constexpr double kRail = 7.0;
    static constexpr double kOutLimit = 3.6;

    void prepare(double sampleRate) {
        fs_ = sampleRate > 1000.0 ? sampleRate : 48000.0;
        osFs_ = fs_ * kOS;
        dt_ = 1.0 / osFs_;

        aIn_ = 1.0 - std::exp(-dt_ / (1.0e6 * 22e-9));
        aSm_ = 1.0 - std::exp(-dt_ / 0.008);
        aPostLight_ = 1.0 - std::exp(-dt_ * 2.0 * kPi * kPostLpHz);
        aBass_ = 1.0 - std::exp(-dt_ * 2.0 * kPi * 220.0);
        aAir_ = 1.0 - std::exp(-dt_ * 2.0 * kPi * 6000.0);
        kC2_ = kC2 / dt_;
        kGBias_ = 1.0 / 1.0e6;

        for (int c = 0; c < 2; ++c) dec_[c].design(osFs_, 0.48 * fs_);
        mode_ = true;
        updateFilters();
        reset();
    }

    void reset() {
        for (int c = 0; c < 2; ++c) {
            lpIn_[c] = 0.0; inPrev_[c] = 0.0; hpfPrev_[c] = 0.0;
            uPrev_[c] = 0.0; u2Prev_[c] = 0.0;
            lpPostLight_[c] = 0.0; bassLp_[c] = 0.0; airHp_[c] = 0.0;
            lastIn_[c] = 0.0;
            preE_[c].reset(); tone_[c].reset(); low_[c].reset();
            dec_[c].reset();
        }
    }

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
    void setBlendAmount(double b) {
        const double x = (b < 0.0 ? 0.0 : (b > 1.0 ? 1.0 : b));
        blend_ = 0.5 * x;
    }
    // SW3：SHELF / FLAT；只改低架系数，不重置其他状态
    void setShelf(bool on) { shelf_ = on; }

    // 调试接口
    void setRecoveryGain(double g) { recGain_ = g; }
    void setCleanBlend(double a) { blend_ = a; }

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

    double dbgU() const { return dbgU_; }
    double dbgV() const { return dbgV_; }
    double dbgV3() const { return dbgV3_; }
    double dbgT1() const { return dbgT1_; }
    double dbgY() const { return dbgY_; }
    double dbgAcc() const { return 0.0; }
    double dbgDec() const { return 0.0; }

private:
    static inline double clampRail(double v) {
        return v > kRail ? kRail : (v < -kRail ? -kRail : v);
    }

    void updateFilters() {
        const double preDb = 7.0;
        const double toneDb = -12.0 + 17.0 * sm_[2];
        const double lowDb = shelf_ ? -kLowShelfMaxDb * sm_[0] : 0.0;

        if (mode_ || std::fabs(preDb - preDb_) > 0.01) {
            for (int c = 0; c < 2; ++c) preE_[c].peaking(osFs_, 2000.0, preDb, 0.9);
            preDb_ = preDb;
        }
        if (mode_ || std::fabs(toneDb - toneDb_) > 0.05) {
            for (int c = 0; c < 2; ++c) tone_[c].highShelf(osFs_, kToneShelfHz, toneDb, 0.707);
            toneDb_ = toneDb;
        }
        if (mode_ || std::fabs(lowDb - lowDb_) > 0.05) {
            for (int c = 0; c < 2; ++c) low_[c].lowShelf(osFs_, kLowShelfHz, lowDb, 0.707);
            lowDb_ = lowDb;
        }
        mode_ = false;
    }

    double processChannel(double x, int c) {
        for (int i = 0; i < 4; ++i) sm_[i] += aSm_ * (tgt_[i] - sm_[i]);
        updateFilters();

        const double rv1 = rv1FromUi(sm_[0]);
        const double rv2 = rv2FromUi(sm_[1]);
        const double lvl = volumeFromUi(sm_[3]);

        double dout = 0.0;
        const double prev = lastIn_[c];
        for (int k = 1; k <= kOS; ++k) {
            const double t = (double)k / (double)kOS;
            const double xo = prev + (x - prev) * t;
            const double acc = oneOsSample(xo, c, rv1, rv2, lvl);
            dout = dec_[c].tick(acc);   // 每个 OS 采样都 tick，rev9 修正
        }
        lastIn_[c] = x;

        double y = dout / kInVolts * kMakeupLight;
        y = kOutLimit * std::tanh(y / kOutLimit);
        if (y > 0.995) y = 0.995;
        if (y < -0.995) y = -0.995;
        // 输出电平在 tanh/clamp 之后：volume 与 kOutTrim 只改响度，不改增益音色
        y *= lvl * kOutTrim;
        dbgY_ = y;
        return y;
    }

    inline double oneOsSample(double x, int c, double rv1, double rv2, double lvl) {
        const double vin = x * kInVolts;

        // U1A 输入 ×2 + 7.2 Hz 高通
        lpIn_[c] += aIn_ * (vin - lpIn_[c]);
        double v1 = 2.0 * (vin - lpIn_[c]);
        v1 = clampRail(v1);

        // 无源低切 C2 + (RV2 + R4)
        const double gHpf = 1.0 / (kRhpfMin + rv2);
        const double num = kC2_ * (v1 - inPrev_[c] + hpfPrev_[c]);
        const double den = kC2_ + gHpf + kGBias_;
        double v = num / den;
        inPrev_[c] = v1;
        hpfPrev_[c] = v;
        v = clampRail(v);

        // preE：2k +7 dB
        v = preE_[c].tick(v);

        // air 的空气感补偿
        airHp_[c] += aAir_ * (v - airHp_[c]);
        v = v + 0.35 * (v - airHp_[c]);

        // U1B：增益级 + 反馈削波（D1/D2 2×Si）
        const double rg = kRg;
        const double rser = kRser;
        double u = uPrev_[c];
        for (int it = 0; it < 24; ++it) {
            double id = 0.0, gd = 0.0;
            clipBranch(u, dio_, rser, id, gd);
            const double f = u / rv1 + id - v / rg;
            const double fp = 1.0 / rv1 + gd;
            double du = -f / fp;
            if (du > 0.1) du = 0.1;
            if (du < -0.2) du = -0.2;
            u += du;
            if (std::fabs(f) < 1e-7 + 1e-6 * std::fabs(v)) break;
            if (std::fabs(du) < 1e-12) break;
        }
        uPrev_[c] = u;
        double v3 = v + u;
        dbgU_ = u;
        dbgV_ = v;

        // 低频 clean blend
        bassLp_[c] += aBass_ * (v1 - bassLp_[c]);
        v3 = clampRail(v3 + 0.85 * bassLp_[c]);

        // U1C：第二级削波（D4b/D4c 2×Si）
        const double v2 = div_ * v3;
        double u2 = u2Prev_[c];
        for (int it = 0; it < 24; ++it) {
            double id2 = 0.0, gd2 = 0.0;
            clipBranch(u2, dio_, kRser2, id2, gd2);
            const double f2 = u2 / rf2_ + id2 - v2 / rg2_;
            const double fp2 = 1.0 / rf2_ + gd2;
            double du2 = -f2 / fp2;
            if (du2 > 0.1) du2 = 0.1;
            if (du2 < -0.2) du2 = -0.2;
            u2 += du2;
            if (std::fabs(f2) < 1e-7 + 1e-6 * std::fabs(v2)) break;
            if (std::fabs(du2) < 1e-12) break;
        }
        u2Prev_[c] = u2;
        v3 = v2 + u2;
        dbgV3_ = v3;

        // 后置低通：9k 一阶
        lpPostLight_[c] += aPostLight_ * (v3 - lpPostLight_[c]);
        v3 = lpPostLight_[c];

        // 恢复级 ×3 + clean blend
        double t1 = clampRail(recGain_ * v3);
        if (blend_ > 0.0) {
            // clean 取 preE 之前，保持 v1.0 轻过载的干声路径
            const double clean = recGain_ * v1;
            t1 = (1.0 - blend_) * t1 + blend_ * clean;
        }
        // TONE + 300 Hz 低架
        t1 = tone_[c].tick(t1);
        t1 = low_[c].tick(t1);
        dbgT1_ = t1;
        return t1;
    }

    void sanitizeState() {
        bool bad = false;
        for (int c = 0; c < 2 && !bad; ++c) {
            bad = !std::isfinite(lpIn_[c]) || !std::isfinite(inPrev_[c]) ||
                  !std::isfinite(hpfPrev_[c]) || !std::isfinite(uPrev_[c]) ||
                  !std::isfinite(u2Prev_[c]) || !std::isfinite(lpPostLight_[c]) ||
                  !std::isfinite(bassLp_[c]) || !std::isfinite(airHp_[c]) ||
                  !std::isfinite(lastIn_[c]) ||
                  !std::isfinite(dec_[c].s1.z1) || !std::isfinite(dec_[c].s2.z1) ||
                  !std::isfinite(tone_[c].z1) || !std::isfinite(low_[c].z1);
        }
        if (bad) reset();
    }

    double fs_ = 48000.0, osFs_ = 192000.0, dt_ = 1.0 / 192000.0;
    double aIn_ = 0.0, aSm_ = 0.0, aPostLight_ = 0.0, aBass_ = 0.0, aAir_ = 0.0;
    double kC2_ = 0.0, kGBias_ = 0.0;
    Diode dio_;
    Biquad preE_[2], tone_[2], low_[2];
    Decimator dec_[2];

    double lpIn_[2] = {0, 0}, inPrev_[2] = {0, 0}, hpfPrev_[2] = {0, 0};
    double uPrev_[2] = {0, 0}, u2Prev_[2] = {0, 0};
    double lpPostLight_[2] = {0, 0}, bassLp_[2] = {0, 0}, airHp_[2] = {0, 0};
    double lastIn_[2] = {0, 0};

    float tgt_[4] = {0.55f, 0.15f, 0.85f, 0.85f};
    double sm_[4] = {0.55, 0.15, 0.85, 0.85};
    bool primed_ = false;
    bool shelf_ = true;
    bool mode_ = true;
    double blend_ = 0.30;
    double recGain_ = kRecoveryGain;
    double div_ = 0.35, rf2_ = 20e3, rg2_ = 10e3;
    double preDb_ = -99.0, toneDb_ = -99.0, lowDb_ = 99.0;
    double dbgU_ = 0.0, dbgV_ = 0.0, dbgV3_ = 0.0, dbgT1_ = 0.0, dbgY_ = 0.0;

    static constexpr double kRser2 = 10e3;
};

} // namespace iris
#endif // Iris_DSP_H
