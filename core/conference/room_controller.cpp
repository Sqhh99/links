#include "room_controller.h"

RoomController::RoomController()
    : room_(std::make_unique<livekit::Room>())
{
}

livekit::Room* RoomController::room() const
{
    return room_.get();
}

void RoomController::setDelegate(livekit::RoomDelegate* delegate)
{
    if (room_) {
        room_->setDelegate(delegate);
    }
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
    return room_->Connect(url.toStdString(), token.toStdString(), options);
}

void RoomController::reset()
{
    room_.reset();
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
