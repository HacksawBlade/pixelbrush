// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#include "base.h"

#include <array>
#include <atomic>
#include <semaphore>
#include <span>
#include <thread>
#include <vector>

template <typename T, usize N = 2>
class Pipeline
{
public:
    explicit Pipeline(usize slot_size)
    {
        for (auto &s : slots_)
            s.buf.resize(slot_size);
    }

    [[nodiscard]] auto
    acquire_write() -> std::span<T>
    {
        empty_.acquire();
        return slots_[write_idx_].buf;
    }

    auto
    release_write(isize len) -> void
    {
        slots_[write_idx_].len = len;
        filled_.release();
        write_idx_ = (write_idx_ + 1) % N;
    }

    [[nodiscard]] auto
    acquire_read() -> std::pair<std::span<T>, isize>
    {
        filled_.acquire();
        auto &slot = slots_[read_idx_];
        return {slot.buf, slot.len};
    }

    auto
    release_read() -> void
    {
        empty_.release();
        read_idx_ = (read_idx_ + 1) % N;
    }

    auto
    signal_fail() -> void
    {
        aborted_ = true;
        empty_.release();
    }

    [[nodiscard]] auto
    is_aborted() const noexcept -> bool
    {
        return aborted_.load();
    }

    auto
    force_release_all() noexcept -> void
    {
        aborted_ = true;
        empty_.release();
        filled_.release();
    }

private:
    struct Slot
    {
        std::vector<T> buf;
        isize          len{0};
    };

    std::array<Slot, N>        slots_;
    std::counting_semaphore<N> empty_{N};
    std::counting_semaphore<N> filled_{0};
    usize                      write_idx_{0};
    usize                      read_idx_{0};
    std::atomic<bool>          aborted_{false};
};

template <typename T, usize N>
class PipelineGuard
{
public:
    PipelineGuard(Pipeline<T, N> &pipe, std::thread &t) noexcept : pipe_{pipe}, t_{t} {}

    PipelineGuard(const PipelineGuard &)                     = delete;
    auto operator=(const PipelineGuard &) -> PipelineGuard & = delete;
    PipelineGuard(PipelineGuard &&)                          = delete;
    auto operator=(PipelineGuard &&) -> PipelineGuard &      = delete;

    ~PipelineGuard() noexcept
    {
        if (t_.joinable())
        {
            pipe_.force_release_all();
            t_.join();
        }
    }

private:
    std::thread    &t_;
    Pipeline<T, N> &pipe_;
};
