// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "base.h"

#include <array>
#include <cmath>
#include <format>
#include <fstream>
#include <limits>
#include <print>

namespace
{

struct Lab
{
    double L;
    double a;
    double b;
};

auto
srgb_to_linear(double v) -> double
{
    if (v <= 0.04045) return v / 12.92;
    return std::pow((v + 0.055) / 1.055, 2.4);
}

auto
lab_cbrt(double x) -> double
{
    if (x > 0.008856) return std::cbrt(x);
    return (7.787 * x) + (16.0 / 116.0);
}

auto
rgb_to_lab(u8 r, u8 g, u8 b) -> Lab
{
    double rr{srgb_to_linear(r / 255.0)};
    double gg{srgb_to_linear(g / 255.0)};
    double bb{srgb_to_linear(b / 255.0)};

    double x{(rr * 0.4124564) + (gg * 0.3575761) + (bb * 0.1804375)};
    double y{(rr * 0.2126729) + (gg * 0.7151522) + (bb * 0.0721750)};
    double z{(rr * 0.0193339) + (gg * 0.1191920) + (bb * 0.9503041)};

    double xn{0.95047};
    double yn{1.0};
    double zn{1.08883};

    double fx{(x / xn > 0.008856) ? lab_cbrt(x / xn)
                                  : ((7.787 * x / xn) + (16.0 / 116.0))};
    double fy{(y / yn > 0.008856) ? lab_cbrt(y / yn)
                                  : ((7.787 * y / yn) + (16.0 / 116.0))};
    double fz{(z / zn > 0.008856) ? lab_cbrt(z / zn)
                                  : ((7.787 * z / zn) + (16.0 / 116.0))};

    double L{(116.0 * fy) - 16.0};
    double a{500.0 * (fx - fy)};
    double b_val{200.0 * (fy - fz)};

    return {L, a, b_val};
}

constexpr auto
generate_standard_palette()
{
    std::array<std::array<u8, 3>, 256> table{};

    table[0]  = {0, 0, 0};
    table[1]  = {128, 0, 0};
    table[2]  = {0, 128, 0};
    table[3]  = {128, 128, 0};
    table[4]  = {0, 0, 128};
    table[5]  = {128, 0, 128};
    table[6]  = {0, 128, 128};
    table[7]  = {192, 192, 192};
    table[8]  = {128, 128, 128};
    table[9]  = {255, 0, 0};
    table[10] = {0, 255, 0};
    table[11] = {255, 255, 0};
    table[12] = {0, 0, 255};
    table[13] = {255, 0, 255};
    table[14] = {0, 255, 255};
    table[15] = {255, 255, 255};

    for (int r{0}; r < 6; r++)
        for (int g{0}; g < 6; g++)
            for (int b{0}; b < 6; b++)
            {
                int idx{16 + (r * 36) + (g * 6) + b};
                table.at(idx)[0] = static_cast<u8>(r == 0 ? 0 : 95 + ((r - 1) * 40));
                table.at(idx)[1] = static_cast<u8>(g == 0 ? 0 : 95 + ((g - 1) * 40));
                table.at(idx)[2] = static_cast<u8>(b == 0 ? 0 : 95 + ((b - 1) * 40));
            }

    for (int i{0}; i < 24; i++)
    {
        int  idx{232 + i};
        auto val      = static_cast<u8>(8 + (i * 10));
        table.at(idx) = {val, val, val};
    }

    return table;
}

}

static constexpr usize TTY16_QUANT_LEVEL{4};
static constexpr usize TTY256_QUANT_LEVEL{16};
static constexpr usize TTY16_MAP_SIZE{TTY16_QUANT_LEVEL * TTY16_QUANT_LEVEL *
                                      TTY16_QUANT_LEVEL};
static constexpr usize TTY256_MAP_SIZE{TTY256_QUANT_LEVEL * TTY256_QUANT_LEVEL *
                                       TTY256_QUANT_LEVEL};

static constexpr auto TTY16_PALETTE = []() -> auto
{
    std::array<std::array<u8, 3>, 16> table{};
    table[0]  = {0, 0, 0};
    table[1]  = {128, 0, 0};
    table[2]  = {0, 128, 0};
    table[3]  = {128, 128, 0};
    table[4]  = {0, 0, 128};
    table[5]  = {128, 0, 128};
    table[6]  = {0, 128, 128};
    table[7]  = {192, 192, 192};
    table[8]  = {128, 128, 128};
    table[9]  = {255, 0, 0};
    table[10] = {0, 255, 0};
    table[11] = {255, 255, 0};
    table[12] = {0, 0, 255};
    table[13] = {255, 0, 255};
    table[14] = {0, 255, 255};
    table[15] = {255, 255, 255};
    return table;
}();

static constexpr auto TTY256_PALETTE = generate_standard_palette();

auto
main() -> int
{
    std::array<Lab, 16> tty16_palette_lab{};
    for (int i{0}; i < 16; i++)
    {
        tty16_palette_lab.at(i) = rgb_to_lab(
            TTY16_PALETTE.at(i)[0], TTY16_PALETTE.at(i)[1], TTY16_PALETTE.at(i)[2]);
    }

    std::array<Lab, 256> tty256_palette_lab{};
    for (usize i{0}; i < 256; i++)
    {
        tty256_palette_lab.at(i) = rgb_to_lab(
            TTY256_PALETTE.at(i)[0], TTY256_PALETTE.at(i)[1], TTY256_PALETTE.at(i)[2]);
    }

    std::array<u8, TTY16_MAP_SIZE> tty16_map{};

    for (usize qr{0}; qr < TTY16_QUANT_LEVEL; qr++)
        for (usize qg{0}; qg < TTY16_QUANT_LEVEL; qg++)
            for (usize qb{0}; qb < TTY16_QUANT_LEVEL; qb++)
            {
                int r{static_cast<int>(qr * 255U / (TTY16_QUANT_LEVEL - 1))};
                int g{static_cast<int>(qg * 255U / (TTY16_QUANT_LEVEL - 1))};
                int b{static_cast<int>(qb * 255U / (TTY16_QUANT_LEVEL - 1))};

                Lab input_lab{rgb_to_lab(static_cast<u8>(r), static_cast<u8>(g),
                                         static_cast<u8>(b))};

                int best_idx{0};
                int best_dist{(std::numeric_limits<int>::max)()};

                for (int i{0}; i < 16; i++)
                {
                    const auto &c{tty16_palette_lab.at(i)};
                    double      dL{input_lab.L - c.L};
                    double      da{input_lab.a - c.a};
                    double      db{input_lab.b - c.b};
                    int         dist{static_cast<int>((dL * dL) + (da * da) + (db * db))};

                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best_idx  = i;
                    }
                }

                int ansi_code{best_idx < 8 ? 30 + best_idx : 90 + best_idx - 8};
                tty16_map.at((qr * TTY16_QUANT_LEVEL * TTY16_QUANT_LEVEL) +
                             (qg * TTY16_QUANT_LEVEL) + qb) = static_cast<u8>(ansi_code);
            }

    std::array<u8, TTY256_MAP_SIZE> tty256_map{};

    for (usize qr{0}; qr < TTY256_QUANT_LEVEL; qr++)
        for (usize qg{0}; qg < TTY256_QUANT_LEVEL; qg++)
            for (usize qb{0}; qb < TTY256_QUANT_LEVEL; qb++)
            {
                int r{static_cast<int>(qr * 255U / (TTY256_QUANT_LEVEL - 1))};
                int g{static_cast<int>(qg * 255U / (TTY256_QUANT_LEVEL - 1))};
                int b{static_cast<int>(qb * 255U / (TTY256_QUANT_LEVEL - 1))};

                Lab input_lab{rgb_to_lab(static_cast<u8>(r), static_cast<u8>(g),
                                         static_cast<u8>(b))};

                int best_idx{0};
                int best_dist{(std::numeric_limits<int>::max)()};

                for (usize i{0}; i < 256; i++)
                {
                    const auto &c = tty256_palette_lab.at(i);
                    double      dL{input_lab.L - c.L};
                    double      da{input_lab.a - c.a};
                    double      db{input_lab.b - c.b};
                    int         dist{static_cast<int>((dL * dL) + (da * da) + (db * db))};

                    if (dist < best_dist)
                    {
                        best_dist = dist;
                        best_idx  = static_cast<int>(i);
                    }
                }

                tty256_map.at((qr * TTY256_QUANT_LEVEL * TTY256_QUANT_LEVEL) +
                              (qg * TTY256_QUANT_LEVEL) + qb) = static_cast<u8>(best_idx);
            }

    std::ofstream ofs{"include/tty_maps.h"};
    if (!ofs) return 1;

    ofs << "// SPDX-License-Identifier: MIT\n";
    ofs << "// Copyright (c) 2026 锯条\n";
    ofs << "\n";
    ofs << "#pragma once\n";
    ofs << "\n";
    ofs << "#include \"base.h\"\n";
    ofs << "\n";
    ofs << "#include <array>\n";
    ofs << "\n";

    ofs << std::format("inline constexpr usize TTY16_QUANT_LEVEL{{{}}};\n",
                       TTY16_QUANT_LEVEL);
    ofs << std::format("inline constexpr usize TTY256_QUANT_LEVEL{{{}}};\n",
                       TTY256_QUANT_LEVEL);
    ofs << std::format("inline constexpr usize TTY16_MAP_SIZE{{{}}};\n", TTY16_MAP_SIZE);
    ofs << std::format("inline constexpr usize TTY256_MAP_SIZE{{{}}};\n",
                       TTY256_MAP_SIZE);
    ofs << "\n";

    ofs << "inline constexpr std::array<u8, TTY16_MAP_SIZE> TTY16_MAP{{\n";

    for (usize i{0}; i < TTY16_MAP_SIZE; i++)
    {
        ofs << std::format("{}, ", static_cast<int>(tty16_map.at(i)));
        if (i % 16 == 15) ofs << "\n";
    }
    ofs << "}};\n";
    ofs << "\n";

    ofs << "inline constexpr std::array<u8, TTY256_MAP_SIZE> TTY256_MAP{{\n";

    for (usize i{0}; i < TTY256_MAP_SIZE; i++)
    {
        ofs << std::format("{}, ", static_cast<int>(tty256_map.at(i)));
        if (i % 16 == 15) ofs << "\n";
    }
    ofs << "}};\n";

    std::println("Generated: TTY16_MAP ({} entries, ANSI codes 30-97, Lab distance)",
                 TTY16_MAP_SIZE);
    std::println("Generated: TTY256_MAP ({} entries, Lab distance)", TTY256_MAP_SIZE);
    std::println("Output: include/tty_maps.h");
}
