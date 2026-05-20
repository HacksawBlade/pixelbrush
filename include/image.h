// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.h"

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

    [[nodiscard]] static auto open(const std::wstring &path) -> Result<Image>;

    [[nodiscard]] auto scale(std::uint32_t new_w, std::uint32_t new_h) -> Result<void>;
    [[nodiscard]] auto width() const -> std::uint32_t;
    [[nodiscard]] auto height() const -> std::uint32_t;
    [[nodiscard]] auto pixels() -> std::span<std::uint8_t>;
    [[nodiscard]] auto pixels() const -> std::span<const std::uint8_t>;
    [[nodiscard]] auto scale_mode() -> ScaleMode;
    [[nodiscard]] auto scale_mode(ScaleMode new_scale_mode) -> ScaleMode;
    [[nodiscard]] auto scale_mode() const -> ScaleMode;
};
