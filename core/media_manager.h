#ifndef MEDIA_MANAGER_H
#define MEDIA_MANAGER_H

#include <QObject>
#include <QString>

// Placeholder for future media device management
class MediaManager : public QObject
{
    Q_OBJECT
    
public:
    explicit MediaManager(QObject* parent = nullptr);
    ~MediaManager() override = default;
    
    // TODO: Add device enumeration
    // TODO: Add device selection
    // TODO: Add audio level monitoring
};

#endif // MEDIA_MANAGER_H
