// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.h"

#include <span>
#include <winrt/base.h>

enum class ScaleMode : u8
{
    Fant,
    NearestNeighbor,
};

class Image
{
private:
    u32                  width_{0};
    u32                  height_{0};
    winrt::com_array<u8> pixels_;
    ScaleMode            scale_mode_{ScaleMode::Fant};

    Image(u32 w, u32 h, winrt::com_array<u8> data)
        : width_{w}, height_{h}, pixels_{std::move(data)}
    {
    }

public:
    Image() = delete;

    [[nodiscard]] static auto open(const std::wstring &path) -> Result<Image>;

    [[nodiscard]] auto scale(u32 new_w, u32 new_h) -> Result<void>;
    [[nodiscard]] auto width() const -> u32;
    [[nodiscard]] auto height() const -> u32;
    [[nodiscard]] auto pixels() -> std::span<u8>;
    [[nodiscard]] auto pixels() const -> std::span<const u8>;
    [[nodiscard]] auto scale_mode() -> ScaleMode;
    [[nodiscard]] auto scale_mode(ScaleMode new_scale_mode) -> ScaleMode;
    [[nodiscard]] auto scale_mode() const -> ScaleMode;
};
