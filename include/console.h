#ifndef CONSOLE_H
#define CONSOLE_H

#include "base.h"

#include <Windows.h>
#include <stdbool.h>

#define CONSOLE_DEFAULT_COLS 80
#define CONSOLE_DEFAULT_ROWS 40

NODISCARD Result Console_init(HANDLE *handle_ref);
NODISCARD Result Console_get_size(const HANDLE *handle, int *cols, int *rows);

#endif
