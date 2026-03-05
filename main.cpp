#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QQuickStyle>

#include "ui/backend/LoginBackend.h"
#include "ui/backend/SettingsBackend.h"
#include "ui/backend/ConferenceBackend.h"
#include "ui/backend/VideoRenderer.h"
#include "ui/backend/ScreenPickerBackend.h"
#include "ui/backend/ShareModeManager.h"
#include "ui/backend/AuthBackend.h"
#include "ui/backend/ThemeManager.h"
#include "ui/backend/AppearanceManager.h"
#include "ui/backend/LocalRecordingManager.h"
#include "utils/logger.h"
#include "livekit/livekit.h"

static QQmlApplicationEngine* g_engine = nullptr;

void createConferenceWindow(const QString& url,
                            const QString& token,
                            const QString& roomName,
                            const QString& meetingNo,
                            const QString& userName,
                            bool isHost,
                            const QString& userAuthToken,
                            bool isGuest)
{
    if (!g_engine) {
        return;
    }

    Logger::instance().info(QString("Creating QML conference window for room: %1").arg(roomName));

    QQmlComponent component(g_engine, QUrl("qrc:/ui/qml/conference/ConferenceWindow.qml"));

    if (component.status() == QQmlComponent::Error) {
        for (const QQmlError& error : component.errors()) {
            Logger::instance().error(QString("QML Error: %1").arg(error.toString()));
        }
        return;
    }

    QObject* object = component.create();
    if (!object) {
        Logger::instance().error("Failed to create conference window");
        return;
    }

    object->setProperty("serverUrl", url);
    object->setProperty("token", token);
    object->setProperty("roomName", roomName);
    object->setProperty("meetingNo", meetingNo);
    object->setProperty("userName", userName);
    object->setProperty("isHost", isHost);
    object->setProperty("userAuthToken", userAuthToken);
    object->setProperty("isGuest", isGuest);

    QMetaObject::invokeMethod(object, [object, url, token, roomName, meetingNo, userName, isHost, userAuthToken]() {
        auto backends = object->findChildren<ConferenceBackend*>();
        for (ConferenceBackend* backend : backends) {
            backend->initialize(url, token, roomName, meetingNo, userName, isHost, userAuthToken);
        }
    }, Qt::QueuedConnection);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(object);
    if (window) {
        window->show();

        QObject::connect(window, &QQuickWindow::visibilityChanged, [window](QWindow::Visibility visibility) {
            if (visibility == QWindow::Hidden) {
                bool inShareMode = false;
                auto backends = window->findChildren<ConferenceBackend*>();
                for (ConferenceBackend* backend : backends) {
                    if (backend->shareMode() && backend->shareMode()->isActive()) {
                        inShareMode = true;
                        break;
                    }
                }

                if (inShareMode) {
                    Logger::instance().info("Conference window hidden for share mode (not closed)");
                    return;
                }

                if (g_engine && !g_engine->rootObjects().isEmpty()) {
                    QObject* loginWindow = g_engine->rootObjects().first();
                    if (loginWindow) {
                        QQuickWindow* loginQmlWindow = qobject_cast<QQuickWindow*>(loginWindow);
                        if (loginQmlWindow) {
                            loginQmlWindow->show();
                            Logger::instance().info("Login window shown after leaving conference");
                        }
                    }
                }

                window->deleteLater();
            }
        });
    }

    Logger::instance().info("Conference window created and shown");
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QQuickWindow::setDefaultAlphaBuffer(true);

    app.setApplicationName("SQLink");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SQLink");

    QQuickStyle::setStyle("Basic");

    Logger::instance().init();
    Logger::instance().log("Application started");

    livekit::initialize();

    qmlRegisterType<LoginBackend>("Links.Backend", 1, 0, "LoginBackend");
    qmlRegisterType<SettingsBackend>("Links.Backend", 1, 0, "SettingsBackend");
    qmlRegisterType<ConferenceBackend>("Links.Backend", 1, 0, "ConferenceBackend");
    qmlRegisterType<VideoRenderer>("Links.Backend", 1, 0, "VideoRenderer");
    qmlRegisterType<ScreenPickerBackend>("Links.Backend", 1, 0, "ScreenPickerBackend");
    qmlRegisterType<AuthBackend>("Links.Backend", 1, 0, "AuthBackend");

    // Theme manager singleton
    auto* themeManager = new ThemeManager(&app);
    qmlRegisterSingletonInstance("Links.Backend", 1, 0, "ThemeManager", themeManager);

    // Appearance manager singleton
    auto* appearanceManager = new AppearanceManager(&app);
    qmlRegisterSingletonInstance("Links.Backend", 1, 0, "AppearanceManager", appearanceManager);

    // Local recording manager singleton
    auto* localRecordingManager = &LocalRecordingManager::instance();
    localRecordingManager->setParent(&app);
    qmlRegisterSingletonInstance("Links.Backend", 1, 0, "LocalRecordingManager", localRecordingManager);

    QQmlApplicationEngine engine;
    g_engine = &engine;

    engine.addImportPath("qrc:/");

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [](QObject* obj, const QUrl& objUrl) {
        Q_UNUSED(objUrl);

        if (!obj) {
            Logger::instance().error("Failed to load QML");
            return;
        }

        const auto backends = obj->findChildren<LoginBackend*>();
        for (LoginBackend* backend : backends) {
            QObject::connect(backend, &LoginBackend::joinConference,
                             &createConferenceWindow);
        }
    }, Qt::QueuedConnection);

    engine.load(QUrl("qrc:/ui/qml/main.qml"));

    if (engine.rootObjects().isEmpty()) {
        Logger::instance().error("Failed to load main.qml");
        return -1;
    }

    return app.exec();
}
