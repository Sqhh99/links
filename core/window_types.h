#ifndef CORE_WINDOW_TYPES_H
#define CORE_WINDOW_TYPES_H

#include <cstdint>
#include <string>

namespace links {
namespace core {

using WindowId = std::uint64_t;

struct WindowRect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

struct WindowInfo {
    std::string title;
    WindowId id{0};
    WindowRect geometry;
};

/// Backend-native monitor handle: HMONITOR on Windows, CGDirectDisplayID on
/// macOS, screen index on X11. 0 means "unspecified / primary".
using MonitorId = std::uint64_t;

struct MonitorInfo {
    MonitorId id{0};
    std::string name;   // platform device name, UTF-8
    WindowRect geometry;
    bool isPrimary{false};
};

}  // namespace core
}  // namespace links

#endif  // CORE_WINDOW_TYPES_H
