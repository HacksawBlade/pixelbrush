// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#ifndef ARGON_H
#define ARGON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define ARGON_VERSION_MAJOR 1
#define ARGON_VERSION_MINOR 0
#define ARGON_VERSION_PATCH 0

// User-defined
#define ARGON_ERRMSG_LEN      256
#define ARGON_OPTNAME_MAX_LEN 64
#define ARGON_ENUM_MAX_LEN    64

typedef uint8_t ArgonFlags;
#define ARGON_COMBINED_MATCHING   (1u << 0)
#define ARGON_CASE_INSENSITIVE    (1u << 1)
#define ARGON_ALLOW_SPECIAL_FLOAT (1u << 2)

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef ARGON_BUILD_SHARED
        #define ARGON_API_ __declspec(dllexport)
    #elif !defined(ARGON_IMPLS)
        #define ARGON_API_ __declspec(dllimport)
    #else
        #define ARGON_API_
    #endif
#elif defined(__GNUC__) && __GNUC__ >= 4 && defined(ARGON_BUILD_SHARED)
    #define ARGON_API_ __attribute__((visibility("default")))
#else
    #define ARGON_API_
#endif

    #define ARGON_PRIVATE_ static inline

    #ifndef __cplusplus
        #define bool _Bool
        #define false 0
        #define true  1
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Argon Result Type
 * - `ARGON_OK`                 success
 * - `ARGON_ERR_UNKNOWN_OPTION` unrecognized option or unexpected positional
 * - `ARGON_ERR_MISSING_VALUE`  option requires a value but none provided
 * - `ARGON_ERR_INVALID_VALUE`  value cannot be parsed for the option type
 * - `ARGON_ERR_UNKNOWN_SUB`    unrecognized subcommand name
 */
typedef enum
{
    ARGON_OK,
    ARGON_ERR_UNKNOWN_OPTION,
    ARGON_ERR_MISSING_VALUE,
    ARGON_ERR_INVALID_VALUE,
    ARGON_ERR_UNKNOWN_SUB,
} ArgonResult;

/**
 * @brief Argon Option Value Type
 * - `ARGON_OPTYPE_BOOL`       bool value
 * - `ARGON_OPTYPE_INT`        int value
 * - `ARGON_OPTYPE_FLOAT`      float value (double)
 * - `ARGON_OPTYPE_STRREF`     string value, returns a `const char *`
 *                             pointer into argv or the matching enum
 *                             string. Enum matching is supported.
 * - `ARGON_OPTYPE_STRREF_MUT` like `STRREF` but returns a mutable
 *                             `char *` pointer into argv. Enum matching
 *                             is not supported.
 */
typedef enum
{
    ARGON_OPTYPE_BOOL,
    ARGON_OPTYPE_INT,
    ARGON_OPTYPE_FLOAT,
    ARGON_OPTYPE_STRREF,
    ARGON_OPTYPE_STRREF_MUT,
} ArgonOptionType;

typedef uint8_t ArgonEnumCnt;
#define ARGON_ENUM_MAX_COUNT 64

/**
 * @brief String-Enum Matching Plugin
 * Attach to an `ARGON_OPTYPE_STRREF` option to restrict values to a known set.
 * If matched, the returned pointer points to the item in `enums` instead of
 * `argv[n]`, which means you can determine the matched enum value by directly
 * comparing the addresses.
 * - `enums`          NULL-terminated array of valid string constants
 * - `case_sensitive` if false, matching ignores case
 */
typedef struct
{
    const char *const*enums;
    bool         case_sensitive;
} ArgonEnumPlugin;

typedef uint8_t ArgonArrLen;
#define ARGON_ARRAY_MAX_LEN 64

/**
 * @brief Array-Value Plugin
 * Collect multiple consecutive values for one option.
 * - `max_len`       maximum number of values (capped at `ARGON_ARRAY_MAX_LEN`)
 * - `force_filling` if true, require exactly `max_len` values or fail
 */
typedef struct
{
    ArgonArrLen max_len;
    bool        force_filling;
} ArgonArrayPlugin;

typedef uint8_t ArgonOptFlags;
#define ARGON_OPTFLAG_WRITTEN_ (1u << 0)

/**
 * @brief Option Definition
 * Describes one command-line option recognized by the parser. If both
 * `fullname` and `alias` are null, treat this option as positional.
 * - `fullname`     [optional] long name (e.g. `"output"` for `--output`), NULL
 *                             for none
 * - `alias`        [optional] single-char short name (e.g. `'o'` for `-o`), 0
 *                             for none
 * - `type`         [required] expected value type
 * - `desc`         [optional] human-readable description for help output
 * - `target`       [required] pointer to user variable that receives the parsed
 *                             value
 * - `enum_plugin`  [optional] enum matching (valid for `STRREF` types)
 * - `array_plugin` [optional] multi-value collection (set `max_len` > 0)
 */
typedef struct
{
    const char     *fullname;
    char            alias;
    ArgonOptionType type;
    const char     *desc;
    void           *target;

    ArgonEnumPlugin  enum_plugin;
    ArgonArrayPlugin array_plugin;

    ArgonOptFlags flags_;
} ArgonOption;

typedef uint8_t ArgonOptCnt;
#define ARGON_OPTION_MAX_COUNT     255
#define ARGON_SUBCOMMAND_MAX_COUNT 64

/**
 * @brief Subcommand Definition
 *
 * Describes one subcommand recognized by the parser.
 * - `name`    subcommand name (e.g. `"build"`)
 * - `alias`   single-char short name (e.g. `'b'`), `0` for none
 * - `desc`    human-readable description for help output
 * - `options` {0}-terminated array of `ArgonOption` definitions.
 *             May point to the same array as the global options for sharing
 */
typedef struct
{
    const char  *name;
    char         alias;
    const char  *desc;
    ArgonOption *options;
} ArgonSub;

/**
 * @brief Parser Context
 *
 * Holds option definitions and parser state.
 * - `options`     [required] {0}-terminated array of global `ArgonOption`
 * - `subcommands` [optional] {0}-terminated array of `ArgonSub`
 * - `flags`       [optional] bitmask of `ArgonFlags`
 * - `errmsg`      [out]      buffer populated with a diagnostic message on
 *                            error
 */
typedef struct
{
    ArgonOption *options;
    ArgonSub    *subs;
    ArgonFlags   flags;
    char         errmsg[ARGON_ERRMSG_LEN];

    ArgonOptCnt opt_cnt_;
    ArgonSub   *active_;
} Argon;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define ARGON_NODISCARD_ [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
    #define ARGON_NODISCARD_ __attribute__((__warn_unused_result__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
    #include <sal.h>
    #define ARGON_NODISCARD_ _Check_return_
#else
    #define ARGON_NODISCARD_
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define ARGON_FORMAT_FN_(fmt_idx, va_idx)                                  \
        __attribute__((format(printf, fmt_idx, va_idx)))
    #define ARGON_FORMAT_STRING_
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
    #define ARGON_FORMAT_FN_(fmt_idx, va_idx)
    #define ARGON_FORMAT_STRING_ _Printf_format_string_
#else
    #define ARGON_FORMAT_FN_(fmt_idx, va_idx)
    #define ARGON_FORMAT_STRING_
#endif

#define ARGON_ARGV(...) ((char *[]) {"", ##__VA_ARGS__, NULL})

/**
 * @brief Parse Command-Line Arguments
 * @param ctx   parser context with global options and optional subcommands
 * @param argv  NULL-terminated argument vector (as from `main`)
 *
 * Walks `argv[1..]` until the NULL sentinel. Global options are matched
 * against `ctx->options`. If `ctx->subs` is set and the first non-option
 * argument matches a subcommand name or alias, subsequent arguments are
 * matched against that subcommand's option set instead.
 *
 * Writes parsed values into the `target` pointers of matching
 * `ArgonOption` definitions. On failure, `ctx->errmsg` is populated
 * with a diagnostic.
 *
 * @return `ARGON_OK` on success, or an error code.
 */
ARGON_API_ ARGON_NODISCARD_ ArgonResult argon_parse(Argon      *ctx,
                                                    char *const argv[]);

/**
 * @brief Print Help Table
 * @param ctx     parser context with global options and optional subcommands
 * @param ostream output stream (e.g. `stdout`)
 *
 * Writes a formatted usage table listing all named global options to
 * `ostream`. Positional options are omitted. If `ctx->subs` is
 * set, lists every subcommand together with its own option table.
 *
 * @return `ARGON_OK` on success, or an error code.
 */
ARGON_API_ ArgonResult argon_print_table(Argon *ctx, FILE *ostream);

/**
 * @brief Check If Option Was Set
 *
 * Returns true if the option's target was written to during parsing (i.e.,
 * the user supplied this option on the command line).
 */
ARGON_API_ ARGON_NODISCARD_ bool argon_option_is_parsed(const ArgonOption *ctx);

/**
 * @brief Get the active subcommand after parsing.
 * @param ctx parser context previously passed to `argon_parse`
 * @return the matched `ArgonSub`, or `NULL` if no subcommand was matched.
 */
ARGON_API_ ARGON_NODISCARD_ const ArgonSub *argon_active_sub(const Argon *ctx);

/**
 * @brief Print help table for a specific subcommand.
 * @param ctx     subcommand whose options should be printed
 * @param ostream output stream (e.g. `stdout`)
 * @return `ARGON_OK` on success, or an error code.
 */
ARGON_API_ ArgonResult argon_print_sub_table(const ArgonSub *ctx,
                                             FILE           *ostream);

#ifdef __cplusplus
}
#endif

#endif // ARGON_H

#ifdef ARGON_IMPLS

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define ARGON_ARRAYLEN_(a) (sizeof(a) / sizeof(*(a)))

#pragma region String utilities

ARGON_NODISCARD_ ARGON_PRIVATE_ bool
argon_streq_(const char *str1, const char *str2, size_t n, bool case_sensitive)
{
    if (str1 == str2) return true;
    if (!str1 || !str2) return false;
    if (case_sensitive) return strncmp(str1, str2, n) == 0;

    size_t i = 0;
    for (; i < n && *str1 != 0 && *str2 != 0; i++, str1++, str2++)
    {
        unsigned char c1 = (unsigned char) *str1;
        unsigned char c2 = (unsigned char) *str2;
        if (c1 >= 'A' && c1 <= 'Z') c1 = (unsigned char) (c1 + ('a' - 'A'));
        if (c2 >= 'A' && c2 <= 'Z') c2 = (unsigned char) (c2 + ('a' - 'A'));
        if (c1 != c2) return false;
    }
    return i == n || *str1 == *str2;
}

#define argon_strrepl_tail_(buf, el_cnt, arr_tail)                             \
    do                                                                         \
    {                                                                          \
        if ((el_cnt) > 1)                                                      \
        {                                                                      \
            const size_t tail_el_cnt_ = ARGON_ARRAYLEN_(arr_tail);             \
            const size_t write_cnt_ =                                          \
                tail_el_cnt_ > (el_cnt) ? (el_cnt) - 1 : tail_el_cnt_ - 1;     \
            memcpy((buf) + (el_cnt) - write_cnt_ - 1, (arr_tail),              \
                   write_cnt_ * sizeof(*(buf)));                               \
            (buf)[(el_cnt) - 1] = 0;                                           \
        }                                                                      \
    } while (0)

ARGON_PRIVATE_ void
argon_strlower_(char *target, const char *src, size_t n)
{
    size_t i = 0;
    for (; i < n - 1 && src[i]; i++)
    {
        unsigned char c = (unsigned char) src[i];
        target[i] =
            (c >= 'A' && c <= 'Z') ? (char) (c + ('a' - 'A')) : (char) c;
    }
    target[i] = 0;
}

ARGON_PRIVATE_ size_t
argon_strlen_(const char *s, size_t maxlen)
{
    size_t i = 0;
    while (i < maxlen && s[i])
        i++;
    return i;
}

#pragma endregion

ARGON_FORMAT_FN_(2, 3)
ARGON_PRIVATE_ void
argon_set_errmsg_(char *buf, ARGON_FORMAT_STRING_ const char *fmt, ...)
{
    va_list args = {0};
    va_start(args, fmt);
    int n = vsnprintf(buf, ARGON_ERRMSG_LEN, fmt, args);
    va_end(args);
    if (n > 0 && (size_t) n >= ARGON_ERRMSG_LEN)
        argon_strrepl_tail_(buf, ARGON_ERRMSG_LEN, "...");
}

#pragma region Classifiers

ARGON_NODISCARD_ ARGON_PRIVATE_ bool
argon_is_option_name_(Argon *ctx, const char *raw)
{
    if (!raw || raw[0] != '-') return false;
    if (raw[1] == 0) return false;
    if (raw[1] == '-' && raw[2] == 0) return false;
    if (isdigit((unsigned char) raw[1])) return false;
    if ((ctx->flags & ARGON_ALLOW_SPECIAL_FLOAT) && raw[2] != 0)
    {
        int saved_errno = errno;
        errno           = 0;
        char *endptr    = NULL;
        (void) strtod(raw, &endptr);
        bool is_float = (endptr != raw && *endptr == '\0' && errno != ERANGE);
        errno         = saved_errno;
        if (is_float) return false;
    }
    return true;
}

#define argon_option_is_pos_(opt)      (!(opt)->fullname && !(opt)->alias)
#define argon_option_is_sentinel_(opt) (!(opt)->target)

#pragma endregion

#pragma region Parser

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_parse_int_(Argon *ctx, const char *raw, const ArgonOption *opt,
                 int *target)
{
    (void) ctx, (void) opt;
    char *endptr = NULL;
    errno        = 0;
    long parsed  = strtol(raw, &endptr, 10);
    if (*endptr != 0 || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX)
        return ARGON_ERR_INVALID_VALUE;
    if (target) *target = (int) parsed;
    return ARGON_OK;
}

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_parse_float_(Argon *ctx, const char *raw, const ArgonOption *opt,
                   double *target)
{
    (void) opt;
    char *endptr  = NULL;
    errno         = 0;
    double parsed = strtod(raw, &endptr);
    if (*endptr != 0 || errno == ERANGE) return ARGON_ERR_INVALID_VALUE;
    if (!isfinite(parsed))
    {
        if (!(ctx->flags & ARGON_ALLOW_SPECIAL_FLOAT))
        {
            argon_set_errmsg_(ctx->errmsg,
                              "inf/nan are not allowed. "
                              "Enable ARGON_ALLOW_SPECIAL_FLOAT if required.");
            return ARGON_ERR_INVALID_VALUE;
        }
    }
    if (target) *target = parsed;
    return ARGON_OK;
}

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_parse_bool_(Argon *ctx, const char *raw, const ArgonOption *opt,
                  bool *target)
{
    (void) ctx, (void) opt;
    if (raw[0] == '0' && raw[1] == 0)
    {
        *target = false;
        return ARGON_OK;
    }
    if (raw[0] == '1' && raw[1] == 0)
    {
        *target = true;
        return ARGON_OK;
    }

    if (argon_streq_(raw, "true", 5, false))
    {
        if (target) *target = true;
        return ARGON_OK;
    }
    if (argon_streq_(raw, "false", 6, false))
    {
        if (target) *target = false;
        return ARGON_OK;
    }

    return ARGON_ERR_INVALID_VALUE;
}

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_parse_str_(Argon *ctx, const char *raw, const ArgonOption *opt,
                 const char **target)
{
    (void) ctx;
    if (!opt->enum_plugin.enums || !opt->enum_plugin.enums[0])
    {
        if (target) *target = raw;
        return ARGON_OK;
    }

    for (ArgonEnumCnt i = 0;
         i < ARGON_ENUM_MAX_COUNT && opt->enum_plugin.enums[i]; i++)
    {
        if (argon_streq_(raw, opt->enum_plugin.enums[i], ARGON_ENUM_MAX_LEN,
                         opt->enum_plugin.case_sensitive))
        {
            if (target) *target = opt->enum_plugin.enums[i];
            return ARGON_OK;
        }
    }

    return ARGON_ERR_INVALID_VALUE;
}

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_parse_str_mut_(Argon *ctx, char *raw, const ArgonOption *opt,
                     char **target_ref)
{
    (void) ctx, (void) opt;
    if (target_ref) *target_ref = raw;
    return ARGON_OK;
}

#define argon_parse_raw_(ctx, raw, opt, target_ref)                            \
    _Generic((target_ref),                                                     \
        bool *: argon_parse_bool_(ctx, raw, opt, (bool *) (target_ref)),       \
        int *: argon_parse_int_(ctx, raw, opt, (int *) (target_ref)),          \
        double *: argon_parse_float_(ctx, raw, opt, (double *) (target_ref)),  \
        char **: argon_parse_str_mut_(ctx, raw, opt, (char **) (target_ref)),  \
        const char **: argon_parse_str_(ctx, raw, opt,                         \
                                        (const char **) (target_ref)),         \
        default: argon_parse_str_(ctx, raw, opt,                               \
                                  (const char **) (target_ref)))

#define ARGON_FOR_EACH_OPTION_TYPE_(PROCESSOR)                                 \
    PROCESSOR(ARGON_OPTYPE_BOOL, bool)                                         \
    PROCESSOR(ARGON_OPTYPE_INT, int)                                           \
    PROCESSOR(ARGON_OPTYPE_FLOAT, double)                                      \
    PROCESSOR(ARGON_OPTYPE_STRREF, const char *)                               \
    PROCESSOR(ARGON_OPTYPE_STRREF_MUT, char *)

#define argon_write_target_(opt, Type, val)                                    \
    do                                                                         \
    {                                                                          \
        *(Type *) (opt)->target = (val);                                       \
        (opt)->flags_ |= ARGON_OPTFLAG_WRITTEN_;                               \
    } while (0)

#define ARGON_PROCESS_SINGLE_VALUE_(type_enum, ValueType)                      \
case type_enum:                                                                \
{                                                                              \
    ValueType   val = {0};                                                     \
    ArgonResult r   = argon_parse_raw_(ctx, argv[cur_idx], opt, &val);         \
    if (r != ARGON_OK)                                                         \
    {                                                                          \
        if (ctx->errmsg[0] == '\0')                                            \
            argon_set_errmsg_(ctx->errmsg,                                     \
                              "invalid value '%s' for option '%s'",            \
                              argv[cur_idx], entered_as);                      \
        return r;                                                              \
    }                                                                          \
    argon_write_target_(opt, ValueType, val);                                  \
    break;                                                                     \
}

#define ARGON_PROCESS_ARRAY_VALUES_(type_enum, ValueType)                      \
case type_enum:                                                                \
{                                                                              \
    ArgonArrLen arr_len = 0;                                                   \
    while (argv[cur_idx] && !argon_is_option_name_(ctx, argv[cur_idx]))        \
    {                                                                          \
        if (arr_len >= max_len) break;                                         \
        ValueType   val = {0};                                                 \
        ArgonResult r   = argon_parse_raw_(ctx, argv[cur_idx], opt, &val);     \
        if (r != ARGON_OK)                                                     \
        {                                                                      \
            if (ctx->errmsg[0] == '\0')                                        \
                argon_set_errmsg_(ctx->errmsg,                                 \
                                  "invalid value '%s' for option '%s'",        \
                                  argv[cur_idx], entered_as);                  \
            if (opt->array_plugin.force_filling) return r;                     \
            break;                                                             \
        }                                                                      \
        ((ValueType *) opt->target)[(arr_len)] = val;                          \
        arr_len++;                                                             \
        cur_idx++;                                                             \
    }                                                                          \
    if (opt->array_plugin.force_filling && arr_len != max_len)                 \
    {                                                                          \
        argon_set_errmsg_(ctx->errmsg,                                         \
                          "option '%s' requires exactly %zu value(s), got %u", \
                          entered_as, max_len, arr_len);                       \
        return ARGON_ERR_INVALID_VALUE;                                        \
    }                                                                          \
    if (arr_len > 0) (opt)->flags_ |= ARGON_OPTFLAG_WRITTEN_;                  \
    *p_idx = cur_idx - 1;                                                      \
    break;                                                                     \
}

#pragma endregion

#pragma region Option processing

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_write_option_value_(Argon *ctx, ArgonOption *opt, char *const argv[],
                          int cur_idx, const char *entered_as)
{
    switch (opt->type)
    {
        ARGON_FOR_EACH_OPTION_TYPE_(ARGON_PROCESS_SINGLE_VALUE_)
    default:
        return ARGON_ERR_UNKNOWN_OPTION;
    }
    return ARGON_OK;
}

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_process_option_(Argon *ctx, ArgonOption *opt, char *const argv[],
                      int *p_idx)
{
    const char *entered_as = argv[*p_idx];
    int         cur_idx    = *p_idx;

    if (opt->type == ARGON_OPTYPE_BOOL && opt->array_plugin.max_len == 0)
    {
        if (!argv[cur_idx + 1] || argon_is_option_name_(ctx, argv[cur_idx + 1]))
        {
            argon_write_target_(opt, bool, true);
            return ARGON_OK;
        }

        bool val = false;
        if (argon_parse_bool_(ctx, argv[cur_idx + 1], opt, &val) == ARGON_OK)
        {
            argon_write_target_(opt, bool, val);
            *p_idx = cur_idx + 1;
            return ARGON_OK;
        }

        argon_write_target_(opt, bool, true);
        return ARGON_OK;
    }

    cur_idx++;
    if (opt->array_plugin.max_len == 0)
    {
        if (!argv[cur_idx] || argon_is_option_name_(ctx, argv[cur_idx]))
        {
            argon_set_errmsg_(ctx->errmsg, "missing value for option '%s'",
                              entered_as);
            return ARGON_ERR_MISSING_VALUE;
        }

        ArgonResult r =
            argon_write_option_value_(ctx, opt, argv, cur_idx, entered_as);
        if (r == ARGON_OK) *p_idx = cur_idx;
        return r;
    }

    const size_t max_len = opt->array_plugin.max_len > ARGON_ARRAY_MAX_LEN
                               ? ARGON_ARRAY_MAX_LEN
                               : opt->array_plugin.max_len;
    switch (opt->type)
    {
        ARGON_FOR_EACH_OPTION_TYPE_(ARGON_PROCESS_ARRAY_VALUES_)
    default:
        return ARGON_ERR_UNKNOWN_OPTION;
    }
    return ARGON_OK;
}

ARGON_NODISCARD_ ARGON_PRIVATE_ ArgonResult
argon_process_combined_short_(Argon *ctx, char *const argv[], int *p_idx,
                              const ArgonOptCnt *short_map,
                              ArgonOption *options, ArgonOptCnt opt_cnt)
{
    const char *names = argv[*p_idx] + 1;
    size_t      len   = argon_strlen_(names, 52);

    for (size_t k = 0; k < len; k++)
    {
        unsigned char c   = (unsigned char) names[k];
        ArgonOptCnt   idx = short_map[c];

        if (idx >= opt_cnt)
        {
            argon_set_errmsg_(ctx->errmsg, "unknown option '-%c'", names[k]);
            return ARGON_ERR_UNKNOWN_OPTION;
        }

        ArgonOption *matched_opt = &options[idx];
        bool         is_last     = (k == len - 1);
        bool         is_flag     = (matched_opt->type == ARGON_OPTYPE_BOOL &&
                                    matched_opt->array_plugin.max_len == 0);

        if (is_flag)
        {
            argon_write_target_(matched_opt, bool, true);
        }
        else
        {
            if (!is_last)
            {
                argon_set_errmsg_(
                    ctx->errmsg,
                    "option '-%c' in combined group must be a boolean flag",
                    names[k]);
                return ARGON_ERR_INVALID_VALUE;
            }
            ArgonResult r =
                argon_process_option_(ctx, matched_opt, argv, p_idx);
            if (r != ARGON_OK) return r;
        }
    }

    return ARGON_OK;
}

ARGON_NODISCARD_ ARGON_PRIVATE_ const ArgonSub *
argon_match_subcommand_(const ArgonSub *subs, const char *name, bool ci)
{
    for (ArgonOptCnt i = 0; i < ARGON_SUBCOMMAND_MAX_COUNT; i++)
    {
        const ArgonSub *sub = &subs[i];
        if (!sub->name) break;
        if (argon_streq_(name, sub->name, ARGON_OPTNAME_MAX_LEN, !ci))
            return sub;
        if (sub->alias != 0 && name[0] != '\0' && name[1] == '\0')
        {
            unsigned char nc = (unsigned char) name[0];
            unsigned char ac = (unsigned char) sub->alias;
            if (ci)
            {
                if (nc >= 'A' && nc <= 'Z') nc += ('a' - 'A');
                if (ac >= 'A' && ac <= 'Z') ac += ('a' - 'A');
            }
            if (nc == ac) return sub;
        }
    }
    return NULL;
}

#pragma endregion

ARGON_API_ ARGON_NODISCARD_ ArgonResult
argon_parse(Argon *ctx, char *const argv[])
{
    if (!ctx || !argv) return ARGON_ERR_INVALID_VALUE;
    if (!argv[0]) return ARGON_OK;

    ArgonOptCnt short_map[256];
    ArgonOptCnt sub_short_map[256];
    ArgonOptCnt pos_indices[ARGON_OPTION_MAX_COUNT];
    ArgonOptCnt sub_pos_indices[ARGON_OPTION_MAX_COUNT];
    ArgonOptCnt pos_count     = 0;
    ArgonOptCnt sub_pos_count = 0;
    const bool  ci            = ctx->flags & ARGON_CASE_INSENSITIVE;
    ctx->errmsg[0]            = 0;

    if (ctx->opt_cnt_ == 0)
    {
        for (int i = 0; i < 256; i++)
            short_map[i] = ARGON_OPTION_MAX_COUNT;

        ArgonOptCnt n = 0;
        for (; n < ARGON_OPTION_MAX_COUNT &&
               !argon_option_is_sentinel_(&ctx->options[n]);
             n++)
        {
            ArgonOption *opt = &ctx->options[n];
            if (opt->alias != 0)
            {
                short_map[(unsigned char) opt->alias] = n;
                if (ci)
                {
                    unsigned char c = (unsigned char) opt->alias;
                    if (c >= 'A' && c <= 'Z') short_map[c + ('a' - 'A')] = n;
                    else if (c >= 'a' && c <= 'z')
                        short_map[c - ('a' - 'A')] = n;
                }
            }

            if (argon_option_is_pos_(opt))
            {
                if (opt->array_plugin.max_len > 0)
                {
                    argon_set_errmsg_(ctx->errmsg,
                                      "positional option %u does not "
                                      "support array",
                                      (unsigned) n);
                    return ARGON_ERR_INVALID_VALUE;
                }
                pos_indices[pos_count++] = n;
            }
        }

        ctx->opt_cnt_ = n;
    }
    else
    {
        for (int i = 0; i < 256; i++)
            short_map[i] = ctx->opt_cnt_;

        for (ArgonOptCnt i = 0; i < ctx->opt_cnt_; i++)
        {
            ArgonOption *opt = &ctx->options[i];

            if (opt->alias != 0)
            {
                short_map[(unsigned char) opt->alias] = i;
                if (ci)
                {
                    unsigned char c = (unsigned char) opt->alias;
                    if (c >= 'A' && c <= 'Z') short_map[c + ('a' - 'A')] = i;
                    else if (c >= 'a' && c <= 'z')
                        short_map[c - ('a' - 'A')] = i;
                }
            }

            if (argon_option_is_pos_(opt)) pos_indices[pos_count++] = i;
        }
    }

    if (ctx->opt_cnt_ == 0 && !ctx->subs) return ARGON_OK;

    ArgonOptCnt *p_short_map = short_map, *p_pos_indices = pos_indices,
                p_opt_cnt = ctx->opt_cnt_, p_pos_count = pos_count;
    ArgonOption *p_options = ctx->options;

    ctx->active_ = NULL;

    ArgonResult result     = ARGON_OK;
    ArgonOptCnt pos_filled = 0;
    bool        pos_only   = false;
    bool        in_sub     = false;

    for (int i = 1; argv[i]; i++)
    {
        if (!pos_only && argv[i][0] == '-' && argv[i][1] == '-' &&
            argv[i][2] == 0)
        {
            if (p_pos_count > 0) pos_only = true;
            else break;
            continue;
        }

        if (pos_only || !argon_is_option_name_(ctx, argv[i]))
        {
            if (!in_sub && ctx->subs)
            {
                const ArgonSub *matched =
                    argon_match_subcommand_(ctx->subs, argv[i], ci);
                if (matched)
                {
                    ctx->active_ = (ArgonSub *) matched;
                    in_sub       = true;

                    for (int s = 0; s < 256; s++)
                        sub_short_map[s] = ARGON_OPTION_MAX_COUNT;
                    sub_pos_count = 0;

                    ArgonOptCnt sn = 0;
                    for (; sn < ARGON_OPTION_MAX_COUNT &&
                           !argon_option_is_sentinel_(&matched->options[sn]);
                         sn++)
                    {
                        ArgonOption *opt = &matched->options[sn];

                        if (opt->alias != 0)
                        {
                            sub_short_map[(unsigned char) opt->alias] = sn;
                            if (ci)
                            {
                                unsigned char c = (unsigned char) opt->alias;
                                if (c >= 'A' && c <= 'Z')
                                    sub_short_map[c + ('a' - 'A')] = sn;
                                else if (c >= 'a' && c <= 'z')
                                    sub_short_map[c - ('a' - 'A')] = sn;
                            }
                        }

                        if (argon_option_is_pos_(opt))
                            sub_pos_indices[sub_pos_count++] = sn;
                    }

                    p_short_map   = sub_short_map;
                    p_options     = matched->options;
                    p_opt_cnt     = sn;
                    p_pos_indices = sub_pos_indices;
                    p_pos_count   = sub_pos_count;
                    pos_filled    = 0;
                    continue;
                }

                argon_set_errmsg_(ctx->errmsg, "unknown subcommand '%s'",
                                  argv[i]);
                return ARGON_ERR_UNKNOWN_SUB;
            }

            if (pos_filled >= p_pos_count)
            {
                argon_set_errmsg_(ctx->errmsg,
                                  "unexpected positional argument '%s'",
                                  argv[i]);
                return ARGON_ERR_UNKNOWN_OPTION;
            }

            ArgonOption *pos_opt = &p_options[p_pos_indices[pos_filled]];
            char         pos_name[24];
            (void) snprintf(pos_name, sizeof(pos_name), "<pos[%u]>",
                            (unsigned) pos_filled);
            result = argon_write_option_value_(ctx, pos_opt, argv, i, pos_name);
            if (result != ARGON_OK)
            {
                argon_set_errmsg_(ctx->errmsg,
                                  "invalid value '%s' for positional argument",
                                  argv[i]);
                return result;
            }
            pos_filled++;
            continue;
        }

        if ((ctx->flags & ARGON_COMBINED_MATCHING) && argv[i][0] == '-' &&
            argv[i][1] != '-' && argv[i][2] != 0)
        {
            result = argon_process_combined_short_(ctx, argv, &i, p_short_map,
                                                   p_options, p_opt_cnt);
            if (result != ARGON_OK) return result;
            continue;
        }

        ArgonOption *matched_opt = NULL;

        if (argv[i][0] == '-' && argv[i][1] != '-' && argv[i][2] == 0)
        {
            unsigned char c   = (unsigned char) argv[i][1];
            ArgonOptCnt   idx = p_short_map[c];
            if (idx < p_opt_cnt) matched_opt = &p_options[idx];
        }
        else if (argv[i][0] == '-' && argv[i][1] == '-')
        {
            const char *key = argv[i] + 2;
            char        lkey[ARGON_OPTNAME_MAX_LEN];
            if (ci)
            {
                argon_strlower_(lkey, key, ARGON_OPTNAME_MAX_LEN);
                key = lkey;
            }

            for (ArgonOptCnt j = 0; j < ARGON_OPTION_MAX_COUNT &&
                                    !argon_option_is_sentinel_(&p_options[j]);
                 j++)
            {
                ArgonOption *opt = &p_options[j];
                if (!opt->fullname) continue;
                if (argon_streq_(opt->fullname, key, ARGON_OPTNAME_MAX_LEN,
                                 !ci))
                {
                    matched_opt = opt;
                    break;
                }
            }
        }

        if (!matched_opt)
        {
            argon_set_errmsg_(ctx->errmsg, "unknown option '%s'", argv[i]);
            return ARGON_ERR_UNKNOWN_OPTION;
        }

        result = argon_process_option_(ctx, matched_opt, argv, &i);
        if (result != ARGON_OK) return result;
    }

    return ARGON_OK;
}

#define argon_size_to_int_(x) (x) > (size_t) INT_MAX ? INT_MAX : (int) x;

ARGON_PRIVATE_ void
argon_print_options_(ArgonOption *options, FILE *ostream, const char *prefix)
{
    static const char TYPE_STRING[] = "<string>", TYPE_NUMBER[] = "<number>",
                      TYPE_BOOL[] = "<0|1>";
    size_t            max_alias = 0, max_full = 0, max_type = 0;

    for (ArgonOptCnt i = 0;
         i < ARGON_OPTION_MAX_COUNT && !argon_option_is_sentinel_(&options[i]);
         i++)
    {
        const ArgonOption *opt = &options[i];
        if (argon_option_is_pos_(opt)) continue;

        if (opt->alias != 0 && max_alias < 3) max_alias = 3;

        size_t full_len = argon_strlen_(opt->fullname, ARGON_OPTNAME_MAX_LEN);
        if (full_len > max_full) max_full = full_len;

        size_t type_len = 0;
        if (opt->array_plugin.max_len > 1)
        {
            const char *base = (opt->type == ARGON_OPTYPE_STRREF ||
                                opt->type == ARGON_OPTYPE_STRREF_MUT)
                                   ? "string"
                                   : "number";
            char        tmp[64];
            int written = snprintf(tmp, sizeof(tmp), "<%s[%u%c]>", base,
                                   opt->array_plugin.max_len,
                                   opt->array_plugin.force_filling ? '!' : '?');
            if (written > 0) type_len = (size_t) written;
        }
        else if (opt->type == ARGON_OPTYPE_BOOL)
            type_len = sizeof(TYPE_BOOL) - 1;
        else if (opt->type == ARGON_OPTYPE_INT ||
                 opt->type == ARGON_OPTYPE_FLOAT)
            type_len = sizeof(TYPE_NUMBER) - 1;
        else if (opt->type == ARGON_OPTYPE_STRREF ||
                 opt->type == ARGON_OPTYPE_STRREF_MUT)
            type_len = sizeof(TYPE_STRING) - 1;

        if (type_len > max_type) max_type = type_len;
    }

    int alias_w    = argon_size_to_int_(max_alias);
    int fullname_w = argon_size_to_int_(max_full);
    int type_w     = argon_size_to_int_(max_type);

    for (ArgonOptCnt i = 0;
         i < ARGON_OPTION_MAX_COUNT && !argon_option_is_sentinel_(&options[i]);
         i++)
    {
        const ArgonOption *opt = &options[i];
        if (argon_option_is_pos_(opt)) continue;

        char alias_buf[16] = {0};
        if (opt->alias != 0)
            (void) snprintf(alias_buf, sizeof(alias_buf), "-%c,", opt->alias);

        size_t full_len  = argon_strlen_(opt->fullname, ARGON_OPTNAME_MAX_LEN);
        int    full_prec = argon_size_to_int_(full_len);

        char        type_buf[64] = {0};
        const char *typstr       = type_buf;
        if (opt->array_plugin.max_len > 1)
        {
            const char *base = (opt->type == ARGON_OPTYPE_STRREF ||
                                opt->type == ARGON_OPTYPE_STRREF_MUT)
                                   ? "string"
                                   : "number";
            (void) snprintf(type_buf, sizeof(type_buf), "<%s[%u%c]>", base,
                            opt->array_plugin.max_len,
                            opt->array_plugin.force_filling ? '!' : '?');
        }
        else if (opt->type == ARGON_OPTYPE_BOOL) typstr = TYPE_BOOL;
        else if (opt->type == ARGON_OPTYPE_STRREF ||
                 opt->type == ARGON_OPTYPE_STRREF_MUT)
            typstr = TYPE_STRING;
        else if (opt->type == ARGON_OPTYPE_FLOAT ||
                 opt->type == ARGON_OPTYPE_INT)
            typstr = TYPE_NUMBER;

        (void) fprintf(ostream, "%s  %-*s --%-*.*s %-*s ", prefix, alias_w,
                       alias_buf, fullname_w, full_prec, opt->fullname, type_w,
                       typstr);

        if (opt->desc) (void) fprintf(ostream, "%s", opt->desc);

        if (opt->enum_plugin.enums && opt->enum_plugin.enums[0])
        {
            (void) fprintf(ostream, " (");
            for (ArgonEnumCnt j = 0;
                 j < ARGON_ENUM_MAX_COUNT && opt->enum_plugin.enums[j]; j++)
            {
                (void) fprintf(ostream, "%s", opt->enum_plugin.enums[j]);
                if (opt->enum_plugin.enums[j + 1]) (void) fprintf(ostream, "|");
            }
            (void) fprintf(ostream, ")");
        }

        (void) fprintf(ostream, "\n");
    }
}

ARGON_API_ ArgonResult
argon_print_table(Argon *ctx, FILE *ostream)
{
    if (!ctx || !ostream) return ARGON_ERR_INVALID_VALUE;

    (void) fprintf(ostream, "Options:\n");
    argon_print_options_(ctx->options, ostream, "");

    if (ctx->subs && ctx->subs[0].name)
    {
        size_t max_name  = 0;
        bool   any_alias = false;
        for (ArgonOptCnt i = 0; i < ARGON_SUBCOMMAND_MAX_COUNT; i++)
        {
            const ArgonSub *sub = &ctx->subs[i];
            if (!sub->name) break;
            size_t nl = argon_strlen_(sub->name, ARGON_OPTNAME_MAX_LEN);
            if (nl > max_name) max_name = nl;
            if (sub->alias != 0) any_alias = true;
        }

        if (max_name > 0)
        {
            (void) fprintf(ostream, "\nSubcommands:\n");
            int name_w = argon_size_to_int_(max_name);
            for (ArgonOptCnt i = 0; i < ARGON_SUBCOMMAND_MAX_COUNT; i++)
            {
                const ArgonSub *sub = &ctx->subs[i];
                if (!sub->name) break;
                char alias_buf[6] = {0};
                if (sub->alias != 0)
                    (void) snprintf(alias_buf, sizeof(alias_buf), "%c",
                                    sub->alias);
                (void) fprintf(ostream, "  %-*s ", name_w, sub->name);
                if (any_alias) (void) fprintf(ostream, "%-3s ", alias_buf);
                if (sub->desc) (void) fprintf(ostream, "%s", sub->desc);
                (void) fprintf(ostream, "\n");
                if (sub->options)
                {
                    argon_print_options_(sub->options, ostream, "  ");
                    (void) fprintf(ostream, "\n");
                }
            }
        }
    }

    return ARGON_OK;
}

ARGON_API_ ArgonResult
argon_print_sub_table(const ArgonSub *ctx, FILE *ostream)
{
    if (!ctx || !ostream || !ctx->options) return ARGON_ERR_INVALID_VALUE;

    (void) fprintf(ostream, "Options:\n");
    argon_print_options_(ctx->options, ostream, "");

    return ARGON_OK;
}

ARGON_API_ ARGON_NODISCARD_ bool
argon_option_is_parsed(const ArgonOption *ctx)
{
    return ctx->flags_ & ARGON_OPTFLAG_WRITTEN_;
}

ARGON_API_ ARGON_NODISCARD_ const ArgonSub *
argon_active_sub(const Argon *ctx)
{
    if (!ctx) return NULL;
    return ctx->active_;
}

#endif // ARGON_IMPLS
