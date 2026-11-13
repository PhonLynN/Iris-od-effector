// Iris R 自检程序：1) 渲染旋钮 UI 到 PNG  2) 用与插件相同的 DSP 跑增益/THD 自测

#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <chrono>
#include <algorithm>

#include "iris_params.h"
#include "iris_dsp.h"
#include "iris_ui.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static const double PI = 3.14159265358979323846;

// 鍗曢 DFT 骞呭害
static double bin(const std::vector<float>& y, int harmonic, double f0, double fs) {
    double re = 0.0, im = 0.0;
    const double w = 2.0 * PI * f0 * harmonic / fs;
    for (size_t i = 0; i < y.size(); ++i) {
        re += y[i] * std::cos(w * (double)i);
        im -= y[i] * std::sin(w * (double)i);
    }
    return 2.0 * std::sqrt(re * re + im * im) / (double)y.size();
}

static void renderUi(const wchar_t* path) {
    irisui::KnobPanel panel;
    panel.init();
    panel.setValue(0, iris::meta(iris::kGain).def);
    panel.setValue(1, iris::meta(iris::kLowcut).def);
    panel.setValue(2, iris::meta(iris::kTone).def);
    panel.setValue(3, iris::meta(iris::kBlend).def);
    panel.setValue(4, iris::meta(iris::kVolume).def);

    Bitmap bmp(560, 300, PixelFormat32bppARGB);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    panel.paint(g, 560, 300);

    CLSID png;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &png);
    bmp.Save(path, &png, nullptr);
    wprintf(L"[ui] wrote %s\n", path);
}

// ---------------------------------------------------------------- attack ----
struct Bq {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double tick(double x) {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void hp(double fs, double fc, double q) {
        const double w = 2.0 * PI * fc / fs, cw = std::cos(w), sw = std::sin(w), al = sw / (2.0 * q);
        const double a0 = 1.0 + al;
        b0 = (1.0 + cw) * 0.5 / a0;
        b1 = -(1.0 + cw) / a0;
        b2 = b0;
        a1 = -2.0 * cw / a0;
        a2 = (1.0 - al) / a0;
    }
};

static double rmsWin(const std::vector<float>& y, int from, int len) {
    double s = 0.0;
    for (int i = 0; i < len; ++i) { const double v = y[from + i]; s += v * v; }
    return std::sqrt(s / (double)len);
}

// 拨弦瞬态：196Hz 衰减 6 次谐波 + 3ms 拨片噪声
static void runAttack() {
    const double fs = 48000.0, f0 = 196.0;
    const int n = (int)(fs * 0.5);
    std::vector<float> base(n);
    unsigned seed = 12345u;
    for (int k = 0; k < n; ++k) {
        const double t = k / fs;
        double g = 0.0;
        for (int h = 1; h <= 6; ++h) g += std::sin(2.0 * PI * f0 * h * t) / (double)h;
        g *= std::exp(-t / 0.30);
        seed = seed * 1664525u + 1013904223u;
        const double noise = ((double)((seed >> 8) & 0xFFFF) / 32768.0) - 1.0;
        base[k] = (float)(g * 0.012 + 0.012 * std::exp(-t / 0.0025) * noise);   // 峰值约 0.25V，贴近真实吉他
    }

    printf("\n[attack] 拨弦瞬态（196Hz 衰减 + 3ms 拨片噪声）：音头 vs 延音\n");
    printf("%6s %4s %6s %9s %9s %8s %8s %8s\n", "GAIN", "CLIP", "Cb", "atk_dB", "sus_dB", "atk-sus", "Hi_atk", "Hi_sus");
    const float gains[4] = {0.0f, 0.25f, 0.5f, 1.0f};
    for (int gi = 0; gi < 4; ++gi) {
        {
        {
            iris::IrisDsp dsp;
            dsp.prepare(fs);
            dsp.setTargets(gains[gi], 0.0f, 1.0f, 1.0f);
            dsp.setClipGe(false);
            dsp.reset();
            std::vector<float> in = base, out(n);
            float* inCh[2] = {in.data(), in.data()};
            float* outCh[2] = {out.data(), out.data()};
            dsp.process(inCh, outCh, n, 2);

            const int aStart = (int)(0.002 * fs), aLen = (int)(0.020 * fs);   // 2–22 ms
            const int sStart = (int)(0.200 * fs), sLen = (int)(0.200 * fs);   // 200–400 ms
            const double atk = rmsWin(out, aStart, aLen);
            const double sus = rmsWin(out, sStart, sLen);

            // 高频能量占比（>2kHz）分别在音头窗与延音窗
            auto hfRatio = [&](int from, int len) {
                Bq hp; hp.hp(fs, 2000.0, 0.7071);
                double s = 0.0, t = 0.0;
                for (int i = 0; i < len; ++i) {
                    const double v = out[from + i];
                    const double h = hp.tick(v);
                    s += h * h; t += v * v;
                }
                return t > 1e-12 ? 10.0 * std::log10(s / t + 1e-12) : 0.0;
            };
            printf("%6.2f %4s %6s %9.2f %9.2f %8.1f %8.1f %8.1f\n", gains[gi], "SI", "1st",
                   20.0 * std::log10(std::max(atk, 1e-9)), 20.0 * std::log10(std::max(sus, 1e-9)),
                   20.0 * std::log10(std::max(atk, 1e-9) / std::max(sus, 1e-9)),
                   hfRatio(aStart, aLen), hfRatio(sStart, sLen));
        }
        }
    }
}


// worst case output level, parameter jump, NaN injection, illegal parameters
static void runSafety() {
    const double fs = 48000.0, f0 = 1000.0;
    printf("\n[safety] worst-case peak (1 kHz sine, per setting: 0.25/0.5/1/2 FS)\n");
    printf("%-5s %-6s %-5s %-5s %9s %9s\n", "gain", "level", "low", "tone", "peakFS", "peakdB");

    const double amps[4] = {0.25, 0.5, 1.0, 2.0};
    double worstDb = -999.0;
    for (int g = 0; g <= 1; ++g)
    for (int l = 0; l <= 1; ++l)
    for (int lo = 0; lo <= 1; ++lo)
    for (int t = 0; t <= 1; ++t) {
        double peak = 0.0;
        for (int ai = 0; ai < 4; ++ai) {
            iris::IrisDsp dsp;
            dsp.prepare(fs);
            dsp.setTargets((float)g, (float)lo, (float)t, (float)l);
            dsp.reset();
            const int n = (int)(0.5 * fs);
            double pa = 0.0;
            for (int i = 0; i < n; ++i) {
                const double x = amps[ai] * std::sin(2.0 * PI * f0 * i / fs);
                const double y = dsp.processMono(x);
                if (std::fabs(y) > pa) pa = std::fabs(y);
            }
            if (g == 1 && l == 1 && t == 1)
                printf("    [detail] vin=%.2f FS -> peak=%.3f FS (%+.2f dBFS)\n",
                       amps[ai], pa, 20.0 * std::log10(std::max(1e-12, pa)));
            if (pa > peak) peak = pa;
        }
        const double db = 20.0 * std::log10(std::max(1e-12, peak));
        if (db > worstDb) worstDb = db;
        printf("%-5d %-6d %-5d %-5d %9.3f %9.2f\n", g, l, lo, t, peak, db);
    }
    printf("[safety] worst peak across all settings = %+.2f dBFS\n", worstDb);

    // parameter jump: GAIN 0 -> 100 % at 0.2 s, 0.5 FS input
    {
        iris::IrisDsp dsp;
        dsp.prepare(fs);
        dsp.setTargets(0.0f, 0.25f, 1.0f, 0.8f);
        dsp.reset();
        double peak = 0.0, maxStep = 0.0, prev = 0.0;
        const int n = (int)(0.6 * fs);
        for (int i = 0; i < n; ++i) {
            if (i == (int)(0.2 * fs)) dsp.setTargets(1.0f, 0.25f, 1.0f, 0.8f);
            const double y = dsp.processMono(0.5 * std::sin(2.0 * PI * f0 * i / fs));
            peak = std::max(peak, std::fabs(y));
            maxStep = std::max(maxStep, std::fabs(y - prev));
            prev = y;
        }
        printf("[safety] GAIN 0->100 jump: peak=%.3f FS (%+.2f dBFS), max step=%.4f\n",
               peak, 20.0 * std::log10(std::max(1e-12, peak)), maxStep);
    }

    // NaN injection: 8 NaN samples in the middle of a clean tone
    {
        iris::IrisDsp dsp;
        dsp.prepare(fs);
        dsp.setTargets(0.6f, 0.25f, 1.0f, 0.8f);
        dsp.reset();
        double peakAfter = 0.0;
        int recover = -1;
        const int n = (int)(0.4 * fs);
        const int bad = (int)(0.2 * fs);
        for (int i = 0; i < n; ++i) {
            double x = 0.3 * std::sin(2.0 * PI * f0 * i / fs);
            if (i >= bad && i < bad + 8) x = std::nan("");
            const double y = dsp.processMono(x);
            if (i >= bad + 8) {
                peakAfter = std::max(peakAfter, std::fabs(y));
                if (recover < 0 && std::fabs(y) > 1e-3) recover = i - (bad + 8);
            }
        }
        printf("[safety] NaN x8 injected: peak after=%.3f FS, signal back in %d samples\n",
               peakAfter, recover);
    }

    // illegal parameter values straight from a broken host
    {
        iris::IrisDsp dsp;
        dsp.prepare(fs);
        dsp.setTargets(std::nanf(""), 5.0f, -1.0f, 100.0f);
        dsp.reset();
        double peak = 0.0;
        bool finite = true;
        for (int i = 0; i < (int)(0.2 * fs); ++i) {
            const double y = dsp.processMono(0.5 * std::sin(2.0 * PI * f0 * i / fs));
            if (!std::isfinite(y)) finite = false;
            peak = std::max(peak, std::fabs(y));
        }
        printf("[safety] params NaN/5/-1/100: output finite=%s, peak=%.3f FS\n",
               finite ? "yes" : "no", peak);
    }
}

// -------------------------------------------------------------- dynamics ----
// 1) 压缩曲线：输入电平 -> 输出基波电平，看斜率（1.0 = 线性，0 = 完全压死）
// 2) 波峰因数 crest = peak/rms（正弦 3.0 dB，硬方波 0 dB）
// 3) 闷音：90 Hz 短促脉冲串（attack 5 ms、decay 120 ms），看起音冲击峰/持续电平
static void runDynamics() {
    const double fs = 48000.0;
    const char* modeName[2] = {"SI (2x1N4148 each way)", "GE (+1N34A asym)"};

    for (int mode = 0; mode < 2; ++mode) {
        printf("\n[dyn] %s — 压缩曲线：GAIN 100%%（LOWCUT 0，TONE/LEVEL 全开，1 kHz 正弦）\n", modeName[mode]);
        printf("%8s %10s %10s %8s %8s\n", "VIN(V)", "OUT1k(dBV)", "in_dB", "slope", "crest");
        const double vins[6] = {0.02, 0.05, 0.10, 0.20, 0.40, 0.80};
        double prevIn = 0.0, prevOut = 0.0;
        for (int i = 0; i < 6; ++i) {
            iris::IrisDsp dsp;
            dsp.prepare(fs);
            dsp.setTargets(1.0f, 0.0f, 1.0f, 1.0f);
            dsp.setClipGe(mode == 1);
            dsp.reset();

            const int settle = (int)(0.30 * fs);
            const int meas = 4800;
            const int total = settle + meas;
            std::vector<float> in(total), out(total);
            const double a = vins[i] / iris::IrisDsp::kInVolts;
            for (int k = 0; k < total; ++k) in[k] = (float)(a * std::sin(2.0 * PI * 1000.0 * k / fs));

            float* inCh[2] = {in.data(), in.data()};
            float* outCh[2] = {out.data(), out.data()};
            dsp.process(inCh, outCh, total, 2);

            double peak = 0.0, sum2 = 0.0;
            for (int k = settle; k < total; ++k) {
                const double y = out[k];
                peak = std::max(peak, std::fabs(y));
                sum2 += y * y;
            }
            const double rms = std::sqrt(sum2 / (double)meas);
            std::vector<float> tail(out.begin() + settle, out.end());
            const double a1 = bin(tail, 1, 1000.0, fs) * iris::IrisDsp::kInVolts;
            const double outDb = 20.0 * std::log10(std::max(a1, 1e-12));
            const double inDb = 20.0 * std::log10(vins[i]);
            const double crest = 20.0 * std::log10(std::max(peak, 1e-12) / std::max(rms, 1e-12));
            double slope = 0.0;
            if (i > 0) slope = (outDb - prevOut) / (inDb - prevIn);
            printf("%8.3f %10.2f %10.2f %8.2f %8.2f\n", vins[i], outDb, inDb, slope, crest);
            prevIn = inDb;
            prevOut = outDb;
        }

        printf("[dyn] %s — 闷音冲击：90 Hz 脉冲（attack 5 ms，decay 120 ms）\n", modeName[mode]);
        printf("%8s %10s %10s %8s\n", "VIN(V)", "peak", "avg", "impact");
        const double vins2[3] = {0.05, 0.10, 0.20};
        for (int i = 0; i < 3; ++i) {
            iris::IrisDsp dsp;
            dsp.prepare(fs);
            dsp.setTargets(1.0f, 0.5f, 1.0f, 1.0f);
            dsp.setClipGe(mode == 1);
            dsp.reset();
            const double a = vins2[i] / iris::IrisDsp::kInVolts;
            const int n = (int)(0.30 * fs);
            double peakEarly = 0.0, sumLate = 0.0;
            int lateN = 0;
            for (int k = 0; k < n; ++k) {
                const double t = (double)k / fs;
                const double tin = std::fmod(t, 0.100);
                double env = 0.0;
                if (tin < 0.150)
                    env = (tin < 0.005) ? (tin / 0.005) : std::exp(-(tin - 0.005) / 0.040);
                const double x = a * env * std::sin(2.0 * PI * 90.0 * t);
                const double y = dsp.processMono(x);
                if (k > (int)(0.05 * fs) && k < (int)(0.085 * fs))
                    peakEarly = std::max(peakEarly, std::fabs(y));
                if (k > (int)(0.10 * fs) && k < (int)(0.16 * fs)) { sumLate += y * y; ++lateN; }
            }
            const double avg = std::sqrt(sumLate / std::max(1, lateN));
            printf("%8.3f %10.3f %10.3f %7.2fx\n", vins2[i], peakEarly, avg,
                   peakEarly / std::max(avg, 1e-9));
        }
    }
}

int wmain() {
    GdiplusStartupInput si;
    ULONG_PTR token = 0;
    GdiplusStartup(&token, &si, nullptr);

    renderUi(L"build/ui_preview.png");

    const double fs = 48000.0;
    const double f0 = 1000.0;
    const int settle = (int)(0.25 * fs);   // 参数平滑 8ms + 耦合电容建立
    const int meas = 48 * 88;              // 48 点/周期 × 88 周期，整周期 → 无泄漏

    const double gains[5] = {0.0, 0.25, 0.5, 0.75, 1.0};
    const double levels[4] = {0.01, 0.05, 0.1, 0.3};

    printf("[dsp] fs=%.0f f0=%.0f settle=%d meas=%d\n", fs, f0, settle, meas);
    printf("%-6s %-7s %9s %9s %9s\n", "GAIN", "VIN(V)", "OUT(dB)", "THD(%)", "H3(%)");

    for (int gi = 0; gi < 5; ++gi) {
        for (int li = 0; li < 4; ++li) {
            iris::IrisDsp dsp;
            dsp.prepare(fs);
            dsp.setTargets((float)gains[gi], 1.0f, 1.0f, 1.0f);  // LOWCUT=1（切最多，对齐设计文档口径）
            dsp.reset();

            const int total = settle + meas;
            std::vector<float> in(total), out(total);
            for (int i = 0; i < total; ++i)
                in[i] = (float)(levels[li] * std::sin(2.0 * PI * f0 * i / fs));

            float* inCh[2] = {in.data(), in.data()};
            float* outCh[2] = {out.data(), out.data()};
            dsp.process(inCh, outCh, total, 2);

            std::vector<float> tail(out.begin() + settle, out.end());
            const double a1 = bin(tail, 1, f0, fs);
            double h2 = 0.0, h3 = 0.0, h4 = 0.0, h5 = 0.0, hs = 0.0;
            for (int k = 2; k <= 9; ++k) {
                const double hk = bin(tail, k, f0, fs);
                hs += hk * hk;
                if (k == 2) h2 = hk;
                if (k == 3) h3 = hk;
                if (k == 4) h4 = hk;
                if (k == 5) h5 = hk;
            }
            const double thd = std::sqrt(hs) / std::max(a1, 1e-12) * 100.0;
            const double vinV = levels[li] * iris::IrisDsp::kInVolts;
            const double a1v = a1 * iris::IrisDsp::kInVolts;   // 输出是归一化 FS，换回伏特
            const double db = 20.0 * std::log10(std::max(a1v, 1e-12) / std::max(vinV, 1e-12));
            printf("%-6.2f %-7.3f %9.2f %9.2f %9.2f\n", gains[gi], vinV, db, thd,
                   h3 / std::max(a1, 1e-12) * 100.0);
        }
    }

    // ---- 杂散（毛刺）检测：1 kHz 单音，扣除 1..12 次谐波后的残余能量 / 基波 ----
    {
        for (int gi = 0; gi < 2; ++gi) {
            iris::IrisDsp dsp;
            dsp.prepare(48000.0);
            dsp.setTargets(gi == 0 ? 0.85f : 1.0f, 0.0f, 1.0f, 0.85f);
            const int total = 24000 + 4800;
            std::vector<float> in(total), out(total);
            double a = 0.8 / iris::IrisDsp::kInVolts;
            for (int k = 0; k < total; ++k) in[k] = (float)(a * std::sin(2.0 * PI * 1000.0 * k / 48000.0));
            float* inCh[2] = {in.data(), in.data()};
            float* outCh[2] = {out.data(), out.data()};
            dsp.process(inCh, outCh, total, 2);
            std::vector<float> tail(out.begin() + 24000, out.end());
            // 最小二乘拟合 1..12 次谐波（sin/cos），残差即杂散
            const int K = 12;
            double A[2 * K + 1][2 * K + 2] = {};
            const int n = (int)tail.size();
            for (int i = 0; i < n; ++i) {
                double basis[2 * K + 1];
                basis[0] = 1.0;
                for (int k = 1; k <= K; ++k) {
                    const double w = 2.0 * PI * 1000.0 * k * i / 48000.0;
                    basis[2 * k - 1] = std::cos(w);
                    basis[2 * k] = std::sin(w);
                }
                for (int r = 0; r <= 2 * K; ++r) {
                    for (int c2 = 0; c2 <= 2 * K; ++c2) A[r][c2] += basis[r] * basis[c2];
                    A[r][2 * K + 1] += basis[r] * tail[i];
                }
            }
            // 高斯消元
            for (int r = 0; r <= 2 * K; ++r) {
                int piv = r;
                for (int q = r + 1; q <= 2 * K; ++q) if (std::fabs(A[q][r]) > std::fabs(A[piv][r])) piv = q;
                if (piv != r) for (int c2 = 0; c2 <= 2 * K + 1; ++c2) { const double t = A[r][c2]; A[r][c2] = A[piv][c2]; A[piv][c2] = t; }
                const double d = A[r][r];
                if (std::fabs(d) < 1e-18) continue;
                for (int q = r + 1; q <= 2 * K; ++q) {
                    const double f2 = A[q][r] / d;
                    if (f2 == 0.0) continue;
                    for (int c2 = r; c2 <= 2 * K + 1; ++c2) A[q][c2] -= f2 * A[r][c2];
                }
            }
            double x[2 * K + 1] = {};
            for (int r = 2 * K; r >= 0; --r) {
                double s2 = A[r][2 * K + 1];
                for (int c2 = r + 1; c2 <= 2 * K; ++c2) s2 -= A[r][c2] * x[c2];
                x[r] = (std::fabs(A[r][r]) < 1e-18) ? 0.0 : s2 / A[r][r];
            }
            double res = 0.0, sig = 0.0;
            for (int i = 0; i < n; ++i) {
                double fit = x[0];
                for (int k = 1; k <= K; ++k) {
                    const double w = 2.0 * PI * 1000.0 * k * i / 48000.0;
                    fit += x[2 * k - 1] * std::cos(w) + x[2 * k] * std::sin(w);
                }
                const double e = tail[i] - fit;
                res += e * e;
                sig += tail[i] * tail[i];
            }
            const double spur = 100.0 * std::sqrt(res / std::max(1e-20, sig));
            const double fund = bin(tail, 1, 1000.0, 48000.0);
            printf("[spur] GAIN=%.2f  fund=%.3f FS  non-harmonic=%.2f%% of total\n",
                         gi == 0 ? 0.85 : 1.0, fund, spur);
        }
    }

    // ---- 爆音/跳变压力测试：噪声驱动的和弦式激励，GAIN=100%，统计样本级跳变 ----
    {
        iris::IrisDsp dsp;
        dsp.prepare(48000.0);
        dsp.setTargets(1.0f, 0.0f, 0.7f, 0.85f);
        const int total = 48000 * 2;
        std::vector<float> in(total), out(total);
        unsigned seed = 12345u;
        double env = 0.0;
        for (int k = 0; k < total; ++k) {
            seed = seed * 1664525u + 1013904223u;
            const double r = ((double)((seed >> 8) & 0xFFFF) / 32768.0) - 1.0;
            // 每 0.5s 一个和弦包络，内部快速起音
            const int phase = k % 24000;
            env = (phase < 2400) ? (double)phase / 2400.0 : (1.0 - (double)(phase - 2400) / 21600.0);
            if (env < 0.0) env = 0.0;
            in[k] = (float)(0.85 * env * (0.55 * std::sin(2.0 * PI * 196.0 * k / 48000.0)
                                        + 0.30 * std::sin(2.0 * PI * 293.7 * k / 48000.0)
                                        + 0.15 * r));
        }
        float* inCh[2] = {in.data(), in.data()};
        float* outCh[2] = {out.data(), out.data()};
        dsp.process(inCh, outCh, total, 2);
        double maxJump = 0.0; int big = 0, huge = 0; int nonFinite = 0;
        double peak = 0.0; int clampHits = 0;
        for (int k = 4800; k < total; ++k) {
            if (!std::isfinite(out[k])) { ++nonFinite; continue; }
            const double a = std::fabs((double)out[k]);
            if (a > peak) peak = a;
            if (a >= 0.9949) ++clampHits;
            const double d = std::fabs((double)out[k] - (double)out[k - 1]);
            if (d > maxJump) maxJump = d;
            if (d > 0.15) ++big;
            if (d > 0.30) ++huge;
        }
        iris::resetSolverStats();
        printf("[stress] GAIN=1.0  maxJump=%.4f FS  jumps>0.15=%d  jumps>0.30=%d  nonFinite=%d\n", maxJump, big, huge, nonFinite);
        iris::resetSolverStats();
        printf("[solver] unconverged=%lld  badly=%lld\n", iris::solverBad, iris::solverWorse);
        printf("[peak]   max=%.3f FS  samples_at_ceiling(>=0.995)=%d  (硬限幅是否介入)\n", peak, clampHits);
    }

    // ---- 实时性能：处理 10 秒立体声，报 realtime factor ----
    {
        iris::IrisDsp dsp;
        dsp.prepare(48000.0);
        dsp.setTargets(1.0f, 0.0f, 0.7f, 0.85f);
        const int total = 48000 * 10;
        std::vector<float> in(total), out(total);
        for (int k = 0; k < total; ++k) in[k] = (float)(0.25 * std::sin(2.0 * PI * 196.0 * k / 48000.0));
        float* inCh[2] = {in.data(), in.data()};
        float* outCh[2] = {out.data(), out.data()};
        const auto t0 = std::chrono::high_resolution_clock::now();
        dsp.process(inCh, outCh, total, 2);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const double sec = std::chrono::duration<double>(t1 - t0).count();
        const double audio = 10.0;
        printf("[perf] 10s stereo audio rendered in %.3f s  ->  %.1f%% of one core (RT factor %.2fx)\n", sec, 100.0 * sec / audio, audio / sec);
    }

    runSafety();
    runDynamics();
    runAttack();

    // ---- SI / GE 谐波对比：GAIN 100%，0.2 V 1 kHz ----
    for (int mode = 0; mode < 2; ++mode) {
        iris::IrisDsp dsp;
        dsp.prepare(48000.0);
        dsp.setTargets(1.0f, 0.0f, 1.0f, 1.0f);
        dsp.setClipGe(mode == 1);
        dsp.reset();
        const int total = 24000 + 4800;
        std::vector<float> in(total), out(total);
        double a = 0.8 / iris::IrisDsp::kInVolts;
        for (int k = 0; k < total; ++k) in[k] = (float)(a * std::sin(2.0 * PI * 1000.0 * k / 48000.0));
        float* inCh[2] = {in.data(), in.data()};
        float* outCh[2] = {out.data(), out.data()};
        dsp.process(inCh, outCh, total, 2);
        std::vector<float> tail(out.begin() + 24000, out.end());
        const double h1 = bin(tail, 1, 1000.0, 48000.0);
        wchar_t line[160];
        std::swprintf(line, 200, L"[harm] %s  H1=%.3f FS  H2=%.2f%%  H3=%.2f%%  H4=%.2f%%  H5=%.2f%%  H7=%.2f%%  H9=%.2f%%\n",
                      mode == 1 ? L"GE (asym)" : L"SI (sym)", h1,
                      100.0 * bin(tail, 2, 1000.0, 48000.0) / std::max(1e-9, h1),
                      100.0 * bin(tail, 3, 1000.0, 48000.0) / std::max(1e-9, h1),
                      100.0 * bin(tail, 4, 1000.0, 48000.0) / std::max(1e-9, h1),
                      100.0 * bin(tail, 5, 1000.0, 48000.0) / std::max(1e-9, h1),
                      100.0 * bin(tail, 7, 1000.0, 48000.0) / std::max(1e-9, h1),
                      100.0 * bin(tail, 9, 1000.0, 48000.0) / std::max(1e-9, h1));
        std::wprintf(L"%ls", line);
    }

    // ---- 临时探针：GAIN 100%，0.2 V 输入，打印内部 u / v / v3 ----
    {
        iris::IrisDsp dsp;
        dsp.prepare(48000.0);
        dsp.setTargets(1.0f, 0.0f, 1.0f, 1.0f);
        dsp.setClipGe(false);
        dsp.reset();
        printf("\n[probe] GAIN100 0.2V 1kHz (u/v/v3 snapshots at peaks)\n");
        double bestU = 0, bestV = 0, bestV3 = 0, bestT1 = 0, bestY = 0, bestOut = 0,
               bestAcc = 0, bestDec = 0;
        for (int i = 0; i < 24000; ++i) {
            const double x = (0.2 / iris::IrisDsp::kInVolts) * std::sin(2.0 * PI * 1000.0 * i / 48000.0);
            const double y = dsp.processMono(x);
            if (i < 19200) continue;   // 只统计最后 0.1 s（稳态）
            if (std::fabs(dsp.dbgU()) > std::fabs(bestU)) bestU = dsp.dbgU();
            if (std::fabs(dsp.dbgV()) > std::fabs(bestV)) bestV = dsp.dbgV();
            if (std::fabs(dsp.dbgV3()) > std::fabs(bestV3)) bestV3 = dsp.dbgV3();
            if (std::fabs(dsp.dbgT1()) > std::fabs(bestT1)) bestT1 = dsp.dbgT1();
            if (std::fabs(dsp.dbgY()) > std::fabs(bestY)) bestY = dsp.dbgY();
            if (std::fabs(y) > std::fabs(bestOut)) bestOut = y;
            if (std::fabs(dsp.dbgAcc()) > std::fabs(bestAcc)) bestAcc = dsp.dbgAcc();
            if (std::fabs(dsp.dbgDec()) > std::fabs(bestDec)) bestDec = dsp.dbgDec();
        }
        printf("[probe] 稳态: u=%+.3f v=%+.3f v3=%+.3f t1=%+.3f y=%+.3f acc=%+.3f dec=%+.3f out=%+.3f FS (rail=%.2f)\n",
               bestU, bestV, bestV3, bestT1, bestY, bestAcc, bestDec, bestOut,
               iris::IrisDsp::kRail);

        // 连续 24 个输入样本的内部状态（稳态）
        printf("[probe] 连续样本: n  v  v3  y  acc  dec\n");
        for (int i = 0; i < 24; ++i) {
            const double x = (0.2 / iris::IrisDsp::kInVolts) * std::sin(2.0 * PI * 1000.0 * i / 48000.0);
            const double y = dsp.processMono(x);
            printf("[probe] %2d  %+.3f  %+.3f  %+.3f  %+.3f  %+.3f\n", i, dsp.dbgV(), dsp.dbgV3(),
                   dsp.dbgY(), dsp.dbgAcc(), dsp.dbgDec());
            (void)y;
        }
    }

    GdiplusShutdown(token);
    return 0;
}


