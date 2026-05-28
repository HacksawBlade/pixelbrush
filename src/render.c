#include "render.h"

#include "base.h"
#include "image.h"

#include <Windows.h>

//! 不使用 CrLf 可能会有换行问题
#define SEQ_NLINE L"\r\n"
#define SEQ_RESET L"\x1b[0m" SEQ_NLINE

Result
render_ascii_art(const RenderAsciiArtOpts *opts)
{
    if (!opts || !opts->image || !opts->brush || opts->brush_len == 0 ||
        opts->image_width <= 0 || opts->image_height <= 0)
    {
        set_error(L"%hs: invalid parameters "
                  "(opts=%p, image=%p, brush=%p, brush_len=%zu, "
                  "width=%d, height=%d)",
                  __func__, (const void *) opts,
                  (const void *) (opts ? opts->image : NULL),
                  (const void *) (opts ? opts->brush : NULL),
                  opts ? opts->brush_len : 0, opts ? opts->image_width : 0,
                  opts ? opts->image_height : 0);
        return RESULT_ERROR_INVALID_ARGS;
    }

    const bool to_console = GetFileType(opts->target) == FILE_TYPE_CHAR;

    const wchar_t *tail     = opts->blackwhite ? SEQ_NLINE : SEQ_RESET;
    const size_t   tail_len = opts->blackwhite
                                  ? (sizeof(SEQ_NLINE) / sizeof(wchar_t)) - 1
                                  : (sizeof(SEQ_RESET) / sizeof(wchar_t)) - 1;

    size_t   max_line_chars = ((size_t) opts->image_width * 20) + tail_len + 8;
    uint8_t *pixel_buf  = malloc((size_t) opts->image_width * IMAGE_PIXEL_BYTE);
    wchar_t *render_buf = malloc(max_line_chars * sizeof(wchar_t));
    if (!pixel_buf || !render_buf)
    {
        set_error(L"Out of memory allocating render buffers "
                  "(width=%d, height=%d, brush_len=%zu)",
                  opts->image_width, opts->image_height, opts->brush_len);
        free(pixel_buf);
        free(render_buf);
        return RESULT_ERROR_OUT_OF_MEMORY;
    }

    if (!to_console)
    {
        static const uint16_t BOM_UTF16 = 0xFEFF;
        WriteFile(opts->target, &BOM_UTF16, sizeof(BOM_UTF16), NULL, NULL);
    }

    for (int y = 0; y < opts->image_height; y++)
    {
        Image_export_line(opts->image, (UINT) y,
                          (size_t) opts->image_width * IMAGE_PIXEL_BYTE,
                          pixel_buf);

        size_t pos = 0;
        for (int x = 0; x < opts->image_width; x++)
        {
            size_t  idx = (size_t) x * IMAGE_PIXEL_BYTE;
            uint8_t b   = pixel_buf[idx];
            uint8_t g   = pixel_buf[idx + 1];
            uint8_t r   = pixel_buf[idx + 2];
            double  lum = ((0.2126 * r) + (0.7152 * g) + (0.0722 * b)) / 255.0;
            // double lum = ((r * 6966) + (g * 23436) + (b * 2366)) >> 15;
            size_t ci = (size_t) (lum * (double) (opts->brush_len - 1));
            if (ci >= opts->brush_len) ci = opts->brush_len - 1;
            wchar_t c = opts->brush[ci];

            int color_len = 0;
            if (!opts->blackwhite)
            {
                if (opts->grayscale)
                {
                    int gray_code = 232 + (int) (lum * 23);
                    if (gray_code > 255) gray_code = 255;
                    color_len = swprintf(render_buf + pos, max_line_chars - pos,
                                         L"\x1b[38;5;%dm", gray_code);
                }
                else
                {
                    color_len = swprintf(render_buf + pos, max_line_chars - pos,
                                         L"\x1b[38;2;%u;%u;%um", r, g, b);
                }
            }

            if (color_len > 0 && (size_t) color_len < max_line_chars - pos)
                pos += (size_t) color_len;

            if (pos < max_line_chars - 1) render_buf[pos++] = c;
        }

        if (pos + tail_len < max_line_chars)
        {
            memcpy(render_buf + pos, tail, tail_len * sizeof(wchar_t));
            pos += tail_len;
        }

        if (to_console)
        {
            WriteConsoleW(opts->target, render_buf, (DWORD) pos, NULL, NULL);
        }
        else
        {
            WriteFile(opts->target, render_buf, (DWORD) (pos * sizeof(wchar_t)),
                      NULL, NULL);
        }
    }

    free(pixel_buf);
    free(render_buf);
    return RESULT_SUCCESS;
}
