#ifndef DESKTOP_CAPTURE_MAC_MAC_CAPTURER_H_
#define DESKTOP_CAPTURE_MAC_MAC_CAPTURER_H_

#ifdef __APPLE__

#include "../desktop_capturer.h"

namespace links {
namespace desktop_capture {
namespace mac {

class MacScreenCapturer : public DesktopCapturer {
public:
    explicit MacScreenCapturer(const CaptureOptions& options);
    ~MacScreenCapturer() override;

    void start(Callback* callback) override;
    void stop() override;
    void captureFrame() override;
    bool getSourceList(SourceList* sources) override;
    bool selectSource(SourceId id) override;
    bool isSourceValid(SourceId id) override;
    SourceId selectedSource() const override;

private:
    void logBackendSelectionIfNeeded();

    Callback* callback_{nullptr};
    SourceId selectedSource_{0};
    bool started_{false};
    CaptureBackend loggedBackend_{CaptureBackend::Unknown};
};

class MacWindowCapturer : public DesktopCapturer {
public:
    explicit MacWindowCapturer(const CaptureOptions& options);
    ~MacWindowCapturer() override;

    void start(Callback* callback) override;
    void stop() override;
    void captureFrame() override;
    bool getSourceList(SourceList* sources) override;
    bool selectSource(SourceId id) override;
    bool isSourceValid(SourceId id) override;
    SourceId selectedSource() const override;

private:
    void logBackendSelectionIfNeeded();

    Callback* callback_{nullptr};
    SourceId selectedSource_{0};
    bool started_{false};
    CaptureBackend loggedBackend_{CaptureBackend::Unknown};
};

}  // namespace mac
}  // namespace desktop_capture
}  // namespace links

#endif  // __APPLE__
#endif  // DESKTOP_CAPTURE_MAC_MAC_CAPTURER_H_
