#ifndef CORE_BASE_TIMER_H
#define CORE_BASE_TIMER_H

#include <chrono>
#include <functional>
#include <memory>

namespace links {
namespace core {

/**
 * A timer driven by the application event loop. Destroying it cancels the
 * timer; the callback never fires afterwards.
 */
class Timer {
public:
    virtual ~Timer() = default;
    virtual void start(std::chrono::milliseconds interval, bool repeat) = 0;
    virtual void stop() = 0;
    virtual bool isActive() const = 0;
};

class TimerFactory {
public:
    virtual ~TimerFactory() = default;

    /// The returned Timer fires `onFire` on the main thread.
    virtual std::unique_ptr<Timer> createTimer(std::function<void()> onFire) = 0;

    /// One-shot convenience; replaces QTimer::singleShot.
    virtual void singleShot(std::chrono::milliseconds delay, std::function<void()> task) = 0;
};

}  // namespace core
}  // namespace links

#endif  // CORE_BASE_TIMER_H
