#ifndef ROOM_EVENT_DELEGATE_H
#define ROOM_EVENT_DELEGATE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/executor.h"
#include "base/signal.h"
#include "livekit/room_delegate.h"
#include "livekit/room_event_types.h"

/**
 * Bridges livekit::RoomDelegate onto the application main thread.
 *
 * The SDK invokes the overrides below on its own threads. Each one reads
 * everything it needs off the SDK objects immediately (still on the SDK
 * thread), then posts an owned-value copy to the main thread, where the
 * matching signal is notified. This is the same contract the previous
 * Qt::QueuedConnection signals provided -- see CLAUDE.md, "Threading rule".
 *
 * Two invariants matter and are easy to break:
 *  - Only owned values may be captured into a posted task. Never a reference
 *    into an SDK event, which dies when the callback returns.
 *  - lifetime_ is declared last, so it is destroyed first and cancels every
 *    in-flight post before the rest of the object is torn down. This replaces
 *    Qt discarding queued events aimed at a destroyed QObject.
 */
class RoomEventDelegate : public livekit::RoomDelegate {
public:
    explicit RoomEventDelegate(links::core::TaskRunner& taskRunner);
    ~RoomEventDelegate() override = default;

    // RoomDelegate overrides - these are called from LiveKit SDK threads
    void onParticipantConnected(livekit::Room& room,
                                const livekit::ParticipantConnectedEvent& event) override;
    void onParticipantDisconnected(livekit::Room& room,
                                   const livekit::ParticipantDisconnectedEvent& event) override;
    void onTrackSubscribed(livekit::Room& room,
                          const livekit::TrackSubscribedEvent& event) override;
    void onTrackUnsubscribed(livekit::Room& room,
                            const livekit::TrackUnsubscribedEvent& event) override;
    void onTrackMuted(livekit::Room& room,
                      const livekit::TrackMutedEvent& event) override;
    void onTrackUnmuted(livekit::Room& room,
                        const livekit::TrackUnmutedEvent& event) override;
    void onTrackUnpublished(livekit::Room& room,
                            const livekit::TrackUnpublishedEvent& event) override;
    void onConnectionQualityChanged(livekit::Room& room,
                                    const livekit::ConnectionQualityChangedEvent& event) override;
    void onConnectionStateChanged(livekit::Room& room,
                                  const livekit::ConnectionStateChangedEvent& event) override;
    void onDisconnected(livekit::Room& room,
                        const livekit::DisconnectedEvent& event) override;
    void onUserPacketReceived(livekit::Room& room,
                              const livekit::UserDataPacketEvent& event) override;
    void onLocalTrackPublished(livekit::Room& room,
                               const livekit::LocalTrackPublishedEvent& event) override;

    // All of these are notified on the main thread.
    links::core::Signal<std::string, std::string, std::string, bool> participantConnected;
    links::core::Signal<std::string, int> participantDisconnected;
    links::core::Signal<std::string, std::string, int, int, bool,
                        std::shared_ptr<livekit::Track>,
                        std::shared_ptr<livekit::RemoteTrackPublication>> trackSubscribed;
    links::core::Signal<std::string, std::string> trackUnsubscribed;
    links::core::Signal<std::string, std::string, int> trackMuted;
    links::core::Signal<std::string, std::string, int> trackUnmuted;
    links::core::Signal<std::string, std::string, int, int> trackUnpublished;
    links::core::Signal<std::string, int> connectionQualityChanged;
    links::core::Signal<int> connectionStateChanged;
    links::core::Signal<int> roomDisconnected;
    links::core::Signal<std::vector<std::uint8_t>, std::string, std::string> dataReceived;
    links::core::Signal<std::string, int, int> localTrackPublished;

private:
    links::core::TaskRunner& taskRunner_;

    // Must stay the last member: destroyed first, invalidating pending posts.
    links::core::LifetimeToken lifetime_;
};

#endif // ROOM_EVENT_DELEGATE_H
