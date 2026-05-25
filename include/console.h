// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.h"
#include "handle.h"

#include <utility>

inline constexpr u16 CONSOLE_DEFAULT_COLS{80};
inline constexpr u16 CONSOLE_DEFAULT_ROWS{40};

namespace console
{

class CpGuard
{
private:
    UINT saved_{};

public:
    CpGuard();
    ~CpGuard();

    CpGuard(const CpGuard &)                     = delete;
    CpGuard(CpGuard &&)                          = delete;
    auto operator=(CpGuard &&) -> CpGuard &      = delete;
    auto operator=(const CpGuard &) -> CpGuard & = delete;
};

[[nodiscard]] auto init() -> Result<ScopedHandle>;
[[nodiscard]] auto get_size(HANDLE handle) -> Result<std::pair<u16, u16>>;

}
