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
#include <mdspan>
#include <semaphore>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

#ifdef BENCH_RENDER
    #include "benchmark.h"
#endif

using namespace std::string_view_literals;

static constexpr std::ptrdiff_t MAX_ESC_LEN{20};
static constexpr int            GRAY_BASE{232};
static constexpr int            GRAY_STEPS{23};
static constexpr std::ptrdiff_t BUF_PAD{8};

namespace
{

template <std::size_t QuantLevel, std::size_t N>
[[nodiscard]] inline auto
map_to_tty_impl(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                const std::array<std::uint8_t, N> &map) -> std::uint8_t
{
    std::size_t qr{(r * (QuantLevel - 1)) / 255};
    std::size_t qg{(g * (QuantLevel - 1)) / 255};
    std::size_t qb{(b * (QuantLevel - 1)) / 255};
    return map.at((qr * QuantLevel * QuantLevel) + (qg * QuantLevel) + qb);
}

[[nodiscard]] inline auto
map_to_tty16(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> std::uint8_t
{
    return map_to_tty_impl<TTY16_QUANT_LEVEL>(r, g, b, TTY16_MAP);
}

[[nodiscard]] inline auto
map_to_tty256(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> std::uint8_t
{
    return map_to_tty_impl<TTY256_QUANT_LEVEL>(r, g, b, TTY256_MAP);
}

[[nodiscard]] inline auto
format_u8(std::span<wchar_t> buf, std::uint8_t v) -> std::ptrdiff_t
{
    if (v == 0)
    {
        buf[0] = L'0';
        return 1;
    }
    std::array<wchar_t, 4> temp{};
    std::ptrdiff_t         i{3};
    while (v > 0)
    {
        temp.at(i--) = L'0' + (v % 10);
        v /= 10;
    }
    std::ptrdiff_t len = 3 - i;
    std::ranges::copy_n(&temp.at(i + 1), len, buf.begin());
    return len;
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

    static constexpr std::wstring_view SEQ_NLINE{L"\r\n"};
    static constexpr std::ptrdiff_t    SEQ_NLINE_LEN{SEQ_NLINE.size()};
    const bool to_console{GetFileType(opts.target) == FILE_TYPE_CHAR};
    auto       max_line_chars{(opts.width * MAX_ESC_LEN) + SEQ_NLINE_LEN + BUF_PAD};

    std::array<std::vector<wchar_t>, 2> render_bufs{{
        std::vector<wchar_t>(max_line_chars),
        std::vector<wchar_t>(max_line_chars),
    }};
    std::array<std::ptrdiff_t, 2>       row_lens{};    // 缓冲区内容的实际长度
    std::counting_semaphore<2>          sem_empty{2};  // 缓冲区空闲数量
    std::counting_semaphore<2>          sem_filled{0}; // 缓冲区填满数量

#ifdef BENCH_RENDER
    bench::Timer  t_io{}, t_other{};
    double        bench_io{0}, bench_calc{0}, bench_format{0};
    std::uint64_t bench_cache_hit{0}, bench_cache_miss{0};
#endif

    std::jthread io_thread(
        [&]() -> void
        {
            if (!to_console)
            {
                static constexpr uint16_t BOM_UTF16{0xFEFF};
                WriteFile(opts.target, &BOM_UTF16, sizeof(BOM_UTF16), nullptr, nullptr);
            }

            std::uint8_t p_read{0};
            for (std::uint32_t y{0}; y < opts.height; y++)
            {
                sem_filled.acquire();
                auto row_buf =
                    std::span{render_bufs.at(p_read)}.first(row_lens.at(p_read));

#ifdef BENCH_RENDER
                t_io.start();
#endif

                if (to_console)
                    WriteConsoleW(opts.target, row_buf.data(),
                                  static_cast<DWORD>(row_buf.size()), nullptr, nullptr);
                else
                    WriteFile(opts.target, row_buf.data(),
                              static_cast<DWORD>(row_buf.size_bytes()), nullptr, nullptr);

#ifdef BENCH_RENDER
                t_io.stop();
                bench_io += t_io.elapsed_us;
#endif

                sem_empty.release();
                p_read = (p_read + 1u) & 1;
            }

            static constexpr std::wstring_view RESET_SEQ{L"\x1b[0m"};
            if (to_console && opts.color_mode != RenderColorMode::BlackWhite)
            {
                WriteConsoleW(opts.target, RESET_SEQ.data(),
                              static_cast<DWORD>(RESET_SEQ.size()), nullptr, nullptr);
            }
        });

    using PixelExtents = std::extents<std::size_t, std::dynamic_extent,
                                      std::dynamic_extent, IMAGE_PIXEL_BYTE>;
    auto px = std::mdspan<const std::uint8_t, PixelExtents>{opts.pixels.data(),
                                                            opts.height, opts.width};

    std::uint8_t p_write{0};
    for (std::uint32_t y{0}; y < opts.height; y++)
    {
        auto &render_buf = render_bufs.at(p_write);
        sem_empty.acquire();

        std::ptrdiff_t pos{0};
        std::uint8_t   prev_r{0}, prev_g{0}, prev_b{0};
        std::ptrdiff_t prev_color_len{0};

        for (std::uint32_t x{0}; x < opts.width; x++)
        {
#ifdef BENCH_RENDER
            t_other.start();
#endif

            std::uint8_t b = px[y, x, 0];
            std::uint8_t g = px[y, x, 1];
            std::uint8_t r = px[y, x, 2];
            double       lum{((0.2126 * r) + (0.7152 * g) + (0.0722 * b)) / 255};

            auto brush_idx = static_cast<std::size_t>(
                lum * static_cast<double>(opts.brush.size() - 1));
            wchar_t        brush_chr{opts.brush[brush_idx]};
            std::ptrdiff_t color_len{0};

#ifdef BENCH_RENDER
            t_other.stop();
            bench_calc += t_other.elapsed_us;
            t_other.start();
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
                    std::uint8_t gray_code = std::clamp(
                        GRAY_BASE + static_cast<int>(lum * GRAY_STEPS), GRAY_BASE, 255);
                    color_len = format_ansi_seq(buf_span, L"38;5;", gray_code);
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
            t_other.stop();
            bench_format += t_other.elapsed_us;
#endif
        }

        if (pos + SEQ_NLINE_LEN < max_line_chars)
        {
            std::ranges::copy(SEQ_NLINE, render_buf.begin() + pos);
            pos += SEQ_NLINE_LEN;
        }
        render_buf[pos] = 0;

        row_lens.at(p_write) = pos;
        sem_filled.release();
        p_write = (p_write + 1u) & 1;
    }

#ifdef BENCH_RENDER
    bench::CsvWriter bench_csv{"benchmark/render.csv"};
    bench_csv.header("width,height,pixels,color_mode,calc_us,format_us,io_us,"
                     "total_us,cache_hit,cache_miss");
    bench_csv.rowf("{},{},{},{},{:.1f},{:.1f},{:.1f},{:.1f},{},{}", opts.width,
                   opts.height, static_cast<std::size_t>(opts.width) * opts.height,
                   static_cast<int>(opts.color_mode), bench_calc, bench_format, bench_io,
                   bench_calc + bench_format + bench_io, bench_cache_hit,
                   bench_cache_miss);
#endif
    return {};
}
