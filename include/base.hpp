// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include <Windows.h>
#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

enum class ErrCode : std::uint8_t
{
    Success,
    MissingArgs,
    InvalidArgs,
    ArgValueNeeded,
    ParseArgs,
    UnknownOption,
    CantGetHandle,
    ConsoleOps,
    ImageCantLoad,
    OutOfMemory,
    FilePath,
    CreateObject,
    IO,
    InvalidValue,
};

class Error
{
public:
    ErrCode     code;
    std::string message;

    Error() : code{ErrCode::Success} {}
    explicit Error(ErrCode c, std::string msg = {}) : code{c}, message{std::move(msg)} {}
};

template <typename T>
using Result = std::expected<T, Error>;

template <typename... Args>
    requires std::constructible_from<Error, Args...>
[[nodiscard]] inline auto
fail(Args &&...args)
{
    return std::unexpected<Error>(std::in_place, std::forward<Args>(args)...);
}

[[nodiscard]] inline std::string
last_system_errmsg()
{
    return std::system_category().message(static_cast<int>(GetLastError()));
}

inline constexpr std::uint8_t IMAGE_PIXEL_BYTE{4}; // BRGA
