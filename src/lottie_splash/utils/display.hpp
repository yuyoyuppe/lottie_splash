#pragma once

#include <utility>
#include <Windows.h>

namespace utils {
bool                enable_dpi_awareness();
std::pair<int, int> primary_monitor_dims();
bool                enable_rounded_corners(HWND window);
bool                enable_shadow(HWND window);
bool                enable_transparency(HWND window);
inline float        get_dpi_scale() { return static_cast<float>(GetDpiForSystem()) / 96.0f; }
}