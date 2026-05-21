// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "render.h"

#include "base.h"
#include "tty_maps.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#ifdef BENCH_RENDER
    #include "benchmark.h"
#endif

using namespace std::string_view_literals;

static constexpr std::size_t    MAX_ESC_LEN{20};
static constexpr int            GRAY_BASE{232};
static constexpr int            GRAY_STEPS{23};
static constexpr std::ptrdiff_t BUF_PAD{8};

namespace
{

[[nodiscard]] inline auto
map_to_tty16(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> std::uint8_t
{
    std::size_t qr = (r * (TTY16_QUANT_LEVEL - 1)) / 255;
    std::size_t qg = (g * (TTY16_QUANT_LEVEL - 1)) / 255;
    std::size_t qb = (b * (TTY16_QUANT_LEVEL - 1)) / 255;
    return TTY16_MAP.at((qr * TTY16_QUANT_LEVEL * TTY16_QUANT_LEVEL) +
                        (qg * TTY16_QUANT_LEVEL) + qb);
}

[[nodiscard]] inline auto
map_to_tty256(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> std::uint8_t
{
    std::size_t qr = (r * (TTY256_QUANT_LEVEL - 1)) / 255;
    std::size_t qg = (g * (TTY256_QUANT_LEVEL - 1)) / 255;
    std::size_t qb = (b * (TTY256_QUANT_LEVEL - 1)) / 255;
    return TTY256_MAP.at((qr * TTY256_QUANT_LEVEL * TTY256_QUANT_LEVEL) +
                         (qg * TTY256_QUANT_LEVEL) + qb);
}

[[nodiscard]] inline auto
format_u8(std::span<wchar_t> buf, std::uint8_t v) -> std::ptrdiff_t
{
    if (v >= 100)
    {
        auto [huns, rem]  = std::div(v, 100);
        auto [tens, ones] = std::div(rem, 10);
        buf[0]            = L'0' + huns;
        buf[1]            = L'0' + tens;
        buf[2]            = L'0' + ones;
        return 3;
    }
    if (v >= 10)
    {
        auto [tens, ones] = std::div(v, 10);
        buf[0]            = L'0' + tens;
        buf[1]            = L'0' + ones;
        return 2;
    }
    buf[0] = L'0' + v;
    return 1;
}

template <typename... Args>
    requires(std::same_as<std::decay_t<Args>, std::uint8_t> && ...)
[[nodiscard]] auto
format_ansi_seq(std::span<wchar_t> buf_span, std::wstring_view prefix, Args... args)
    -> std::ptrdiff_t
{
    std::ptrdiff_t written{0};
    buf_span[written++] = L'\x1b';
    buf_span[written++] = L'[';

    std::ranges::copy(prefix, buf_span.begin() + written);
    written += static_cast<std::ptrdiff_t>(prefix.size());

    bool sep = false;
    ((sep ? (buf_span[written++] = L';', 0) : 0,
      written += format_u8(buf_span.subspan(written), args), sep = true),
     ...);
    buf_span[written++] = L'm';
    return written;
}

}

auto
render_ascii_art(const RenderOpts &opts) -> Result<void>
{
    if (!opts.target || opts.target == INVALID_HANDLE_VALUE)
        return fail(ErrCode::InvalidValue, "Invalid render target handle");
    if (opts.pixels.empty() || opts.brush.empty() || opts.width == 0 || opts.height == 0)
        return fail(ErrCode::InvalidValue, "Invalid render parameters");

    static constexpr auto SEQ_NLINE{L"\r\n"sv};
    bool                  to_console{GetFileType(opts.target) == FILE_TYPE_CHAR};
    auto constexpr NLINE_LEN{static_cast<std::ptrdiff_t>(SEQ_NLINE.size())};
    auto max_line_chars{(static_cast<std::ptrdiff_t>(opts.width * MAX_ESC_LEN)) +
                        NLINE_LEN + BUF_PAD};
    std::vector<wchar_t> render_buf(max_line_chars);
#ifdef BENCH_RENDER
    bench::Timer  t{};
    double        bench_calc{0}, bench_format{0}, bench_io{0};
    std::uint64_t bench_cache_hit{0}, bench_cache_miss{0};
#endif

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
        std::uint8_t   prev_r{0}, prev_g{0}, prev_b{0};
        std::ptrdiff_t prev_color_len{0};

        for (std::uint32_t x = 0; x < opts.width; x++)
        {
#ifdef BENCH_RENDER
            t.start();
#endif
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
            wchar_t        brush_chr{opts.brush[brush_idx]};
            std::ptrdiff_t color_len{0};
#ifdef BENCH_RENDER
            t.stop();
            bench_calc += t.elapsed_us;
            t.start();
#endif
            if (opts.color_mode != RenderColorMode::BlackWhite && r == prev_r &&
                g == prev_g && b == prev_b && prev_color_len > 0)
            {
                std::ranges::copy_n(&render_buf[pos - prev_color_len - 1], prev_color_len,
                                    &render_buf[pos]);
                color_len = prev_color_len;
#ifdef BENCH_RENDER
                bench_cache_hit++;
#endif
            }
            else
            {
                std::span<wchar_t> buf_span(&render_buf[pos], max_line_chars - pos);
                switch (opts.color_mode)
                {
                case RenderColorMode::TrueColor:
                    color_len = format_ansi_seq(buf_span, L"38;2;", r, g, b);
                    break;
                case RenderColorMode::TTY16:
                    color_len = format_ansi_seq(buf_span, L"", map_to_tty16(r, g, b));
                    break;
                case RenderColorMode::TTY256:
                    color_len =
                        format_ansi_seq(buf_span, L"38;5;", map_to_tty256(r, g, b));
                    break;
                case RenderColorMode::Grayscale:
                {
                    int gray_code{GRAY_BASE + static_cast<int>(lum * GRAY_STEPS)};
                    gray_code = (std::min) (gray_code, 255);
                    color_len = format_ansi_seq(buf_span, L"38;5;",
                                                static_cast<std::uint8_t>(gray_code));
                    break;
                }
                case RenderColorMode::BlackWhite:
                    break;
                }

                if (opts.color_mode != RenderColorMode::BlackWhite)
                {
                    prev_r         = r;
                    prev_g         = g;
                    prev_b         = b;
                    prev_color_len = color_len;
#ifdef BENCH_RENDER
                    bench_cache_miss++;
#endif
                }
            }

            pos += color_len;
            if (pos < max_line_chars) render_buf[pos++] = brush_chr;
#ifdef BENCH_RENDER
            t.stop();
            bench_format += t.elapsed_us;
#endif
        }

#ifdef BENCH_RENDER
        t.start();
#endif
        if (pos + NLINE_LEN < max_line_chars)
        {
            std::ranges::copy(SEQ_NLINE, render_buf.begin() + pos);
            pos += NLINE_LEN;
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

        static constexpr std::wstring_view RESET_SEQ{L"\x1b[0m"};
        if (opts.color_mode != RenderColorMode::BlackWhite)
        {
            if (to_console)
                WriteConsoleW(opts.target, RESET_SEQ.data(),
                              static_cast<DWORD>(RESET_SEQ.size()), nullptr, nullptr);
            else
                WriteFile(opts.target, RESET_SEQ.data(),
                          static_cast<DWORD>(RESET_SEQ.size() * sizeof(wchar_t)), nullptr,
                          nullptr);
        }
#ifdef BENCH_RENDER
        t.stop();
        bench_io += t.elapsed_us;
#endif
    }
#ifdef BENCH_RENDER
    static bench::CsvWriter csv("bench_render.csv");
    static bool             csv_header{false};
    if (!csv_header)
    {
        csv.header("width,height,pixels,color_mode,calc_us,format_us,io_us,total_us,"
                   "cache_hit,cache_miss");
        csv_header = true;
    }
    csv.rowf("{},{},{},{},{:.1f},{:.1f},{:.1f},{:.1f},{},{}", opts.width, opts.height,
             static_cast<std::size_t>(opts.width) * opts.height,
             static_cast<int>(opts.color_mode), bench_calc, bench_format, bench_io,
             bench_calc + bench_format + bench_io, bench_cache_hit, bench_cache_miss);
#endif
    return {};
}
