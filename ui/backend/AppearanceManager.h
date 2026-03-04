#ifndef APPEARANCEMANAGER_H
#define APPEARANCEMANAGER_H

#include <QObject>

class AppearanceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool autoHideConferenceChrome READ autoHideConferenceChrome WRITE setAutoHideConferenceChrome NOTIFY autoHideConferenceChromeChanged)

public:
    explicit AppearanceManager(QObject* parent = nullptr);
    ~AppearanceManager() override = default;

    bool autoHideConferenceChrome() const { return autoHideConferenceChrome_; }

    Q_INVOKABLE void setAutoHideConferenceChrome(bool enabled);

signals:
    void autoHideConferenceChromeChanged();

private:
    bool autoHideConferenceChrome_{true};
};

#endif // APPEARANCEMANAGER_H
