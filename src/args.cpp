// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "args.h"

#define ARGON_IMPLS

#include "argon.h"
#include "base.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

auto
Args::parse(Argon &argon, std::span<char *> arguments) -> Result<Args>
{
    Args        args{};
    char       *p_image_path{nullptr};
    const char *p_brush_name{nullptr};
    const char *p_color_mode{nullptr};
    const char *p_output_path{nullptr};
    const char *p_output_format{nullptr};
    const char *p_scale_mode{nullptr};

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
        {
            .fullname = "output",
            .alias    = 'o',
            .type     = ARGON_OPTYPE_STRREF,
            .desc     = "Output to file instead of console",
            .target   = static_cast<void *>(&p_output_path),
        },
        {
            .fullname    = "format",
            .alias       = 'f',
            .type        = ARGON_OPTYPE_STRREF,
            .desc        = "Output file format (default: UTF16LE)",
            .target      = static_cast<void *>(&p_output_format),
            .enum_plugin = {.enums = static_cast<const char *const *>(OUTFMT_NAMES)},
        },
        {
            .fullname    = "scale-mode",
            .alias       = 'm',
            .type        = ARGON_OPTYPE_STRREF,
            .desc        = "Image scaling algorithm (default: fant)",
            .target      = static_cast<void *>(&p_scale_mode),
            .enum_plugin = {.enums = static_cast<const char *const *>(SCALE_MODE_NAMES)},
        },
        {nullptr},
    });

    argon.options = opts.data();
    argon.flags   = ARGON_COMBINED_MATCHING | ARGON_CASE_INSENSITIVE;

    if (arguments.size() < 2) return fail(ErrCode::MissingArgs, "Missing arguments");
    if (argon_parse(&argon, arguments.data()) != ARGON_OK)
        return fail(ErrCode::ParseArgs, std::string{static_cast<char *>(argon.errmsg)});

    // -- 后处理 --

    if (!p_image_path) return fail(ErrCode::MissingArgs, "No image path was specified");
    if (args.size[0] < 0 || args.size[1] < 0)
        return fail(ErrCode::InvalidArgs, "Image size must be positive");

    args.image_path = strutil::widen(p_image_path);
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

    if (p_output_format && !p_output_path)
        return fail(ErrCode::InvalidArgs, "option '--format' requires '--output'");

    if (p_output_path) args.output_path = strutil::widen(p_output_path);

    if (p_output_format)
    {
        std::span<const char *const> span_output_formats{OUTFMT_NAMES};
        if (auto it = std::ranges::find(span_output_formats, p_output_format);
            it != span_output_formats.end())
        {
            args.output_format =
                static_cast<OutputFormat>(it - span_output_formats.begin());
        }
    }

    if (p_scale_mode)
    {
        std::span<const char *const> span_scale_modes{SCALE_MODE_NAMES};
        if (auto it = std::ranges::find(span_scale_modes, p_scale_mode);
            it != span_scale_modes.end())
        {
            args.scale_mode = static_cast<ScaleMode>(it - span_scale_modes.begin());
        }
    }

    return args;
}
