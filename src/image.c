#include "image.h"

#include "base.h"

#include <wincodec.h>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

Result
ImageFactory_init(ImageFactory *self)
{
    if (!self)
    {
        set_error(L"%hs: NULL self pointer", __func__);
        return RESULT_ERROR_INVALID_ARGS;
    }

    HRESULT hr     = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool    coinit = SUCCEEDED(hr);

    IWICImagingFactory *factory = NULL;
    if (FAILED(hr = CoCreateInstance(
                   &CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                   &IID_IWICImagingFactory, (void **) &factory)))
    {
        if (coinit) CoUninitialize();
        set_error(
            L"CoCreateInstance(IWICImagingFactory) failed (HRESULT=0x%08lX)",
            (unsigned long) hr);
        return RESULT_ERROR_CANT_CREATE_COM;
    }

    self->ptr_   = factory;
    self->coinit = coinit;
    return RESULT_SUCCESS;
}

void
ImageFactory_deinit(ImageFactory *self)
{
    if (!self || !self->ptr_) return;
    self->ptr_->lpVtbl->Release(self->ptr_);
    if (self->coinit) CoUninitialize();
}

Result
Image_open(Image *self, const ImageFactory *factory, const wchar_t *path)
{
    Result result = RESULT_ERROR_INVALID_ARGS;
    if (!self || !factory || !factory->ptr_ || !path)
    {
        set_error(L"%hs: NULL parameter", __func__);
        return result;
    }

    IWICBitmapDecoder     *decoder   = NULL;
    IWICBitmapFrameDecode *frame     = NULL;
    IWICFormatConverter   *converter = NULL;
    UINT                   wic_w = 0, wic_h = 0;

    if (FAILED(factory->ptr_->lpVtbl->CreateDecoderFromFilename(
            factory->ptr_, path, NULL, GENERIC_READ,
            WICDecodeMetadataCacheOnLoad, &decoder)))
    {
        set_error(L"Failed to decode image file: %ls", path);
        result = RESULT_ERROR_IMAGE_CANT_LOAD;
        goto cleanup;
    }

    if (FAILED(decoder->lpVtbl->GetFrame(decoder, 0, &frame)))
    {
        set_error(L"Failed to get first frame from: %ls", path);
        result = RESULT_ERROR_IMAGE_CANT_LOAD;
        goto cleanup;
    }

    if (FAILED(factory->ptr_->lpVtbl->CreateFormatConverter(factory->ptr_,
                                                            &converter)))
    {
        set_error(L"Failed to create format converter");
        result = RESULT_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }

    if (FAILED(converter->lpVtbl->Initialize(
            converter, (IWICBitmapSource *) frame,
            &GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.f,
            WICBitmapPaletteTypeMedianCut)))
    {
        set_error(L"Failed to initialize pixel format converter for: %ls",
                  path);
        result = RESULT_ERROR_IMAGE_CANT_LOAD;
        goto cleanup;
    }

    if (FAILED(converter->lpVtbl->GetSize(converter, &wic_w, &wic_h)))
    {
        set_error(L"Failed to get image dimensions for: %ls", path);
        result = RESULT_ERROR_IMAGE_CANT_LOAD;
        goto cleanup;
    }

    if (wic_w == 0 || wic_h == 0 || wic_w > IMAGE_MAX_W ||
        wic_h > IMAGE_MAX_H || wic_w > INT_MAX || wic_h > INT_MAX ||
        (size_t) wic_w * wic_h > SIZE_MAX / IMAGE_PIXEL_BYTE)
    {
        set_error(L"Invalid image dimensions (%ux%u) from: %ls", wic_w, wic_h,
                  path);
        result = RESULT_ERROR_IMAGE_CANT_LOAD;
        goto cleanup;
    }

    self->decoder_   = decoder;
    self->frame_     = frame;
    self->converter_ = converter;
    self->scaler_    = NULL;
    self->width      = wic_w;
    self->height     = wic_h;
    result           = RESULT_SUCCESS;

cleanup:
    if (!Result_succeed(result))
    {
        if (converter) converter->lpVtbl->Release(converter);
        if (frame) frame->lpVtbl->Release(frame);
        if (decoder) decoder->lpVtbl->Release(decoder);
    }
    return result;
}

void
Image_close(Image *self)
{
    if (!self) return;
    if (self->converter_) self->converter_->lpVtbl->Release(self->converter_);
    if (self->scaler_) self->scaler_->lpVtbl->Release(self->scaler_);
    if (self->frame_) self->frame_->lpVtbl->Release(self->frame_);
    if (self->decoder_) self->decoder_->lpVtbl->Release(self->decoder_);
}

static Result
Image_export_raw_(Image *self, const WICRect *rect, size_t buf_size,
                  uint8_t *buf_ref)
{
    if (!self || !buf_ref)
    {
        set_error(L"%hs: NULL parameter", __func__);
        return RESULT_ERROR_INVALID_ARGS;
    }

    IWICBitmapSource *source = self->scaler_
                                   ? (IWICBitmapSource *) self->scaler_
                                   : (IWICBitmapSource *) self->converter_;
    if (!source)
    {
        set_error(L"%hs: no bitmap source available", __func__);
        return RESULT_ERROR_IMAGE_CANT_LOAD;
    }

    UINT stride = 0, required = 0;
    if (rect == NULL)
    {
        stride   = self->width * IMAGE_PIXEL_BYTE;
        required = stride * self->height;
    }
    else
    {
        if (rect->Width == 0 || rect->Height == 0)
        {
            set_error(L"%hs: zero-area rect (%ux%u)", __func__, rect->Width,
                      rect->Height);
            return RESULT_ERROR_INVALID_ARGS;
        }
        if ((UINT) rect->X + rect->Width > self->width ||
            (UINT) rect->Y + rect->Height > self->height)
        {
            set_error(L"%hs: rect (%d,%d %ux%u) exceeds image bounds (%ux%u)",
                      __func__, rect->X, rect->Y, rect->Width, rect->Height,
                      self->width, self->height);
            return RESULT_ERROR_INVALID_ARGS;
        }

        stride   = rect->Width * IMAGE_PIXEL_BYTE;
        required = stride * rect->Height;
    }

    if (buf_size < required)
    {
        set_error(L"%hs: buffer too small (%zu < %u)", __func__, buf_size,
                  required);
        return RESULT_ERROR_INVALID_ARGS;
    }

    HRESULT hr =
        source->lpVtbl->CopyPixels(source, rect, stride, required, buf_ref);
    if (FAILED(hr))
    {
        set_error(L"%hs: CopyPixels failed (HRESULT=0x%08lX)", __func__,
                  (unsigned long) hr);
        return RESULT_ERROR_IMAGE_CANT_LOAD;
    }
    return RESULT_SUCCESS;
}

Result
Image_set_export_size(Image *self, const ImageFactory *factory, UINT w, UINT h)
{
    if (!self || !factory || !factory->ptr_ || !self->converter_)
    {
        set_error(L"%hs: NULL parameter", __func__);
        return RESULT_ERROR_INVALID_ARGS;
    }
    if (w == 0 || h == 0)
    {
        set_error(L"%hs: invalid target size (%ux%u)", __func__, w, h);
        return RESULT_ERROR_INVALID_ARGS;
    }
    if (w >= self->width && h >= self->height) return RESULT_SUCCESS;

    //! 应用 `IWICBitmapScaler` 后会变为 BGRA
    IWICBitmapScaler *scaler = NULL;
    HRESULT           hr =
        factory->ptr_->lpVtbl->CreateBitmapScaler(factory->ptr_, &scaler);
    if (FAILED(hr))
    {
        set_error(L"Failed to create bitmap scaler (HRESULT=0x%08lX)",
                  (unsigned long) hr);
        return RESULT_ERROR_OUT_OF_MEMORY;
    }

    hr = scaler->lpVtbl->Initialize(scaler,
                                    (IWICBitmapSource *) self->converter_, w, h,
                                    WICBitmapInterpolationModeFant);
    if (FAILED(hr))
    {
        set_error(L"Failed to initialize scaler to %ux%u (HRESULT=0x%08lX)", w,
                  h, (unsigned long) hr);
        scaler->lpVtbl->Release(scaler);
        return RESULT_ERROR_IMAGE_CANT_LOAD;
    }

    if (self->scaler_) self->scaler_->lpVtbl->Release(self->scaler_);
    self->scaler_ = scaler;
    self->width   = w;
    self->height  = h;
    return RESULT_SUCCESS;
}

Result
Image_export_line(Image *self, UINT y, size_t buf_size, uint8_t *buf_ref)
{
    if (y > self->height)
    {
        set_error(L"%hs: row %u exceeds image height %u", __func__, y,
                  self->height);
        return RESULT_ERROR_INVALID_ARGS;
    }
    return Image_export_raw_(self, &(WICRect){0, (INT) y, (INT) self->width, 1},
                             buf_size, buf_ref);
}
