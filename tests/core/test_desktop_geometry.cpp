#include <gtest/gtest.h>

#include "desktop_capture/desktop_geometry.h"

namespace links {
namespace desktop_capture {

TEST(DesktopVectorTest, AddSubtractAndEquality) {
    DesktopVector v1(3, 4);
    DesktopVector v2(-1, 5);
    DesktopVector sum = v1.add(v2);
    DesktopVector diff = v1.subtract(v2);

    EXPECT_EQ(sum, DesktopVector(2, 9));
    EXPECT_EQ(diff, DesktopVector(4, -1));
    EXPECT_TRUE(v1 == DesktopVector(3, 4));
    EXPECT_TRUE(v1 != DesktopVector(3, 5));
}

TEST(DesktopSizeTest, EmptyCheck) {
    EXPECT_TRUE(DesktopSize(0, 1).isEmpty());
    EXPECT_TRUE(DesktopSize(-1, 1).isEmpty());
    EXPECT_TRUE(DesktopSize(1, 0).isEmpty());
    EXPECT_FALSE(DesktopSize(1, 2).isEmpty());
}

TEST(DesktopRectTest, ConstructionAndIntersect) {
    DesktopRect rect = DesktopRect::makeXYWH(10, 20, 30, 40);
    EXPECT_EQ(rect.left(), 10);
    EXPECT_EQ(rect.top(), 20);
    EXPECT_EQ(rect.right(), 40);
    EXPECT_EQ(rect.bottom(), 60);

    DesktopRect rect2 = DesktopRect::makeLTRB(0, 0, 5, 5);
    EXPECT_EQ(rect2.width(), 5);
    EXPECT_EQ(rect2.height(), 5);

    DesktopRect rect3 = DesktopRect::makeSize(DesktopSize(3, 4));
    EXPECT_EQ(rect3.left(), 0);
    EXPECT_EQ(rect3.top(), 0);
    EXPECT_EQ(rect3.right(), 3);
    EXPECT_EQ(rect3.bottom(), 4);

    DesktopRect a = DesktopRect::makeXYWH(0, 0, 10, 10);
    DesktopRect b = DesktopRect::makeXYWH(5, 5, 10, 10);
    DesktopRect intersection = a.intersect(b);
    EXPECT_EQ(intersection, DesktopRect::makeLTRB(5, 5, 10, 10));

    EXPECT_TRUE(a.contains(0, 0));
    EXPECT_TRUE(a.contains(9, 9));
    EXPECT_FALSE(a.contains(10, 10));
    EXPECT_TRUE(a.containsRect(DesktopRect::makeXYWH(2, 2, 3, 3)));
    EXPECT_FALSE(a.containsRect(b));
}

TEST(DesktopRectTest, Translate) {
    DesktopRect rect = DesktopRect::makeXYWH(1, 2, 3, 4);
    rect.translate(2, -1);
    EXPECT_EQ(rect, DesktopRect::makeLTRB(3, 1, 6, 5));
}

}  // namespace desktop_capture
}  // namespace links
