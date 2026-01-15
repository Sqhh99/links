/*
 * Copyright (c) 2026 Links Project
 * ShareModeManager - Manages Screen Share Mode with floating overlay controls
 */

#ifndef SHARE_MODE_MANAGER_H
#define SHARE_MODE_MANAGER_H

#include <QObject>
#include <QWindow>
#include <QElapsedTimer>
#include <QTimer>

class ShareModeManager : public QObject
{
    Q_OBJECT
    
    // Core state
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(QString formattedTime READ formattedTime NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    
    // Overlay support and visibility
    Q_PROPERTY(bool overlaySupported READ overlaySupported CONSTANT)
    Q_PROPERTY(bool overlayEnabled READ overlayEnabled WRITE setOverlayEnabled NOTIFY overlayEnabledChanged)
    Q_PROPERTY(bool cameraThumbnailVisible READ cameraThumbnailVisible WRITE setCameraThumbnailVisible NOTIFY cameraThumbnailVisibleChanged)
    
public:
    explicit ShareModeManager(QObject* parent = nullptr);
    ~ShareModeManager() override;
    
    // Property getters
    bool isActive() const { return isActive_; }
    bool overlaySupported() const { return overlaySupported_; }
    bool overlayEnabled() const { return overlayEnabled_; }
    bool cameraThumbnailVisible() const { return cameraThumbnailVisible_; }
    int elapsedSeconds() const;
    QString formattedTime() const;
    
    // Property setters
    void setOverlayEnabled(bool enabled);
    void setCameraThumbnailVisible(bool visible);
    
    // Invokable methods
    Q_INVOKABLE void enterShareMode();
    Q_INVOKABLE void exitShareMode();
    Q_INVOKABLE void excludeFromCapture(QWindow* window);
    
    // Static utility to check Windows version support
    static bool checkOverlaySupport();
    
signals:
    void isActiveChanged();
    void overlayEnabledChanged();
    void cameraThumbnailVisibleChanged();
    void elapsedSecondsChanged();
    
    // Signals for QML to show/hide overlay windows
    void enterShareModeRequested();
    void exitShareModeRequested();
    
private slots:
    void updateTimer();
    
private:
    bool isActive_{false};
    bool overlaySupported_{false};
    bool overlayEnabled_{true};  // Default: show overlay if supported
    bool cameraThumbnailVisible_{true};
    
    QElapsedTimer elapsedTimer_;
    QTimer* updateTimer_{nullptr};
    
    // Windows version check cached result
    static bool overlaySupported_cached_;
    static bool overlaySupported_checked_;
};

#endif // SHARE_MODE_MANAGER_H
