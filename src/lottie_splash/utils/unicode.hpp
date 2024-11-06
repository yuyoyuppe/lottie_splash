#pragma once

#include <string>
#include <string_view>

namespace utils {
std::string  wide_to_utf8(const std::wstring_view wstr);
std::wstring utf8_to_wide(const std::u8string_view u8str);
}