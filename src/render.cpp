// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "render.h"

#include "base.h"
#include "pipeline.h"
#include "tty_maps.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <mdspan>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

#ifdef BENCH_RENDER
    #include "benchmark.h"
#endif

using namespace std::string_view_literals;

static constexpr isize MAX_ESC_LEN{20};
static constexpr int   GRAY_BASE{232};
static constexpr int   GRAY_STEPS{23};
static constexpr isize BUF_PAD{8};

namespace
{

template <usize QuantLevel, usize N>
[[nodiscard]] inline auto
map_to_tty_impl(u8 r, u8 g, u8 b, const std::array<u8, N> &map) -> u8
{
    usize qr{(r * (QuantLevel - 1)) / 255};
    usize qg{(g * (QuantLevel - 1)) / 255};
    usize qb{(b * (QuantLevel - 1)) / 255};
    return map.at((qr * QuantLevel * QuantLevel) + (qg * QuantLevel) + qb);
}

[[nodiscard]] inline auto
map_to_tty16(u8 r, u8 g, u8 b) -> u8
{
    return map_to_tty_impl<TTY16_QUANT_LEVEL>(r, g, b, TTY16_MAP);
}

[[nodiscard]] inline auto
map_to_tty256(u8 r, u8 g, u8 b) -> u8
{
    return map_to_tty_impl<TTY256_QUANT_LEVEL>(r, g, b, TTY256_MAP);
}

[[nodiscard]] inline auto
format_u8(std::span<wchar_t> buf, u8 v) -> isize
{
    if (v == 0)
    {
        buf[0] = L'0';
        return 1;
    }
    std::array<wchar_t, 4> temp{};
    isize                  i{3};
    while (v > 0)
    {
        temp.at(i--) = L'0' + (v % 10);
        v /= 10;
    }
    isize len{3 - i};
    std::ranges::copy_n(&temp.at(i + 1), len, buf.begin());
    return len;
}

template <typename... Args>
    requires(std::same_as<std::decay_t<Args>, u8> && ...)
[[nodiscard]] auto
format_ansi_seq(std::span<wchar_t> buf_span, std::wstring_view prefix, Args... args)
    -> isize
{
    isize written{0};
    buf_span[written++] = L'\x1b';
    buf_span[written++] = L'[';

    std::ranges::copy(prefix, buf_span.begin() + written);
    written += static_cast<isize>(prefix.size());

    bool sep       = false;
    auto write_arg = [&](u8 arg) -> auto
    {
        if (sep) buf_span[written++] = L';';
        written += format_u8(buf_span.subspan(written), arg);
        sep = true;
    };
    (write_arg(args), ...);
    buf_span[written++] = L'm';
    return written;
}

struct CacheEntry
{
    u32     key{0xFFFFFFFF}; // RGB(255, 0, 128) -> 255000128
    u16     len{0};          // ANSI 序列内容长度，使用 16 位消除类型转换警告
    wchar_t seq[MAX_ESC_LEN]{};
};

[[nodiscard]] auto
render_ascii_art_impl(const RenderOpts &opts, std::span<CacheEntry> cache,
                      usize cache_n_log2) -> Result<void>
{
    static constexpr std::wstring_view SEQ_NLINE{L"\r\n"};
    static constexpr isize             SEQ_NLINE_LEN{SEQ_NLINE.size()};
    const bool to_console{GetFileType(opts.target) == FILE_TYPE_CHAR};
    const auto max_line_chars{(opts.width * MAX_ESC_LEN) + SEQ_NLINE_LEN + BUF_PAD};

#ifdef BENCH_RENDER
    bench::Timer t_io{}, t_other{};
    double       bench_io{0}, bench_calc{0}, bench_format{0};
    u64          bench_cache_hit{0}, bench_cache_miss{0};
#endif

    std::string utf8_buf;
    if (opts.output_format == OutputFormat::UTF8)
        utf8_buf.reserve(max_line_chars * strutil::MAX_UTF8_BYTES_PER_WCHAR);

    Pipeline<wchar_t, 2> pipe(max_line_chars);

    std::thread io_thread(
        [&]() -> void
        {
            if (!to_console && opts.output_format == OutputFormat::UTF16LE)
            {
                static constexpr u16 BOM_UTF16LE{0xFEFF};
                if (!WriteFile(opts.target, &BOM_UTF16LE, sizeof(BOM_UTF16LE), nullptr,
                               nullptr)) [[unlikely]]
                {
                    pipe.signal_fail();
                    return;
                }
            }

            for (u32 y{0}; y < opts.height; y++)
            {
                auto [buf, len] = pipe.acquire_read();
                if (pipe.is_aborted()) [[unlikely]]
                    return;

#ifdef BENCH_RENDER
                t_io.start();
#endif

                if (to_console)
                {
                    if (!WriteConsoleW(opts.target, buf.data(), static_cast<DWORD>(len),
                                       nullptr, nullptr)) [[unlikely]]
                    {
                        pipe.signal_fail();
                        return;
                    }
                }
                else
                {
                    switch (opts.output_format)
                    {
                    case OutputFormat::UTF8:
                    {
                        if (auto conv = strutil::to_narrow(
                                std::wstring_view{buf.data(), static_cast<usize>(len)},
                                utf8_buf);
                            !conv) [[unlikely]]
                        {
                            pipe.signal_fail();
                            return;
                        }
                        if (!utf8_buf.empty())
                            if (!WriteFile(opts.target, utf8_buf.data(),
                                           static_cast<DWORD>(utf8_buf.size()), nullptr,
                                           nullptr)) [[unlikely]]
                            {
                                pipe.signal_fail();
                                return;
                            }
                        break;
                    }
                    case OutputFormat::UTF16LE:
                        if (!WriteFile(opts.target, buf.data(),
                                       static_cast<DWORD>(len * sizeof(wchar_t)), nullptr,
                                       nullptr)) [[unlikely]]
                        {
                            pipe.signal_fail();
                            return;
                        }
                        break;
                    }
                }

#ifdef BENCH_RENDER
                t_io.stop();
                bench_io += t_io.elapsed_us;
#endif

                pipe.release_read();
            }

            if (to_console && opts.color_mode != RenderColorMode::BlackWhite)
            {
                static constexpr std::wstring_view RESET_SEQ{L"\x1b[0m"};
                WriteConsoleW(opts.target, RESET_SEQ.data(),
                              static_cast<DWORD>(RESET_SEQ.size()), nullptr, nullptr);
            }
        });

    PipelineGuard guard{pipe, io_thread};

    using PixelExtents =
        std::extents<usize, std::dynamic_extent, std::dynamic_extent, IMAGE_PIXEL_BYTE>;
    std::mdspan<const u8, PixelExtents> px{opts.pixels.data(), opts.height, opts.width};

    for (u32 y{0}; y < opts.height; y++)
    {
        auto buf = pipe.acquire_write();
        if (pipe.is_aborted()) [[unlikely]]
            break;

        isize pos{0};
        for (u32 x{0}; x < opts.width; x++)
        {
#ifdef BENCH_RENDER
            t_other.start();
#endif

            u8     b{px[y, x, 0]};
            u8     g{px[y, x, 1]};
            u8     r{px[y, x, 2]};
            double lum{((0.2126 * r) + (0.7152 * g) + (0.0722 * b)) / 255}; // BT.709

            auto brush_idx =
                static_cast<usize>(lum * static_cast<double>(opts.brush.size() - 1));
            wchar_t brush_chr{opts.brush[brush_idx]};
            isize   color_len{0};

#ifdef BENCH_RENDER
            t_other.stop();
            bench_calc += t_other.elapsed_us;
            t_other.start();
#endif

            if (opts.color_mode != RenderColorMode::BlackWhite)
            {
                auto key = (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | b;
                // 0x9E3779B1: Donald Knuth 黄金分割哈希
                auto  hash_idx = (key * 0x9E3779B1) >> (32 - cache_n_log2);
                auto &entry    = cache[hash_idx];

                if (entry.key == key)
                {
                    std::ranges::copy_n(static_cast<wchar_t *>(entry.seq), entry.len,
                                        &buf[pos]);
                    color_len = static_cast<isize>(entry.len);
#ifdef BENCH_RENDER
                    bench_cache_hit++;
#endif
                }
                else
                {
                    std::span<wchar_t> buf_span(&buf[pos], max_line_chars - pos);
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
                        u8 gray_code =
                            std::clamp(static_cast<int>(GRAY_BASE + (lum * GRAY_STEPS)),
                                       GRAY_BASE, 255);
                        color_len = format_ansi_seq(buf_span, L"38;5;", gray_code);
                        break;
                    }
                    case RenderColorMode::BlackWhite:
                        break;
                    }

                    entry.key = key;
                    entry.len = static_cast<u16>(color_len);
                    std::ranges::copy_n(&buf[pos], color_len,
                                        static_cast<wchar_t *>(entry.seq));
#ifdef BENCH_RENDER
                    bench_cache_miss++;
#endif
                }
            }

            pos += color_len;
            if (pos < max_line_chars) [[likely]]
                buf[pos++] = brush_chr;

#ifdef BENCH_RENDER
            t_other.stop();
            bench_format += t_other.elapsed_us;
#endif
        }

        if (pos + SEQ_NLINE_LEN < max_line_chars) [[likely]]
        {
            std::ranges::copy(SEQ_NLINE, buf.begin() + pos);
            pos += SEQ_NLINE_LEN;
        }
        buf[pos] = 0;

        pipe.release_write(pos);
    }

    io_thread.join();

#ifdef BENCH_RENDER
    bench::CsvWriter bench_csv{"benchmark/render.csv"};
    bench_csv.header("width,height,pixels,color_mode,calc_us,format_us,io_us,"
                     "total_us,cache_hit,cache_miss");
    bench_csv.rowf("{},{},{},{},{:.1f},{:.1f},{:.1f},{:.1f},{},{}", opts.width,
                   opts.height, static_cast<usize>(opts.width) * opts.height,
                   static_cast<int>(opts.color_mode), bench_calc, bench_format, bench_io,
                   bench_calc + bench_format + bench_io, bench_cache_hit,
                   bench_cache_miss);
#endif

    if (pipe.is_aborted()) [[unlikely]]
        return fail(ErrCode::IO, "Failed to write output");

    return {};
}

}

auto
render_ascii_art(const RenderOpts &opts) -> Result<void>
{
    if (!opts.target || opts.target == INVALID_HANDLE_VALUE)
        return fail(ErrCode::InvalidValue, "Invalid render target handle");
    if (opts.pixels.empty() || opts.brush.empty() || opts.width == 0 || opts.height == 0)
        return fail(ErrCode::InvalidValue, "Invalid render parameters");
    if (opts.pixels.size() <
        static_cast<usize>(opts.width) * opts.height * IMAGE_PIXEL_BYTE)
        return fail(ErrCode::InvalidValue, "Pixel buffer too small for given dimensions");

    static constexpr usize SMALL_CACHE_N_LOG2{12};
    static constexpr usize LARGE_CACHE_N_LOG2{16};
    static constexpr u64   CACHE_THRESHOLD{8'000'000};

    if (static_cast<u64>(opts.width) * opts.height <= CACHE_THRESHOLD) [[likely]]
    {
        std::array<CacheEntry, 1 << SMALL_CACHE_N_LOG2> cache{};
        return render_ascii_art_impl(opts, cache, SMALL_CACHE_N_LOG2);
    }

    std::vector<CacheEntry> cache(1 << LARGE_CACHE_N_LOG2);
    return render_ascii_art_impl(opts, cache, LARGE_CACHE_N_LOG2);
}
