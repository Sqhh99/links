#ifndef QT_TIMER_FACTORY_H
#define QT_TIMER_FACTORY_H

#include <QObject>

#include "core/base/timer.h"

namespace links {
namespace qt_adapter {

/**
 * TimerFactory backed by QTimer, so every core timer keeps running on the Qt
 * event loop with the timing behaviour it has today.
 */
class QtTimerFactory : public core::TimerFactory {
public:
    explicit QtTimerFactory(QObject* context = nullptr);

    std::unique_ptr<core::Timer> createTimer(std::function<void()> onFire) override;
    void singleShot(std::chrono::milliseconds delay, std::function<void()> task) override;

private:
    QObject* context_{nullptr};
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_TIMER_FACTORY_H
