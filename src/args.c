#include "args.h"

#include "base.h"

#include <Windows.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

typedef void (*OptionHandler)(const void *value, Args *args_ref);
#define OPTION_HANDLE_GET(arg_name) (handle_##arg_name)
#define OPTION_HANDLE_DEFINE(arg_name, ArgType)                                \
    static inline void OPTION_HANDLE_GET(arg_name)(const void *value,          \
                                                   Args       *args_ref)       \
    {                                                                          \
        args_ref->arg_name = *(ArgType *) value;                               \
    }

typedef void (*OptionArrayHandler)(const void *values, uint8_t count,
                                   Args *args_ref);
#define OPTION_ARRAY_HANDLE_DEFINE(arg_name, ArgType, max_count)               \
    static inline void OPTION_HANDLE_GET(arg_name)(                            \
        const void *values, uint8_t count, Args *args_ref)                     \
    {                                                                          \
        const ArgType *vals = (const ArgType *) values;                        \
        uint8_t        n    = (count < (max_count)) ? count : (max_count);     \
        for (uint8_t i = 0; i < n; i++)                                        \
            args_ref->arg_name[i] = vals[i];                                   \
    }

typedef struct
{
    bool              case_sensitive;
    const StringEnum *enums;
    uint32_t          enums_count;
} OptionStringPlugin;

typedef struct
{
    uint8_t max_len;
    bool    force_filling;
} OptionArrayPlugin;

typedef struct
{
    const wchar_t *fullname;
    const wchar_t *alias;
    OptionType     type;
    const wchar_t *desc;

    union
    {
        OptionHandler      handler;
        OptionArrayHandler array_handler;
    };

    OptionStringPlugin string_plugin;
    OptionArrayPlugin  array_plugin;
} Option;

static const StringEnum BRUSH_ENUMS[] = {
    {L"block", L"█"},
    {L"dot", L"⬤"},
    {L"shades", L" ░▒▓█"},
    {L"symbols", L" .:-=+*#%@"},
    {L"letters",
     L" .'`^\",:;Il!i~+_-?][}{1)(\\/,tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"},
};

OPTION_HANDLE_DEFINE(brush, wchar_t *);
OPTION_ARRAY_HANDLE_DEFINE(size, int, 2);
OPTION_HANDLE_DEFINE(width_scale, double);
OPTION_HANDLE_DEFINE(grayscale, bool);
OPTION_HANDLE_DEFINE(blackwhite, bool);

static const Option OPTS[] = {
    {
        .fullname = L"--brush",
        .alias    = L"-b",
        .type     = OPTYPE_WCS,
        .desc     = L"Specify the brush to use. (default: block)",
        .handler  = OPTION_HANDLE_GET(brush),
        .string_plugin =
            {
                .case_sensitive = false,
                .enums          = BRUSH_ENUMS,
                .enums_count    = ARRAY_LEN(BRUSH_ENUMS),
            },
    },
    {
        .fullname = L"--size",
        .alias    = L"-s",
        .type     = OPTYPE_INT,
        .desc     = L"Set output character columns and rows, e.g. --size 40 30",
        .array_handler = OPTION_HANDLE_GET(size),
        .array_plugin =
            {
                .max_len       = 2,
                .force_filling = true,
            },
    },
    {
        .fullname = L"--wscale",
        .alias    = L"-w",
        .type     = OPTYPE_DOUBLE,
        .desc     = L"Set the width scaling ratio to fit the current console. "
                    L"(default: 2.0)",
        .handler  = OPTION_HANDLE_GET(width_scale),
    },
    {
        .fullname = L"--grayscale",
        .alias    = L"-g",
        .type     = OPTYPE_BOOL,
        .desc     = L"Generate photos in Grayscale mode. (default: false)",
        .handler  = OPTION_HANDLE_GET(grayscale),
    },
    {
        .fullname = L"--blackwhite",
        .alias    = NULL,
        .type     = OPTYPE_BOOL,
        .desc     = L"Generate photos in Black&White mode. (default: false)",
        .handler  = OPTION_HANDLE_GET(blackwhite),
    },
};
enum
{
    OPTS_COUNT = ARRAY_LEN(OPTS),
};

NODISCARD static inline bool
wcs_eq(const wchar_t *wcs1, const wchar_t *wcs2, bool case_insensitive)
{
    if (case_insensitive)
    {
        return CompareStringOrdinal(wcs1, -1, wcs2, -1, TRUE) == CSTR_EQUAL;
    }
    return wcscmp(wcs1, wcs2) == 0;
}

NODISCARD static inline bool
arg_name_match(const wchar_t *arg, const wchar_t *fullname,
               const wchar_t *alias)
{
    if (arg == NULL || arg[0] != L'-' || (fullname == NULL && alias == NULL))
        return false;

    if (arg[1] == L'-' && fullname != NULL) return wcscmp(arg, fullname) == 0;

    if (arg[1] != L'-' && alias != NULL) return wcscmp(arg, alias) == 0;

    return false;
}

NODISCARD static inline bool
is_option_arg(const wchar_t *arg)
{
    if (arg == NULL || arg[0] != L'-') return false;

    if (arg[1] == L'\0') return false;

    if (arg[1] == L'-' && arg[2] == L'\0') return false;

    if (iswdigit(arg[1]))
    {
        return false;
    }

    return true;
}

NODISCARD static inline Result
parse_int(const wchar_t *raw, const Option *opt, int *target_ref)
{
    (void) opt;
    wchar_t *endptr = NULL;
    int      parsed = (int) wcstol(raw, &endptr, 10);
    if (*endptr == 0)
    {
        *target_ref = parsed;
        return RESULT_SUCCESS;
    }
    set_error(L"Invalid integer value for %ls: %ls", opt->fullname, raw);
    return RESULT_ERROR_PARSE_ARGS;
}

NODISCARD static inline Result
parse_double(const wchar_t *raw, const Option *opt, double *target_ref)
{
    (void) opt;
    wchar_t *endptr = NULL;
    double   parsed = wcstod(raw, &endptr);
    if (*endptr == 0)
    {
        *target_ref = parsed;
        return RESULT_SUCCESS;
    }
    set_error(L"Invalid number value for %ls: %ls", opt->fullname, raw);
    return RESULT_ERROR_PARSE_ARGS;
}

NODISCARD static inline Result
parse_bool(const wchar_t *raw, const Option *opt, bool *target_ref)
{
    (void) opt;
    double value_num = 0;
    if (Result_succeed(parse_double(raw, opt, &value_num)))
    {
        *target_ref = (value_num != 0);
        return RESULT_SUCCESS;
    }

    if (wcs_eq(raw, L"true", true))
    {
        *target_ref = true;
        return RESULT_SUCCESS;
    }
    if (wcs_eq(raw, L"false", true))
    {
        *target_ref = false;
        return RESULT_SUCCESS;
    }

    return RESULT_ERROR_PARSE_ARGS;
}

NODISCARD static inline Result
parse_wcs(const wchar_t *raw, const Option *opt, const wchar_t **target_ref)
{
    if (opt->string_plugin.enums == NULL || opt->string_plugin.enums_count == 0)
    {
        *target_ref = raw;
        return RESULT_SUCCESS;
    }

    for (uint32_t i = 0; i < opt->string_plugin.enums_count; i++)
    {
        if (wcs_eq(raw, opt->string_plugin.enums[i].name,
                   !opt->string_plugin.case_sensitive))
        {
            *target_ref = opt->string_plugin.enums[i].val;
            return RESULT_SUCCESS;
        }
    }

    set_error(L"Invalid value for %ls: %ls", opt->fullname, raw);
    return RESULT_ERROR_PARSE_ARGS;
}

#define parse_raw(arg, opt, target_ref)                                        \
    _Generic((target_ref),                                                     \
        bool *: parse_bool(arg, opt, (bool *) (target_ref)),                   \
        int *: parse_int(arg, opt, (int *) (target_ref)),                      \
        double *: parse_double(arg, opt, (double *) (target_ref)),             \
        wchar_t **: parse_wcs(arg, opt, (const wchar_t **) (target_ref)),      \
        const wchar_t **: parse_wcs(arg, opt,                                  \
                                    (const wchar_t **) (target_ref)),          \
        default: parse_wcs(arg, opt, (const wchar_t **) (target_ref)))

// 参数不大写会有格式化错误
#define FOR_EACH_OPTION_TYPE(PROCESSOR)                                        \
    PROCESSOR(OPTYPE_BOOL, bool)                                               \
    PROCESSOR(OPTYPE_INT, int)                                                 \
    PROCESSOR(OPTYPE_DOUBLE, double)                                           \
    PROCESSOR(OPTYPE_WCS, const wchar_t *)

#define PROCESS_SINGLE_VALUE(type_enum, ValueType)                             \
    case type_enum:                                                            \
    {                                                                          \
        ValueType v;                                                           \
        Result    r = parse_raw(argv[cur_idx], opt, &v);                       \
        if (!Result_succeed(r)) return r;                                      \
        opt->handler((const void *) &v, args);                                 \
        break;                                                                 \
    }

#define PROCESS_ARRAY_VALUES(type_enum, ValueType)                             \
    case type_enum:                                                            \
    {                                                                          \
        ValueType vals[OPTYPE_ARRAY_LEN_MAX];                                  \
        for (uint8_t i = 0; i < count; i++)                                    \
        {                                                                      \
            Result r = parse_raw(raw[i], opt, &vals[i]);                       \
            if (!Result_succeed(r)) return r;                                  \
        }                                                                      \
        opt->array_handler((const void *) vals, count, args);                  \
        break;                                                                 \
    }

static inline Result
process_option(const Option *opt, int argc, wchar_t *const argv[], int *p_idx,
               Args *args)
{
    int cur_idx = *p_idx;

    // 单个布尔类参数可能不尾随值
    if (opt->type == OPTYPE_BOOL && opt->array_plugin.max_len == 0)
    {
        bool is_toggle =
            cur_idx + 1 >= argc || is_option_arg(argv[cur_idx + 1]);
        if (is_toggle)
        {
            opt->handler(&is_toggle, args);
            return RESULT_SUCCESS;
        }
    }

    // 自此可以确定当前不是最后一个参数，下一个参数也不是参数名
    cur_idx++;
    if (opt->array_plugin.max_len == 0)
    {
        if (cur_idx >= argc || is_option_arg(argv[cur_idx]))
        {
            set_error(L"Option %ls requires a value", opt->fullname);
            return RESULT_ERROR_ARG_VALUE_NEEDED;
        }

        switch (opt->type)
        {
            FOR_EACH_OPTION_TYPE(PROCESS_SINGLE_VALUE)
        default:
            return RESULT_ERROR_UNKNOWN_OPTION;
        }

        *p_idx = cur_idx;
        return RESULT_SUCCESS;
    }

    const size_t   max_len = opt->array_plugin.max_len > OPTYPE_ARRAY_LEN_MAX
                                 ? OPTYPE_ARRAY_LEN_MAX
                                 : opt->array_plugin.max_len;
    const wchar_t *raw[OPTYPE_ARRAY_LEN_MAX]; // NO VLA
    uint8_t        count = 0;

    while (cur_idx < argc && !is_option_arg(argv[cur_idx]))
    {
        if (count >= max_len) break;
        raw[count++] = argv[cur_idx++];
    }

    if (opt->array_plugin.force_filling && count != max_len)
    {
        set_error(L"Option %ls requires exactly %zu values, got %u",
                  opt->fullname, max_len, count);
        return RESULT_ERROR_INVALID_ARGS;
    }

    switch (opt->type)
    {
        FOR_EACH_OPTION_TYPE(PROCESS_ARRAY_VALUES)
    default:
        return RESULT_ERROR_UNKNOWN_OPTION;
    }

    *p_idx = cur_idx - 1;
    return RESULT_SUCCESS;
}

static inline void
init_args(Args *args_ref)
{
    args_ref->brush       = BRUSH_ENUMS[0].val;
    args_ref->size[0]     = -1;
    args_ref->size[1]     = -1;
    args_ref->width_scale = 2;
    args_ref->grayscale   = false;
    args_ref->blackwhite  = false;
}

Result
Args_init(Args *self, int argc, wchar_t *const argv[])
{
    if (!argv || !self)
    {
        set_error(L"%hs: NULL parameter", __func__);
        return RESULT_ERROR_INVALID_ARGS;
    }

    init_args(self);

    if (argc < 2)
    {
        set_error(L"Missing arguments");
        return RESULT_ERROR_MISSING_ARGS;
    }

    self->image_path = argv[1];

    for (int i = 2; i < argc; i++)
    {
        const Option *matched_opt = NULL;
        for (uint32_t j = 0; j < OPTS_COUNT; j++)
        {
            if (arg_name_match(argv[i], OPTS[j].fullname, OPTS[j].alias))
            {
                matched_opt = &OPTS[j];
                break;
            }
        }

        if (matched_opt == NULL)
        {
            set_error(L"Unknown option: %ls", argv[i]);
            return RESULT_ERROR_UNKNOWN_OPTION;
        }

        Result r = process_option(matched_opt, argc, argv, &i, self);
        if (!Result_succeed(r)) return r;
    }

    return RESULT_SUCCESS;
}

void
print_args_help(FILE *ostream)
{
    (void) fwprintf(ostream, L"Usage: pixelbrush <image-path> [OPTIONS]\n\n");
    (void) fwprintf(ostream, L"Options:\n");

    size_t max_alias    = 0;
    size_t max_fullname = 0;
    size_t max_hint     = 0;

    wchar_t hint_strs[OPTS_COUNT][64];
    wchar_t alias_strs[OPTS_COUNT][32];

    for (uint32_t i = 0; i < OPTS_COUNT; i++)
    {
        const Option *opt = &OPTS[i];

        if (opt->alias)
        {
            (void) swprintf(alias_strs[i], ARRAY_LEN(alias_strs[i]), L"%ls,",
                            opt->alias);
        }
        else
        {
            alias_strs[i][0] = L'\0';
        }
        size_t alias_len = wcslen(alias_strs[i]);
        if (alias_len > max_alias) max_alias = alias_len;

        size_t full_len = wcslen(opt->fullname);
        if (full_len > max_fullname) max_fullname = full_len;

        if (opt->array_plugin.max_len > 1)
        {
            const wchar_t *type_str =
                (opt->type == OPTYPE_WCS) ? L"string" : L"number";
            if (opt->array_plugin.force_filling)
                (void) swprintf(hint_strs[i], ARRAY_LEN(hint_strs[i]),
                                L"<%ls[%u!]>", type_str,
                                opt->array_plugin.max_len);
            else
                (void) swprintf(hint_strs[i], ARRAY_LEN(hint_strs[i]),
                                L"<%ls[%u?]>", type_str,
                                opt->array_plugin.max_len);
        }
        else if (opt->type == OPTYPE_WCS)
            wcsncpy_s(hint_strs[i], ARRAY_LEN(hint_strs[i]), L"<string>",
                      _TRUNCATE);
        else if (opt->type == OPTYPE_DOUBLE || opt->type == OPTYPE_INT)
            wcsncpy_s(hint_strs[i], ARRAY_LEN(hint_strs[i]), L"<number>",
                      _TRUNCATE);
        else if (opt->type == OPTYPE_BOOL)
            wcsncpy_s(hint_strs[i], ARRAY_LEN(hint_strs[i]), L"<true|false>",
                      _TRUNCATE);
        else hint_strs[i][0] = L'\0';

        size_t hint_len = wcslen(hint_strs[i]);
        if (hint_len > max_hint) max_hint = hint_len;
    }

    for (uint32_t i = 0; i < OPTS_COUNT; i++)
    {
        const Option *opt = &OPTS[i];

        (void) fwprintf(ostream, L"  %-*ls %-*ls %-*ls ", (int) max_alias,
                        alias_strs[i], (int) max_fullname, opt->fullname,
                        (int) max_hint, hint_strs[i]);

        if (opt->desc) (void) fwprintf(ostream, L"%ls", opt->desc);

        if (opt->string_plugin.enums_count > 0)
        {
            (void) fwprintf(ostream, L" (");
            for (uint32_t j = 0; j < opt->string_plugin.enums_count; j++)
            {
                (void) fwprintf(ostream, L"%ls",
                                opt->string_plugin.enums[j].name);
                if (j + 1 < opt->string_plugin.enums_count)
                    (void) fwprintf(ostream, L"|");
            }
            (void) fwprintf(ostream, L")");
        }

        (void) fwprintf(ostream, L"\n");
    }

    (void) fwprintf(ostream, L"\nExamples:\n");
    (void) fwprintf(ostream,
                    L"  pixelbrush image.bmp --brush block --grayscale\n");
    (void) fwprintf(ostream,
                    L"  pixelbrush image.jpg -b symbol --size 400 300\n");
    (void) fwprintf(ostream, L"\n");
}
