#pragma once
#include <windows.h>
#include <vector>
#include <cstring>

// A fixed in-memory device context; no HWND, screen capture, or visible window.
class DrawingSurface {
    HDC dc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HFONT font_ = nullptr;
    HGDIOBJ oldBitmap_ = nullptr, oldFont_ = nullptr;
    void* pixels_ = nullptr;
public:
    static constexpr LONG width = 640, height = 20;
    DrawingSurface(LONG clipLeft = 0, LONG clipRight = width) {
        dc_ = CreateCompatibleDC(nullptr);
        if (!dc_) return;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width; info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        bitmap_ = CreateDIBSection(dc_, &info, DIB_RGB_COLORS, &pixels_, nullptr, 0);
        if (!bitmap_) return;
        oldBitmap_ = SelectObject(dc_, bitmap_);
        auto pixels = static_cast<DWORD*>(pixels_);
        for (size_t i = 0; i < static_cast<size_t>(width * height); ++i) pixels[i] = 0x00ffffff;
        font_ = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, "Arial");
        if (font_) oldFont_ = SelectObject(dc_, font_);
        SetTextColor(dc_, RGB(0,0,0)); SetBkColor(dc_, RGB(255,255,255)); SetBkMode(dc_, TRANSPARENT);
        IntersectClipRect(dc_, clipLeft, 0, clipRight, height);
    }
    DrawingSurface(const DrawingSurface&) = delete;
    DrawingSurface& operator=(const DrawingSurface&) = delete;
    ~DrawingSurface() {
        if (oldFont_) SelectObject(dc_, oldFont_);
        if (oldBitmap_) SelectObject(dc_, oldBitmap_);
        if (font_) DeleteObject(font_);
        if (bitmap_) DeleteObject(bitmap_);
        if (dc_) DeleteDC(dc_);
    }
    bool valid() const { return dc_ && bitmap_ && pixels_ && font_; }
    HDC dc() const { return dc_; }
    bool font_preserved() const { return GetCurrentObject(dc_, OBJ_FONT) == font_; }
    std::vector<unsigned char> font_data() const {
        const DWORD size = GetFontData(dc_, 0, 0, nullptr, 0);
        if (size == GDI_ERROR || size > 16 * 1024 * 1024) return {};
        std::vector<unsigned char> bytes(size);
        if (GetFontData(dc_, 0, 0, bytes.data(), size) != size) return {};
        return bytes;
    }
    std::vector<unsigned char> bitmap_bytes() const {
        GdiFlush();
        BITMAPFILEHEADER file{}; BITMAPINFOHEADER info{};
        file.bfType = 0x4d42; file.bfOffBits = sizeof(file) + sizeof(info);
        file.bfSize = file.bfOffBits + width * height * 4;
        info.biSize = sizeof(info); info.biWidth = width; info.biHeight = -height;
        info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
        info.biSizeImage = width * height * 4;
        std::vector<unsigned char> bytes(file.bfSize);
        std::memcpy(bytes.data(), &file, sizeof(file));
        std::memcpy(bytes.data() + sizeof(file), &info, sizeof(info));
        std::memcpy(bytes.data() + file.bfOffBits, pixels_, info.biSizeImage);
        return bytes;
    }
};
