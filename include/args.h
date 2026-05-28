#ifndef ARGS_H
#define ARGS_H

#include "base.h"

#include <stdint.h>
#include <wchar.h>

typedef enum
{
    OPTYPE_BOOL,
    OPTYPE_INT,
    OPTYPE_DOUBLE,
    OPTYPE_WCS,
} OptionType;

#define OPTYPE_ARRAY_LEN_MAX 64

typedef struct
{
    const wchar_t *name;
    const wchar_t *val;
} StringEnum;

typedef struct
{
    wchar_t       *image_path;
    const wchar_t *brush;
    int            size[2];
    double         width_scale;
    bool           grayscale;
    bool           blackwhite;
} Args;

#define Args_FMT                                                               \
    "{image_path=%ls, size=[%d, %d], width_scale=%.2f, blackwhite=" Bool_FMT   \
    ", "                                                                       \
    "grayscale=" Bool_FMT "}"
#define Args_ARG(args)                                                         \
    (args).image_path, (args).size[0], (args).size[1], (args).width_scale,     \
        Bool_ARG((args).blackwhite), Bool_ARG(args.grayscale)

NODISCARD Result Args_init(Args *self, int argc, wchar_t *const argv[]);
void             print_args_help(FILE *ostream);

#endif
