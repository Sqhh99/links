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

}  // namespace core
}  // namespace links

#endif  // CORE_WINDOW_TYPES_H
