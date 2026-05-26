// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.h"
#include "handle.h"

#include <format>
#include <span>
#include <string>
#include <string_view>

namespace strutil
{

inline constexpr u8 MAX_UTF8_BYTES_PER_WCHAR{4};

[[nodiscard]] inline auto
narrow(std::wstring_view wsv, std::string &out) -> Result<void>
{
    if (wsv.empty())
    {
        out.clear();
        return {};
    }
    const usize max_len{wsv.size() * MAX_UTF8_BYTES_PER_WCHAR};
    if (max_len > INT_MAX)
    {
        out.clear();
        return fail(ErrCode::InbufTooLong,
                    std::format("Buffer too large for conversion ({})", max_len));
    }

    out.resize_and_overwrite(max_len,
                             [&](char *buf, usize buf_size) noexcept -> int
                             {
                                 int written{WideCharToMultiByte(
                                     CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()),
                                     buf, static_cast<int>(buf_size), nullptr, nullptr)};
                                 return written > 0 ? written : 0;
                             });
    if (out.empty())
        return fail(ErrCode::OutbufTooShort, "String conversion produced no output");
    return {};
}

[[nodiscard]] inline auto
narrow(std::wstring_view wsv) -> std::string
{
    std::string out;
    if (auto result = narrow(wsv, out); !result) return {};
    return out;
}

[[nodiscard]] inline auto
narrow(std::wstring_view wsv, std::span<char> buf) -> int
{
    if (wsv.empty() || buf.empty()) return 0;
    int len{WideCharToMultiByte(CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()),
                                nullptr, 0, nullptr, nullptr)};
    if (len == 0) return 0;
    if (len > static_cast<int>(buf.size())) return 0;
    int written{WideCharToMultiByte(CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()),
                                    buf.data(), static_cast<int>(buf.size()), nullptr,
                                    nullptr)};
    return written > 0 ? written : 0;
}

[[nodiscard]] inline auto
widen(std::string_view sv, std::wstring &out) -> Result<void>
{
    if (sv.empty())
    {
        out.clear();
        return {};
    }
    const usize max_len{sv.size()};
    if (max_len > INT_MAX)
    {
        out.clear();
        return fail(ErrCode::InbufTooLong,
                    std::format("Buffer too large for conversion ({})", max_len));
    }

    out.resize_and_overwrite(max_len,
                             [&](wchar_t *buf, usize buf_size) noexcept -> int
                             {
                                 int written{MultiByteToWideChar(
                                     CP_UTF8, 0, sv.data(), static_cast<int>(sv.size()),
                                     buf, static_cast<int>(buf_size))};
                                 return written > 0 ? written : 0;
                             });
    if (out.empty())
        return fail(ErrCode::OutbufTooShort, "String conversion produced no output");
    return {};
}

[[nodiscard]] inline auto
widen(std::string_view sv) -> std::wstring
{
    std::wstring out;
    if (auto result = widen(sv, out); !result) return {};
    return out;
}

[[nodiscard]] inline auto
widen(std::string_view sv, std::span<wchar_t> buf) -> int
{
    if (sv.empty() || buf.empty()) return 0;
    int len{MultiByteToWideChar(CP_UTF8, 0, sv.data(), static_cast<int>(sv.size()),
                                nullptr, 0)};
    if (len == 0) return 0;
    if (len > static_cast<int>(buf.size())) return 0;
    int written{MultiByteToWideChar(CP_UTF8, 0, sv.data(), static_cast<int>(sv.size()),
                                    buf.data(), static_cast<int>(buf.size()))};
    return written > 0 ? written : 0;
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
                    std::format("{} ({})", errmsg, strutil::narrow(path)));
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
                    std::format("Failed to find file: {}", strutil::narrow(path)));
    return fullpath;
}

[[nodiscard]] inline auto
create_file(const std::wstring &path) -> Result<ScopedHandle>
{
    HANDLE handle{CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle == INVALID_HANDLE_VALUE)
        return fail(ErrCode::FilePath,
                    std::format("Failed to create output file: {} ({})",
                                strutil::narrow(path), last_system_errmsg()));
    return ScopedHandle{handle, true};
}

}
