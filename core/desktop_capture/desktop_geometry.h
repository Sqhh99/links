/*
 * Copyright (c) 2026 Links Project
 * Desktop Capture - Geometry Types
 */

#ifndef DESKTOP_CAPTURE_DESKTOP_GEOMETRY_H_
#define DESKTOP_CAPTURE_DESKTOP_GEOMETRY_H_

#include <cstdint>
#include <algorithm>

namespace links {
namespace desktop_capture {

// A vector in the 2D integer space. E.g. a cursor position or a screen size.
class DesktopVector {
public:
    DesktopVector() : x_(0), y_(0) {}
    DesktopVector(int32_t x, int32_t y) : x_(x), y_(y) {}

    int32_t x() const { return x_; }
    int32_t y() const { return y_; }

    void set(int32_t x, int32_t y) {
        x_ = x;
        y_ = y;
    }

    bool equals(const DesktopVector& other) const {
        return x_ == other.x_ && y_ == other.y_;
    }

    DesktopVector add(const DesktopVector& other) const {
        return DesktopVector(x_ + other.x_, y_ + other.y_);
    }

    DesktopVector subtract(const DesktopVector& other) const {
        return DesktopVector(x_ - other.x_, y_ - other.y_);
    }

    bool operator==(const DesktopVector& other) const { return equals(other); }
    bool operator!=(const DesktopVector& other) const { return !equals(other); }

private:
    int32_t x_;
    int32_t y_;
};

// A size of a desktop region.
class DesktopSize {
public:
    DesktopSize() : width_(0), height_(0) {}
    DesktopSize(int32_t width, int32_t height) : width_(width), height_(height) {}

    int32_t width() const { return width_; }
    int32_t height() const { return height_; }

    bool isEmpty() const { return width_ <= 0 || height_ <= 0; }

    void set(int32_t width, int32_t height) {
        width_ = width;
        height_ = height;
    }

    bool equals(const DesktopSize& other) const {
        return width_ == other.width_ && height_ == other.height_;
    }

    bool operator==(const DesktopSize& other) const { return equals(other); }
    bool operator!=(const DesktopSize& other) const { return !equals(other); }

private:
    int32_t width_;
    int32_t height_;
};

// A rectangle in the 2D integer space.
class DesktopRect {
public:
    DesktopRect() : left_(0), top_(0), right_(0), bottom_(0) {}

    static DesktopRect makeXYWH(int32_t x, int32_t y, int32_t width, int32_t height) {
        return DesktopRect(x, y, x + width, y + height);
    }

    static DesktopRect makeLTRB(int32_t left, int32_t top, int32_t right, int32_t bottom) {
        return DesktopRect(left, top, right, bottom);
    }

    static DesktopRect makeSize(const DesktopSize& size) {
        return DesktopRect(0, 0, size.width(), size.height());
    }

    static DesktopRect makeOriginSize(const DesktopVector& origin, const DesktopSize& size) {
        return makeXYWH(origin.x(), origin.y(), size.width(), size.height());
    }

    int32_t left() const { return left_; }
    int32_t top() const { return top_; }
    int32_t right() const { return right_; }
    int32_t bottom() const { return bottom_; }
    int32_t x() const { return left_; }
    int32_t y() const { return top_; }
    int32_t width() const { return right_ - left_; }
    int32_t height() const { return bottom_ - top_; }

    DesktopVector topLeft() const { return DesktopVector(left_, top_); }
    DesktopSize size() const { return DesktopSize(width(), height()); }

    bool isEmpty() const { return left_ >= right_ || top_ >= bottom_; }

    bool equals(const DesktopRect& other) const {
        return left_ == other.left_ && top_ == other.top_ &&
               right_ == other.right_ && bottom_ == other.bottom_;
    }

    bool contains(int32_t x, int32_t y) const {
        return x >= left_ && x < right_ && y >= top_ && y < bottom_;
    }

    bool containsRect(const DesktopRect& other) const {
        return other.left_ >= left_ && other.right_ <= right_ &&
               other.top_ >= top_ && other.bottom_ <= bottom_;
    }

    void translate(int32_t dx, int32_t dy) {
        left_ += dx;
        top_ += dy;
        right_ += dx;
        bottom_ += dy;
    }

    void translate(const DesktopVector& d) { translate(d.x(), d.y()); }

    DesktopRect intersect(const DesktopRect& other) const {
        int32_t l = std::max(left_, other.left_);
        int32_t t = std::max(top_, other.top_);
        int32_t r = std::min(right_, other.right_);
        int32_t b = std::min(bottom_, other.bottom_);
        if (l >= r || t >= b) {
            return DesktopRect();
        }
        return makeLTRB(l, t, r, b);
    }

    bool operator==(const DesktopRect& other) const { return equals(other); }
    bool operator!=(const DesktopRect& other) const { return !equals(other); }

private:
    DesktopRect(int32_t l, int32_t t, int32_t r, int32_t b)
        : left_(l), top_(t), right_(r), bottom_(b) {}

    int32_t left_;
    int32_t top_;
    int32_t right_;
    int32_t bottom_;
};

}  // namespace desktop_capture
}  // namespace links

#endif  // DESKTOP_CAPTURE_DESKTOP_GEOMETRY_H_
