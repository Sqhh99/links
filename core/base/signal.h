#ifndef CORE_BASE_SIGNAL_H
#define CORE_BASE_SIGNAL_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// IMPORTANT: this header is included from translation units that also include
// Qt (every ui/backend/*.cpp). Qt defines `emit`, `signals`, `slots`,
// `foreach` and `forever` as preprocessor macros, so none of those words may
// appear as an identifier here. That is why the multicast entry point is
// called notify() and not emit().
// ---------------------------------------------------------------------------

namespace links {
namespace core {

namespace detail {
struct SlotListBase {
    virtual ~SlotListBase() = default;
    virtual void detach(std::uint64_t id) = 0;
};
}  // namespace detail

/**
 * RAII subscription handle. Destroying it unsubscribes.
 *
 * This is the replacement for Qt's context-object auto-disconnect: with
 * connect(sender, sig, receiver, slot), Qt drops the connection when either
 * object dies. Nothing does that for a bare std::function, so callbacks
 * outliving their receiver is the main hazard introduced by dropping QObject.
 */
class Connection {
public:
    Connection() = default;
    Connection(std::weak_ptr<detail::SlotListBase> owner, std::uint64_t id)
        : owner_(std::move(owner)), id_(id) {}

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept { *this = std::move(other); }
    Connection& operator=(Connection&& other) noexcept
    {
        if (this != &other) {
            release();
            owner_ = std::move(other.owner_);
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }

    ~Connection() { release(); }

    void release()
    {
        if (id_ == 0) {
            return;
        }
        if (auto owner = owner_.lock()) {
            owner->detach(id_);
        }
        id_ = 0;
        owner_.reset();
    }

    bool active() const { return id_ != 0 && !owner_.expired(); }

private:
    std::weak_ptr<detail::SlotListBase> owner_;
    std::uint64_t id_{0};
};

/**
 * Holds many Connections. Destroying or clearing it unsubscribes all of them.
 *
 * Every subscriber declares one of these and calls clear() as the FIRST
 * statement of its destructor, so no callback can arrive while its members are
 * being torn down.
 */
class ConnectionBag {
public:
    ConnectionBag() = default;
    ConnectionBag(const ConnectionBag&) = delete;
    ConnectionBag& operator=(const ConnectionBag&) = delete;

    ConnectionBag& operator+=(Connection connection)
    {
        items_.push_back(std::move(connection));
        return *this;
    }

    void clear() { items_.clear(); }
    std::size_t size() const { return items_.size(); }

private:
    std::vector<Connection> items_;
};

/**
 * Typed multicast callback list -- the replacement for a Qt signal.
 *
 * Not thread-safe by design: connect() and notify() must run on the same
 * thread (the application main thread). Producers on other threads hop through
 * TaskRunner::post first, which is exactly what Qt::QueuedConnection did.
 */
template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    Signal() : list_(std::make_shared<List>()) {}
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    [[nodiscard]] Connection connect(Slot slot)
    {
        const std::uint64_t id = ++list_->nextId;
        list_->entries.push_back(Entry{id, std::move(slot)});
        return Connection(std::static_pointer_cast<detail::SlotListBase>(list_), id);
    }

    /// For subscribers that provably outlive this signal (typically the owner).
    void connectPermanent(Slot slot)
    {
        const std::uint64_t id = ++list_->nextId;
        list_->entries.push_back(Entry{id, std::move(slot)});
    }

    /**
     * Invoke every connected slot. Safe against slots that connect, disconnect,
     * or destroy the emitter while running -- Qt tolerates all three and some
     * existing handlers do exactly that.
     */
    void notify(Args... args) const
    {
        auto list = list_;  // keep the list alive even if the emitter dies
        ++list->depth;
        const std::size_t initial = list->entries.size();
        for (std::size_t i = 0; i < initial && i < list->entries.size(); ++i) {
            const Slot& slot = list->entries[i].slot;
            if (slot) {
                slot(args...);
            }
        }
        if (--list->depth == 0) {
            list->compact();
        }
    }

    void operator()(Args... args) const { notify(args...); }

    bool hasSlots() const { return !list_->entries.empty(); }

private:
    struct Entry {
        std::uint64_t id;
        Slot slot;
    };

    struct List : detail::SlotListBase {
        std::vector<Entry> entries;
        std::uint64_t nextId{0};
        int depth{0};

        void detach(std::uint64_t id) override
        {
            for (auto& entry : entries) {
                if (entry.id == id) {
                    entry.slot = nullptr;  // tombstone; compacted when not iterating
                    break;
                }
            }
            if (depth == 0) {
                compact();
            }
        }

        void compact()
        {
            entries.erase(std::remove_if(entries.begin(), entries.end(),
                                         [](const Entry& e) { return !e.slot; }),
                          entries.end());
        }
    };

    std::shared_ptr<List> list_;
};

}  // namespace core
}  // namespace links

#endif  // CORE_BASE_SIGNAL_H
