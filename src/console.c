#include "console.h"

#include "base.h"

Result
Console_init(HANDLE *handle_ref)
{
    if (handle_ref == INVALID_HANDLE_VALUE)
    {
        set_error(L"Invalid console handle");
        return RESULT_ERROR_CANT_GET_HANDLE;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(*handle_ref, &mode))
    {
        set_error(L"GetConsoleMode failed (GetLastError=%lu)", GetLastError());
        return RESULT_ERROR_CONSOLE_OPS;
    }

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(*handle_ref, mode))
    {
        set_error(L"SetConsoleMode failed (GetLastError=%lu)", GetLastError());
        return RESULT_ERROR_CONSOLE_OPS;
    }
    return RESULT_SUCCESS;
}

Result
Console_get_size(const HANDLE *handle, int *cols_ref, int *rows_ref)
{
    if (!cols_ref || !rows_ref)
    {
        set_error(L"%hs: NULL output pointer", __func__);
        return RESULT_ERROR_INVALID_ARGS;
    }

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (handle == INVALID_HANDLE_VALUE)
    {
        set_error(L"%hs: invalid console handle", __func__);
        return RESULT_ERROR_CANT_GET_HANDLE;
    }

    if (!GetConsoleScreenBufferInfo(*handle, &info))
    {
        set_error(L"GetConsoleScreenBufferInfo failed (GetLastError=%lu)",
                  GetLastError());
        return RESULT_ERROR_CONSOLE_OPS;
    }

    *cols_ref = info.srWindow.Right - info.srWindow.Left + 1;
    *rows_ref = info.srWindow.Bottom - info.srWindow.Top + 1;
    return RESULT_SUCCESS;
}
