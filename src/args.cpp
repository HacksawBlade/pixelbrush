// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "args.h"

#include "base.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

auto
Args::parse(Argon &argon, std::span<wchar_t *> arguments) -> Result<Args>
{
    Args        args{};
    char       *p_image_path{nullptr};
    const char *p_brush_name{nullptr};
    const char *p_color_mode{nullptr};

    static auto opts = std::to_array<ArgonOption>({
        {
            .type   = ARGON_OPTYPE_STRREF_MUT,
            .target = static_cast<void *>(&p_image_path),
        },
        {
            .fullname    = "brush",
            .alias       = 'b',
            .type        = ARGON_OPTYPE_STRREF,
            .desc        = "Specify the brush to use. (default: block)",
            .target      = static_cast<void *>(&p_brush_name),
            .enum_plugin = {.enums = static_cast<const char *const *>(BRUSH_NAMES)},
        },
        {
            .fullname     = "size",
            .alias        = 's',
            .type         = ARGON_OPTYPE_INT,
            .desc         = "Set output character columns and rows, e.g. --size 40 30",
            .target       = args.size.data(),
            .array_plugin = {.max_len = 2, .force_filling = true},
        },
        {
            .fullname = "wscale",
            .alias    = 'w',
            .type     = ARGON_OPTYPE_FLOAT,
            .desc     = "Set the width scaling ratio to fit the console. (default: 2.0)",
            .target   = &args.width_scale,
        },
        {
            .fullname    = "color",
            .alias       = 'c',
            .type        = ARGON_OPTYPE_STRREF,
            .desc        = "Set color output mode. (default: truecolor)",
            .target      = static_cast<void *>(&p_color_mode),
            .enum_plugin = {.enums = static_cast<const char *const *>(COLOR_MODE_NAMES)},
        },
        {nullptr},
    });

    argon.options = opts.data();
    argon.flags   = ARGON_COMBINED_MATCHING | ARGON_CASE_INSENSITIVE;

    if (arguments.size() < 2) return fail(ErrCode::MissingArgs, "Missing arguments");

    auto arg_strs = arguments | std::views::transform(strutil::to_narrow) |
                    std::ranges::to<std::vector<std::string>>();
    auto arg_ptrs = arg_strs |
                    std::views::transform([](auto &s) -> auto { return s.data(); }) |
                    std::ranges::to<std::vector<char *>>();
    arg_ptrs.emplace_back(nullptr);

    if (argon_parse(&argon, arg_ptrs.data()) != ARGON_OK)
        return fail(ErrCode::ParseArgs, std::string{static_cast<char *>(argon.errmsg)});

    // -- 后处理 --

    if (!p_image_path) return fail(ErrCode::MissingArgs, "No image path was specified");
    if (args.size[0] < 0 || args.size[1] < 0)
        return fail(ErrCode::InvalidArgs, "Image size must be positive");

    args.image_path = strutil::to_wide(p_image_path);
    if (p_brush_name)
    {
        std::span<const char *const> span_brush_names{BRUSH_NAMES};
        if (auto it = std::ranges::find(span_brush_names, p_brush_name);
            it != span_brush_names.end())
        {
            args.brush = BRUSHES.at(it - span_brush_names.begin());
        }
    }

    if (p_color_mode)
    {
        std::span<const char *const> span_color_modes{COLOR_MODE_NAMES};
        if (auto it = std::ranges::find(span_color_modes, p_color_mode);
            it != span_color_modes.end())
        {
            args.color_mode = static_cast<RenderColorMode>(it - span_color_modes.begin());
        }
    }

    return args;
}
