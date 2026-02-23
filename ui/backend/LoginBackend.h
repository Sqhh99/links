#ifndef LOGINBACKEND_H
#define LOGINBACKEND_H

#include <QObject>
#include <QString>
#include <QVariantList>

#include "../core/network_client.h"

class LoginBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString roomName READ roomName WRITE setRoomName NOTIFY roomNameChanged)
    Q_PROPERTY(bool micEnabled READ micEnabled WRITE setMicEnabled NOTIFY micEnabledChanged)
    Q_PROPERTY(bool camEnabled READ camEnabled WRITE setCamEnabled NOTIFY camEnabledChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString scheduledTime READ scheduledTime WRITE setScheduledTime NOTIFY scheduledTimeChanged)
    Q_PROPERTY(bool allowGuestJoin READ allowGuestJoin WRITE setAllowGuestJoin NOTIFY allowGuestJoinChanged)
    Q_PROPERTY(bool sessionLoggedIn READ sessionLoggedIn WRITE setSessionLoggedIn NOTIFY sessionLoggedInChanged)
    Q_PROPERTY(QString sessionAuthToken READ sessionAuthToken WRITE setSessionAuthToken NOTIFY sessionAuthTokenChanged)

public:
    explicit LoginBackend(QObject* parent = nullptr);
    ~LoginBackend() override = default;

    QString userName() const { return userName_; }
    void setUserName(const QString& name);

    QString roomName() const { return roomName_; }
    void setRoomName(const QString& name);

    bool micEnabled() const { return micEnabled_; }
    void setMicEnabled(bool enabled);

    bool camEnabled() const { return camEnabled_; }
    void setCamEnabled(bool enabled);

    bool loading() const { return loading_; }
    QString errorMessage() const { return errorMessage_; }

    QString scheduledTime() const { return scheduledTime_; }
    void setScheduledTime(const QString& time);
    bool allowGuestJoin() const { return allowGuestJoin_; }
    void setAllowGuestJoin(bool enabled);
    bool sessionLoggedIn() const { return sessionLoggedIn_; }
    void setSessionLoggedIn(bool loggedIn);
    QString sessionAuthToken() const { return sessionAuthToken_; }
    void setSessionAuthToken(const QString& token);

    Q_INVOKABLE void join();
    Q_INVOKABLE void quickJoin();
    Q_INVOKABLE void createScheduledRoom();
    Q_INVOKABLE void createScheduledMeeting(const QString& topic,
                                            const QString& localDate,
                                            int hour,
                                            int minute,
                                            bool allowGuestJoin,
                                            const QString& meetingPassword,
                                            int noJoinAutoEndMinutes,
                                            int emptyAutoEndMinutes);
    Q_INVOKABLE void loadMeetingRecords();
    Q_INVOKABLE void loadHostMeetings();
    Q_INVOKABLE void cancelHostedMeeting(const QString& meetingNo);
    Q_INVOKABLE void joinHostedMeeting(const QString& meetingNo);
    Q_INVOKABLE void submitMeetingPassword(const QString& meetingPassword);
    Q_INVOKABLE void cancelPasswordRetry();
    Q_INVOKABLE void syncParticipantNameFromSession();
    Q_INVOKABLE void showSettings();
    Q_INVOKABLE QString currentTime() const;
    Q_INVOKABLE QString currentDate() const;

signals:
    void userNameChanged();
    void roomNameChanged();
    void micEnabledChanged();
    void camEnabledChanged();
    void loadingChanged();
    void errorMessageChanged();
    void scheduledTimeChanged();
    void allowGuestJoinChanged();
    void sessionLoggedInChanged();
    void sessionAuthTokenChanged();
    void joinConference(const QString& url,
                        const QString& token,
                        const QString& roomName,
                        const QString& meetingNo,
                        const QString& userName,
                        bool isHost,
                        const QString& userAuthToken,
                        bool isGuest);
    void meetingRecordsLoaded(const QVariantList& records);
    void hostMeetingsLoaded(const QVariantList& records);
    void scheduledMeetingCreated(const QString& meetingNo, const QString& roomName, const QString& shareUrl);
    void meetingPasswordRequired(const QString& meetingNo, const QString& message, bool invalidAttempt);
    void sessionExpired(const QString& message);
    void settingsRequested();

private slots:
    void onTokenReceived(const TokenResponse& response);
    void onMeetingCreated(const QString& meetingNo, const QString& roomName, const QString& shareUrl);
    void onMeetingRecordsReceived(const QJsonArray& records);
    void onHostMeetingsReceived(const QJsonArray& records);
    void onMeetingCancelled(const QString& meetingNo);
    void onAuthExpired(const QString& message);
    void onNetworkError(const QString& error);

private:
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    void saveSettings();
    void loadSettings();
    QString ensureGuestDisplayName();
    QString defaultAuthDisplayName() const;
    QString effectiveParticipantName(bool allowUserOverride = true);
    bool isGuestMode() const;
    static bool isBusinessRoomName(const QString& value);
    static bool isBusinessMeetingInput(const QString& value);
    bool hasAuthToken() const;
    QString authToken() const;
    static bool isMeetingNo(const QString& value);
    static QString extractMeetingNo(const QString& value);
    void clearPendingPasswordContext();
    QString mapJoinFailureMessage(const TokenResponse& response) const;
    bool shouldPromptMeetingPassword(const TokenResponse& response) const;
    void beginBusinessJoin(const QString& meetingNo,
                           const QString& participantName,
                           bool guestMode,
                           const QString& meetingPassword = QString());

    NetworkClient* networkClient_;
    QString userName_;
    QString roomName_;
    QString scheduledTime_;
    bool allowGuestJoin_{false};
    bool sessionLoggedIn_{false};
    QString sessionAuthToken_;
    bool micEnabled_{false};
    bool camEnabled_{false};
    bool loading_{false};
    QString errorMessage_;
    QString guestDisplayName_;
    QString pendingParticipantName_;

    enum class MeetingCreateFlow {
        None,
        QuickJoin,
        ScheduledOnly
    };
    MeetingCreateFlow pendingCreateFlow_{MeetingCreateFlow::None};
    CreateMeetingRequest pendingCreateRequest_;

    QString pendingPasswordMeetingNo_;
    QString pendingPasswordParticipantName_;
    bool pendingPasswordGuestMode_{false};
};

#endif // LOGINBACKEND_H
