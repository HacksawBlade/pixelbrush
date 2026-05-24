// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "argon.h"
#include "args.h"
#include "console.h"
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

    HANDLE handle{args->output_path.empty()
                      ? GetStdHandle(STD_OUTPUT_HANDLE)
                      : CreateFileW(args->output_path.c_str(), GENERIC_WRITE, 0, nullptr,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle == INVALID_HANDLE_VALUE)
    {
        std::println(stderr, "{} Failed to get output handle: {}", ERROR_PREFIX,
                     strutil::to_narrow(args->output_path));
        return EXIT_FAILURE;
    }
    const bool to_console{GetFileType(handle) == FILE_TYPE_CHAR};

    u16 term_cols{CONSOLE_DEFAULT_COLS};
    u16 term_rows{CONSOLE_DEFAULT_ROWS};
    if (to_console)
    {
        if (auto cinit_result = console::init(handle); !cinit_result)
        {
            std::println(stderr, "{} {}", ERROR_PREFIX, cinit_result.error().message);
            if (cinit_result.error().code == ErrCode::CantGetHandle) return EXIT_FAILURE;
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

    u32 image_width{image->width()};
    u32 image_height{image->height()};

    u32 target_width{0};
    u32 target_height{0};
    if (args->size[0] > 0 && args->size[1] > 0)
    {
        target_width  = args->size[0];
        target_height = args->size[1];
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

    if (auto scale_result = image->scale(target_width, target_height); !scale_result)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, scale_result.error().message);
        return EXIT_FAILURE;
    }

    RenderOpts render_opts{
        .target        = handle,
        .pixels        = image->pixels(),
        .width         = image->width(),
        .height        = image->height(),
        .brush         = args->brush,
        .color_mode    = args->color_mode,
        .output_format = args->output_format,
    };

    if (auto render_result = render_ascii_art(render_opts); !render_result)
    {
        std::println(stderr, "{} {}", ERROR_PREFIX, render_result.error().message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
