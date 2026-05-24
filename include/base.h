// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include <Windows.h>
#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

using i8    = std::int8_t;
using u8    = std::uint8_t;
using i16   = std::int16_t;
using u16   = std::uint16_t;
using i32   = std::int32_t;
using u32   = std::uint32_t;
using i64   = std::int64_t;
using u64   = std::uint64_t;
using isize = std::ptrdiff_t;
using usize = std::size_t;

enum class ErrCode : u8
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

[[nodiscard]] inline auto
last_system_errmsg() -> std::string
{
    return std::system_category().message(static_cast<int>(GetLastError()));
}

inline constexpr u8 IMAGE_PIXEL_BYTE{4}; // BGRA
