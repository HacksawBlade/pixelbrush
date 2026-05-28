#ifndef BASE_H
#define BASE_H

#include <stdbool.h>
#include <stdint.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(*(a)))

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define NODISCARD [[nodiscard]]
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
    #include <sal.h>
    #define NODISCARD _Check_return_
#elif defined(__GNUC__)
    #define NODISCARD __attribute__((__warn_unused_result__))
#else
    #define NODISCARD
#endif

typedef enum
{
    RESULT_SUCCESS,
    RESULT_ERROR_MISSING_ARGS,
    RESULT_ERROR_INVALID_ARGS,
    RESULT_ERROR_ARG_VALUE_NEEDED,
    RESULT_ERROR_PARSE_ARGS,
    RESULT_ERROR_UNKNOWN_OPTION,
    RESULT_ERROR_CANT_GET_HANDLE,
    RESULT_ERROR_CONSOLE_OPS,
    RESULT_ERROR_IMAGE_CANT_LOAD,
    RESULT_ERROR_OUT_OF_MEMORY,
    RESULT_ERROR_FILE_PATH,
    RESULT_ERROR_CANT_CREATE_COM,
    RESULT_ERROR_IO,
    RESULT_ERROR_UNHANDLED,
} Result;

#define Result_succeed(r) ((r) == RESULT_SUCCESS)

#define Flags_add_flag(f, flag) ((f) |= (flag), (f))
#define Flags_has_flag(f, flag) (((f) & (flag)) != 0)

#define Bool_FMT    "%s"
#define Bool_ARG(b) ((b) ? "true" : "false")

#define ERROR_BUF_SIZE 512
#define ERROR_PREFIX   "\x1b[1;37;41m ERROR \x1b[0m"

#if defined(_MSC_VER)
    #define THREAD_LOCAL __declspec(thread)
#else
    #define THREAD_LOCAL _Thread_local
#endif

extern THREAD_LOCAL wchar_t g_errmsg[ERROR_BUF_SIZE];
void                        set_error(const wchar_t *fmt, ...);

#endif
