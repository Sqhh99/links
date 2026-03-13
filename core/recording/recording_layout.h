#ifndef CORE_RECORDING_RECORDING_LAYOUT_H
#define CORE_RECORDING_RECORDING_LAYOUT_H

#include <vector>

namespace links::recording {

struct Rect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

struct GridLayout {
    int cols{1};
    int rows{1};
    std::vector<Rect> cells;
};

struct ScreenShareLayout {
    Rect screenRect;
    std::vector<Rect> thumbnailRects;
};

GridLayout buildGridLayout(int canvasWidth, int canvasHeight, int participantCount);

ScreenShareLayout buildScreenShareLayout(int canvasWidth,
                                         int canvasHeight,
                                         int thumbnailCount,
                                         int thumbnailWidth = 240,
                                         int thumbnailHeight = 135,
                                         int margin = 12,
                                         int maxThumbnails = 4);

} // namespace links::recording

#endif // CORE_RECORDING_RECORDING_LAYOUT_H
