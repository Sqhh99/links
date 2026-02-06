#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include "core/desktop_capture/desktop_capturer.h"
#include "core/platform_window_ops.h"

namespace {

class CaptureObserver : public links::desktop_capture::DesktopCapturer::Callback {
public:
    void reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        done_ = false;
        hasFrame_ = false;
        width_ = 0;
        height_ = 0;
        result_ = links::desktop_capture::DesktopCapturer::Result::ERROR_TEMPORARY;
    }

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

    links::desktop_capture::DesktopCapturer::Result result() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return result_;
    }

    bool hasFrame() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return hasFrame_;
    }

    int width() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return width_;
    }

    int height() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return height_;
    }

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

bool benchmarkEnabled()
{
    const char* value = std::getenv("LINKS_RUN_CAPTURE_BENCHMARK");
    return value && std::string(value) == "1";
}

}  // namespace

TEST(CaptureBenchmarkTest, ScreenCaptureLatency)
{
    if (!benchmarkEnabled()) {
        GTEST_SKIP() << "Set LINKS_RUN_CAPTURE_BENCHMARK=1 to run capture benchmark.";
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

    constexpr int kAttempts = 20;
    int successCount = 0;
    double totalMs = 0.0;

    for (int i = 0; i < kAttempts; ++i) {
        observer.reset();

        const auto begin = std::chrono::steady_clock::now();
        capturer->captureFrame();
        if (!observer.wait(std::chrono::seconds(2))) {
            continue;
        }

        if (observer.result() != links::desktop_capture::DesktopCapturer::Result::SUCCESS) {
            continue;
        }

        if (!observer.hasFrame() || observer.width() <= 0 || observer.height() <= 0) {
            continue;
        }

        const auto end = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
        totalMs += static_cast<double>(elapsed.count()) / 1000.0;
        ++successCount;
    }

    capturer->stop();

    ASSERT_GT(successCount, 0) << "No successful capture frames for benchmark.";

    const double avgMs = totalMs / static_cast<double>(successCount);
    std::cout << "capture benchmark: backend=" << static_cast<int>(capturer->backend())
              << ", success=" << successCount << "/" << kAttempts
              << ", avg_ms=" << avgMs << std::endl;
}
