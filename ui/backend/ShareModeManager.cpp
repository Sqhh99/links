/*
 * Copyright (c) 2026 Links Project
 * ShareModeManager - Manages Screen Share Mode with floating overlay controls
 */

#include "ShareModeManager.h"
#include "../utils/logger.h"
#include "../../core/platform_window_ops.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <versionhelpers.h>
#endif

// Static member initialization
bool ShareModeManager::overlaySupported_cached_ = false;
bool ShareModeManager::overlaySupported_checked_ = false;

ShareModeManager::ShareModeManager(QObject* parent)
    : QObject(parent)
{
    // Check overlay support once
    overlaySupported_ = checkOverlaySupport();
    
    // Default: overlay enabled if supported, otherwise disabled
    overlayEnabled_ = overlaySupported_;
    
    // Timer for updating elapsed time display
    updateTimer_ = new QTimer(this);
    updateTimer_->setInterval(1000);  // Update every second
    connect(updateTimer_, &QTimer::timeout, this, &ShareModeManager::updateTimer);
    
    Logger::instance().info(QString("ShareModeManager created, overlay supported: %1, enabled: %2")
        .arg(overlaySupported_ ? "yes" : "no")
        .arg(overlayEnabled_ ? "yes" : "no"));
}

ShareModeManager::~ShareModeManager()
{
    if (isActive_) {
        exitShareMode();
    }
}

bool ShareModeManager::checkOverlaySupport()
{
    if (overlaySupported_checked_) {
        return overlaySupported_cached_;
    }
    
    overlaySupported_checked_ = true;
    
#ifdef Q_OS_WIN
    // WDA_EXCLUDEFROMCAPTURE requires Windows 10 version 2004 (build 19041) or later
    // We use RtlGetVersion to get accurate version info
    
    OSVERSIONINFOEXW osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    
    // RtlGetVersion is more reliable than GetVersionEx which may be shimmed
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
        if (RtlGetVersion) {
            RtlGetVersion((PRTL_OSVERSIONINFOW)&osvi);
        }
    }
    
    // Windows 10 (10.0.19041+) supports WDA_EXCLUDEFROMCAPTURE
    if (osvi.dwMajorVersion > 10 ||
        (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 19041)) {
        overlaySupported_cached_ = true;
        Logger::instance().info(QString("Windows build %1 supports WDA_EXCLUDEFROMCAPTURE")
            .arg(osvi.dwBuildNumber));
    } else {
        overlaySupported_cached_ = false;
        Logger::instance().warning(QString("Windows build %1 does not support WDA_EXCLUDEFROMCAPTURE (requires 19041+)")
            .arg(osvi.dwBuildNumber));
    }
#else
    // Non-Windows platforms don't have this API
    overlaySupported_cached_ = false;
#endif
    
    return overlaySupported_cached_;
}

void ShareModeManager::setOverlayEnabled(bool enabled)
{
    if (overlayEnabled_ != enabled) {
        overlayEnabled_ = enabled;
        // Note: Preference persistence can be added when Settings class supports generic key/value
        emit overlayEnabledChanged();
        
        Logger::instance().info(QString("Share mode overlay %1").arg(enabled ? "enabled" : "disabled"));
    }
}

void ShareModeManager::setCameraThumbnailVisible(bool visible)
{
    if (cameraThumbnailVisible_ != visible) {
        cameraThumbnailVisible_ = visible;
        emit cameraThumbnailVisibleChanged();
    }
}

int ShareModeManager::elapsedSeconds() const
{
    if (!isActive_ || !elapsedTimer_.isValid()) {
        return 0;
    }
    return static_cast<int>(elapsedTimer_.elapsed() / 1000);
}

QString ShareModeManager::formattedTime() const
{
    int total = elapsedSeconds();
    int hours = total / 3600;
    int minutes = (total % 3600) / 60;
    int seconds = total % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void ShareModeManager::enterShareMode()
{
    if (isActive_) {
        Logger::instance().warning("Already in share mode");
        return;
    }
    
    Logger::instance().info("Entering share mode");
    isActive_ = true;
    elapsedTimer_.start();
    updateTimer_->start();
    
    emit isActiveChanged();
    emit elapsedSecondsChanged();
    emit enterShareModeRequested();
}

void ShareModeManager::exitShareMode()
{
    if (!isActive_) {
        Logger::instance().warning("Not in share mode");
        return;
    }
    
    Logger::instance().info(QString("Exiting share mode after %1").arg(formattedTime()));
    
    updateTimer_->stop();
    elapsedTimer_.invalidate();
    isActive_ = false;
    
    emit isActiveChanged();
    emit elapsedSecondsChanged();
    emit exitShareModeRequested();
}

void ShareModeManager::excludeFromCapture(QWindow* window)
{
    if (!window) {
        Logger::instance().warning("excludeFromCapture: null window");
        return;
    }
    
#ifdef Q_OS_WIN
    if (!overlaySupported_) {
        Logger::instance().info("excludeFromCapture: not supported on this Windows version");
        return;
    }
    
    const auto windowId = static_cast<links::core::WindowId>(window->winId());
    if (windowId == 0) {
        Logger::instance().warning("excludeFromCapture: failed to get window id");
        return;
    }

    bool result = links::core::excludeFromCapture(windowId);
    if (result) {
        Logger::instance().info(QString("Window excluded from capture: WId=0x%1")
            .arg(static_cast<quintptr>(windowId), 0, 16));
    } else {
        DWORD error = GetLastError();
        Logger::instance().error(QString("SetWindowDisplayAffinity failed: error=%1").arg(error));
    }
#else
    Q_UNUSED(window);
    Logger::instance().info("excludeFromCapture: not implemented on this platform");
#endif
}

void ShareModeManager::updateTimer()
{
    emit elapsedSecondsChanged();
}
