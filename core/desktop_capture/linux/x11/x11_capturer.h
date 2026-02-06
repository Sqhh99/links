#ifndef DESKTOP_CAPTURE_LINUX_X11_X11_CAPTURER_H_
#define DESKTOP_CAPTURE_LINUX_X11_X11_CAPTURER_H_

#ifdef __linux__

#include "../../desktop_capturer.h"

namespace links {
namespace desktop_capture {
namespace linux_x11 {

class X11ScreenCapturer : public DesktopCapturer {
public:
    explicit X11ScreenCapturer(const CaptureOptions& options);
    ~X11ScreenCapturer() override;

    void start(Callback* callback) override;
    void stop() override;
    void captureFrame() override;
    bool getSourceList(SourceList* sources) override;
    bool selectSource(SourceId id) override;
    bool isSourceValid(SourceId id) override;
    SourceId selectedSource() const override;

private:
    Callback* callback_{nullptr};
    SourceId selectedSource_{1};
    bool started_{false};
};

class X11WindowCapturer : public DesktopCapturer {
public:
    explicit X11WindowCapturer(const CaptureOptions& options);
    ~X11WindowCapturer() override;

    void start(Callback* callback) override;
    void stop() override;
    void captureFrame() override;
    bool getSourceList(SourceList* sources) override;
    bool selectSource(SourceId id) override;
    bool isSourceValid(SourceId id) override;
    SourceId selectedSource() const override;

private:
    Callback* callback_{nullptr};
    SourceId selectedSource_{0};
    bool started_{false};
};

}  // namespace linux_x11
}  // namespace desktop_capture
}  // namespace links

#endif  // __linux__
#endif  // DESKTOP_CAPTURE_LINUX_X11_X11_CAPTURER_H_
