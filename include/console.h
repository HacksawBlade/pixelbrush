#pragma once

#include "base.hpp"

#include <utility>

inline constexpr int CONSOLE_DEFAULT_COLS{80};
inline constexpr int CONSOLE_DEFAULT_ROWS{40};

namespace console
{

class CpGuard
{
private:
    UINT saved_{};

public:
    CpGuard();
    ~CpGuard();

    CpGuard(const CpGuard &)            = delete;
    CpGuard(CpGuard &&)                 = delete;
    CpGuard &operator=(CpGuard &&)      = delete;
    CpGuard &operator=(const CpGuard &) = delete;
};

[[nodiscard]] Result<void>                init(HANDLE &handle);
[[nodiscard]] Result<std::pair<int, int>> get_size(HANDLE handle);

}
