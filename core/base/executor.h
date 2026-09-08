#ifndef CORE_BASE_EXECUTOR_H
#define CORE_BASE_EXECUTOR_H

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace links {
namespace core {

/**
 * Marshals work onto the application main thread.
 *
 * Replaces the Qt::QueuedConnection hop that RoomEventDelegate and
 * MediaPipeline rely on today.
 */
class TaskRunner {
public:
    virtual ~TaskRunner() = default;

    /**
     * Post work to the main thread. ALWAYS asynchronous, even when called from
     * the main thread -- exactly like Qt::QueuedConnection.
     *
     * Do not "optimise" this into a direct call when already on the main
     * thread. RoomController::disconnectFromRoom documents a deadlock inside
     * livekit::Room::disconnect that is avoided only because the caller's
     * stack (an SDK thread holding SDK locks) unwinds before the handler runs.
     */
    virtual void post(std::function<void()> task) = 0;

    virtual bool isCurrentThread() const = 0;
};

/**
 * Runs work off the main thread. The Qt adapter uses the same global thread
 * pool QtConcurrent::run uses today, so the concurrency profile is unchanged.
 */
class BackgroundExecutor {
public:
    virtual ~BackgroundExecutor() = default;
    virtual void run(std::function<void()> job) = 0;
};

/**
 * Lifetime sentinel for cross-thread posts.
 *
 * Declare one as the LAST member of any object that posts to a TaskRunner, so
 * it is destroyed first and invalidates every in-flight task before the rest
 * of the object is torn down. This reproduces Qt's "queued events to a
 * destroyed QObject are discarded".
 */
class LifetimeToken {
public:
    LifetimeToken() : alive_(std::make_shared<char>(0)) {}
    LifetimeToken(const LifetimeToken&) = delete;
    LifetimeToken& operator=(const LifetimeToken&) = delete;

    std::weak_ptr<char> weak() const { return alive_; }

private:
    std::shared_ptr<char> alive_;
};

/**
 * Run `fn` on the main thread, but only if the token's owner is still alive
 * when the task is dispatched.
 *
 * Correct because destruction also happens on the main thread: once the task
 * is running, nothing can destroy the owner underneath it.
 */
template <typename Fn>
void postGuarded(TaskRunner& runner, const LifetimeToken& token, Fn&& fn)
{
    // std::function requires a copy-constructible target, but these tasks
    // routinely capture move-only payloads -- a livekit::VideoFrameEvent, for
    // instance. Holding the callable through a shared_ptr makes the wrapper
    // copyable without copying the payload.
    auto held = std::make_shared<std::decay_t<Fn>>(std::forward<Fn>(fn));
    runner.post([weak = token.weak(), held = std::move(held)]() {
        if (weak.expired()) {
            return;
        }
        (*held)();
    });
}

}  // namespace core
}  // namespace links

#endif  // CORE_BASE_EXECUTOR_H
