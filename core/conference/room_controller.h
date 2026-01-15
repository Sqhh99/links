#ifndef CORE_CONFERENCE_ROOM_CONTROLLER_H
#define CORE_CONFERENCE_ROOM_CONTROLLER_H

#include <memory>
#include <vector>
#include <QString>
#include "livekit/room.h"
#include "livekit/livekit.h"

class RoomController {
public:
    RoomController();

    livekit::Room* room() const;
    void setDelegate(livekit::RoomDelegate* delegate);
    void clearDelegate();

    bool connectToRoom(const QString& url, const QString& token, const livekit::RoomOptions& options);
    void reset();

    livekit::RoomInfoData roomInfo() const;
    livekit::LocalParticipant* localParticipant() const;
    std::vector<std::shared_ptr<livekit::RemoteParticipant>> remoteParticipants() const;

private:
    std::unique_ptr<livekit::Room> room_;
};

#endif // CORE_CONFERENCE_ROOM_CONTROLLER_H
