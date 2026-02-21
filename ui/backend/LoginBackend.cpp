#include "LoginBackend.h"

#include "../utils/logger.h"
#include "../utils/settings.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTime>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

namespace {

QString statusTagText(const QString& status)
{
    const QString normalized = status.trimmed().toLower();
    if (normalized == "active" || normalized == "open") {
        return QStringLiteral("进行中");
    }
    if (normalized == "scheduled") {
        return QStringLiteral("待开始");
    }
    if (normalized == "ended") {
        return QStringLiteral("已结束");
    }
    if (normalized == "cancelled") {
        return QStringLiteral("已取消");
    }
    return status;
}

QString toLocalDateTimeText(const QString& isoString)
{
    if (isoString.trimmed().isEmpty()) {
        return QString{};
    }

    const QDateTime utcTime = QDateTime::fromString(isoString, Qt::ISODate);
    if (!utcTime.isValid()) {
        return isoString;
    }

    return utcTime.toLocalTime().toString("yyyy-MM-dd HH:mm");
}

} // namespace

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
    connect(networkClient_, &NetworkClient::hostMeetingsReceived,
            this, &LoginBackend::onHostMeetingsReceived);
    connect(networkClient_, &NetworkClient::meetingCancelled,
            this, &LoginBackend::onMeetingCancelled);
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
                            : "Please enter a meeting number or share link");
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
        beginBusinessJoin(resolvedMeetingNo, name, false);
        return;
    }

    if (!resolvedMeetingNo.isEmpty()) {
        Logger::instance().info(QString("Guest joining via meetingNo: %1").arg(resolvedMeetingNo));
        beginBusinessJoin(resolvedMeetingNo, name, true);
        return;
    }

    clearPendingPasswordContext();
    setErrorMessage("");
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

    setLoading(true);
    setErrorMessage("");

    if (!hasAuthToken()) {
        setLoading(false);
        setErrorMessage("登录状态无效，请重新登录");
        return;
    }

    pendingCreateFlow_ = MeetingCreateFlow::QuickJoin;
    pendingCreateRequest_ = CreateMeetingRequest{};
    pendingCreateRequest_.allowGuestJoin = allowGuestJoin_;

    Logger::instance().info("Creating meeting for quick join");
    networkClient_->createMeeting(authToken(), pendingCreateRequest_);
}

void LoginBackend::createScheduledRoom()
{
    const QDateTime defaultTime = QDateTime::currentDateTime().addSecs(30 * 60);
    createScheduledMeeting(QString{},
                           defaultTime.date().toString("yyyy-MM-dd"),
                           defaultTime.time().hour(),
                           defaultTime.time().minute(),
                           false,
                           QString{},
                           15,
                           10);
}

void LoginBackend::createScheduledMeeting(const QString& topic,
                                          const QString& localDate,
                                          int hour,
                                          int minute,
                                          bool allowGuestJoin,
                                          const QString& meetingPassword,
                                          int noJoinAutoEndMinutes,
                                          int emptyAutoEndMinutes)
{
    if (isGuestMode()) {
        setErrorMessage("游客无法预定会议，请登录后使用");
        return;
    }

    if (!hasAuthToken()) {
        setErrorMessage("登录状态无效，请重新登录");
        return;
    }

    const QDate date = QDate::fromString(localDate.trimmed(), "yyyy-MM-dd");
    const QTime time(hour, minute);
    if (!date.isValid() || !time.isValid()) {
        setErrorMessage("请选择有效的预定日期和时间");
        return;
    }

    const QDateTime localDateTime(date, time);
    if (!localDateTime.isValid()) {
        setErrorMessage("预定时间无效，请重新选择");
        return;
    }

    if (localDateTime < QDateTime::currentDateTime().addSecs(-60)) {
        setErrorMessage("预定时间不能早于当前时间");
        return;
    }

    const QString trimmedPassword = meetingPassword.trimmed();
    if (!trimmedPassword.isEmpty() && (trimmedPassword.size() < 6 || trimmedPassword.size() > 32)) {
        setErrorMessage("会议密码长度需为 6-32 位");
        return;
    }

    pendingCreateFlow_ = MeetingCreateFlow::ScheduledOnly;
    pendingCreateRequest_ = CreateMeetingRequest{};
    pendingCreateRequest_.topic = topic.trimmed();
    pendingCreateRequest_.scheduledStartAt = localDateTime.toUTC().toString(Qt::ISODate);
    pendingCreateRequest_.allowGuestJoin = allowGuestJoin;
    pendingCreateRequest_.password = trimmedPassword;
    pendingCreateRequest_.noJoinAutoEndMinutes = std::clamp(noJoinAutoEndMinutes, 1, 180);
    pendingCreateRequest_.emptyAutoEndMinutes = std::clamp(emptyAutoEndMinutes, 1, 180);

    setLoading(true);
    setErrorMessage("");

    Logger::instance().info("Creating scheduled meeting via /api/meetings");
    networkClient_->createMeeting(authToken(), pendingCreateRequest_);
}

void LoginBackend::loadMeetingRecords()
{
    if (!hasAuthToken()) {
        emit meetingRecordsLoaded(QVariantList{});
        return;
    }

    networkClient_->fetchMeetingRecords(authToken(), 1, 20);
}

void LoginBackend::loadHostMeetings()
{
    if (!hasAuthToken()) {
        emit hostMeetingsLoaded(QVariantList{});
        return;
    }

    networkClient_->fetchHostMeetings(authToken(), 1, 20, QString{}, QString{}, QString{}, false);
}

void LoginBackend::cancelHostedMeeting(const QString& meetingNo)
{
    const QString normalizedMeetingNo = meetingNo.trimmed();
    if (!isMeetingNo(normalizedMeetingNo)) {
        setErrorMessage("会议号无效，无法取消");
        return;
    }

    if (!hasAuthToken()) {
        setErrorMessage("登录状态无效，请重新登录");
        return;
    }

    setLoading(true);
    setErrorMessage("");
    networkClient_->cancelMeeting(normalizedMeetingNo, authToken());
}

void LoginBackend::joinHostedMeeting(const QString& meetingNo)
{
    const QString normalizedMeetingNo = meetingNo.trimmed();
    if (!isMeetingNo(normalizedMeetingNo)) {
        setErrorMessage("会议号无效，无法加入");
        return;
    }

    if (!hasAuthToken()) {
        setErrorMessage("登录状态无效，请重新登录");
        return;
    }

    setRoomName(normalizedMeetingNo);
    saveSettings();
    setLoading(true);
    setErrorMessage("");

    beginBusinessJoin(normalizedMeetingNo, effectiveParticipantName(true), false);
}

void LoginBackend::submitMeetingPassword(const QString& meetingPassword)
{
    const QString password = meetingPassword.trimmed();
    if (pendingPasswordMeetingNo_.isEmpty()) {
        setErrorMessage("当前没有需要密码重试的会议");
        return;
    }

    if (password.isEmpty()) {
        setErrorMessage("请输入会议密码");
        return;
    }

    setLoading(true);
    setErrorMessage("");

    QString participantName = pendingPasswordParticipantName_.trimmed();
    if (participantName.isEmpty()) {
        participantName = effectiveParticipantName(true);
    }

    beginBusinessJoin(pendingPasswordMeetingNo_, participantName, pendingPasswordGuestMode_, password);
}

void LoginBackend::cancelPasswordRetry()
{
    clearPendingPasswordContext();
    setErrorMessage("");
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

        const bool passwordPrompt = shouldPromptMeetingPassword(response);
        QString mappedMessage = mapJoinFailureMessage(response);
        if (mappedMessage.isEmpty()) {
            mappedMessage = !response.error.trimmed().isEmpty()
                ? response.error
                : QStringLiteral("获取入会令牌失败");
        }

        setErrorMessage(mappedMessage);
        if (passwordPrompt) {
            const bool invalidAttempt = response.errorCode.trimmed().compare(
                QStringLiteral("PASSWORD_INVALID"), Qt::CaseInsensitive) == 0;
            emit meetingPasswordRequired(response.meetingNo.trimmed(), mappedMessage, invalidAttempt);
        } else {
            clearPendingPasswordContext();
        }

        Logger::instance().error("Token request failed: " + response.error);
        return;
    }

    clearPendingPasswordContext();

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
    if (hasAuthToken()) {
        loadHostMeetings();
    }
}

void LoginBackend::onMeetingCreated(const QString& meetingNo, const QString& roomName, const QString& shareUrl)
{
    const MeetingCreateFlow flow = pendingCreateFlow_;
    pendingCreateFlow_ = MeetingCreateFlow::None;

    const QString resolvedMeetingNo = !meetingNo.trimmed().isEmpty() ? meetingNo.trimmed() : extractMeetingNo(shareUrl);
    if (!resolvedMeetingNo.isEmpty()) {
        setRoomName(resolvedMeetingNo);
    } else {
        setRoomName(roomName);
    }
    saveSettings();

    if (flow == MeetingCreateFlow::QuickJoin) {
        const QString participantName = effectiveParticipantName(true);

        if (!resolvedMeetingNo.isEmpty() && hasAuthToken()) {
            beginBusinessJoin(resolvedMeetingNo, participantName, false);
            return;
        }

        setLoading(false);
        setErrorMessage("Meeting created but no meeting number returned");
        return;
    }

    setLoading(false);
    if (flow == MeetingCreateFlow::ScheduledOnly) {
        setErrorMessage("");
        emit scheduledMeetingCreated(resolvedMeetingNo, roomName, shareUrl);
        loadHostMeetings();
        return;
    }

    setErrorMessage("");
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
        QString meetingStatus = obj.value("meetingStatus").toString();
        if (meetingStatus.isEmpty()) {
            meetingStatus = obj.value("status").toString();
        }

        QString timeText = obj.value("lastJoinedAt").toString();
        if (timeText.isEmpty()) {
            timeText = obj.value("scheduledStartAt").toString();
        }
        const QString normalizedTimeText = toLocalDateTimeText(timeText);
        if (!normalizedTimeText.isEmpty()) {
            timeText = normalizedTimeText;
        }

        const int joinCount = obj.value("joinCount").toInt(0);
        QString title;
        const QString topic = obj.value("topic").toString().trimmed();
        if (!topic.isEmpty()) {
            title = topic;
            if (!meetingNo.isEmpty()) {
                title += QString(" · %1").arg(meetingNo);
            }
        } else if (!meetingNo.isEmpty()) {
            title = QString("会议号 %1").arg(meetingNo);
        } else if (!roomName.isEmpty()) {
            title = roomName;
        } else {
            title = QStringLiteral("会议记录");
        }

        if (joinCount > 1) {
            title += QString(" · %1次").arg(joinCount);
        }

        QVariantMap row;
        row.insert("title", title);
        row.insert("time", timeText);
        row.insert("tag", statusTagText(meetingStatus));
        row.insert("meetingNo", meetingNo);
        row.insert("roomName", roomName);
        formatted.append(row);
    }

    emit meetingRecordsLoaded(formatted);
}

void LoginBackend::onHostMeetingsReceived(const QJsonArray& records)
{
    QVariantList formatted;
    formatted.reserve(records.size());

    for (const QJsonValue& item : records) {
        if (!item.isObject()) {
            continue;
        }

        const QJsonObject obj = item.toObject();
        const QString meetingNo = obj.value("meetingNo").toString().trimmed();
        const QString roomName = obj.value("roomName").toString().trimmed();
        const QString topic = obj.value("topic").toString().trimmed();
        const QString status = obj.value("status").toString().trimmed().toLower();
        const QString scheduledStartAt = obj.value("scheduledStartAt").toString();
        const bool requiresPassword = obj.value("requiresPassword").toBool(false);
        const bool allowGuestJoin = obj.value("allowGuestJoin").toBool(false);

        QString title;
        if (!topic.isEmpty()) {
            title = topic;
        } else if (!meetingNo.isEmpty()) {
            title = QString("会议号 %1").arg(meetingNo);
        } else {
            title = QStringLiteral("我的预定会议");
        }

        QString timeText = toLocalDateTimeText(scheduledStartAt);
        if (timeText.isEmpty()) {
            timeText = scheduledStartAt;
        }
        if (timeText.isEmpty()) {
            timeText = QStringLiteral("未设置预定时间");
        }

        QVariantMap row;
        row.insert("title", title);
        row.insert("time", timeText);
        row.insert("tag", statusTagText(status));
        row.insert("meetingNo", meetingNo);
        row.insert("roomName", roomName);
        row.insert("status", status);
        row.insert("requiresPassword", requiresPassword);
        row.insert("allowGuestJoin", allowGuestJoin);
        row.insert("canCancel", status == "scheduled");
        row.insert("canJoin", status == "scheduled" || status == "open");
        formatted.append(row);
    }

    emit hostMeetingsLoaded(formatted);
}

void LoginBackend::onMeetingCancelled(const QString& meetingNo)
{
    setLoading(false);
    Q_UNUSED(meetingNo);
    setErrorMessage("");
    loadHostMeetings();
}

void LoginBackend::onNetworkError(const QString& error)
{
    setLoading(false);
    if (error.contains(QStringLiteral("HTTP 401"))) {
        return;
    }

    QString mapped = error;
    if (error.contains("MEETING_NOT_CANCELLABLE", Qt::CaseInsensitive)) {
        mapped = QStringLiteral("该会议当前状态不可取消");
    } else if (error.contains("MEETING_NOT_STARTED", Qt::CaseInsensitive)) {
        mapped = QStringLiteral("会议未到预定开始时间");
    } else if (error.contains("HOST_NOT_JOINED", Qt::CaseInsensitive)) {
        mapped = QStringLiteral("主持者尚未开启会议");
    } else if (error.contains("PASSWORD_REQUIRED", Qt::CaseInsensitive)) {
        mapped = QStringLiteral("该会议需要密码");
    } else if (error.contains("PASSWORD_INVALID", Qt::CaseInsensitive)) {
        mapped = QStringLiteral("会议密码错误");
    }

    setErrorMessage(mapped);
}

void LoginBackend::onAuthExpired(const QString& message)
{
    setLoading(false);
    clearPendingPasswordContext();
    pendingCreateFlow_ = MeetingCreateFlow::None;
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

void LoginBackend::clearPendingPasswordContext()
{
    pendingPasswordMeetingNo_.clear();
    pendingPasswordParticipantName_.clear();
    pendingPasswordGuestMode_ = false;
}

QString LoginBackend::mapJoinFailureMessage(const TokenResponse& response) const
{
    const QString code = response.errorCode.trimmed().toUpper();
    if (code == QStringLiteral("MEETING_NOT_STARTED")) {
        return QStringLiteral("会议未到预定开始时间");
    }
    if (code == QStringLiteral("HOST_NOT_JOINED")) {
        return QStringLiteral("主持者尚未开启会议，请稍后再试");
    }
    if (code == QStringLiteral("PASSWORD_REQUIRED")) {
        return QStringLiteral("该会议需要密码，请输入会议密码");
    }
    if (code == QStringLiteral("PASSWORD_INVALID")) {
        return QStringLiteral("会议密码错误，请重试");
    }

    if (response.httpStatus == 404) {
        return QStringLiteral("会议或房间不存在，请确认输入");
    }
    if (response.httpStatus == 409) {
        return QStringLiteral("会议当前不可加入");
    }
    if (response.httpStatus == 400) {
        return QStringLiteral("请输入有效的会议号、分享链接或房间名称");
    }
    if (isGuestMode() && response.httpStatus == 403) {
        return QStringLiteral("该会议未开放游客加入");
    }

    return QString{};
}

bool LoginBackend::shouldPromptMeetingPassword(const TokenResponse& response) const
{
    const QString code = response.errorCode.trimmed().toUpper();
    return (code == QStringLiteral("PASSWORD_REQUIRED") || code == QStringLiteral("PASSWORD_INVALID"))
        && isMeetingNo(response.meetingNo);
}

void LoginBackend::beginBusinessJoin(const QString& meetingNo,
                                     const QString& participantName,
                                     bool guestMode,
                                     const QString& meetingPassword)
{
    const QString normalizedMeetingNo = meetingNo.trimmed();
    const QString normalizedParticipant = participantName.trimmed();

    pendingParticipantName_ = normalizedParticipant;
    pendingPasswordMeetingNo_ = normalizedMeetingNo;
    pendingPasswordParticipantName_ = normalizedParticipant;
    pendingPasswordGuestMode_ = guestMode;

    if (guestMode) {
        networkClient_->guestJoinMeeting(normalizedMeetingNo, normalizedParticipant, meetingPassword.trimmed());
        return;
    }

    networkClient_->joinMeeting(normalizedMeetingNo,
                                normalizedParticipant,
                                authToken().trimmed(),
                                meetingPassword.trimmed());
}
