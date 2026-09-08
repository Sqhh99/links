#include "room_controller.h"

RoomController::RoomController()
{
    ensureRoom();
}

livekit::Room* RoomController::room() const
{
    return room_.get();
}

void RoomController::setDelegate(livekit::RoomDelegate* delegate)
{
    delegate_ = delegate;
    ensureRoom();
    room_->setDelegate(delegate);
}

void RoomController::clearDelegate()
{
    if (room_) {
        room_->setDelegate(nullptr);
    }
}

bool RoomController::connectToRoom(const std::string& url,
                                   const std::string& token,
                                   const livekit::RoomOptions& options)
{
    ensureRoom();
    return room_->connect(url, token, options);
}

void RoomController::disconnectFromRoom(livekit::DisconnectReason reason)
{
    if (!room_) {
        return;
    }

    // Must never be called from inside a RoomDelegate callback: the SDK documents
    // that as a deadlock of its event listener. All delegate events reach us through
    // queued connections, so callers are always on the main thread.
    room_->disconnect(reason);
}

void RoomController::reset()
{
    room_.reset();
    ensureRoom();
}

livekit::RoomInfoData RoomController::roomInfo() const
{
    if (!room_) {
        return livekit::RoomInfoData{};
    }
    return room_->roomInfo();
}

std::shared_ptr<livekit::LocalParticipant> RoomController::localParticipant() const
{
    return room_ ? room_->localParticipant().lock() : nullptr;
}

std::shared_ptr<livekit::RemoteParticipant> RoomController::remoteParticipant(const std::string& identity) const
{
    return room_ ? room_->remoteParticipant(identity).lock() : nullptr;
}

std::vector<std::shared_ptr<livekit::RemoteParticipant>> RoomController::remoteParticipants() const
{
    std::vector<std::shared_ptr<livekit::RemoteParticipant>> participants;
    if (!room_) {
        return participants;
    }

    const auto handles = room_->remoteParticipants();
    participants.reserve(handles.size());
    for (const auto& handle : handles) {
        if (auto participant = handle.lock()) {
            participants.push_back(std::move(participant));
        }
    }
    return participants;
}

void RoomController::ensureRoom()
{
    if (room_) {
        return;
    }

    room_ = std::make_unique<livekit::Room>();
    if (delegate_) {
        room_->setDelegate(delegate_);
    }
}
