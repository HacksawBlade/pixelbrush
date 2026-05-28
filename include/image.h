#ifndef IMAGE_H
#define IMAGE_H

#include "base.h"

typedef struct IWICImagingFactory    IWICImagingFactory;
typedef struct IWICBitmapDecoder     IWICBitmapDecoder;
typedef struct IWICBitmapFrameDecode IWICBitmapFrameDecode;
typedef struct IWICFormatConverter   IWICFormatConverter;
typedef struct IWICBitmapScaler      IWICBitmapScaler;
typedef unsigned int                 UINT;

typedef struct
{
    IWICImagingFactory *ptr_;
    bool                coinit;
} ImageFactory;

typedef struct
{
    IWICBitmapDecoder     *decoder_;
    IWICBitmapFrameDecode *frame_;
    IWICFormatConverter   *converter_;
    IWICBitmapScaler      *scaler_;
    UINT                   width;
    UINT                   height;
} Image;

#define IMAGE_MAX_H      20000
#define IMAGE_MAX_W      20000
#define IMAGE_PIXEL_BYTE 4

NODISCARD Result ImageFactory_init(ImageFactory *self);
void             ImageFactory_deinit(ImageFactory *self);

NODISCARD Result Image_open(Image *self, const ImageFactory *factory,
                            const wchar_t *path);
void             Image_close(Image *self);
NODISCARD Result Image_set_export_size(Image *self, const ImageFactory *factory,
                                       UINT w, UINT h);
NODISCARD Result Image_export(Image *self, size_t buf_size, uint8_t *buf_ref);
NODISCARD Result Image_export_line(Image *self, UINT y, size_t buf_size,
                                   uint8_t *buf_ref);

#endif
