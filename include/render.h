#ifndef RENDER_H
#define RENDER_H

#include "base.h"
#include "image.h"

#include <Windows.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

typedef struct
{
    HANDLE         target;
    Image         *image;
    int            image_width;
    int            image_height;
    const wchar_t *brush;
    size_t         brush_len;
    bool           grayscale;
    bool           blackwhite;
} RenderAsciiArtOpts;

NODISCARD Result render_ascii_art(const RenderAsciiArtOpts *opts);

#endif
