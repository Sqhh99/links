#ifdef __linux__

#include "platform_window_ops_linux_x11.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <utility>

namespace links {
namespace core {
namespace linux_x11 {
namespace {

class ScopedDisplay {
public:
    ScopedDisplay()
        : display_(XOpenDisplay(nullptr))
    {
    }

    ~ScopedDisplay()
    {
        if (display_) {
            XCloseDisplay(display_);
        }
    }

    Display* get() const { return display_; }

private:
    Display* display_{nullptr};
};

unsigned maskShift(unsigned long mask)
{
    unsigned shift = 0;
    while (((mask >> shift) & 0x1UL) == 0 && shift < (sizeof(unsigned long) * 8U)) {
        ++shift;
    }
    return shift;
}

std::uint8_t extractChannel(unsigned long pixel, unsigned long mask)
{
    if (mask == 0) {
        return 0;
    }

    const unsigned shift = maskShift(mask);
    const unsigned long value = (pixel & mask) >> shift;
    const unsigned long maxValue = mask >> shift;
    if (maxValue == 0) {
        return 0;
    }

    return static_cast<std::uint8_t>((value * 255UL + maxValue / 2UL) / maxValue);
}

std::optional<std::string> readWindowPropertyString(Display* display, Window window, Atom property)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* value = nullptr;

    const int status = XGetWindowProperty(
        display,
        window,
        property,
        0,
        1024,
        False,
        AnyPropertyType,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &value);

    if (status != Success || !value || itemCount == 0) {
        if (value) {
            XFree(value);
        }
        return std::nullopt;
    }

    std::string result(reinterpret_cast<char*>(value), itemCount);
    XFree(value);
    return result;
}

std::vector<Window> clientWindows(Display* display, Window root)
{
    std::vector<Window> windows;

    const Atom netClientList = XInternAtom(display, "_NET_CLIENT_LIST", True);
    if (netClientList != None) {
        Atom actualType = None;
        int actualFormat = 0;
        unsigned long itemCount = 0;
        unsigned long bytesAfter = 0;
        unsigned char* value = nullptr;

        const int status = XGetWindowProperty(
            display,
            root,
            netClientList,
            0,
            16384,
            False,
            XA_WINDOW,
            &actualType,
            &actualFormat,
            &itemCount,
            &bytesAfter,
            &value);

        if (status == Success && value && actualType == XA_WINDOW && actualFormat == 32) {
            const auto* data = reinterpret_cast<Window*>(value);
            windows.assign(data, data + itemCount);
            XFree(value);
            if (!windows.empty()) {
                return windows;
            }
        } else if (value) {
            XFree(value);
        }
    }

    Window rootOut = 0;
    Window parentOut = 0;
    Window* children = nullptr;
    unsigned int childCount = 0;

    if (XQueryTree(display, root, &rootOut, &parentOut, &children, &childCount) != 0 && children) {
        windows.assign(children, children + childCount);
        XFree(children);
    }

    return windows;
}

bool isShareableWindow(Display* display, Window root, Window window)
{
    if (window == 0 || window == root) {
        return false;
    }

    XWindowAttributes attributes{};
    if (XGetWindowAttributes(display, window, &attributes) == 0) {
        return false;
    }

    if (attributes.map_state != IsViewable || attributes.width < 100 || attributes.height < 80) {
        return false;
    }

    const Atom wmState = XInternAtom(display, "WM_STATE", True);
    if (wmState == None) {
        return false;
    }

    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* value = nullptr;
    const int status = XGetWindowProperty(
        display,
        window,
        wmState,
        0,
        2,
        False,
        wmState,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &value);

    if (value) {
        XFree(value);
    }

    return status == Success;
}

std::optional<RawImage> captureXImage(Display* display, Drawable drawable, int width, int height)
{
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }

    XImage* xImage = XGetImage(display, drawable, 0, 0, static_cast<unsigned int>(width), static_cast<unsigned int>(height), AllPlanes, ZPixmap);
    if (!xImage) {
        return std::nullopt;
    }

    RawImage image;
    image.width = width;
    image.height = height;
    image.stride = width * 4;
    image.format = PixelFormat::RGBA8888;
    image.pixels.resize(static_cast<std::size_t>(image.stride) * static_cast<std::size_t>(height));

    for (int y = 0; y < height; ++y) {
        std::uint8_t* dst = image.pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(image.stride);
        for (int x = 0; x < width; ++x) {
            const unsigned long pixel = XGetPixel(xImage, x, y);
            dst[x * 4 + 0] = extractChannel(pixel, static_cast<unsigned long>(xImage->red_mask));
            dst[x * 4 + 1] = extractChannel(pixel, static_cast<unsigned long>(xImage->green_mask));
            dst[x * 4 + 2] = extractChannel(pixel, static_cast<unsigned long>(xImage->blue_mask));
            dst[x * 4 + 3] = 255;
        }
    }

    XDestroyImage(xImage);
    if (!image.isValid()) {
        return std::nullopt;
    }
    return image;
}

Window toX11Window(WindowId id)
{
    return static_cast<Window>(id);
}

}  // namespace

bool isX11Session()
{
    const char* display = std::getenv("DISPLAY");
    if (!display || std::strlen(display) == 0) {
        return false;
    }

    const char* sessionType = std::getenv("XDG_SESSION_TYPE");
    if (!sessionType || std::strlen(sessionType) == 0) {
        return true;
    }

    std::string normalized(sessionType);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return normalized == "x11";
}

bool isWindowShareSupported()
{
    if (!isX11Session()) {
        return false;
    }

    ScopedDisplay display;
    return display.get() != nullptr;
}

bool isScreenShareSupported()
{
    return isWindowShareSupported();
}

std::vector<WindowInfo> enumerateWindows()
{
    std::vector<WindowInfo> windows;
    if (!isWindowShareSupported()) {
        return windows;
    }

    ScopedDisplay display;
    Display* dpy = display.get();
    if (!dpy) {
        return windows;
    }

    const Window root = DefaultRootWindow(dpy);
    const Atom netWmName = XInternAtom(dpy, "_NET_WM_NAME", True);
    const Atom utf8String = XInternAtom(dpy, "UTF8_STRING", True);

    const auto candidates = clientWindows(dpy, root);
    windows.reserve(candidates.size());

    for (Window window : candidates) {
        if (!isShareableWindow(dpy, root, window)) {
            continue;
        }

        std::string title;
        if (netWmName != None && utf8String != None) {
            if (auto netTitle = readWindowPropertyString(dpy, window, netWmName)) {
                title = *netTitle;
            }
        }
        if (title.empty()) {
            char* legacyName = nullptr;
            if (XFetchName(dpy, window, &legacyName) != 0 && legacyName) {
                title = legacyName;
                XFree(legacyName);
            }
        }

        if (title.empty()) {
            continue;
        }

        XWindowAttributes attrs{};
        if (XGetWindowAttributes(dpy, window, &attrs) == 0) {
            continue;
        }

        int rootX = 0;
        int rootY = 0;
        Window child = 0;
        XTranslateCoordinates(dpy, window, root, 0, 0, &rootX, &rootY, &child);

        WindowInfo info;
        info.id = static_cast<WindowId>(window);
        info.title = std::move(title);
        info.geometry = {rootX, rootY, attrs.width, attrs.height};
        windows.push_back(std::move(info));
    }

    return windows;
}

bool bringWindowToForeground(WindowId id)
{
    if (id == 0 || !isWindowShareSupported()) {
        return false;
    }

    ScopedDisplay display;
    Display* dpy = display.get();
    if (!dpy) {
        return false;
    }

    const Window root = DefaultRootWindow(dpy);
    const Atom netActive = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    if (netActive == None) {
        return false;
    }

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = toX11Window(id);
    event.xclient.message_type = netActive;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = CurrentTime;

    const Status status = XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(dpy);
    return status != 0;
}

bool excludeFromCapture(WindowId id)
{
    (void)id;
    return false;
}

bool isWindowValid(WindowId id)
{
    if (id == 0 || !isWindowShareSupported()) {
        return false;
    }

    ScopedDisplay display;
    Display* dpy = display.get();
    if (!dpy) {
        return false;
    }

    XWindowAttributes attrs{};
    return XGetWindowAttributes(dpy, toX11Window(id), &attrs) != 0;
}

bool isWindowMinimized(WindowId id)
{
    if (id == 0 || !isWindowShareSupported()) {
        return false;
    }

    ScopedDisplay display;
    Display* dpy = display.get();
    if (!dpy) {
        return false;
    }

    XWindowAttributes attrs{};
    if (XGetWindowAttributes(dpy, toX11Window(id), &attrs) == 0) {
        return false;
    }

    if (attrs.map_state != IsViewable) {
        return true;
    }

    const Atom netWmState = XInternAtom(dpy, "_NET_WM_STATE", True);
    const Atom hiddenAtom = XInternAtom(dpy, "_NET_WM_STATE_HIDDEN", True);
    if (netWmState == None || hiddenAtom == None) {
        return false;
    }

    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* value = nullptr;

    const int status = XGetWindowProperty(
        dpy,
        toX11Window(id),
        netWmState,
        0,
        16,
        False,
        XA_ATOM,
        &actualType,
        &actualFormat,
        &itemCount,
        &bytesAfter,
        &value);

    if (status != Success || !value || actualType != XA_ATOM || actualFormat != 32) {
        if (value) {
            XFree(value);
        }
        return false;
    }

    bool hidden = false;
    const auto* atoms = reinterpret_cast<Atom*>(value);
    for (unsigned long i = 0; i < itemCount; ++i) {
        if (atoms[i] == hiddenAtom) {
            hidden = true;
            break;
        }
    }

    XFree(value);
    return hidden;
}

std::optional<RawImage> captureWindowWithX11(WindowId id)
{
    if (id == 0 || !isWindowShareSupported()) {
        return std::nullopt;
    }

    ScopedDisplay display;
    Display* dpy = display.get();
    if (!dpy) {
        return std::nullopt;
    }

    XWindowAttributes attrs{};
    if (XGetWindowAttributes(dpy, toX11Window(id), &attrs) == 0) {
        return std::nullopt;
    }

    return captureXImage(dpy, toX11Window(id), attrs.width, attrs.height);
}

std::optional<RawImage> captureRootScreenWithX11()
{
    if (!isScreenShareSupported()) {
        return std::nullopt;
    }

    ScopedDisplay display;
    Display* dpy = display.get();
    if (!dpy) {
        return std::nullopt;
    }

    const int screen = DefaultScreen(dpy);
    const int width = DisplayWidth(dpy, screen);
    const int height = DisplayHeight(dpy, screen);
    return captureXImage(dpy, DefaultRootWindow(dpy), width, height);
}

}  // namespace linux_x11
}  // namespace core
}  // namespace links

#endif  // __linux__
