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

bool RoomController::connectToRoom(const QString& url,
                                   const QString& token,
                                   const livekit::RoomOptions& options)
{
    ensureRoom();
    return room_->Connect(url.toStdString(), token.toStdString(), options);
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
    return room_->room_info();
}

livekit::LocalParticipant* RoomController::localParticipant() const
{
    return room_ ? room_->localParticipant() : nullptr;
}

std::vector<std::shared_ptr<livekit::RemoteParticipant>> RoomController::remoteParticipants() const
{
    return room_ ? room_->remoteParticipants() : std::vector<std::shared_ptr<livekit::RemoteParticipant>>{};
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
