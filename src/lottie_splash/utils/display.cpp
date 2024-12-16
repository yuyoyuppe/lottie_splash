#include "display.hpp"
#include <shellscalingapi.h>
#include <dwmapi.h>
#include <winternl.h>

namespace {
struct ACCENTPOLICY {
    DWORD AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

struct WINCOMPATTR {
    DWORD Attribute;
    PVOID Data;
    ULONG DataSize;
};

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

bool enable_transparency(HWND window) {
    if(!window)
        return false;

#ifdef THORVG_GL_RASTER_SUPPORT
    DWORD ex_style = GetWindowLongW(window, GWL_EXSTYLE);
    ex_style |= WS_EX_LAYERED;
    if(!SetWindowLongW(window, GWL_EXSTYLE, ex_style))
        return false;

    return SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA);
#else
    const HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if(!user32)
        return false;

    using SetWindowCompositionAttributeFunc = BOOL(WINAPI *)(HWND, WINCOMPATTR *);
    const auto SetWindowCompositionAttribute =
      reinterpret_cast<SetWindowCompositionAttributeFunc>(GetProcAddress(user32, "SetWindowCompositionAttribute"));

    if(!SetWindowCompositionAttribute)
        return false;

    constexpr DWORD ACCENT_ENABLE_BLURBEHIND = 3;
    constexpr DWORD ACCENT_ENABLE_ACRYLIC = 4;
    constexpr DWORD ENABLE_BLUR_BEHIND_MASKS = 0x20;
    constexpr DWORD WCA_ACCENT_POLICY = 19;

    ACCENTPOLICY accent = {
        .AccentState = is_windows_11_or_newer ? ACCENT_ENABLE_ACRYLIC : ACCENT_ENABLE_BLURBEHIND,
        .AccentFlags = ENABLE_BLUR_BEHIND_MASKS,
        .GradientColor = is_windows_11_or_newer ? 0x20FFFFFFUL : 0UL, // 0x20 = 32 alpha for acrylic
        .AnimationId = 0
    };

    WINCOMPATTR data = {.Attribute = WCA_ACCENT_POLICY, .Data = &accent, .DataSize = sizeof(accent)};

    return SetWindowCompositionAttribute(window, &data);
#endif
}
}