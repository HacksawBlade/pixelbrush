// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "console.h"

#include <format>

namespace console
{

CpGuard::CpGuard() : saved_(GetConsoleOutputCP()) { SetConsoleOutputCP(CP_UTF8); }

CpGuard::~CpGuard() { SetConsoleOutputCP(saved_); }

Result<void>
init(HANDLE &handle)
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

Result<std::pair<int, int>>
get_size(HANDLE handle)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (handle == INVALID_HANDLE_VALUE)
        return fail(ErrCode::CantGetHandle, "Invalid console handle");

    if (!GetConsoleScreenBufferInfo(handle, &info))
        return fail(ErrCode::ConsoleOps,
                    std::format("Failed to get console screen buffer info (code={})",
                                GetLastError()));

    int cols{info.srWindow.Right - info.srWindow.Left + 1};
    int rows{info.srWindow.Bottom - info.srWindow.Top + 1};
    return std::pair{cols, rows};
}

}
