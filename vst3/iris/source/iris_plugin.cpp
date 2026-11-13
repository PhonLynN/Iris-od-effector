// Iris R — VST3 单组件插件（处理器 + 编辑器视图）
#include "iris_dsp.h"
#include "iris_params.h"
#include <cmath>
#include "iris_ui.h"
#include <pluginterfaces/base/ibstream.h>

#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <cstring>
#include <windowsx.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

#ifndef IRIS_VERSION_STR
#define IRIS_VERSION_STR "0.4.0"
#endif

namespace {

void setStr(TChar* dst, const char* s) {
    int i = 0;
    for (; s[i] && i < 127; ++i) dst[i] = (TChar)(unsigned char)s[i];
    dst[i] = 0;
}

double toDouble(const TChar* s) {
    double v = 0.0, f = 0.1;
    int sign = 1;
    bool frac = false;
    if (!s) return 0.0;
    while (*s == ' ') ++s;
    if (*s == '-') { sign = -1; ++s; } else if (*s == '+') { ++s; }
    for (; *s; ++s) {
        if (*s >= '0' && *s <= '9') {
            if (!frac) v = v * 10.0 + (*s - '0');
            else { v += (*s - '0') * f; f *= 0.1; }
        } else if (*s == '.') { frac = true; }
        else break;
    }
    return sign * v;
}

bool iidEq(const TUID a, const TUID b) { return std::memcmp(a, b, 16) == 0; }

// ---------- 编辑器视图（Win32 + GDI+） ------------------------------
class IrisView : public IPlugView {
public:
    explicit IrisView(EditController* ctrl) : ctrl_(ctrl) {
        ctrl_->addRef();
        panel_.init();
        syncFromParams();
        gdiplusStart();
    }
    ~IrisView() {
        if (hwnd_) DestroyWindow(hwnd_);
        ctrl_->release();
        gdiplusStop();
    }

    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override {
        if (!obj) return kInvalidArgument;
        if (iidEq(iid, FUnknown::iid) || iidEq(iid, IPlugView::iid)) {
            *obj = static_cast<IPlugView*>(this);
            addRef();
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs_; }
    uint32 PLUGIN_API release() override {
        if (--refs_ == 0) { delete this; return 0; }
        return refs_;
    }

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override {
        if (type && std::strcmp(type, kPlatformTypeHWND) == 0) return kResultTrue;
        return kResultFalse;
    }

    tresult PLUGIN_API attached(void* parent, FIDString /*type*/) override {
        if (!parent) return kInvalidArgument;
        if (!ensureClass()) return kResultFalse;
        ViewRect r;
        getSize(&r);
        hwnd_ = CreateWindowExW(0, kClassName, L"Iris R",
                                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                                0, 0, r.getWidth(), r.getHeight(),
                                (HWND)parent, nullptr, GetModuleHandleW(nullptr), this);
        if (!hwnd_) return kResultFalse;
        SetTimer(hwnd_, 1, 33, nullptr);   // 定时刷新界面
        return kResultOk;
    }

    tresult PLUGIN_API removed() override {
        if (hwnd_) { KillTimer(hwnd_, 1); DestroyWindow(hwnd_); hwnd_ = nullptr; }
        return kResultOk;
    }

    tresult PLUGIN_API onWheel(float distance) override {
        if (active_ >= 0) adjust(active_, distance > 0 ? 0.02 : -0.02);
        return kResultOk;
    }
    tresult PLUGIN_API onKeyDown(char16 /*key*/, int16 /*code*/, int16 /*mods*/) override { return kResultFalse; }
    tresult PLUGIN_API onKeyUp(char16 /*key*/, int16 /*code*/, int16 /*mods*/) override { return kResultFalse; }
    tresult PLUGIN_API onFocus(TBool /*state*/) override { return kResultOk; }
    tresult PLUGIN_API setFrame(IPlugFrame* frame) override { frame_ = frame; return kResultOk; }
    tresult PLUGIN_API canResize() override { return kResultFalse; }
    tresult PLUGIN_API checkSizeConstraint(ViewRect* /*rect*/) override { return kResultTrue; }

    tresult PLUGIN_API getSize(ViewRect* size) override {
        if (!size) return kInvalidArgument;
        size->left = 0; size->top = 0;
        size->right = irisui::kWidth;
        size->bottom = irisui::kHeight;
        return kResultOk;
    }

    tresult PLUGIN_API onSize(ViewRect* newSize) override {
        if (hwnd_ && newSize)
            SetWindowPos(hwnd_, nullptr, 0, 0, newSize->getWidth(), newSize->getHeight(),
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return kResultOk;
    }

private:
// --- GDI+ 生命周期（同一 DLL 内共用） ---
    static void gdiplusStart() {
        if (gdiRefs_++ == 0) {
            Gdiplus::GdiplusStartupInput in;
            Gdiplus::GdiplusStartup(&gdiToken_, &in, nullptr);
        }
    }
    static void gdiplusStop() {
        if (--gdiRefs_ == 0 && gdiToken_) { Gdiplus::GdiplusShutdown(gdiToken_); gdiToken_ = 0; }
    }

    void syncFromParams() {
        for (int i = 0; i < iris::kNumParams; ++i)
            panel_.setValue(i, (float)ctrl_->getParamNormalized((ParamID)i));
    }

    void adjust(int idx, float delta) {
        if (idx < 0 || idx >= iris::kNumParams) return;
        float v = panel_.value(idx) + delta;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        panel_.setValue(idx, v);
        ctrl_->beginEdit((ParamID)idx);
        ctrl_->setParamNormalized((ParamID)idx, v);
        ctrl_->performEdit((ParamID)idx, v);
        ctrl_->endEdit((ParamID)idx);
        invalidate();
    }

    void invalidate() { if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE); }

    void paint(HDC dc) {
        Gdiplus::Bitmap bmp(irisui::kWidth, irisui::kHeight, PixelFormat32bppPARGB);
        {
            Gdiplus::Graphics g(&bmp);
            panel_.paint(g, irisui::kWidth, irisui::kHeight);
        }
        Gdiplus::Graphics screen(dc);
        screen.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        screen.DrawImage(&bmp, 0, 0);
    }

    static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
        auto* self = reinterpret_cast<IrisView*>(GetWindowLongPtrW(h, GWLP_USERDATA));
        switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            if (!self) break;
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            self->paint(dc);
            EndPaint(h, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (!self) break;
            SetFocus(h);
            const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
            const int idx = self->panel_.hitTest(x, y);
            self->active_ = idx;
            self->dragY_ = y;
            if (idx >= 0) { SetCapture(h); self->ctrl_->beginEdit((ParamID)idx); }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (!self || self->active_ < 0) break;
            const int y = GET_Y_LPARAM(lp);
            const float fine = (GetKeyState(VK_SHIFT) & 0x8000) ? 0.0016f : 0.0060f;
            const float d = (float)(self->dragY_ - y) * fine;
            self->dragY_ = y;
            if (d != 0.0f) self->adjust(self->active_, d);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (!self) break;
            if (self->active_ >= 0) {
                ReleaseCapture();
                self->ctrl_->endEdit((ParamID)self->active_);
            }
            self->active_ = -1;
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            if (!self) break;
            const int idx = self->panel_.hitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            if (idx >= 0) {
                const float def = iris::meta(idx).def;
                self->adjust(idx, def - self->panel_.value(idx));
            }
            return 0;
        }
        case WM_TIMER: {
            if (!self) break;
            bool dirty = false;
            for (int i = 0; i < iris::kNumParams; ++i) {
                const float pv = (float)self->ctrl_->getParamNormalized((ParamID)i);
                if (std::fabs(pv - self->panel_.value(i)) > 1e-5f) { self->panel_.setValue(i, pv); dirty = true; }
            }
            if (dirty) self->invalidate();
            return 0;
        }
        case WM_SETCURSOR:
            SetCursor(LoadCursorW(nullptr, (LPCWSTR)IDC_SIZENS));
            return TRUE;
        default:
            break;
        }
        return DefWindowProcW(h, msg, wp, lp);
    }

    static bool ensureClass() {
        if (classReg_) return true;
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = wndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
        wc.lpszClassName = kClassName;
        classReg_ = RegisterClassExW(&wc) != 0;
        return classReg_;
    }

    static constexpr const wchar_t* kClassName = L"GlassBellIrisView";
    static inline bool classReg_ = false;
    static inline ULONG_PTR gdiToken_ = 0;
    static inline int gdiRefs_ = 0;

    EditController* ctrl_ = nullptr;
    IPlugFrame* frame_ = nullptr;
    HWND hwnd_ = nullptr;
    int active_ = -1;
    int dragY_ = 0;
    uint32 refs_ = 1;
    irisui::KnobPanel panel_;
};

// ---------- 处理器（单组件：同时是 EditController） ----------------
class IrisProcessor : public SingleComponentEffect {
public:
    IrisProcessor() {
        for (int i = 0; i < iris::kNumParams; ++i) norm_[i] = iris::meta(i).def;
    }

    static FUnknown* createInstance(void*) {
        return static_cast<IAudioProcessor*>(new IrisProcessor());
    }

    tresult PLUGIN_API initialize(FUnknown* context) override {
        tresult r = SingleComponentEffect::initialize(context);
        if (r != kResultOk) return r;
        String128 name{};
        setStr(name, "Input");
        addAudioInput(name, SpeakerArr::kStereo);
        setStr(name, "Output");
        addAudioOutput(name, SpeakerArr::kStereo);
        return kResultOk;
    }

    tresult PLUGIN_API setActive(TBool state) override {
        if (state) dsp_.reset();
        return SingleComponentEffect::setActive(state);
    }

    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) override {
        dsp_.prepare(setup.sampleRate);
        applyParams();
        return SingleComponentEffect::setupProcessing(setup);
    }

    tresult PLUGIN_API setProcessing(TBool state) override {
        if (state) dsp_.reset();
        return SingleComponentEffect::setProcessing(state);
    }

    // 工程保存：把 11 个归一化参数原样写进 chunk
    tresult PLUGIN_API getState(IBStream* state) override {
        if (!state) return kResultFalse;
        for (int i = 0; i < iris::kNumParams; ++i) {
            float v = norm_[i];
            if (state->write(&v, sizeof(float), nullptr) != kResultOk) break;
        }
        return kResultOk;
    }

    // 工程加载：读回参数并同步 DSP / 面板
    tresult PLUGIN_API setState(IBStream* state) override {
        if (!state) return kResultFalse;
        for (int i = 0; i < iris::kNumParams; ++i) {
            float v = iris::meta(i).def;
            int32 got = 0;
            if (state->read(&v, sizeof(float), &got) != kResultOk || got != (int32)sizeof(float)) break;
            if (!std::isfinite(v)) v = iris::meta(i).def;
            v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            norm_[i] = v;
            setParamNormalized((ParamID)i, v);
        }
        applyParams();
        dsp_.reset();
        return kResultOk;
    }

    tresult PLUGIN_API process(ProcessData& data) override {
        // 参数自动化
        if (data.inputParameterChanges) {
            const int32 n = data.inputParameterChanges->getParameterCount();
            for (int32 i = 0; i < n; ++i) {
                if (auto* q = data.inputParameterChanges->getParameterData(i)) {
                    if (q->getPointCount() <= 0) continue;
                    int32 sampleOffset = 0;
                    ParamValue v = 0;
                    q->getPoint(0, sampleOffset, v);
                    const ParamID id = q->getParameterId();
                    if (id < iris::kNumParams) norm_[id] = (float)iris::uiValue(v);
                }
            }
        }
        applyParams();

        if (data.numSamples <= 0) return kResultOk;
        const int32 nCh = data.numOutputs > 0 ? data.outputs[0].numChannels : 0;
        const int32 nIn = data.numInputs > 0 ? data.inputs[0].numChannels : 0;
        float* out[2] = {nullptr, nullptr};
        float* in[2] = {nullptr, nullptr};
        for (int32 c = 0; c < 2 && c < nCh; ++c) out[c] = data.outputs[0].channelBuffers32[c];
        for (int32 c = 0; c < 2 && c < nIn; ++c) in[c] = data.inputs[0].channelBuffers32[c];
        if (!out[0]) return kResultOk;
        if (!in[0]) {
            for (int32 c = 0; c < nCh; ++c)
                if (out[c]) std::memset(out[c], 0, sizeof(float) * (size_t)data.numSamples);
            return kResultOk;
        }
        dsp_.process(in, out, data.numSamples, (int)(nCh > 2 ? 2 : nCh));
        return kResultOk;
    }

    // ---- EditController ----
    int32 PLUGIN_API getParameterCount() override { return iris::kNumParams; }

    tresult PLUGIN_API getParameterInfo(int32 index, ParameterInfo& info) override {
        if (index < 0 || index >= iris::kNumParams) return kInvalidArgument;
        const auto& m = iris::meta(index);
        std::memset(&info, 0, sizeof(info));
        info.id = (ParamID)index;
        setStr(info.title, m.name);
        setStr(info.shortTitle, m.name);
        setStr(info.units, m.unit);
        info.stepCount = 0;
        info.defaultNormalizedValue = m.def;
        info.unitId = kRootUnitId;
        info.flags = ParameterInfo::kCanAutomate;
        return kResultOk;
    }

    tresult PLUGIN_API getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string) override {
        char buf[64] = {0};
        const int i = (int)tag;
        const double v = valueNormalized < 0 ? 0 : (valueNormalized > 1 ? 1 : valueNormalized);
        switch (i) {
        case iris::kGain:
            std::snprintf(buf, sizeof(buf), "%+.1f dB", iris::gainDb(v, norm_[iris::kLowcut]));
            break;
        case iris::kLowcut:
            std::snprintf(buf, sizeof(buf), "%.0f Hz", iris::lowcutHz(v));
            break;
        case iris::kTone:
            std::snprintf(buf, sizeof(buf), "%+.1f dB", iris::toneShelfDb(v));
            break;
        case iris::kBlend:
            std::snprintf(buf, sizeof(buf), "%.0f %%", iris::blendPct(v));
            break;
        default:
            std::snprintf(buf, sizeof(buf), "%+.1f dB", iris::volumeDb(v));
            break;
        }
        setStr(string, buf);
        return kResultOk;
    }

    tresult PLUGIN_API getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized) override {
        const double x = toDouble(string);
        const double lo = iris::gainDb(0.0, norm_[iris::kLowcut]);
        const double hi = iris::gainDb(1.0, norm_[iris::kLowcut]);
        double n = 0.5;
        switch ((int)tag) {
        case iris::kGain: n = (x - lo) / (hi - lo + 1e-9); break;
        case iris::kLowcut: n = 1.0 - (x - 27.0) / (175.0 - 27.0); break;
        case iris::kTone: n = (x + 12.0) / 17.0; break;
        case iris::kBlend: n = x / 50.0; break;
        default: n = std::sqrt(std::pow(10.0, x / 20.0)); break;
        }
        valueNormalized = n < 0 ? 0 : (n > 1 ? 1 : n);
        return kResultOk;
    }

    ParamValue PLUGIN_API getParamNormalized(ParamID tag) override {
        return (tag < iris::kNumParams) ? norm_[tag] : 0.0;
    }

    tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) override {
        if (tag >= iris::kNumParams) return kInvalidArgument;
        norm_[tag] = (float)(value < 0 ? 0 : (value > 1 ? 1 : value));
        return kResultOk;
    }

    IPlugView* PLUGIN_API createView(FIDString /*name*/) override { return new IrisView(this); }

private:
    void applyParams() {
        dsp_.setTargets(norm_[iris::kGain], norm_[iris::kLowcut], norm_[iris::kTone], norm_[iris::kVolume]);
        dsp_.setBlendAmount(norm_[iris::kBlend]);
    }

    iris::IrisDsp dsp_;
    float norm_[iris::kNumParams] = {0.55f, 0.15f, 0.85f, 0.60f, 0.85f};
};

// ---------- 工厂注册 ----------------------------------------------------
} // namespace

static const FUID kIrisUID(0x8B21C4D0, 0x13A74E62, 0x9C0B5F31, 0x2A6E7D18);

BEGIN_FACTORY_DEF("PhonLynN", "PhonLynN DSP", "mailto:none@example.com")
DEF_CLASS2(INLINE_UID_FROM_FUID(kIrisUID),
           PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           "Iris R",
           Vst::kSimpleModeSupported,
           Vst::PlugType::kFxDistortion,
           IRIS_VERSION_STR,
           kVstVersionString,
           IrisProcessor::createInstance)
END_FACTORY

