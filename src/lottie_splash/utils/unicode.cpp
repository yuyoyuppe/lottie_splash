#include "unicode.hpp"

#include <Windows.h>

namespace utils {
std::string wide_to_utf8(const std::wstring_view wstr) {
    if(wstr.empty())
        return {};

    const int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);

    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring utf8_to_wide(const std::u8string_view u8str) {
    if(u8str.empty())
        return {};

    const int size = MultiByteToWideChar(
      CP_UTF8, 0, reinterpret_cast<const char *>(u8str.data()), static_cast<int>(u8str.size()), nullptr, 0);

    std::wstring result(size, 0);
    MultiByteToWideChar(
      CP_UTF8, 0, reinterpret_cast<const char *>(u8str.data()), static_cast<int>(u8str.size()), result.data(), size);

    return result;
}
}