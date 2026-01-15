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
#include "utils/logger.h"
#include "livekit/livekit.h"

// Global engine pointer for conference window creation
static QQmlApplicationEngine* g_engine = nullptr;

void createConferenceWindow(const QString& url, const QString& token,
                            const QString& roomName, const QString& userName, bool isHost)
{
    if (!g_engine) return;
    
    Logger::instance().info(QString("Creating QML conference window for room: %1").arg(roomName));
    
    // Load conference window QML
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
    
    // Set properties on the window
    object->setProperty("serverUrl", url);
    object->setProperty("token", token);
    object->setProperty("roomName", roomName);
    object->setProperty("userName", userName);
    object->setProperty("isHost", isHost);
    
    // Initialize the backend
    QMetaObject::invokeMethod(object, [object, url, token, roomName, userName, isHost]() {
        // Find ConferenceBackend and initialize
        auto backends = object->findChildren<ConferenceBackend*>();
        for (ConferenceBackend* backend : backends) {
            backend->initialize(url, token, roomName, userName, isHost);
        }
    }, Qt::QueuedConnection);
    
    // Show the window
    QQuickWindow* window = qobject_cast<QQuickWindow*>(object);
    if (window) {
        window->show();
        
        // When conference window is closed, show login window again
        // Note: We check if share mode is active - in share mode, window is intentionally hidden
        QObject::connect(window, &QQuickWindow::visibilityChanged, [window](QWindow::Visibility visibility) {
            if (visibility == QWindow::Hidden) {
                // Check if any ConferenceBackend is in share mode (hidden but not closed)
                bool inShareMode = false;
                auto backends = window->findChildren<ConferenceBackend*>();
                for (ConferenceBackend* backend : backends) {
                    if (backend->shareMode() && backend->shareMode()->isActive()) {
                        inShareMode = true;
                        break;
                    }
                }
                
                if (inShareMode) {
                    // Window is hidden for share mode, don't treat as close
                    Logger::instance().info("Conference window hidden for share mode (not closed)");
                    return;
                }
                
                // Conference window was actually closed, show login window
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
                // Clean up conference window
                window->deleteLater();
            }
        });
    }
    
    Logger::instance().info("Conference window created and shown");
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QQuickWindow::setDefaultAlphaBuffer(true);
    
    // Set application info
    app.setApplicationName("SQLink");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SQLink");
    
    // Set Quick Controls style
    QQuickStyle::setStyle("Basic");

    // Initialize logger
    Logger::instance().init();
    Logger::instance().log("Application started");
    
    // Initialize LiveKit SDK (required by new SDK version)
    livekit::initialize();
    
    // Register QML types
    qmlRegisterType<LoginBackend>("Links.Backend", 1, 0, "LoginBackend");
    qmlRegisterType<SettingsBackend>("Links.Backend", 1, 0, "SettingsBackend");
    qmlRegisterType<ConferenceBackend>("Links.Backend", 1, 0, "ConferenceBackend");
    qmlRegisterType<VideoRenderer>("Links.Backend", 1, 0, "VideoRenderer");
    qmlRegisterType<ScreenPickerBackend>("Links.Backend", 1, 0, "ScreenPickerBackend");
    
    // Create QML engine
    QQmlApplicationEngine engine;
    g_engine = &engine;
    
    // Add QML import paths for module discovery
    engine.addImportPath("qrc:/");
    
    // Connection handler for joining conference
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [](QObject *obj, const QUrl &objUrl) {
        if (!obj) {
            Logger::instance().error("Failed to load QML");
            return;
        }
        
        // Find LoginBackend and connect to conference signal
        auto backends = obj->findChildren<LoginBackend*>();
        for (LoginBackend* backend : backends) {
            QObject::connect(backend, &LoginBackend::joinConference,
                           &createConferenceWindow);
        }
    }, Qt::QueuedConnection);
    
    // Load QML
    engine.load(QUrl("qrc:/ui/qml/main.qml"));
    
    if (engine.rootObjects().isEmpty()) {
        Logger::instance().error("Failed to load main.qml");
        return -1;
    }
    
    return app.exec();
}
