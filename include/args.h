// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "argon.h"
#include "base.h"
#include "render.h"

#include <array>
#include <span>
#include <string>
#include <string_view>

inline constexpr const char *BRUSH_NAMES[] = {
    "block", "dot", "shades", "symbols", "letters", nullptr,
};

inline constexpr const char *COLOR_MODE_NAMES[] = {
    "truecolor", "tty16", "tty256", "grayscale", "blackwhite", nullptr,
};

inline constexpr const char *OUTFMT_NAMES[] = {
    "UTF8",
    "UTF16LE",
    nullptr,
};

inline constexpr auto BRUSHES = std::to_array<std::wstring_view>({
    L"█",
    L"⬤",
    L" ░▒▓█",
    L" .:-=+*#%@",
    L" .'`^\",:;Il!i~+_-?][}{1)(\\/,tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$",
});

class Args
{
public:
    std::wstring       image_path;
    std::wstring_view  brush{BRUSHES[0]};
    std::array<int, 2> size{0, 0};
    double             width_scale{2.0};
    RenderColorMode    color_mode{RenderColorMode::TrueColor};
    std::wstring       output_path;
    OutputFormat       output_format{OutputFormat::UTF16LE};

    Args() = default;

    [[nodiscard]] static auto parse(Argon &argon, std::span<wchar_t *> arguments)
        -> Result<Args>;
};
