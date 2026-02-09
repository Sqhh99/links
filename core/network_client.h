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
};

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;

    void requestToken(const QString& roomName, const QString& participantName);
    void createMeeting(const QString& authToken);
    void joinMeeting(const QString& meetingNo, const QString& participantName, const QString& authToken);
    void fetchMeetingRecords(const QString& authToken, int page = 1, int pageSize = 20);

    void createRoom(const QString& roomName);
    void listRooms();
    void kickParticipant(const QString& roomName, const QString& identity);
    void endRoom(const QString& roomName);

    void login(const QString& email, const QString& password);
    void requestVerificationCode(const QString& email);
    void registerUser(const QString& email, const QString& password, const QString& code);

    void setApiUrl(const QString& url);
    QString getApiUrl() const { return apiUrl_; }

signals:
    void tokenReceived(const TokenResponse& response);
    void meetingCreated(const QString& meetingNo, const QString& roomName, const QString& shareUrl);
    void meetingRecordsReceived(const QJsonArray& records);
    void roomCreated(const QString& roomName);
    void roomsListed(const QJsonArray& rooms);
    void error(const QString& message);

    void loginSuccess(const QString& userId, const QString& email, const QString& token);
    void registerSuccess(const QString& userId, const QString& email, const QString& token);
    void codeRequestSuccess(int expiresInSecs);
    void authError(const QString& message);

private slots:
    void onTokenReplyFinished();
    void onCreateRoomReplyFinished();
    void onListRoomsReplyFinished();

private:
    void handleNetworkError(QNetworkReply* reply);
    QNetworkRequest createJsonRequest(const QUrl& url) const;
    QNetworkRequest createAuthorizedJsonRequest(const QUrl& url, const QString& authToken) const;
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
