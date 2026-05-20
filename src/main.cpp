// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "argon.h"
#include "args.h"
#include "console.h"
#include "image.h"
#include "render.h"

#include <cstdlib>
#include <print>
#include <span>
#include <winrt/Windows.Foundation.h>

static constexpr auto ERROR_PREFIX = "\x1b[1;37;41m ERROR \x1b[0m";

auto
wmain(int argc, wchar_t *argv[]) -> int
{
    winrt::init_apartment();
    console::CpGuard cp_guard{};
    (void) std::atexit([] -> void { winrt::uninit_apartment(); });

    Argon argon{};
    auto  args = Args::parse(argon, std::span(argv, argc));
    if (!args)
    {
        std::println("Usage: pixelbrush <image-path> [OPTIONS]\n");
        if (args.error().code == ErrCode::MissingArgs) argon_print_table(&argon, stderr);
        std::println(stderr, "{} {}", ERROR_PREFIX, args.error().message);
        return EXIT_FAILURE;
    }

    HANDLE     handle{GetStdHandle(STD_OUTPUT_HANDLE)};
    const bool to_console{GetFileType(handle) == FILE_TYPE_CHAR};

    int term_cols{CONSOLE_DEFAULT_COLS};
    int term_rows{CONSOLE_DEFAULT_ROWS};
    if (to_console)
    {
        auto r = console::init(handle);
        if (!r)
        {
            std::println(stderr, "{} {}", ERROR_PREFIX, r.error().message);
            if (r.error().code == ErrCode::CantGetHandle) return EXIT_FAILURE;
        }
        else
        {
            auto size = console::get_size(handle);
            if (!size)
            {
                std::println(stderr, "{} {}", ERROR_PREFIX, size.error().message);
                if (size.error().code == ErrCode::CantGetHandle) return EXIT_FAILURE;
            }
            else
            {
                term_cols = size->first;
                term_rows = size->second;
            }
        }
    }

    auto image = Image::open(args->image_path);
    if (!image)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, image.error().message);
        return EXIT_FAILURE;
    }

    std::uint32_t image_width{image->width()};
    std::uint32_t image_height{image->height()};

    int target_width{0};
    int target_height{0};
    if (args->size[0] > 0 && args->size[1] > 0)
    {
        target_width  = args->size[0];
        target_height = args->size[1];
    }
    else
    {
        target_width  = static_cast<int>(image_width * args->width_scale);
        target_height = static_cast<int>(static_cast<double>(image_height) *
                                         target_width / image_width / args->width_scale);

        if (to_console)
        {
            if (target_width > term_cols)
            {
                target_width = term_cols;
                target_height =
                    static_cast<int>(static_cast<double>(image_height) * target_width /
                                     image_width / args->width_scale);
            }

            if (target_height > term_rows)
            {
                target_width  = target_width * term_rows / target_height;
                target_height = term_rows;
            }
        }
    }

    auto scale_result = image->scale(static_cast<std::uint32_t>(target_width),
                                     static_cast<std::uint32_t>(target_height));
    if (!scale_result)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, scale_result.error().message);
        return EXIT_FAILURE;
    }

    RenderOpts render_opts = {
        .target     = handle,
        .pixels     = image->pixels(),
        .width      = image->width(),
        .height     = image->height(),
        .brush      = args->brush,
        .color_mode = args->color_mode,
    };

    auto render_result = render_ascii_art(render_opts);
    if (!render_result)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, render_result.error().message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
