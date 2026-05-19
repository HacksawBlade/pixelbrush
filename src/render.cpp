// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "render.h"

#include "base.hpp"
#include "tty_maps.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <vector>

using namespace std::string_view_literals;

static constexpr std::size_t    MAX_ESC_LEN{20};
static constexpr int            GRAY_BASE{232};
static constexpr int            GRAY_STEPS{23};
static constexpr std::ptrdiff_t BUF_PAD{8};

static constexpr auto SEQ_NLINE{L"\r\n"sv};
static constexpr auto SEQ_RESET{L"\x1b[0m\r\n"sv};

[[nodiscard]] static inline std::uint8_t
map_to_tty16(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::size_t qr = (r * (TTY16_QUANT_LEVEL - 1)) / 255;
    std::size_t qg = (g * (TTY16_QUANT_LEVEL - 1)) / 255;
    std::size_t qb = (b * (TTY16_QUANT_LEVEL - 1)) / 255;
    return TTY16_MAP.at((qr * TTY16_QUANT_LEVEL * TTY16_QUANT_LEVEL) +
                        (qg * TTY16_QUANT_LEVEL) + qb);
}

[[nodiscard]] static inline std::uint8_t
map_to_tty256(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::size_t qr = (r * (TTY256_QUANT_LEVEL - 1)) / 255;
    std::size_t qg = (g * (TTY256_QUANT_LEVEL - 1)) / 255;
    std::size_t qb = (b * (TTY256_QUANT_LEVEL - 1)) / 255;
    return TTY256_MAP.at((qr * TTY256_QUANT_LEVEL * TTY256_QUANT_LEVEL) +
                         (qg * TTY256_QUANT_LEVEL) + qb);
}

Result<void>
render_ascii_art(const RenderOpts &opts)
{
    if (!opts.target || opts.target == INVALID_HANDLE_VALUE)
        return fail(ErrCode::InvalidValue, "Invalid render target handle");
    if (opts.pixels.empty() || opts.brush.empty() || opts.width == 0 || opts.height == 0)
        return fail(ErrCode::InvalidValue, "Invalid render parameters");

    bool              to_console{GetFileType(opts.target) == FILE_TYPE_CHAR};
    std::wstring_view tail{opts.color_mode == RenderColorMode::BlackWhite ? SEQ_NLINE
                                                                          : SEQ_RESET};
    auto              tail_len{static_cast<std::ptrdiff_t>(tail.size())};
    auto max_line_chars{(static_cast<std::ptrdiff_t>(opts.width * MAX_ESC_LEN)) +
                        tail_len + BUF_PAD};
    std::vector<wchar_t> render_buf(max_line_chars);

    if (!to_console)
    {
        static constexpr uint16_t BOM_UTF16 = 0xFEFF;
        WriteFile(opts.target, &BOM_UTF16, sizeof(BOM_UTF16), nullptr, nullptr);
    }

    for (std::uint32_t y = 0; y < opts.height; y++)
    {
        std::size_t src_offset{static_cast<std::size_t>(y) * opts.width *
                               IMAGE_PIXEL_BYTE};

        std::ptrdiff_t pos{0};
        for (std::uint32_t x = 0; x < opts.width; x++)
        {
            std::size_t  idx{src_offset +
                             (static_cast<std::size_t>(x) * IMAGE_PIXEL_BYTE)};
            std::uint8_t b{opts.pixels[idx]};
            std::uint8_t g{opts.pixels[idx + 1]};
            std::uint8_t r{opts.pixels[idx + 2]};
            double       lum{((0.2126 * r) + (0.7152 * g) + (0.0722 * b)) / 255};

            auto brush_idx =
                (std::min) (static_cast<std::size_t>(
                                lum * static_cast<double>(opts.brush.size() - 1)),
                            opts.brush.size() - 1);
            wchar_t brush_chr{opts.brush[brush_idx]};

            std::ptrdiff_t color_len{0};
            switch (opts.color_mode)
            {
            case RenderColorMode::TrueColor:
                color_len = static_cast<std::ptrdiff_t>(
                    std::format_to_n(&render_buf[pos], max_line_chars - pos,
                                     L"\x1b[38;2;{};{};{}m", r, g, b)
                        .size);
                break;
            case RenderColorMode::TTY16:
                color_len = static_cast<std::ptrdiff_t>(
                    std::format_to_n(&render_buf[pos], max_line_chars - pos, L"\x1b[{}m",
                                     map_to_tty16(r, g, b))
                        .size);
                break;
            case RenderColorMode::TTY256:
                color_len = static_cast<std::ptrdiff_t>(
                    std::format_to_n(&render_buf[pos], max_line_chars - pos,
                                     L"\x1b[38;5;{}m", map_to_tty256(r, g, b))
                        .size);
                break;
            case RenderColorMode::Grayscale:
            {
                int gray_code{GRAY_BASE + static_cast<int>(lum * GRAY_STEPS)};
                gray_code = (std::min) (gray_code, 255);
                color_len = static_cast<std::ptrdiff_t>(
                    std::format_to_n(&render_buf[pos], max_line_chars - pos,
                                     L"\x1b[38;5;{}m", gray_code)
                        .size);
                break;
            }
            case RenderColorMode::BlackWhite:
                break;
            }

            pos += color_len;
            if (pos < max_line_chars) render_buf[pos++] = brush_chr;
        }

        if (pos + tail_len < max_line_chars)
        {
            std::ranges::copy(tail, render_buf.begin() + pos);
            pos += tail_len;
        }
        render_buf[pos] = 0;

        if (to_console)
        {
            WriteConsoleW(opts.target, render_buf.data(), static_cast<DWORD>(pos),
                          nullptr, nullptr);
        }
        else
        {
            WriteFile(opts.target, render_buf.data(),
                      static_cast<DWORD>(pos * sizeof(wchar_t)), nullptr, nullptr);
        }
    }

    return {};
}
