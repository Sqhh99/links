/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - GDI Fallback Capturer Implementation
 */

#ifdef _WIN32

#include "gdi_capturer.h"
#include "window_utils.h"
#include <chrono>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace links {
namespace desktop_capture {
namespace win {

GdiCapturer::GdiCapturer(const CaptureOptions& options) {
    options_ = options;
}

GdiCapturer::~GdiCapturer() {
    stop();
}

void GdiCapturer::start(Callback* callback) {
    callback_ = callback;
    started_ = true;
}

void GdiCapturer::stop() {
    started_ = false;
    callback_ = nullptr;
}

std::unique_ptr<DesktopFrame> GdiCapturer::captureWindow(void* hwndPtr) {
    HWND hwnd = static_cast<HWND>(hwndPtr);
    if (!hwnd || !IsWindow(hwnd)) {
        return nullptr;
    }

    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        return nullptr;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        return nullptr;
    }

    HDC hdcWindow = GetWindowDC(hwnd);
    if (!hdcWindow) {
        return nullptr;
    }

    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        ReleaseDC(hwnd, hdcWindow);
        return nullptr;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hbm = CreateDIBSection(hdcMemDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hbm || !bits) {
        if (hbm) DeleteObject(hbm);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return nullptr;
    }

    HGDIOBJ old = SelectObject(hdcMemDC, hbm);
    BOOL ok = PrintWindow(hwnd, hdcMemDC, PW_RENDERFULLCONTENT);
    if (!ok) {
        SelectObject(hdcMemDC, old);
        DeleteObject(hbm);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return nullptr;
    }

    // Create desktop frame and copy data
    auto frame = std::make_unique<BasicDesktopFrame>(DesktopSize(width, height));

    // Convert BGRA to RGBA
    const uint8_t* src = static_cast<const uint8_t*>(bits);
    uint8_t* dst = frame->data();
    int srcStride = ((width * 4 + 3) / 4) * 4;  // DWORD aligned

    for (int y = 0; y < height; ++y) {
        const uint8_t* srcRow = src + y * srcStride;
        uint8_t* dstRow = dst + y * frame->stride();
        for (int x = 0; x < width; ++x) {
            dstRow[x * 4 + 0] = srcRow[x * 4 + 2];  // R <- B
            dstRow[x * 4 + 1] = srcRow[x * 4 + 1];  // G <- G
            dstRow[x * 4 + 2] = srcRow[x * 4 + 0];  // B <- R
            dstRow[x * 4 + 3] = srcRow[x * 4 + 3];  // A <- A
        }
    }

    SelectObject(hdcMemDC, old);
    DeleteObject(hbm);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);

    frame->setCaptureTimeUs(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    return frame;
}

void GdiCapturer::captureFrame() {
    if (!started_ || !callback_) {
        return;
    }

    if (selectedSource_ == 0) {
        callback_->onCaptureResult(Result::ERROR_PERMANENT, nullptr);
        return;
    }

    auto frame = captureWindow(reinterpret_cast<HWND>(selectedSource_));
    if (frame) {
        callback_->onCaptureResult(Result::SUCCESS, std::move(frame));
    } else {
        callback_->onCaptureResult(Result::ERROR_TEMPORARY, nullptr);
    }
}

bool GdiCapturer::getSourceList(SourceList* sources) {
    if (!sources) {
        return false;
    }
    sources->clear();
    auto windows = enumerateCaptureWindows();
    for (const auto& w : windows) {
        Source src;
        src.id = reinterpret_cast<SourceId>(w.hwnd);
        int len = WideCharToMultiByte(CP_UTF8, 0, w.title.c_str(), -1,
                                      nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            std::string utf8(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, w.title.c_str(), -1,
                               utf8.data(), len, nullptr, nullptr);
            src.title = utf8;
        }
        sources->push_back(src);
    }
    return true;
}

bool GdiCapturer::selectSource(SourceId id) {
    selectedSource_ = id;
    return true;
}

bool GdiCapturer::isSourceValid(SourceId id) {
    return isWindowValid(reinterpret_cast<HWND>(id));
}

GdiCapturer::SourceId GdiCapturer::selectedSource() const {
    return selectedSource_;
}

}  // namespace win
}  // namespace desktop_capture
}  // namespace links

#endif  // _WIN32
