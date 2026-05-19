// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.hpp"

#include <span>
#include <string_view>

enum class RenderColorMode : std::uint8_t
{
    TrueColor,
    TTY16,
    TTY256,
    Grayscale,
    BlackWhite,
};

struct RenderOpts
{
    HANDLE                        target;
    std::span<const std::uint8_t> pixels;
    std::uint32_t                 width;
    std::uint32_t                 height;
    std::wstring_view             brush;
    RenderColorMode               color_mode{RenderColorMode::TrueColor};
};

[[nodiscard]] Result<void> render_ascii_art(const RenderOpts &opts);
