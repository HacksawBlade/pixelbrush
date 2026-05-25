// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "console.h"

#include "base.h"
#include "handle.h"

#include <format>

namespace console
{

CpGuard::CpGuard() : saved_(GetConsoleOutputCP()) { SetConsoleOutputCP(CP_UTF8); }

CpGuard::~CpGuard() { SetConsoleOutputCP(saved_); }

auto
init() -> Result<ScopedHandle>
{
    HANDLE handle{GetStdHandle(STD_OUTPUT_HANDLE)};
    if (handle == INVALID_HANDLE_VALUE)
        return fail(ErrCode::CantGetHandle, "Invalid console handle");

    DWORD mode{0};
    if (!GetConsoleMode(handle, &mode))
        return fail(ErrCode::ConsoleOps,
                    std::format("Failed to get console mode ({})", last_system_errmsg()));

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(handle, mode))
        return fail(ErrCode::ConsoleOps,
                    std::format("Failed to set console mode ({})", last_system_errmsg()));

    return ScopedHandle{handle, false};
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

    int cols_raw{info.srWindow.Right - info.srWindow.Left + 1};
    int rows_raw{info.srWindow.Bottom - info.srWindow.Top + 1};
    if (cols_raw <= 0 || rows_raw <= 0)
        return fail(ErrCode::ConsoleOps,
                    "Console window has no available columns or rows");

    return std::pair{static_cast<u16>(cols_raw), static_cast<u16>(rows_raw)};
}

}
