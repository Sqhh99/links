#ifndef CHAT_PANEL_H
#define CHAT_PANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "../core/conference_manager.h"

class ChatPanel : public QWidget
{
    Q_OBJECT
    
public:
    explicit ChatPanel(QWidget* parent = nullptr);
    ~ChatPanel() override = default;
    
    void addMessage(const ChatMessage& message);
    void clear();
    
signals:
    void sendMessageRequested(const QString& message);
    
private slots:
    void onSendClicked();
    
private:
    void setupUI();
    void applyStyles();
    
    QTextEdit* messagesView_;
    QLineEdit* messageInput_;
    QPushButton* sendButton_;
};

#endif // CHAT_PANEL_H
