// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.hpp"

#include <cstdint>
#include <span>
#include <winrt/base.h>

enum class ScaleMode : std::uint8_t
{
    Fant,
    NearestNeighbor,
};

class Image
{
private:
    std::uint32_t                  width_{0};
    std::uint32_t                  height_{0};
    winrt::com_array<std::uint8_t> pixels_;
    ScaleMode                      scale_mode_{ScaleMode::Fant};

    Image(std::uint32_t w, std::uint32_t h, winrt::com_array<std::uint8_t> data)
        : width_{w}, height_{h}, pixels_{std::move(data)}
    {
    }

public:
    Image() = delete;

    [[nodiscard]] static Result<Image> open(const std::wstring &path);

    [[nodiscard]] Result<void>            scale(std::uint32_t new_w, std::uint32_t new_h);
    [[nodiscard]] std::uint32_t           width() const;
    [[nodiscard]] std::uint32_t           height() const;
    [[nodiscard]] std::span<std::uint8_t> pixels();
    [[nodiscard]] std::span<const std::uint8_t> pixels() const;
    [[nodiscard]] ScaleMode                     scale_mode();
    [[nodiscard]] ScaleMode                     scale_mode(ScaleMode new_scale_mode);
    [[nodiscard]] ScaleMode                     scale_mode() const;
};
