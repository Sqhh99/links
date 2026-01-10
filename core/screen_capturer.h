#ifndef SCREEN_CAPTURER_H
#define SCREEN_CAPTURER_H

#include <QObject>
#include <QScreen>
#include <QWindow>
#include <QTimer>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include "livekit/video_source.h"

class ScreenCapturer : public QObject
{
    Q_OBJECT
public:
    enum class Mode {
        Screen,
        Window
    };

    explicit ScreenCapturer(QObject* parent = nullptr);
    ~ScreenCapturer() override;

    bool start();
    void stop();
    bool isActive() const { return isActive_; }

    void setMode(Mode mode) { mode_ = mode; }
    void setScreen(QScreen* screen) { screen_ = screen; windowId_ = 0; }
    void setWindow(WId windowId) { windowId_ = windowId; screen_ = nullptr; }

    std::shared_ptr<livekit::VideoSource> getVideoSource() const { return videoSource_; }

signals:
    void frameCaptured(const QImage& image);
    void error(const QString& message);

private slots:
    void captureOnce();

private:
    bool prepareTarget();
    bool validateWindowHandle() const;
    bool isWindowMinimized() const;
    QScreen* resolveScreenForWindow() const;
#ifdef Q_OS_WIN
    void runWinRtCapture(HWND hwnd);
#endif

    std::shared_ptr<livekit::VideoSource> videoSource_;
    QScreen* screen_{nullptr};
    WId windowId_{0};
    std::unique_ptr<QTimer> timer_;
    QImage lastValidFrame_;  // Cache last valid frame for minimized windows
#ifdef Q_OS_WIN
    class WinGraphicsWindowCapturer;
    std::unique_ptr<WinGraphicsWindowCapturer> winWindowCapturer_;
    std::atomic_bool winrtEnabled_{false};
    std::atomic_int winrtFailCount_{0};
    class DxgiWindowDuplicator;
    std::unique_ptr<DxgiWindowDuplicator> dxgiDuplicator_;
    int dxgiFailCount_{0};
    int dxgiSoftMissStreak_{0};
    std::thread winrtThread_;
    std::atomic_bool winrtStop_{false};
    HWND winrtHwnd_{nullptr};
    std::atomic_bool winrtRunning_{false};
    std::mutex winrtMutex_;
#endif

    bool isActive_{false};
    Mode mode_{Mode::Screen};
    int fps_{15};
    std::chrono::steady_clock::time_point lastFrameTime_;
    int stallRecoverMs_{5000};  // Increased to 5s to reduce unnecessary reinits and avoid UI blocking
    bool pendingResizeReset_{false};
    std::chrono::steady_clock::time_point lastResizeTime_;
    QRect lastWindowRect_;
};

#endif // SCREEN_CAPTURER_H
