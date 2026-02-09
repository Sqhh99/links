#include "LoginBackend.h"

#include "../utils/logger.h"
#include "../utils/settings.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

LoginBackend::LoginBackend(QObject* parent)
    : QObject(parent),
      networkClient_(new NetworkClient(this))
{
    connect(networkClient_, &NetworkClient::tokenReceived,
            this, &LoginBackend::onTokenReceived);
    connect(networkClient_, &NetworkClient::meetingCreated,
            this, &LoginBackend::onMeetingCreated);
    connect(networkClient_, &NetworkClient::meetingRecordsReceived,
            this, &LoginBackend::onMeetingRecordsReceived);
    connect(networkClient_, &NetworkClient::error,
            this, &LoginBackend::onNetworkError);

    networkClient_->setApiUrl(Settings::instance().getSignalingServerUrl());
    loadSettings();
}

void LoginBackend::setUserName(const QString& name)
{
    if (userName_ != name) {
        userName_ = name;
        emit userNameChanged();
    }
}

void LoginBackend::setRoomName(const QString& name)
{
    if (roomName_ != name) {
        roomName_ = name;
        emit roomNameChanged();
    }
}

void LoginBackend::setMicEnabled(bool enabled)
{
    if (micEnabled_ != enabled) {
        micEnabled_ = enabled;
        emit micEnabledChanged();
    }
}

void LoginBackend::setCamEnabled(bool enabled)
{
    if (camEnabled_ != enabled) {
        camEnabled_ = enabled;
        emit camEnabledChanged();
    }
}

void LoginBackend::setScheduledTime(const QString& time)
{
    if (scheduledTime_ != time) {
        scheduledTime_ = time;
        emit scheduledTimeChanged();
    }
}

void LoginBackend::setLoading(bool loading)
{
    if (loading_ != loading) {
        loading_ = loading;
        emit loadingChanged();
    }
}

void LoginBackend::setErrorMessage(const QString& message)
{
    if (errorMessage_ != message) {
        errorMessage_ = message;
        emit errorMessageChanged();
    }
}

bool LoginBackend::hasAuthToken() const
{
    return !authToken().isEmpty();
}

QString LoginBackend::authToken() const
{
    return Settings::instance().getAuthToken().trimmed();
}

bool LoginBackend::isMeetingNo(const QString& value)
{
    static const QRegularExpression pattern(QStringLiteral("^\\d{9}$"));
    return pattern.match(value.trimmed()).hasMatch();
}

QString LoginBackend::extractMeetingNo(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (isMeetingNo(trimmed)) {
        return trimmed;
    }

    const QUrl url(trimmed);
    if (!url.isValid()) {
        return QString{};
    }

    const QUrlQuery query(url);
    const QString meetingNo = query.queryItemValue(QStringLiteral("meetingNo")).trimmed();
    return isMeetingNo(meetingNo) ? meetingNo : QString{};
}

void LoginBackend::join()
{
    const QString name = userName_.trimmed();
    const QString input = roomName_.trimmed();

    if (name.isEmpty()) {
        setErrorMessage("Please enter your name");
        return;
    }

    if (input.isEmpty()) {
        setErrorMessage("Please enter a meeting number, share link, or room name");
        return;
    }

    saveSettings();
    setLoading(true);
    setErrorMessage("");

    const QString resolvedMeetingNo = extractMeetingNo(input);
    if (hasAuthToken()) {
        if (resolvedMeetingNo.isEmpty()) {
            setLoading(false);
            setErrorMessage("Please enter a valid 9-digit meeting number or share link");
            return;
        }

        Logger::instance().info(QString("Joining via meetingNo: %1").arg(resolvedMeetingNo));
        networkClient_->joinMeeting(resolvedMeetingNo, name, authToken());
        return;
    }

    Logger::instance().info(QString("Requesting token for room '%1', user '%2'")
                               .arg(input, name));
    networkClient_->requestToken(input, name);
}

void LoginBackend::quickJoin()
{
    const QString name = userName_.trimmed();

    if (name.isEmpty()) {
        setErrorMessage("Please enter your name");
        return;
    }

    setLoading(true);
    setErrorMessage("");

    if (hasAuthToken()) {
        Logger::instance().info("Creating meeting for quick join");
        networkClient_->createMeeting(authToken());
        return;
    }

    const QString randomRoom = QString("room-%1").arg(QDateTime::currentMSecsSinceEpoch());
    setRoomName(randomRoom);
    join();
}

void LoginBackend::createScheduledRoom()
{
    const QString name = userName_.trimmed();

    if (name.isEmpty()) {
        setErrorMessage("Please enter your name");
        return;
    }

    setLoading(true);
    setErrorMessage("");

    if (hasAuthToken()) {
        Logger::instance().info("Creating scheduled meeting via /api/meetings");
        networkClient_->createMeeting(authToken());
        return;
    }

    const QString note = scheduledTime_.trimmed();
    const QString suffix = note.isEmpty() ? QString::number(QDateTime::currentMSecsSinceEpoch())
                                          : note.simplified().replace(" ", "-");
    const QString privateRoom = QString("scheduled-%1").arg(suffix);
    setRoomName(privateRoom);

    join();
}

void LoginBackend::loadMeetingRecords()
{
    if (!hasAuthToken()) {
        emit meetingRecordsLoaded(QVariantList{});
        return;
    }

    networkClient_->fetchMeetingRecords(authToken(), 1, 20);
}

void LoginBackend::showSettings()
{
    emit settingsRequested();
}

QString LoginBackend::currentTime() const
{
    return QDateTime::currentDateTime().toString("HH:mm");
}

QString LoginBackend::currentDate() const
{
    return QDateTime::currentDateTime().toString("yyyy年MM月dd日 dddd");
}

void LoginBackend::onTokenReceived(const TokenResponse& response)
{
    setLoading(false);

    if (!response.success) {
        setErrorMessage("Failed to get token: " + response.error);
        Logger::instance().error("Token request failed: " + response.error);
        return;
    }

    Logger::instance().info("Token received, joining conference");
    emit joinConference(response.url,
                        response.token,
                        response.roomName,
                        response.meetingNo,
                        userName_,
                        response.isHost);

    loadMeetingRecords();
}

void LoginBackend::onMeetingCreated(const QString& meetingNo, const QString& roomName, const QString& shareUrl)
{
    const QString name = userName_.trimmed();
    if (name.isEmpty()) {
        setLoading(false);
        setErrorMessage("Please enter your name");
        return;
    }

    const QString resolvedMeetingNo = !meetingNo.isEmpty() ? meetingNo : extractMeetingNo(shareUrl);
    if (!resolvedMeetingNo.isEmpty()) {
        setRoomName(resolvedMeetingNo);
    } else {
        setRoomName(roomName);
    }
    saveSettings();

    if (!resolvedMeetingNo.isEmpty() && hasAuthToken()) {
        networkClient_->joinMeeting(resolvedMeetingNo, name, authToken());
        return;
    }

    if (!roomName.isEmpty() && !hasAuthToken()) {
        networkClient_->requestToken(roomName, name);
        return;
    }

    setLoading(false);
    setErrorMessage("Meeting created but no room information returned");
}

void LoginBackend::onMeetingRecordsReceived(const QJsonArray& records)
{
    QVariantList formatted;
    formatted.reserve(records.size());

    for (const QJsonValue& item : records) {
        if (!item.isObject()) {
            continue;
        }

        const QJsonObject obj = item.toObject();
        const QString meetingNo = obj.value("meetingNo").toString();
        const QString roomName = obj.value("roomName").toString();
        const QString meetingStatus = obj.value("meetingStatus").toString();
        const QString lastJoinedAt = obj.value("lastJoinedAt").toString();
        const int joinCount = obj.value("joinCount").toInt(0);

        QString timeText = lastJoinedAt;
        const QDateTime joinedTime = QDateTime::fromString(lastJoinedAt, Qt::ISODate);
        if (joinedTime.isValid()) {
            timeText = joinedTime.toLocalTime().toString("yyyy-MM-dd HH:mm");
        }

        QString title;
        if (!meetingNo.isEmpty()) {
            title = QString("会议号 %1").arg(meetingNo);
        } else if (!roomName.isEmpty()) {
            title = roomName;
        } else {
            title = QStringLiteral("会议记录");
        }

        if (joinCount > 1) {
            title += QString(" · %1次").arg(joinCount);
        }

        QString tag;
        if (meetingStatus == "active") {
            tag = QStringLiteral("进行中");
        } else if (meetingStatus == "ended") {
            tag = QStringLiteral("已结束");
        } else {
            tag = meetingStatus;
        }

        QVariantMap row;
        row.insert("title", title);
        row.insert("time", timeText);
        row.insert("tag", tag);
        row.insert("meetingNo", meetingNo);
        row.insert("roomName", roomName);
        formatted.append(row);
    }

    emit meetingRecordsLoaded(formatted);
}

void LoginBackend::onNetworkError(const QString& error)
{
    setLoading(false);
    setErrorMessage("Network error: " + error);
}

void LoginBackend::saveSettings()
{
    Settings::instance().setLastUserName(userName_);
    Settings::instance().setLastRoomName(roomName_);
}

void LoginBackend::loadSettings()
{
    setUserName(Settings::instance().getLastUserName());
    setRoomName(Settings::instance().getLastRoomName());
}
