#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setTheme NOTIFY themeChanged)

public:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    QString currentTheme() const { return currentTheme_; }
    Q_INVOKABLE void setTheme(const QString& theme);

signals:
    void themeChanged();

private:
    QString currentTheme_;
};

#endif // THEMEMANAGER_H
