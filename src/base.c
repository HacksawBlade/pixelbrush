#include "base.h"

#include <stdarg.h>
#include <wchar.h>

THREAD_LOCAL wchar_t g_errmsg[ERROR_BUF_SIZE] = L"";

void
set_error(const wchar_t *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    (void) vswprintf(g_errmsg, ERROR_BUF_SIZE, fmt, args);
    va_end(args);
}
