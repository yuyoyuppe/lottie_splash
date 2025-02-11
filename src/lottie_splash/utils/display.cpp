#include "display.hpp"
#include <shellscalingapi.h>
#include <dwmapi.h>
#include <winternl.h>

#include <algorithm>

namespace {

// This method works consistently regardless whether the app has a manifest or not.
const bool is_windows_11_or_newer = [] {
    using RtlGetVersionPtr = NTSTATUS(WINAPI *)(PRTL_OSVERSIONINFOW);
    const auto rtlGetVersion =
      reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    if(!rtlGetVersion)
        return false;

    RTL_OSVERSIONINFOW osvi = {.dwOSVersionInfoSize = sizeof(osvi)};
    if(NT_SUCCESS(rtlGetVersion(&osvi)))
        return osvi.dwBuildNumber >= 22000;

    return false;
}();
}

namespace utils {
bool enable_dpi_awareness() {
    if(const DPI_AWARENESS_CONTEXT currentContext = GetThreadDpiAwarenessContext();
       AreDpiAwarenessContextsEqual(currentContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ||
       AreDpiAwarenessContextsEqual(currentContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
        return true;

    return SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ||
           SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE) ||
           SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) == S_OK;
}

std::pair<int, int> primary_monitor_dims() {
    const HMONITOR monitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO    mi      = {.cbSize = sizeof(mi)};
    GetMonitorInfoW(monitor, &mi);

    const int monitor_width  = mi.rcMonitor.right - mi.rcMonitor.left;
    const int monitor_height = mi.rcMonitor.bottom - mi.rcMonitor.top;

    return {monitor_width, monitor_height};
}

bool enable_rounded_corners(HWND window) {
    if(!window || !is_windows_11_or_newer)
        return false;

    const DWORD value = DWMWCP_ROUND;
    return SUCCEEDED(
      DwmSetWindowAttribute(window, DWMWINDOWATTRIBUTE::DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value)));
}

bool enable_shadow(HWND window) {
    if(!window)
        return false;

    if(is_windows_11_or_newer)
        return true;

    const DWMNCRENDERINGPOLICY render_policy = DWMNCRP_ENABLED;
    if(FAILED(DwmSetWindowAttribute(window, DWMWA_NCRENDERING_POLICY, &render_policy, sizeof(render_policy))))
        return false;

    const BOOL shadow = TRUE;
    if(FAILED(DwmSetWindowAttribute(window, DWMWA_ALLOW_NCPAINT, &shadow, sizeof(shadow))))
        return false;

    const MARGINS margins = {1, 1, 1, 1};
    return SUCCEEDED(DwmExtendFrameIntoClientArea(window, &margins));
}

bool enable_transparency(HWND window, const float transparency) {
    if(!window)
        return false;

    const HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if(!user32)
        return false;

    using SetWindowCompositionAttributeFunc = BOOL(WINAPI *)(HWND, void *);
    const auto SetWindowCompositionAttribute =
      reinterpret_cast<SetWindowCompositionAttributeFunc>(GetProcAddress(user32, "SetWindowCompositionAttribute"));

    if(!SetWindowCompositionAttribute)
        return false;

    constexpr DWORD ACCENT_ENABLE_ACRYLIC    = 4;
    constexpr DWORD ENABLE_BLUR_BEHIND_MASKS = 0x10;

    struct AccentPolicy {
        int   AccentState;
        DWORD AccentFlags;
        DWORD GradientColor;
        DWORD AnimationId;
    };

    struct WindowCompositionAttribData {
        DWORD  Attribute;
        PVOID  pvData;
        SIZE_T cbData;
    };

    const uint8_t alpha = static_cast<uint8_t>(std::clamp(transparency, 0.0f, 1.0f) * 255.0f);
    // Black color with custom alpha
    const DWORD gradientColor = (static_cast<DWORD>(alpha) << 24) | 0x000000;

    AccentPolicy accent = {.AccentState   = ACCENT_ENABLE_ACRYLIC,
                           .AccentFlags   = ENABLE_BLUR_BEHIND_MASKS,
                           .GradientColor = gradientColor,
                           .AnimationId   = 0};

    constexpr DWORD             WCA_ACCENT_POLICY = 19;
    WindowCompositionAttribData data = {.Attribute = WCA_ACCENT_POLICY, .pvData = &accent, .cbData = sizeof(accent)};

    return SetWindowCompositionAttribute(window, &data);
}

bool set_background_color(HWND window, DWORD color) {
    if(!window)
        return false;

    if(is_windows_11_or_newer)
        return true; // Already handled in enable_transparency

    DWORD ex_style = GetWindowLongW(window, GWL_EXSTYLE);
    ex_style |= WS_EX_LAYERED;
    if(!SetWindowLongW(window, GWL_EXSTYLE, ex_style))
        return false;

    // color is in format 0xAARRGGBB
    BYTE alpha = (color >> 24) & 0xFF;
    return SetLayeredWindowAttributes(window, 0, alpha, LWA_ALPHA);
}
}