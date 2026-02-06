#ifdef __APPLE__

#include "platform_window_ops_mac.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>

#include <cstring>

#include <string>
#include <utility>

namespace links {
namespace core {
namespace mac {
namespace {

std::string toUtf8(CFStringRef value)
{
    if (!value) {
        return {};
    }

    const char* direct = CFStringGetCStringPtr(value, kCFStringEncodingUTF8);
    if (direct) {
        return direct;
    }

    const CFIndex length = CFStringGetLength(value);
    const CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::string result(static_cast<std::size_t>(maxSize), '\0');

    if (!CFStringGetCString(value, result.data(), maxSize, kCFStringEncodingUTF8)) {
        return {};
    }

    result.resize(std::strlen(result.c_str()));
    return result;
}

bool getWindowBounds(CFDictionaryRef windowDict, CGRect* outBounds)
{
    if (!windowDict || !outBounds) {
        return false;
    }

    const CFDictionaryRef boundsDict = static_cast<CFDictionaryRef>(
        CFDictionaryGetValue(windowDict, kCGWindowBounds));
    if (!boundsDict) {
        return false;
    }

    return CGRectMakeWithDictionaryRepresentation(boundsDict, outBounds);
}

bool isShareableWindow(CFDictionaryRef windowDict)
{
    if (!windowDict) {
        return false;
    }

    int layer = 0;
    if (const CFNumberRef layerNumber = static_cast<CFNumberRef>(CFDictionaryGetValue(windowDict, kCGWindowLayer))) {
        CFNumberGetValue(layerNumber, kCFNumberIntType, &layer);
    }
    if (layer != 0) {
        return false;
    }

    double alpha = 1.0;
    if (const CFNumberRef alphaNumber = static_cast<CFNumberRef>(CFDictionaryGetValue(windowDict, kCGWindowAlpha))) {
        CFNumberGetValue(alphaNumber, kCFNumberDoubleType, &alpha);
    }
    if (alpha <= 0.0) {
        return false;
    }

    CGRect bounds{};
    if (!getWindowBounds(windowDict, &bounds)) {
        return false;
    }

    return bounds.size.width >= 100.0 && bounds.size.height >= 80.0;
}

std::optional<RawImage> captureCgImage(CGImageRef image)
{
    if (!image) {
        return std::nullopt;
    }

    const std::size_t width = CGImageGetWidth(image);
    const std::size_t height = CGImageGetHeight(image);
    if (width == 0 || height == 0) {
        return std::nullopt;
    }

    RawImage raw;
    raw.width = static_cast<int>(width);
    raw.height = static_cast<int>(height);
    raw.stride = static_cast<int>(width * 4);
    raw.format = PixelFormat::BGRA8888;
    raw.pixels.resize(static_cast<std::size_t>(raw.stride) * static_cast<std::size_t>(raw.height));

    const CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    if (!colorSpace) {
        return std::nullopt;
    }

    CGContextRef context = CGBitmapContextCreate(
        raw.pixels.data(),
        width,
        height,
        8,
        static_cast<std::size_t>(raw.stride),
        colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGColorSpaceRelease(colorSpace);

    if (!context) {
        return std::nullopt;
    }

    const CGRect rect = CGRectMake(0.0, 0.0, static_cast<CGFloat>(width), static_cast<CGFloat>(height));
    CGContextDrawImage(context, rect, image);
    CGContextRelease(context);

    if (!raw.isValid()) {
        return std::nullopt;
    }

    return raw;
}

}  // namespace

bool isWindowShareSupported()
{
    return true;
}

bool isScreenShareSupported()
{
    return true;
}

std::vector<std::uint32_t> enumerateDisplays()
{
    uint32_t count = 0;
    if (CGGetActiveDisplayList(0, nullptr, &count) != kCGErrorSuccess || count == 0) {
        return {};
    }

    std::vector<CGDirectDisplayID> displays(count);
    if (CGGetActiveDisplayList(count, displays.data(), &count) != kCGErrorSuccess) {
        return {};
    }

    std::vector<std::uint32_t> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(static_cast<std::uint32_t>(displays[i]));
    }
    return result;
}

std::vector<WindowInfo> enumerateWindows()
{
    std::vector<WindowInfo> windows;

    const CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);
    if (!windowList) {
        return windows;
    }

    const CFIndex count = CFArrayGetCount(windowList);
    windows.reserve(static_cast<std::size_t>(count));

    for (CFIndex i = 0; i < count; ++i) {
        const auto* windowDict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
        if (!isShareableWindow(windowDict)) {
            continue;
        }

        std::uint32_t windowNumber = 0;
        if (const CFNumberRef numberRef = static_cast<CFNumberRef>(CFDictionaryGetValue(windowDict, kCGWindowNumber))) {
            CFNumberGetValue(numberRef, kCFNumberSInt32Type, &windowNumber);
        }

        CGRect bounds{};
        if (!getWindowBounds(windowDict, &bounds)) {
            continue;
        }

        const auto* ownerName = static_cast<CFStringRef>(CFDictionaryGetValue(windowDict, kCGWindowOwnerName));
        const auto* windowName = static_cast<CFStringRef>(CFDictionaryGetValue(windowDict, kCGWindowName));

        std::string owner = toUtf8(ownerName);
        std::string name = toUtf8(windowName);
        if (owner.empty() && name.empty()) {
            continue;
        }

        WindowInfo info;
        info.id = static_cast<WindowId>(windowNumber);
        info.title = name.empty() ? owner : owner + " - " + name;
        info.geometry = {
            static_cast<int>(bounds.origin.x),
            static_cast<int>(bounds.origin.y),
            static_cast<int>(bounds.size.width),
            static_cast<int>(bounds.size.height)
        };
        windows.push_back(std::move(info));
    }

    CFRelease(windowList);
    return windows;
}

bool bringWindowToForeground(WindowId id)
{
    (void)id;
    return false;
}

bool excludeFromCapture(WindowId id)
{
    (void)id;
    return false;
}

bool isWindowValid(WindowId id)
{
    const auto windows = enumerateWindows();
    for (const auto& window : windows) {
        if (window.id == id) {
            return true;
        }
    }
    return false;
}

bool isWindowMinimized(WindowId id)
{
    if (id == 0) {
        return false;
    }

    const auto windows = enumerateWindows();
    for (const auto& window : windows) {
        if (window.id == id) {
            return false;
        }
    }

    const CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionIncludingWindow,
        static_cast<CGWindowID>(id));
    if (!windowList) {
        return false;
    }

    const bool exists = CFArrayGetCount(windowList) > 0;
    CFRelease(windowList);
    return exists;
}

std::optional<RawImage> captureWindowWithCoreGraphics(WindowId id)
{
    if (id == 0) {
        return std::nullopt;
    }

    const CGWindowID windowId = static_cast<CGWindowID>(id);
    const CGImageRef image = CGWindowListCreateImage(
        CGRectNull,
        kCGWindowListOptionIncludingWindow,
        windowId,
        kCGWindowImageBoundsIgnoreFraming | kCGWindowImageBestResolution);

    const auto result = captureCgImage(image);
    if (image) {
        CGImageRelease(image);
    }
    return result;
}

std::optional<RawImage> captureDisplayWithCoreGraphics(std::uint32_t displayId)
{
    const CGImageRef image = CGDisplayCreateImage(static_cast<CGDirectDisplayID>(displayId));
    const auto result = captureCgImage(image);
    if (image) {
        CGImageRelease(image);
    }
    return result;
}

}  // namespace mac
}  // namespace core
}  // namespace links

#endif  // __APPLE__
