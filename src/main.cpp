// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "argon.h"
#include "args.h"
#include "console.h"
#include "handle.h"
#include "image.h"
#include "render.h"
#include "utils.h"

#include <cstdlib>
#include <print>
#include <span>
#include <winrt/Windows.Foundation.h>

static constexpr auto ERROR_PREFIX = "\x1b[1;37;41m ERROR \x1b[0m";

auto
wmain(int argc, wchar_t *argv[]) -> int // NOLINT(misc-use-internal-linkage)
{
    winrt::init_apartment();
    console::CpGuard cp_guard{};

    Argon argon{};
    auto  args = Args::parse(argon, std::span(argv, argc));
    if (!args)
    {
        std::println("Usage: pixelbrush <image-path> [OPTIONS]\n");
        if (args.error().code == ErrCode::MissingArgs) argon_print_table(&argon, stderr);
        std::println(stderr, "{} {}", ERROR_PREFIX, args.error().message);
        return EXIT_FAILURE;
    }

    ScopedHandle handle{};
    bool         to_console{false};
    if (args->output_path.empty())
    {
        auto cinit = console::init();
        if (!cinit)
        {
            std::println(stderr, "{} {}", ERROR_PREFIX, cinit.error().message);
            return EXIT_FAILURE;
        }
        handle     = std::move(*cinit);
        to_console = true;
    }
    else
    {
        auto file_handle = fsutil::create_file(args->output_path);
        if (!file_handle)
        {
            std::println(stderr, "{} {}", ERROR_PREFIX, file_handle.error().message);
            return EXIT_FAILURE;
        }
        handle = std::move(*file_handle);
    }

    u16 term_cols{CONSOLE_DEFAULT_COLS};
    u16 term_rows{CONSOLE_DEFAULT_ROWS};
    if (to_console)
    {
        if (auto size = console::get_size(*handle); !size)
        {
            std::println(stderr, "{} {}", ERROR_PREFIX, size.error().message);
        }
        else
        {
            term_cols = size->first;
            term_rows = size->second;
        }
    }

    auto image = Image::open(args->image_path);
    if (!image)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, image.error().message);
        return EXIT_FAILURE;
    }

    u32 image_width{image->width()};
    u32 image_height{image->height()};

    u32 target_width{0};
    u32 target_height{0};
    if (args->size[0] > 0 && args->size[1] > 0)
    {
        target_width  = static_cast<u32>(args->size[0]);
        target_height = static_cast<u32>(args->size[1]);
    }
    else
    {
        target_width  = static_cast<u32>(image_width * args->width_scale);
        target_height = static_cast<u32>(static_cast<double>(image_height) *
                                         target_width / image_width / args->width_scale);

        if (to_console)
        {
            if (target_width > term_cols)
            {
                target_width = term_cols;
                target_height =
                    static_cast<u32>(static_cast<double>(image_height) * target_width /
                                     image_width / args->width_scale);
            }

            if (target_height > term_rows)
            {
                target_width  = target_width * term_rows / target_height;
                target_height = term_rows;
            }
        }
    }

    if (auto r_scale = image->scale(target_width, target_height); !r_scale)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, r_scale.error().message);
        return EXIT_FAILURE;
    }

    RenderOpts render_opts{
        .target        = *handle,
        .pixels        = image->pixels(),
        .width         = image->width(),
        .height        = image->height(),
        .brush         = args->brush,
        .color_mode    = args->color_mode,
        .output_format = args->output_format,
    };

    if (auto r_render = render_ascii_art(render_opts); !r_render)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, r_render.error().message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
