#include "args.h"
#include "base.h"
#include "console.h"
#include "image.h"
#include "render.h"

#include <stdio.h>

#define print_error() (void) fprintf(stderr, ERROR_PREFIX " %ls\n", g_errmsg)

int
wmain(int argc, wchar_t *argv[])
{
    Args   args   = {};
    Result result = Args_init(&args, argc, argv);
    if (argc < 2 || !Result_succeed(result))
    {
        print_args_help(stderr);
        print_error();
        return EXIT_FAILURE;
    }

    HANDLE     handle     = GetStdHandle(STD_OUTPUT_HANDLE);
    const bool to_console = GetFileType(handle) == FILE_TYPE_CHAR;

    int term_cols = CONSOLE_DEFAULT_COLS;
    int term_rows = CONSOLE_DEFAULT_ROWS;
    if (to_console)
    {
        result = Console_init(&handle);
        if (!Result_succeed(result))
        {
            print_error();
            if (result == RESULT_ERROR_CANT_GET_HANDLE) return EXIT_FAILURE;
        }
        else
        {
            result = Console_get_size(&handle, &term_cols, &term_rows);
            if (!Result_succeed(result))
            {
                print_error();
                if (result == RESULT_ERROR_CANT_GET_HANDLE) return EXIT_FAILURE;
                term_cols = CONSOLE_DEFAULT_COLS;
                term_rows = CONSOLE_DEFAULT_ROWS;
            }
        }
    }

    ImageFactory fac = {};
    result           = ImageFactory_init(&fac);
    if (!Result_succeed(result))
    {
        print_error();
        return EXIT_FAILURE;
    }

    Image image = {};
    result      = Image_open(&image, &fac, args.image_path);
    if (!Result_succeed(result))
    {
        print_error();
        ImageFactory_deinit(&fac);
        return EXIT_FAILURE;
    }

    int image_width = (int) image.width, image_height = (int) image.height;

    int target_width  = 0;
    int target_height = 0;
    if (args.size[0] > 0 && args.size[1] > 0)
    {
        target_width  = args.size[0];
        target_height = args.size[1];
    }
    else
    {
        target_width  = (int) (image_width * args.width_scale);
        target_height = (int) ((double) image_height * target_width /
                               image_width / args.width_scale);

        if (to_console)
        {
            if (target_width > term_cols)
            {
                target_width  = term_cols;
                target_height = (int) ((double) image_height * target_width /
                                       image_width / args.width_scale);
            }

            if (target_height > term_rows)
            {
                target_width  = target_width * term_rows / target_height;
                target_height = term_rows;
            }
        }
    }

    Image_set_export_size(&image, &fac, target_width, target_height);

    RenderAsciiArtOpts render_opts = {
        .target       = handle,
        .image        = &image,
        .image_width  = target_width,
        .image_height = target_height,
        .brush        = args.brush,
        .brush_len    = wcslen(args.brush),
        .grayscale    = args.grayscale,
        .blackwhite   = args.blackwhite,
    };

    result = render_ascii_art(&render_opts);
    if (!Result_succeed(result))
    {
        print_error();
        Image_close(&image);
        ImageFactory_deinit(&fac);
        return EXIT_FAILURE;
    }

    Image_close(&image);
    ImageFactory_deinit(&fac);

    return EXIT_SUCCESS;
}
