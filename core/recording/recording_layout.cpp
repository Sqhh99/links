#include "recording_layout.h"

#include <algorithm>

namespace links::recording {

GridLayout buildGridLayout(int canvasWidth, int canvasHeight, int participantCount)
{
    GridLayout layout;
    const int count = std::max(1, participantCount);

    if (count == 2) {
        layout.cols = 2;
        layout.rows = 1;
    } else if (count <= 4) {
        layout.cols = 2;
        layout.rows = 2;
    } else if (count <= 6) {
        layout.cols = 3;
        layout.rows = 2;
    } else if (count <= 9) {
        layout.cols = 3;
        layout.rows = 3;
    } else {
        layout.cols = 4;
        layout.rows = (count + 3) / 4;
    }

    const int cellW = canvasWidth / layout.cols;
    const int cellH = canvasHeight / layout.rows;

    layout.cells.reserve(layout.cols * layout.rows);
    for (int i = 0; i < layout.cols * layout.rows; ++i) {
        const int col = i % layout.cols;
        const int row = i / layout.cols;
        layout.cells.push_back(Rect{col * cellW, row * cellH, cellW, cellH});
    }

    return layout;
}

ScreenShareLayout buildScreenShareLayout(int canvasWidth,
                                         int canvasHeight,
                                         int thumbnailCount,
                                         int thumbnailWidth,
                                         int thumbnailHeight,
                                         int margin,
                                         int maxThumbnails)
{
    ScreenShareLayout layout;
    layout.screenRect = Rect{0, 0, canvasWidth, canvasHeight};

    const int count = std::max(0, std::min(thumbnailCount, maxThumbnails));
    if (count == 0) {
        return layout;
    }

    const int startX = canvasWidth - margin - thumbnailWidth;
    const int startY = canvasHeight - margin - count * (thumbnailHeight + margin) + margin;

    layout.thumbnailRects.reserve(count);
    for (int i = 0; i < count; ++i) {
        layout.thumbnailRects.push_back(
            Rect{startX, startY + i * (thumbnailHeight + margin), thumbnailWidth, thumbnailHeight});
    }

    return layout;
}

} // namespace links::recording
