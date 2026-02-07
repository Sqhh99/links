#include "network_client.h"
#include "../utils/logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

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
    // Remove trailing slash to prevent double slashes in URL construction
    apiUrl_ = url;
    if (apiUrl_.endsWith('/')) {
        apiUrl_.chop(1);
    }
    Logger::instance().info("API URL set to: " + apiUrl_);
}

void NetworkClient::requestToken(const QString& roomName, const QString& participantName)
{
    Logger::instance().info(QString("Requesting token for room '%1', participant '%2'")
                           .arg(roomName, participantName));
    
    QUrl url(apiUrl_ + "/api/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["roomName"] = roomName;
    json["participantName"] = participantName;
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    QNetworkReply* reply = networkManager_->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onTokenReplyFinished);
}

void NetworkClient::createRoom(const QString& roomName)
{
    Logger::instance().info("Creating room: " + roomName);
    
    QUrl url(apiUrl_ + "/api/rooms");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["name"] = roomName;
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    QNetworkReply* reply = networkManager_->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onCreateRoomReplyFinished);
}

void NetworkClient::listRooms()
{
    Logger::instance().info("Listing rooms");
    
    QUrl url(apiUrl_ + "/api/rooms");
    QNetworkRequest request(url);
    
    QNetworkReply* reply = networkManager_->get(request);
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onListRoomsReplyFinished);
}

void NetworkClient::onTokenReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    TokenResponse response;
    
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        response.success = false;
        response.error = reply->errorString();
        emit tokenReceived(response);
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        response.token = obj["token"].toString();
        response.url = obj["url"].toString();
        response.roomName = obj["roomName"].toString();
        response.isHost = obj["isHost"].toBool(false);
        response.success = true;
        
        Logger::instance().info(QString("Token received successfully (isHost: %1)").arg(response.isHost));
        emit tokenReceived(response);
    } else {
        response.success = false;
        response.error = "Invalid response format";
        Logger::instance().error("Invalid token response format");
        emit tokenReceived(response);
    }
}

void NetworkClient::onCreateRoomReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString roomName = obj["name"].toString();
        Logger::instance().info("Room created: " + roomName);
        emit roomCreated(roomName);
    }
}

void NetworkClient::onListRoomsReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isArray()) {
        QJsonArray rooms = doc.array();
        Logger::instance().info(QString("Received %1 rooms").arg(rooms.size()));
        emit roomsListed(rooms);
    }
}

void NetworkClient::handleNetworkError(QNetworkReply* reply)
{
    QString errorMsg = QString("Network error: %1").arg(reply->errorString());
    Logger::instance().error(errorMsg);
    emit error(errorMsg);
}

void NetworkClient::kickParticipant(const QString& roomName, const QString& identity)
{
    Logger::instance().info(QString("Kicking participant '%1' from room '%2'").arg(identity, roomName));
    
    QUrl url(apiUrl_ + QString("/api/rooms/%1/participants/%2").arg(roomName, identity));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = networkManager_->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, identity]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            QString errorMsg = QString("Failed to kick participant: %1").arg(reply->errorString());
            Logger::instance().error(errorMsg);
            emit error(errorMsg);
            return;
        }
        
        Logger::instance().info(QString("Successfully kicked participant '%1'").arg(identity));
    });
}

void NetworkClient::endRoom(const QString& roomName)
{
    Logger::instance().info(QString("Ending room '%1'").arg(roomName));
    
    QUrl url(apiUrl_ + QString("/api/rooms/%1/end").arg(roomName));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = networkManager_->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, roomName]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            QString errorMsg = QString("Failed to end room: %1").arg(reply->errorString());
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

    QNetworkRequest request(QUrl(apiUrl_ + primaryPath));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager_->post(request, data);
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

        QNetworkRequest fallbackRequest(QUrl(apiUrl_ + fallbackPath));
        fallbackRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply* fallbackReply = networkManager_->post(fallbackRequest, data);
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

// Auth API implementations
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

            Logger::instance().info("Login successful for: " + responseEmail);
            emit loginSuccess(userId, responseEmail, token);
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

void NetworkClient::registerUser(const QString& email, const QString& password, const QString& code)
{
    Logger::instance().info(QString("Registering user: %1").arg(email));

    QJsonObject payload;
    payload["email"] = email;
    payload["password"] = password;
    payload["code"] = code;

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

            Logger::instance().info("Registration successful for: " + responseEmail);
            emit registerSuccess(userId, responseEmail, token);
        },
        [this](const QString& errorMsg) {
            Logger::instance().error("Registration failed: " + errorMsg);
            emit authError(errorMsg);
        });
}
