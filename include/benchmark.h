// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include <Windows.h>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <string_view>

namespace bench
{

struct Timer
{
    LARGE_INTEGER ticks{};
    LARGE_INTEGER freq{};
    double        elapsed_us{};

    auto
    start() -> void
    {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&ticks);
    }

    auto
    stop() -> void
    {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        elapsed_us = static_cast<double>(end.QuadPart - ticks.QuadPart) * 1000000.0 /
                     static_cast<double>(freq.QuadPart);
    }
};

class CsvWriter
{
    std::ofstream ofs_;

public:
    explicit CsvWriter(const std::string &path) : ofs_{path} {}

    auto
    header(const std::string &cols) -> void
    {
        ofs_ << cols << '\n';
    }

    auto
    row(std::string_view data) -> void
    {
        ofs_ << data << '\n';
    }

    template <typename... Args>
    auto
    rowf(std::string_view fmt, const Args &...args) -> void
    {
        ofs_ << std::vformat(fmt, std::make_format_args(args...)) << '\n';
    }

    auto
    stream() -> auto &
    {
        return ofs_;
    }
};

inline void
print_separator(std::string_view title = {})
{
    if (title.empty()) std::print("{}", std::string(60, '='));
    else std::print("{} {} {}", std::string(60, '='), title, std::string(60, '='));
    std::print("\n");
}

inline void
print_row(std::string_view fmt, auto &&...args)
{
    std::print(fmt, std::forward<decltype(args)>(args)...);
    std::print("\n");
}

}
