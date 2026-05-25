// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include <Windows.h>
#include <memory>

class ScopedHandle
{
private:
    struct HandleDeleter
    {
        bool owning = false;

        void
        operator()(HANDLE h) const noexcept
        {
            if (owning && h != INVALID_HANDLE_VALUE) CloseHandle(h);
        }
    };

    using PtrHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleDeleter>;
    PtrHandle ptr_;

public:
    ScopedHandle() noexcept : ptr_{nullptr, HandleDeleter{false}} {}
    ScopedHandle(HANDLE h, bool own) noexcept : ptr_{h, HandleDeleter{own}} {}

    ~ScopedHandle() = default;

    ScopedHandle(ScopedHandle &&other) noexcept                     = default;
    auto operator=(ScopedHandle &&other) noexcept -> ScopedHandle & = default;

    ScopedHandle(const ScopedHandle &)                     = delete;
    auto operator=(const ScopedHandle &) -> ScopedHandle & = delete;

    auto
    operator*() const -> HANDLE
    {
        return ptr_.get();
    }

    explicit
    operator bool() const noexcept
    {
        HANDLE h{ptr_.get()};
        return h != nullptr && h != INVALID_HANDLE_VALUE;
    }

    [[nodiscard]] auto
    get() const noexcept -> HANDLE
    {
        return ptr_.get();
    }
};
