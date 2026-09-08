#include "qt_task_runner.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>

namespace links {
namespace qt_adapter {

QtTaskRunner::QtTaskRunner(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(QCoreApplication::instance() != nullptr);
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
}

void QtTaskRunner::post(std::function<void()> task)
{
    // Qt::QueuedConnection is always asynchronous and always delivers on this
    // object's thread -- identical to the invokeMethod call MediaPipeline made
    // inline before, and required by the disconnect deadlock note in
    // core/base/executor.h.
    QMetaObject::invokeMethod(this, std::move(task), Qt::QueuedConnection);
}

bool QtTaskRunner::isCurrentThread() const
{
    return QThread::currentThread() == thread();
}

void QtBackgroundExecutor::run(std::function<void()> job)
{
    QThreadPool::globalInstance()->start(QRunnable::create(std::move(job)));
}

}  // namespace qt_adapter
}  // namespace links
