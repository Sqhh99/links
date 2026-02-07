#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>

#include "core/desktop_capture/desktop_capturer.h"
#include "core/platform_window_ops.h"

namespace {

class CaptureObserver : public links::desktop_capture::DesktopCapturer::Callback {
public:
    void onCaptureResult(links::desktop_capture::DesktopCapturer::Result result,
                         std::unique_ptr<links::desktop_capture::DesktopFrame> frame) override
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            result_ = result;
            hasFrame_ = (frame != nullptr);
            if (frame) {
                width_ = frame->width();
                height_ = frame->height();
            }
            done_ = true;
        }
        condition_.notify_all();
    }

    bool wait(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return condition_.wait_for(lock, timeout, [this]() { return done_; });
    }

    links::desktop_capture::DesktopCapturer::Result result() const { return result_; }
    bool hasFrame() const { return hasFrame_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool done_{false};
    bool hasFrame_{false};
    int width_{0};
    int height_{0};
    links::desktop_capture::DesktopCapturer::Result result_{
        links::desktop_capture::DesktopCapturer::Result::ERROR_TEMPORARY};
};

bool integrationEnabled()
{
    const char* value = std::getenv("LINKS_RUN_INTEGRATION_CAPTURE");
    return value && std::string(value) == "1";
}

}  // namespace

TEST(ScreenCaptureIntegrationTest, CaptureOneFrame)
{
    if (!integrationEnabled()) {
        GTEST_SKIP() << "Set LINKS_RUN_INTEGRATION_CAPTURE=1 to run capture integration tests.";
    }

    if (!links::core::hasScreenCapturePermission()) {
        GTEST_SKIP() << "Screen capture permission is not granted on this machine.";
    }

    auto capturer = links::desktop_capture::DesktopCapturer::createScreenCapturer();
    if (!capturer) {
        GTEST_SKIP() << "Screen capturer is unavailable on this platform/session.";
    }

    links::desktop_capture::DesktopCapturer::SourceList sources;
    if (!capturer->getSourceList(&sources) || sources.empty()) {
        GTEST_SKIP() << "No capturable screen source available.";
    }

    ASSERT_TRUE(capturer->selectSource(sources.front().id));

    CaptureObserver observer;
    capturer->start(&observer);
    capturer->captureFrame();
    ASSERT_TRUE(observer.wait(std::chrono::seconds(2))) << "Capture callback timeout.";
    capturer->stop();

    EXPECT_EQ(observer.result(), links::desktop_capture::DesktopCapturer::Result::SUCCESS);
    EXPECT_TRUE(observer.hasFrame());
    EXPECT_GT(observer.width(), 0);
    EXPECT_GT(observer.height(), 0);
}
