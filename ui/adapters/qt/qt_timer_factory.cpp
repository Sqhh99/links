#include "qt_timer_factory.h"

#include <QCoreApplication>
#include <QTimer>

#include <utility>

namespace links {
namespace qt_adapter {
namespace {

class QtTimer : public core::Timer {
public:
    explicit QtTimer(std::function<void()> onFire)
        : timer_(std::make_unique<QTimer>())
    {
        QObject::connect(timer_.get(), &QTimer::timeout,
                         timer_.get(), [fire = std::move(onFire)]() {
                             if (fire) {
                                 fire();
                             }
                         });
    }

    // Destroying the QTimer stops it, so no queued timeout can arrive
    // afterwards.
    ~QtTimer() override = default;

    void start(std::chrono::milliseconds interval, bool repeat) override
    {
        timer_->setSingleShot(!repeat);
        timer_->start(static_cast<int>(interval.count()));
    }

    void stop() override { timer_->stop(); }
    bool isActive() const override { return timer_->isActive(); }

private:
    std::unique_ptr<QTimer> timer_;
};

}  // namespace

QtTimerFactory::QtTimerFactory(QObject* context)
    : context_(context ? context : QCoreApplication::instance())
{
}

std::unique_ptr<core::Timer> QtTimerFactory::createTimer(std::function<void()> onFire)
{
    return std::make_unique<QtTimer>(std::move(onFire));
}

void QtTimerFactory::singleShot(std::chrono::milliseconds delay, std::function<void()> task)
{
    QTimer::singleShot(static_cast<int>(delay.count()), context_, std::move(task));
}

}  // namespace qt_adapter
}  // namespace links
