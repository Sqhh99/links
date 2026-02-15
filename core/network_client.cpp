#include "network_client.h"
#include "../utils/logger.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrlQuery>

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent),
      networkManager_(new QNetworkAccessManager(this)),
      apiUrl_("https://sqhh99.dpdns.org:8443")
{
}

NetworkClient::~NetworkClient()
{
}

void NetworkClient::setApiUrl(const QString& url)
{
    apiUrl_ = url;
    if (apiUrl_.endsWith('/')) {
        apiUrl_.chop(1);
    }
    Logger::instance().info("API URL set to: " + apiUrl_);
}

QNetworkRequest NetworkClient::createJsonRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

QNetworkRequest NetworkClient::createAuthorizedJsonRequest(const QUrl& url, const QString& authToken) const
{
    QNetworkRequest request = createJsonRequest(url);
    if (!authToken.trimmed().isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(authToken.trimmed()).toUtf8());
    }
    return request;
}

bool NetworkClient::isUnauthorized(QNetworkReply* reply) const
{
    if (!reply) {
        return false;
    }

    return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401;
}

void NetworkClient::emitAuthExpiredIfNeeded(QNetworkReply* reply,
                                            const QByteArray& responseData,
                                            const QJsonObject& obj)
{
    if (!isUnauthorized(reply)) {
        return;
    }

    const QString message = buildAuthErrorMessage(reply, responseData, obj);
    Logger::instance().warning("Auth expired: " + message);
    emit authExpired(message);
}

void NetworkClient::requestToken(const QString& roomName, const QString& participantName)
{
    Logger::instance().info(QString("Requesting token for room '%1', participant '%2'")
                               .arg(roomName, participantName));

    QJsonObject json;
    json["roomName"] = roomName;
    json["participantName"] = participantName;

    const QJsonDocument doc(json);
    QNetworkReply* reply = networkManager_->post(createJsonRequest(QUrl(apiUrl_ + "/api/token")), doc.toJson());
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onTokenReplyFinished);
}

void NetworkClient::createMeeting(const QString& authToken)
{
    Logger::instance().info("Creating meeting via /api/meetings");

    QNetworkReply* reply = networkManager_->post(
        createAuthorizedJsonRequest(QUrl(apiUrl_ + "/api/meetings"), authToken),
        QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() != QNetworkReply::NoError) {
            emitAuthExpiredIfNeeded(reply, responseData, obj);
            const QString errorMsg = buildAuthErrorMessage(reply, responseData, obj);
            Logger::instance().error("Create meeting failed: " + errorMsg);
            emit error(errorMsg);
            return;
        }

        const QString meetingNo = obj.value("meetingNo").toString();
        const QString roomName = obj.value("roomName").toString();
        const QString shareUrl = obj.value("shareUrl").toString();
        Logger::instance().info(QString("Meeting created successfully: meetingNo=%1, roomName=%2")
                                   .arg(meetingNo, roomName));
        emit meetingCreated(meetingNo, roomName, shareUrl);
    });
}

void NetworkClient::joinMeeting(const QString& meetingNo,
                                const QString& participantName,
                                const QString& authToken)
{
    Logger::instance().info(QString("Joining meeting '%1' via /api/meetings/{meetingNo}/join").arg(meetingNo));

    QJsonObject payload;
    if (!participantName.trimmed().isEmpty()) {
        payload["participantName"] = participantName.trimmed();
    }

    const QJsonDocument doc(payload);
    const QString path = QString("/api/meetings/%1/join").arg(meetingNo);

    QNetworkReply* reply = networkManager_->post(
        createAuthorizedJsonRequest(QUrl(apiUrl_ + path), authToken),
        doc.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, meetingNo]() {
        reply->deleteLater();

        TokenResponse response;
        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() != QNetworkReply::NoError) {
            response.success = false;
            response.error = buildAuthErrorMessage(reply, responseData, obj);
            emitAuthExpiredIfNeeded(reply, responseData, obj);
            Logger::instance().error("Join meeting failed: " + response.error);
            emit error(response.error);
            emit tokenReceived(response);
            return;
        }

        response.token = obj.value("token").toString();
        response.url = obj.value("url").toString();
        response.roomName = obj.value("roomName").toString();
        response.meetingNo = obj.value("meetingNo").toString();
        if (response.meetingNo.isEmpty()) {
            response.meetingNo = meetingNo;
        }
        response.isHost = obj.value("isHost").toBool(false);
        response.success = true;

        Logger::instance().info(QString("Join meeting succeeded: meetingNo=%1, roomName=%2")
                                   .arg(response.meetingNo, response.roomName));
        emit tokenReceived(response);
    });
}

void NetworkClient::leaveMeeting(const QString& meetingNo, const QString& authToken)
{
    Logger::instance().info(QString("Leaving meeting '%1' via /api/meetings/{meetingNo}/leave").arg(meetingNo));

    const QString path = QString("/api/meetings/%1/leave").arg(meetingNo.trimmed());
    QNetworkReply* reply = networkManager_->post(
        createAuthorizedJsonRequest(QUrl(apiUrl_ + path), authToken),
        QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply, meetingNo]() {
        reply->deleteLater();

        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() != QNetworkReply::NoError) {
            emitAuthExpiredIfNeeded(reply, responseData, obj);
            const QString errorMsg = buildAuthErrorMessage(reply, responseData, obj);
            Logger::instance().error("Leave meeting failed: " + errorMsg);
            emit error(errorMsg);
            return;
        }

        const QString responseMeetingNo = obj.value("meetingNo").toString().isEmpty()
            ? meetingNo
            : obj.value("meetingNo").toString();
        const bool left = obj.value("left").toBool(true);
        const QString roomName = obj.value("roomName").toString();
        const QString identity = obj.value("identity").toString();
        Logger::instance().info(QString("Leave meeting succeeded: meetingNo=%1, left=%2")
                                   .arg(responseMeetingNo)
                                   .arg(left ? "true" : "false"));
        emit meetingLeft(responseMeetingNo, left, roomName, identity);
    });
}

void NetworkClient::fetchMeetingRecords(const QString& authToken, int page, int pageSize)
{
    QUrl url(apiUrl_ + "/api/me/meeting-records");
    QUrlQuery query;
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("pageSize", QString::number(pageSize));
    url.setQuery(query);

    QNetworkReply* reply = networkManager_->get(createAuthorizedJsonRequest(url, authToken));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() != QNetworkReply::NoError) {
            emitAuthExpiredIfNeeded(reply, responseData, obj);
            const QString errorMsg = buildAuthErrorMessage(reply, responseData, obj);
            Logger::instance().error("Fetch meeting records failed: " + errorMsg);
            emit error(errorMsg);
            emit meetingRecordsReceived(QJsonArray{});
            return;
        }

        const QJsonArray records = obj.value("records").toArray();
        Logger::instance().info(QString("Fetched %1 meeting records").arg(records.size()));
        emit meetingRecordsReceived(records);
    });
}

void NetworkClient::refreshAuthToken(const QString& authToken)
{
    Logger::instance().info("Refreshing auth token via /api/auth/refresh");

    QNetworkReply* reply = networkManager_->post(
        createAuthorizedJsonRequest(QUrl(apiUrl_ + "/api/auth/refresh"), authToken),
        QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() != QNetworkReply::NoError) {
            emitAuthExpiredIfNeeded(reply, responseData, obj);
            const QString errorMsg = buildAuthErrorMessage(reply, responseData, obj);
            Logger::instance().error("Refresh auth token failed: " + errorMsg);
            emit authError(errorMsg);
            return;
        }

        QString userId = obj.value("userId").toString();
        if (userId.isEmpty()) {
            userId = obj.value("user_id").toString();
        }
        const QString email = obj.value("email").toString();
        const QString token = obj.value("token").toString();
        QString displayName = obj.value("displayName").toString();
        if (displayName.isEmpty()) {
            displayName = obj.value("display_name").toString();
        }
        const int expiresInSecs = obj.value("expiresInSecs").toInt(0);

        if (userId.isEmpty() || email.isEmpty() || token.isEmpty()) {
            const QString errorMsg = "Invalid refresh response format";
            Logger::instance().error(errorMsg);
            emit authError(errorMsg);
            return;
        }

        Logger::instance().info("Auth token refreshed for: " + email);
        emit authRefreshed(userId, email, token, displayName, expiresInSecs);
    });
}

void NetworkClient::createRoom(const QString& roomName)
{
    Logger::instance().info("Creating room: " + roomName);

    QJsonObject json;
    json["name"] = roomName;

    const QJsonDocument doc(json);
    QNetworkReply* reply = networkManager_->post(createJsonRequest(QUrl(apiUrl_ + "/api/rooms")), doc.toJson());
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onCreateRoomReplyFinished);
}

void NetworkClient::listRooms()
{
    Logger::instance().info("Listing rooms");

    QNetworkReply* reply = networkManager_->get(QNetworkRequest(QUrl(apiUrl_ + "/api/rooms")));
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onListRoomsReplyFinished);
}

void NetworkClient::onTokenReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    TokenResponse response;

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        response.success = false;
        response.error = reply->errorString();
        emit tokenReceived(response);
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        response.success = false;
        response.error = "Invalid response format";
        Logger::instance().error("Invalid token response format");
        emit tokenReceived(response);
        return;
    }

    const QJsonObject obj = doc.object();
    response.token = obj.value("token").toString();
    response.url = obj.value("url").toString();
    response.roomName = obj.value("roomName").toString();
    response.meetingNo = obj.value("meetingNo").toString();
    response.isHost = obj.value("isHost").toBool(false);
    response.success = true;

    Logger::instance().info(QString("Token received successfully (isHost: %1)").arg(response.isHost));
    emit tokenReceived(response);
}

void NetworkClient::onCreateRoomReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        const QString roomName = obj.value("name").toString();
        Logger::instance().info("Room created: " + roomName);
        emit roomCreated(roomName);
    }
}

void NetworkClient::onListRoomsReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isArray()) {
        const QJsonArray rooms = doc.array();
        Logger::instance().info(QString("Received %1 rooms").arg(rooms.size()));
        emit roomsListed(rooms);
    }
}

void NetworkClient::handleNetworkError(QNetworkReply* reply)
{
    const QString errorMsg = QString("Network error: %1").arg(reply->errorString());
    Logger::instance().error(errorMsg);
    emit error(errorMsg);
}

void NetworkClient::kickParticipant(const QString& roomName,
                                    const QString& identity,
                                    const QString& authToken)
{
    Logger::instance().info(QString("Kicking participant '%1' from room '%2'").arg(identity, roomName));

    QUrl url(apiUrl_ + QString("/api/rooms/%1/participants/%2").arg(roomName, identity));
    QNetworkReply* reply = networkManager_->deleteResource(createAuthorizedJsonRequest(url, authToken));
    connect(reply, &QNetworkReply::finished, this, [this, reply, identity]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() != QNetworkReply::NoError) {
            emitAuthExpiredIfNeeded(reply, responseData, obj);
            const QString errorMsg = QString("Failed to kick participant: %1")
                .arg(buildAuthErrorMessage(reply, responseData, obj));
            Logger::instance().error(errorMsg);
            emit error(errorMsg);
            return;
        }

        Logger::instance().info(QString("Successfully kicked participant '%1'").arg(identity));
    });
}

void NetworkClient::endRoom(const QString& roomName, const QString& authToken)
{
    Logger::instance().info(QString("Ending room '%1'").arg(roomName));

    QUrl url(apiUrl_ + QString("/api/rooms/%1/end").arg(roomName));
    QNetworkReply* reply = networkManager_->post(createAuthorizedJsonRequest(url, authToken), QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, roomName]() {
        reply->deleteLater();
        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() != QNetworkReply::NoError) {
            emitAuthExpiredIfNeeded(reply, responseData, obj);
            const QString errorMsg = QString("Failed to end room: %1")
                .arg(buildAuthErrorMessage(reply, responseData, obj));
            Logger::instance().error(errorMsg);
            emit error(errorMsg);
            return;
        }

        Logger::instance().info(QString("Successfully ended room '%1'").arg(roomName));
    });
}

QString NetworkClient::buildAuthErrorMessage(QNetworkReply* reply,
                                             const QByteArray& responseData,
                                             const QJsonObject& obj) const
{
    QString message;
    if (obj.contains("error") && obj.value("error").isString()) {
        message = obj.value("error").toString();
    } else {
        const QString rawResponse = QString::fromUtf8(responseData).trimmed();
        message = rawResponse.isEmpty() ? reply->errorString() : rawResponse;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString reasonPhrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    if (statusCode > 0) {
        const QString statusText = reasonPhrase.isEmpty()
            ? QString("HTTP %1").arg(statusCode)
            : QString("HTTP %1 %2").arg(statusCode).arg(reasonPhrase);
        message = QString("%1 (%2)").arg(message, statusText);
    }

    const QString allowHeader = QString::fromUtf8(reply->rawHeader("Allow")).trimmed();
    if (!allowHeader.isEmpty()) {
        message = QString("%1 [Allow: %2]").arg(message, allowHeader);
    }

    return message;
}

void NetworkClient::postAuthJsonWithFallback(
    const QString& primaryPath,
    const QString& fallbackPath,
    const QJsonObject& payload,
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onFailure)
{
    const QJsonDocument doc(payload);
    const QByteArray data = doc.toJson();

    QNetworkReply* reply = networkManager_->post(createJsonRequest(QUrl(apiUrl_ + primaryPath)), data);
    connect(reply, &QNetworkReply::finished, this, [this,
                                                     reply,
                                                     primaryPath,
                                                     fallbackPath,
                                                     data,
                                                     onSuccess,
                                                     onFailure]() {
        reply->deleteLater();

        const QByteArray responseData = reply->readAll();
        const QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        const QJsonObject obj = responseDoc.isObject() ? responseDoc.object() : QJsonObject{};

        if (reply->error() == QNetworkReply::NoError) {
            onSuccess(obj);
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool shouldFallback = !fallbackPath.isEmpty()
            && fallbackPath != primaryPath
            && statusCode == 404;

        if (!shouldFallback) {
            onFailure(buildAuthErrorMessage(reply, responseData, obj));
            return;
        }

        Logger::instance().warning(
            QString("Auth endpoint '%1' returned HTTP %2, retrying fallback '%3'")
                .arg(primaryPath)
                .arg(statusCode)
                .arg(fallbackPath));

        QNetworkReply* fallbackReply = networkManager_->post(createJsonRequest(QUrl(apiUrl_ + fallbackPath)), data);
        connect(fallbackReply, &QNetworkReply::finished, this, [this, fallbackReply, onSuccess, onFailure]() {
            fallbackReply->deleteLater();

            const QByteArray fallbackResponseData = fallbackReply->readAll();
            const QJsonDocument fallbackDoc = QJsonDocument::fromJson(fallbackResponseData);
            const QJsonObject fallbackObj = fallbackDoc.isObject() ? fallbackDoc.object() : QJsonObject{};

            if (fallbackReply->error() != QNetworkReply::NoError) {
                onFailure(buildAuthErrorMessage(fallbackReply, fallbackResponseData, fallbackObj));
                return;
            }

            onSuccess(fallbackObj);
        });
    });
}

void NetworkClient::login(const QString& email, const QString& password)
{
    Logger::instance().info(QString("Attempting login for email: %1").arg(email));

    QJsonObject payload;
    payload["email"] = email;
    payload["password"] = password;

    postAuthJsonWithFallback(
        "/api/auth/login",
        "/auth/login",
        payload,
        [this](const QJsonObject& obj) {
            QString userId = obj.value("userId").toString();
            if (userId.isEmpty()) {
                userId = obj.value("user_id").toString();
            }
            const QString responseEmail = obj.value("email").toString();
            const QString token = obj.value("token").toString();
            QString displayName = obj.value("displayName").toString();
            if (displayName.isEmpty()) {
                displayName = obj.value("display_name").toString();
            }

            Logger::instance().info("Login successful for: " + responseEmail);
            emit loginSuccess(userId, responseEmail, token, displayName);
        },
        [this](const QString& errorMsg) {
            Logger::instance().error("Login failed: " + errorMsg);
            emit authError(errorMsg);
        });
}

void NetworkClient::requestVerificationCode(const QString& email)
{
    Logger::instance().info(QString("Requesting verification code for: %1").arg(email));

    QJsonObject payload;
    payload["email"] = email;

    postAuthJsonWithFallback(
        "/api/auth/register/request-code",
        "/auth/register/request-code",
        payload,
        [this](const QJsonObject& obj) {
            int retryAfterSecs = obj.value("retryAfterSecs").toInt(0);
            if (retryAfterSecs <= 0) {
                retryAfterSecs = obj.value("expires_in_secs").toInt(600);
            }
            Logger::instance().info("Verification code sent successfully");
            emit codeRequestSuccess(retryAfterSecs);
        },
        [this](const QString& errorMsg) {
            Logger::instance().error("Request code failed: " + errorMsg);
            emit authError(errorMsg);
        });
}

void NetworkClient::registerUser(const QString& email, const QString& password,
                                 const QString& code, const QString& displayName)
{
    Logger::instance().info(QString("Registering user: %1").arg(email));

    QJsonObject payload;
    payload["email"] = email;
    payload["password"] = password;
    payload["code"] = code;
    if (!displayName.trimmed().isEmpty()) {
        payload["displayName"] = displayName.trimmed();
    }

    postAuthJsonWithFallback(
        "/api/auth/register",
        "/auth/register",
        payload,
        [this](const QJsonObject& obj) {
            QString userId = obj.value("userId").toString();
            if (userId.isEmpty()) {
                userId = obj.value("user_id").toString();
            }
            const QString responseEmail = obj.value("email").toString();
            const QString token = obj.value("token").toString();
            QString responseDisplayName = obj.value("displayName").toString();
            if (responseDisplayName.isEmpty()) {
                responseDisplayName = obj.value("display_name").toString();
            }

            Logger::instance().info("Registration successful for: " + responseEmail);
            emit registerSuccess(userId, responseEmail, token, responseDisplayName);
        },
        [this](const QString& errorMsg) {
            Logger::instance().error("Registration failed: " + errorMsg);
            emit authError(errorMsg);
        });
}
