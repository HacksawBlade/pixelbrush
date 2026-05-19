// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#include "image.h"

#include "base.hpp"
#include "utils.hpp"

#include <cstdint>
#include <utility>
#include <wincodec.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.h>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Foundation;

Result<Image>
Image::open(const std::wstring &path)
try
{
    auto fullpath = fsutil::abspath(path);
    if (!fullpath) return fail(std::move(fullpath).error());

    auto file    = StorageFile::GetFileFromPathAsync((*fullpath).data()).get();
    auto stream  = file.OpenReadAsync().get();
    auto decoder = BitmapDecoder::CreateAsync(stream).get();
    auto frame   = decoder.GetFrameAsync(0).get();

    auto provider =
        frame
            .GetPixelDataAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Ignore, {},
                               ExifOrientationMode::IgnoreExifOrientation,
                               ColorManagementMode::ColorManageToSRgb)
            .get();
    auto pixel_data = provider.DetachPixelData();

    std::uint32_t w{frame.PixelWidth()};
    std::uint32_t h{frame.PixelHeight()};

    return Image{w, h, std::move(pixel_data)};
}
catch (const winrt::hresult_error &e)
{
    return fail(ErrCode::ImageCantLoad,
                std::format("Failed to open image: {} ({})", strutil::to_narrow(path),
                            strutil::to_narrow(e.message())));
}

static WICBitmapInterpolationMode
wic_scale(ScaleMode scale_mode)
{
    switch (scale_mode)
    {
    case ScaleMode::Fant:
        return WICBitmapInterpolationModeFant;
    case ScaleMode::NearestNeighbor:
        return WICBitmapInterpolationModeNearestNeighbor;
    }
    std::unreachable();
}

Result<void>
Image::scale(std::uint32_t new_w, std::uint32_t new_h)
{
    if (new_w == 0 || new_h == 0)
        return fail(ErrCode::InvalidValue,
                    std::format("Invalid target size ({}x{})", new_w, new_h));

    if (new_w == width_ && new_h == height_) return {};

    winrt::com_ptr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(factory.put()));
    if (FAILED(hr))
        return fail(ErrCode::CreateObject, "Failed to create WIC imaging factory");

    winrt::com_ptr<IWICBitmap> bitmap;
    UINT                       w{width_}, h{height_};
    hr = factory->CreateBitmapFromMemory(
        w, h, GUID_WICPixelFormat32bppBGRA, w * IMAGE_PIXEL_BYTE,
        static_cast<UINT>(pixels_.size()), pixels_.data(), bitmap.put());
    if (FAILED(hr))
        return fail(ErrCode::ImageCantLoad, "Failed to create bitmap from memory");

    winrt::com_ptr<IWICBitmapScaler> scaler;
    hr = factory->CreateBitmapScaler(scaler.put());
    if (FAILED(hr)) return fail(ErrCode::OutOfMemory, "Failed to create bitmap scaler");

    hr = scaler->Initialize(bitmap.get(), new_w, new_h, wic_scale(scale_mode_));
    if (FAILED(hr))
        return fail(ErrCode::ImageCantLoad, "Failed to initialize bitmap scaler");

    UINT sw{0}, sh{0};
    scaler->GetSize(&sw, &sh);
    UINT stride = sw * IMAGE_PIXEL_BYTE;
    UINT buf_size{stride * sh};

    winrt::com_array<uint8_t> scaled(buf_size);
    hr = scaler->CopyPixels(nullptr, stride, buf_size, scaled.data());
    if (FAILED(hr)) return fail(ErrCode::ImageCantLoad, "Failed to copy scaled pixels");

    width_ = sw, height_ = sh, pixels_ = std::move(scaled);
    return {};
}

std::uint32_t
Image::width() const
{
    return width_;
}

std::uint32_t
Image::height() const
{
    return height_;
}

std::span<std::uint8_t>
Image::pixels()
{
    return {pixels_.data(), pixels_.size()};
}

std::span<const std::uint8_t>
Image::pixels() const
{
    return {pixels_.data(), pixels_.size()};
}

ScaleMode
Image::scale_mode()
{
    return scale_mode_;
}

ScaleMode
Image::scale_mode(ScaleMode new_scale_mode)
{
    scale_mode_ = new_scale_mode;
    return scale_mode_;
}

ScaleMode
Image::scale_mode() const
{
    return scale_mode_;
}
