#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <functional>

struct TokenResponse {
    QString token;
    QString url;
    QString roomName;
    QString meetingNo;
    bool success{false};
    bool isHost{false};
    QString error;
    QString errorCode;
    int httpStatus{0};
};

struct CreateMeetingRequest {
    QString topic;
    QString scheduledStartAt;
    bool allowGuestJoin{false};
    QString password;
    int noJoinAutoEndMinutes{15};
    int emptyAutoEndMinutes{10};
};

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;

    void requestToken(const QString& roomName, const QString& participantName);
    void createMeeting(const QString& authToken, const CreateMeetingRequest& request = CreateMeetingRequest{});
    void joinMeeting(const QString& meetingNo,
                     const QString& participantName,
                     const QString& authToken,
                     const QString& meetingPassword = QString());
    void guestJoinMeeting(const QString& meetingNo,
                          const QString& participantName,
                          const QString& meetingPassword = QString());
    void leaveMeeting(const QString& meetingNo, const QString& authToken);
    void fetchMeetingRecords(const QString& authToken, int page = 1, int pageSize = 20);
    void fetchHostMeetings(const QString& authToken,
                           int page = 1,
                           int pageSize = 20,
                           const QString& status = QString(),
                           const QString& timeFrom = QString(),
                           const QString& timeTo = QString(),
                           bool includeEnded = false);
    void cancelMeeting(const QString& meetingNo, const QString& authToken);
    void refreshAuthToken(const QString& authToken);

    void createRoom(const QString& roomName);
    void listRooms();
    void kickParticipant(const QString& roomName, const QString& identity,
                         const QString& authToken = QString());
    void endRoom(const QString& roomName, const QString& authToken = QString());

    void login(const QString& email, const QString& password);
    void requestVerificationCode(const QString& email);
    void registerUser(const QString& email, const QString& password,
                      const QString& code, const QString& displayName);

    void setApiUrl(const QString& url);
    QString getApiUrl() const { return apiUrl_; }

signals:
    void tokenReceived(const TokenResponse& response);
    void meetingCreated(const QString& meetingNo, const QString& roomName, const QString& shareUrl);
    void meetingLeft(const QString& meetingNo, bool left, const QString& roomName, const QString& identity);
    void meetingRecordsReceived(const QJsonArray& records);
    void hostMeetingsReceived(const QJsonArray& meetings);
    void meetingCancelled(const QString& meetingNo);
    void roomCreated(const QString& roomName);
    void roomsListed(const QJsonArray& rooms);
    void error(const QString& message);
    void authExpired(const QString& message);

    void loginSuccess(const QString& userId, const QString& email, const QString& token,
                      const QString& displayName);
    void registerSuccess(const QString& userId, const QString& email, const QString& token,
                         const QString& displayName);
    void codeRequestSuccess(int expiresInSecs);
    void authRefreshed(const QString& userId, const QString& email, const QString& token,
                       const QString& displayName, int expiresInSecs);
    void authError(const QString& message);

private slots:
    void onTokenReplyFinished();
    void onCreateRoomReplyFinished();
    void onListRoomsReplyFinished();

private:
    void handleNetworkError(QNetworkReply* reply);
    QNetworkRequest createJsonRequest(const QUrl& url) const;
    QNetworkRequest createAuthorizedJsonRequest(const QUrl& url, const QString& authToken) const;
    bool isUnauthorized(QNetworkReply* reply) const;
    void emitAuthExpiredIfNeeded(QNetworkReply* reply, const QByteArray& responseData, const QJsonObject& obj);
    QString buildAuthErrorMessage(QNetworkReply* reply, const QByteArray& responseData, const QJsonObject& obj) const;
    void postAuthJsonWithFallback(const QString& primaryPath,
                                  const QString& fallbackPath,
                                  const QJsonObject& payload,
                                  const std::function<void(const QJsonObject&)>& onSuccess,
                                  const std::function<void(const QString&)>& onFailure);

    QNetworkAccessManager* networkManager_;
    QString apiUrl_;
};

#endif // NETWORK_CLIENT_H
