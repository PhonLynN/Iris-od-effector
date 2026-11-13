// Iris R — 旋钮面板绘制（GDI+，供 VST3 编辑器与独立预览工具共用）
#pragma once
#ifndef Iris_UI_H
#define Iris_UI_H

#include "iris_params.h"

#include <windows.h>
#include <gdiplus.h>

#include <cmath>
#include <cstdio>
#include <string>

namespace irisui {

using namespace Gdiplus;

inline Color C(unsigned rgb, BYTE a = 255) {
    return Color(a, (BYTE)((rgb >> 16) & 0xFF), (BYTE)((rgb >> 8) & 0xFF), (BYTE)(rgb & 0xFF));
}

inline constexpr int kWidth = 560;
inline constexpr int kHeight = 300;

class KnobPanel {
public:
    void init() {
        for (int i = 0; i < iris::kNumParams; ++i) v_[i] = iris::meta(i).def;
    }

    float value(int i) const { return v_[i]; }
    void setValue(int i, float x) {
        if (x < 0.0f) x = 0.0f;
        if (x > 1.0f) x = 1.0f;
        v_[i] = x;
    }

    int hitTest(int x, int y) const {
        for (int i = 0; i < iris::kNumParams; ++i) {
            const float dx = (float)x - kx[i];
            const float dy = (float)y - ky;
            if (dx * dx + dy * dy <= 44.0f * 44.0f) return i;
        }
        return -1;
    }

    void paint(Graphics& g, int w, int h) {
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);

        g.Clear(C(0xf7f4ed));
        {
            SolidBrush bg(C(0xf7f4ed));
            g.FillRectangle(&bg, 0, 0, w, h);
            Pen border(C(0xd8d2c4), 1.0f);
            g.DrawRectangle(&border, 0, 0, w - 1, h - 1);
        }

        {
            FontFamily serif(L"Georgia"), cjk(L"Noto Serif SC");
            Font f1(&serif, 22.0f, FontStyleBold, UnitPixel);
            SolidBrush ink(C(0x16150f));
            drawTracked(g, L"Iris R", f1, ink, 26.0f, 22.0f, 1.8f);

            Font f2(&cjk, 8.5f, FontStyleRegular, UnitPixel);
            SolidBrush muted(C(0x6b6558));
            drawTracked(g, L"TRANSPARENT OVERDRIVE", f2, muted, 27.0f, 50.0f, 1.6f);

            Pen hair(C(0x16150f), 1.0f);
            g.DrawLine(&hair, 26.0f, 66.0f, (float)(w - 26), 66.0f);
        }

        for (int i = 0; i < iris::kNumParams; ++i) drawKnob(g, i);

        {
            FontFamily mono(L"Cascadia Mono");
            Font f3(&mono, 8.0f, FontStyleRegular, UnitPixel);
            SolidBrush muted(C(0x6b6558));
            drawTracked(g, L"v1.0  \u00b7  ngspice-47 verified", f3, muted, 27.0f, (float)(h - 26), 0.6f);
        }
    }

private:
    void drawKnob(Graphics& g, int i) {
        const float cx = kx[i], cy = ky, r = 31.0f;
        SolidBrush paper(C(0xf7f4ed));
        SolidBrush white(C(0xfffcf4));
        SolidBrush ink(C(0x16150f));
        Pen ringP(C(0x16150f), 1.3f);
        Pen softP(C(0xe2dbcc), 1.0f);
        Pen tickP(C(0xd0c8b7), 1.0f);
        Pen tickHotP(C(0x0f766e), 1.6f);

        for (int t = 0; t <= 10; ++t) {
            const double frac = t / 10.0;
            const double ang = (-135.0 + 270.0 * frac) * iris::kPi / 180.0;
            const float r1 = r + 4.0f, r2 = r + (t % 5 == 0 ? 10.0f : 7.0f);
            const bool hot = std::fabs(frac - v_[i]) < 0.055;
            Pen& p = hot ? tickHotP : tickP;
            g.DrawLine(&p, cx + (float)(std::sin(ang) * r1), cy - (float)(std::cos(ang) * r1),
                       cx + (float)(std::sin(ang) * r2), cy - (float)(std::cos(ang) * r2));
        }

        g.FillEllipse(&white, cx - r + 3.0f, cy - r + 3.0f, (r - 3.0f) * 2, (r - 3.0f) * 2);
        g.DrawEllipse(&ringP, cx - r + 3.0f, cy - r + 3.0f, (r - 3.0f) * 2, (r - 3.0f) * 2);
        g.DrawEllipse(&softP, cx - r, cy - r, r * 2, r * 2);

        const double ang = (-135.0 + 270.0 * v_[i]) * iris::kPi / 180.0;
        const float ix = cx + (float)(std::sin(ang) * 7.0), iy = cy - (float)(std::cos(ang) * 7.0);
        const float ox = cx + (float)(std::sin(ang) * 25.0), oy = cy - (float)(std::cos(ang) * 25.0);
        Pen ptr(C(0x16150f), 2.6f);
        ptr.SetStartCap(LineCapRound);
        ptr.SetEndCap(LineCapRound);
        g.DrawLine(&ptr, ix, iy, ox, oy);

        FontFamily cjk(L"Noto Serif SC"), mono(L"Cascadia Mono");
        Font fl(&cjk, 10.0f, FontStyleRegular, UnitPixel);
        Font fv(&mono, 9.5f, FontStyleRegular, UnitPixel);
        SolidBrush muted(C(0x6b6558));

        const wchar_t* label = L"GAIN";
        switch (i) {
        case 0: label = L"GAIN"; break;
        case 1: label = L"LOWCUT"; break;
        case 2: label = L"TONE"; break;
        case 3: label = L"BLEND"; break;
        default: label = L"VOLUME"; break;
        }
        RectF lb;
        g.MeasureString(label, (INT)wcslen(label), &fl, PointF(0, 0), &lb);
        drawTracked(g, label, fl, ink, cx - lb.Width * 0.5f, cy + 42.0f, 2.0f);

        const std::wstring val = readout(i);
        RectF vb;
        g.MeasureString(val.c_str(), (INT)val.size(), &fv, PointF(0, 0), &vb);
        drawTracked(g, val.c_str(), fv, muted, cx - vb.Width * 0.5f, cy + 56.0f, 0.4f);
    }

    std::wstring readout(int i) const {
        wchar_t buf[64];
        switch (i) {
        case 0: std::swprintf(buf, 64, L"%+.1f dB", iris::gainDb(v_[0], v_[1])); break;
        case 1: {
            const double f = iris::lowcutHz(v_[1]);
            if (f >= 100.0) std::swprintf(buf, 64, L"%d Hz", (int)(f + 0.5));
            else std::swprintf(buf, 64, L"%.1f Hz", f);
            break;
        }
        case 2: std::swprintf(buf, 64, L"%+.1f dB", iris::toneShelfDb(v_[2])); break;
        case 3: std::swprintf(buf, 64, L"%.0f %%", iris::blendPct(v_[3])); break;
        default: {
            const double d = iris::volumeDb(v_[4]);
            if (d < -59.0) std::swprintf(buf, 64, L"-inf");
            else std::swprintf(buf, 64, L"%+.1f dB", d);
            break;
        }
        }
        return std::wstring(buf);
    }

    static float drawTracked(Graphics& g, const wchar_t* s, Font& f, Brush& b, float x, float y, float track) {
        SolidBrush* sb = static_cast<SolidBrush*>(&b);
        float cur = x;
        const size_t n = wcslen(s);
        for (size_t i = 0; i < n; ++i) {
            const wchar_t ch[2] = {s[i], 0};
            RectF r;
            g.MeasureString(ch, 1, &f, PointF(0, 0), &r);
            g.DrawString(ch, 1, &f, PointF(cur, y), sb);
            cur += r.Width + track;
        }
        return cur - x - (n ? track : 0.0f);
    }

    float v_[iris::kNumParams] = {0.55f, 0.15f, 0.85f, 0.60f, 0.85f};
    static constexpr float kx[iris::kNumParams] = {78.0f, 180.0f, 282.0f, 384.0f, 486.0f};
    static constexpr float ky = 158.0f;
};

} // namespace irisui
#endif // Iris_UI_H
