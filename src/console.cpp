// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "console.h"

#include "base.h"

#include <algorithm>
#include <cstdint>
#include <format>

namespace console
{

CpGuard::CpGuard() : saved_(GetConsoleOutputCP()) { SetConsoleOutputCP(CP_UTF8); }

CpGuard::~CpGuard() { SetConsoleOutputCP(saved_); }

auto
init(HANDLE &handle) -> Result<void>
{
    if (handle == INVALID_HANDLE_VALUE)
        return fail(ErrCode::CantGetHandle, "Invalid console handle");

    DWORD mode{0};
    if (!GetConsoleMode(handle, &mode))
        return fail(ErrCode::ConsoleOps,
                    std::format("Failed to get console mode (code={})", GetLastError()));

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(handle, mode))
        return fail(ErrCode::ConsoleOps,
                    std::format("Failed to set console mode (code={})", GetLastError()));

    return {};
}

auto
get_size(HANDLE handle) -> Result<std::pair<u16, u16>>
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (handle == INVALID_HANDLE_VALUE)
        return fail(ErrCode::CantGetHandle, "Invalid console handle");

    if (!GetConsoleScreenBufferInfo(handle, &info))
        return fail(ErrCode::ConsoleOps,
                    std::format("Failed to get console screen buffer info ({})",
                                last_system_errmsg()));

    u16 cols{static_cast<u16>(std::clamp(info.srWindow.Right - info.srWindow.Left + 1, 0,
                                         static_cast<int>(UINT16_MAX)))};
    u16 rows{static_cast<u16>(std::clamp(info.srWindow.Bottom - info.srWindow.Top + 1, 0,
                                         static_cast<int>(UINT16_MAX)))};
    return std::pair{cols, rows};
}

}
