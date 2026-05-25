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
    if (wsv.empty()) return {};
    const usize max_len{wsv.size() * 3};
    if (max_len > INT_MAX) return {};

    std::string s;
    s.resize_and_overwrite(max_len,
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
    if (sv.empty()) return {};
    const usize maxlen{sv.size()};
    if (maxlen > INT_MAX) return {};

    std::wstring ws;
    ws.resize_and_overwrite(maxlen,
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

static constexpr usize INITIAL_BUFSIZE{256};

[[nodiscard]] inline auto
abspath(const std::wstring &path) -> Result<std::wstring>
{
    std::wstring fullpath;
    DWORD        required{0};

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
