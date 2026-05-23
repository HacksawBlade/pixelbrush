// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.h"

#include <span>
#include <string_view>

enum class RenderColorMode : u8
{
    TrueColor,
    TTY16,
    TTY256,
    Grayscale,
    BlackWhite,
};

struct RenderOpts
{
    HANDLE              target;
    std::span<const u8> pixels;
    u32                 width;
    u32                 height;
    std::wstring_view   brush;
    RenderColorMode     color_mode{RenderColorMode::TrueColor};
};

[[nodiscard]] auto render_ascii_art(const RenderOpts &opts) -> Result<void>;
