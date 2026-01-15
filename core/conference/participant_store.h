#ifndef CORE_CONFERENCE_PARTICIPANT_STORE_H
#define CORE_CONFERENCE_PARTICIPANT_STORE_H

#include <QList>
#include <QMap>
#include <QString>
#include "conference_types.h"

class ParticipantStore {
public:
    ParticipantInfo addParticipant(const QString& identity, const QString& sid, const QString& name);
    void removeParticipant(const QString& identity);
    bool contains(const QString& identity) const;
    ParticipantInfo participantInfo(const QString& identity) const;
    QList<ParticipantInfo> participants() const;
    int size() const;

    void clear();

    void setTrackSource(const QString& trackSid, livekit::TrackSource source);
    void setTrackKind(const QString& trackSid, livekit::TrackKind kind);
    void removeTrack(const QString& trackSid);
    bool hasTrackSource(const QString& trackSid) const;
    livekit::TrackSource trackSource(const QString& trackSid) const;
    livekit::TrackKind trackKind(const QString& trackSid) const;

    void setScreenShareActive(const QString& identity, bool active);
    bool screenShareActive(const QString& identity) const;

    ParticipantInfo refreshParticipantInfo(const QString& identity);

private:
    QMap<QString, ParticipantInfo> participants_;
    QMap<QString, livekit::TrackSource> trackSources_;
    QMap<QString, livekit::TrackKind> trackKinds_;
    QMap<QString, bool> screenShareActive_;
};

#endif // CORE_CONFERENCE_PARTICIPANT_STORE_H
