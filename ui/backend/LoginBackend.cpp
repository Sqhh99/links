#include "LoginBackend.h"

#include "../utils/logger.h"
#include "../utils/settings.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
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
    connect(networkClient_, &NetworkClient::authExpired,
            this, &LoginBackend::onAuthExpired);
    connect(networkClient_, &NetworkClient::error,
            this, &LoginBackend::onNetworkError);

    networkClient_->setApiUrl(Settings::instance().getSignalingServerUrl());
    loadSettings();
}

void LoginBackend::setUserName(const QString& name)
{
    QString nextName = name.trimmed();
    if (!hasAuthToken()) {
        nextName = ensureGuestDisplayName();
    }

    if (userName_ != nextName) {
        userName_ = nextName;
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

void LoginBackend::setAllowGuestJoin(bool enabled)
{
    if (allowGuestJoin_ != enabled) {
        allowGuestJoin_ = enabled;
        emit allowGuestJoinChanged();
    }
}

void LoginBackend::setSessionLoggedIn(bool loggedIn)
{
    if (sessionLoggedIn_ == loggedIn) {
        return;
    }

    sessionLoggedIn_ = loggedIn;
    emit sessionLoggedInChanged();
    syncParticipantNameFromSession();
}

void LoginBackend::setSessionAuthToken(const QString& token)
{
    const QString trimmed = token.trimmed();
    if (sessionAuthToken_ == trimmed) {
        return;
    }

    sessionAuthToken_ = trimmed;
    emit sessionAuthTokenChanged();
    syncParticipantNameFromSession();
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
    return sessionLoggedIn_ && !authToken().isEmpty();
}

QString LoginBackend::authToken() const
{
    return sessionAuthToken_.trimmed();
}

bool LoginBackend::isGuestMode() const
{
    return !hasAuthToken();
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
    const QString name = effectiveParticipantName(true);
    const QString input = roomName_.trimmed();
    const QString resolvedMeetingNo = extractMeetingNo(input);
    const bool authed = hasAuthToken();

    if (input.isEmpty()) {
        setErrorMessage(!authed
                            ? "请输入会议号、分享链接或普通房间名称"
                            : "Please enter a meeting number, share link, or room name");
        return;
    }

    if (authed && resolvedMeetingNo.isEmpty()) {
        setErrorMessage("Please enter a valid 9-digit meeting number or share link");
        return;
    }

    if (!authed && resolvedMeetingNo.isEmpty() && isBusinessMeetingInput(input)) {
        setErrorMessage("请输入会议号或分享链接加入业务会议");
        return;
    }

    saveSettings();
    setLoading(true);
    setErrorMessage("");

    if (authed) {
        Logger::instance().info(QString("Joining via meetingNo: %1").arg(resolvedMeetingNo));
        pendingParticipantName_ = name;
        networkClient_->joinMeeting(resolvedMeetingNo, name, authToken().trimmed());
        return;
    }

    if (!resolvedMeetingNo.isEmpty()) {
        Logger::instance().info(QString("Guest joining via meetingNo: %1").arg(resolvedMeetingNo));
        pendingParticipantName_ = name;
        networkClient_->guestJoinMeeting(resolvedMeetingNo, name);
        return;
    }

    Logger::instance().info(QString("Requesting token for room '%1', user '%2'")
                               .arg(input, name));
    pendingParticipantName_ = name;
    networkClient_->requestToken(input, name);
}

void LoginBackend::quickJoin()
{
    if (isGuestMode()) {
        setErrorMessage("游客无法创建快速会议，请登录后使用");
        return;
    }

    const QString name = effectiveParticipantName(true);

    setLoading(true);
    setErrorMessage("");

    if (hasAuthToken()) {
        Logger::instance().info("Creating meeting for quick join");
        networkClient_->createMeeting(authToken(), allowGuestJoin_);
        return;
    }

    const QString randomRoom = QString("room-%1").arg(QDateTime::currentMSecsSinceEpoch());
    setRoomName(randomRoom);
    pendingParticipantName_ = name;
    join();
}

void LoginBackend::createScheduledRoom()
{
    if (isGuestMode()) {
        setErrorMessage("游客无法预定会议，请登录后使用");
        return;
    }

    const QString name = effectiveParticipantName(true);

    setLoading(true);
    setErrorMessage("");

    if (hasAuthToken()) {
        Logger::instance().info("Creating scheduled meeting via /api/meetings");
        networkClient_->createMeeting(authToken(), false);
        return;
    }

    const QString note = scheduledTime_.trimmed();
    const QString suffix = note.isEmpty() ? QString::number(QDateTime::currentMSecsSinceEpoch())
                                          : note.simplified().replace(" ", "-");
    const QString privateRoom = QString("scheduled-%1").arg(suffix);
    setRoomName(privateRoom);

    pendingParticipantName_ = name;
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

void LoginBackend::syncParticipantNameFromSession()
{
    setUserName(hasAuthToken() ? defaultAuthDisplayName() : ensureGuestDisplayName());
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
        pendingParticipantName_.clear();
        QString mappedMessage;
        const QString lowerError = response.error.toLower();
        if (isGuestMode()) {
            if (lowerError.contains("403") || lowerError.contains("forbidden")) {
                mappedMessage = "该会议未开放游客加入";
            } else if (lowerError.contains("404") || lowerError.contains("not found")) {
                mappedMessage = "会议或房间不存在，请确认输入";
            } else if (lowerError.contains("409") || lowerError.contains("conflict")) {
                mappedMessage = "会议已结束";
            } else if (lowerError.contains("400") || lowerError.contains("bad request")) {
                mappedMessage = "请输入有效的会议号、分享链接或普通房间名称";
            }
        }

        setErrorMessage(mappedMessage.isEmpty()
                            ? ("Failed to get token: " + response.error)
                            : mappedMessage);
        Logger::instance().error("Token request failed: " + response.error);
        return;
    }

    const QString participantName = pendingParticipantName_.trimmed().isEmpty()
        ? effectiveParticipantName(true)
        : pendingParticipantName_.trimmed();
    pendingParticipantName_.clear();

    Logger::instance().info("Token received, joining conference");
    emit joinConference(response.url,
                        response.token,
                        response.roomName,
                        response.meetingNo,
                        participantName,
                        response.isHost,
                        authToken(),
                        isGuestMode());

    loadMeetingRecords();
}

void LoginBackend::onMeetingCreated(const QString& meetingNo, const QString& roomName, const QString& shareUrl)
{
    const QString name = effectiveParticipantName(true);

    const QString resolvedMeetingNo = !meetingNo.isEmpty() ? meetingNo : extractMeetingNo(shareUrl);
    if (!resolvedMeetingNo.isEmpty()) {
        setRoomName(resolvedMeetingNo);
    } else {
        setRoomName(roomName);
    }
    saveSettings();

    if (!resolvedMeetingNo.isEmpty() && hasAuthToken()) {
        pendingParticipantName_ = name;
        networkClient_->joinMeeting(resolvedMeetingNo, name, authToken());
        return;
    }

    if (!roomName.isEmpty() && !hasAuthToken()) {
        pendingParticipantName_ = name;
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
    if (error.contains(QStringLiteral("HTTP 401"))) {
        return;
    }
    setErrorMessage("Network error: " + error);
}

void LoginBackend::onAuthExpired(const QString& message)
{
    setLoading(false);
    setErrorMessage("登录已过期，请重新登录");
    if (sessionLoggedIn_ || !sessionAuthToken_.isEmpty()) {
        emit sessionExpired(message);
    }
}

void LoginBackend::saveSettings()
{
    Settings::instance().setLastRoomName(roomName_);
    if (hasAuthToken()) {
        Settings::instance().setLastUserName(userName_);
    }
}

void LoginBackend::loadSettings()
{
    setRoomName(Settings::instance().getLastRoomName());
    syncParticipantNameFromSession();
}

QString LoginBackend::ensureGuestDisplayName()
{
    if (!guestDisplayName_.isEmpty()) {
        return guestDisplayName_;
    }

    const quint32 value = QRandomGenerator::global()->bounded(0x10000u);
    guestDisplayName_ = QString("Guest-%1")
                            .arg(value, 4, 16, QChar('0'))
                            .toUpper();
    return guestDisplayName_;
}

QString LoginBackend::defaultAuthDisplayName() const
{
    QString displayName = Settings::instance().getDisplayName().trimmed();
    if (!displayName.isEmpty()) {
        return displayName;
    }

    const QString email = Settings::instance().getUserEmail().trimmed();
    if (!email.isEmpty()) {
        return email.split("@").first();
    }

    return QStringLiteral("User");
}

QString LoginBackend::effectiveParticipantName(bool allowUserOverride)
{
    if (!hasAuthToken()) {
        const QString guestName = ensureGuestDisplayName();
        setUserName(guestName);
        return guestName;
    }

    if (allowUserOverride && !userName_.trimmed().isEmpty()) {
        setUserName(userName_);
        return userName_.trimmed();
    }

    const QString fallback = defaultAuthDisplayName();
    setUserName(fallback);
    return fallback;
}

bool LoginBackend::isBusinessRoomName(const QString& value)
{
    static const QRegularExpression pattern(QStringLiteral("^m-\\d{9}$"));
    return pattern.match(value.trimmed()).hasMatch();
}

bool LoginBackend::isBusinessMeetingInput(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (isBusinessRoomName(trimmed)) {
        return true;
    }

    const QUrl url(trimmed);
    if (!url.isValid()) {
        return false;
    }

    const QUrlQuery query(url);
    const QString roomName = query.queryItemValue(QStringLiteral("roomName")).trimmed();
    if (isBusinessRoomName(roomName)) {
        return true;
    }

    QString path = url.path().trimmed();
    if (path.startsWith('/')) {
        path.remove(0, 1);
    }
    return isBusinessRoomName(path);
}
