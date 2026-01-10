#ifndef PARTICIPANT_ITEM_H
#define PARTICIPANT_ITEM_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include "../core/conference_manager.h"

class ParticipantItem : public QWidget
{
    Q_OBJECT
    
public:
    explicit ParticipantItem(const ParticipantInfo& info, QWidget* parent = nullptr);
    ~ParticipantItem() override = default;
    
    void updateInfo(const ParticipantInfo& info);
    QString getIdentity() const { return identity_; }
    
    // Set whether the local user is a host (controls what buttons are shown)
    void setHostMode(bool isLocalHost);
    
    // Mark this as the local participant's item
    void setIsLocalParticipant(bool isLocal);

signals:
    void micToggleClicked(const QString& identity);
    void cameraToggleClicked(const QString& identity);
    void kickClicked(const QString& identity);

protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    void setupUI();
    void updateButtonStates();
    
    QString identity_;
    QString name_;
    bool isLocalParticipant_{false};
    bool isLocalHost_{false};
    bool micEnabled_{false};
    bool camEnabled_{false};
    
    QLabel* nameLabel_;
    QPushButton* micButton_;
    QPushButton* cameraButton_;
    QPushButton* kickButton_;
};

#endif // PARTICIPANT_ITEM_H