// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.h"

#include <format>
#include <string>
#include <string_view>

namespace strutil
{

[[nodiscard]] inline auto
to_narrow(std::wstring_view wsv) -> std::string
{
    int len = WideCharToMultiByte(CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()),
                                  nullptr, 0, nullptr, nullptr);
    if (len == 0) return {};
    std::string s{};
    s.resize_and_overwrite(len,
                           [&](char *buf, usize buf_size) noexcept -> int
                           {
                               int written = WideCharToMultiByte(
                                   CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()),
                                   buf, static_cast<int>(buf_size), nullptr, nullptr);
                               return written > 0 ? written : 0;
                           });
    return s;
}

[[nodiscard]] inline auto
to_wide(std::string_view sv) -> std::wstring
{
    int len = MultiByteToWideChar(CP_UTF8, 0, sv.data(), static_cast<int>(sv.size()),
                                  nullptr, 0);
    if (len == 0) return {};
    std::wstring ws{};
    ws.resize_and_overwrite(len,
                            [&](wchar_t *buf, usize buf_size) noexcept -> int
                            {
                                int written = MultiByteToWideChar(
                                    CP_UTF8, 0, sv.data(), static_cast<int>(sv.size()),
                                    buf, static_cast<int>(buf_size));
                                return written > 0 ? written : 0;
                            });
    return ws;
}

}

namespace fsutil
{

[[nodiscard]] inline auto
abspath(const std::wstring &path) -> Result<std::wstring>
{
    static constexpr DWORD INITIAL_BUFSIZE{512};
    DWORD                  required{0};
    std::wstring           fullpath;

    fullpath.resize_and_overwrite(
        INITIAL_BUFSIZE,
        [&](wchar_t *buf, usize buf_size) noexcept -> DWORD
        {
            DWORD len{GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf_size), buf,
                                       nullptr)};
            if (len == 0) return 0;
            if (len < buf_size) return len;
            required = len;
            return 0;
        });

    if (!fullpath.empty()) return fullpath;

    if (required == 0)
    {
        std::string errmsg{last_system_errmsg()};
        return fail(ErrCode::FilePath,
                    std::format("{} ({})", errmsg, strutil::to_narrow(path)));
    }

    fullpath.resize_and_overwrite(
        required,
        [&](wchar_t *buf, usize buf_size) noexcept -> DWORD
        {
            DWORD len{GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf_size), buf,
                                       nullptr)};
            return (len == 0 || len >= buf_size) ? 0 : len;
        });

    if (fullpath.empty())
        return fail(ErrCode::FilePath,
                    std::format("Failed to find file: {}", strutil::to_narrow(path)));
    return fullpath;
}

}
