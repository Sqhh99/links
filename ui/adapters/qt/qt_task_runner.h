#ifndef QT_TASK_RUNNER_H
#define QT_TASK_RUNNER_H

#include <QObject>

#include "core/base/executor.h"

namespace links {
namespace qt_adapter {

/**
 * TaskRunner backed by the Qt event loop. Must be constructed on the main
 * thread; post() then delivers onto that thread.
 */
class QtTaskRunner : public QObject, public core::TaskRunner {
    Q_OBJECT
public:
    explicit QtTaskRunner(QObject* parent = nullptr);

    void post(std::function<void()> task) override;
    bool isCurrentThread() const override;
};

/**
 * BackgroundExecutor backed by Qt's global thread pool -- the same pool
 * QtConcurrent::run used for network stats polling.
 */
class QtBackgroundExecutor : public core::BackgroundExecutor {
public:
    void run(std::function<void()> job) override;
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_TASK_RUNNER_H
