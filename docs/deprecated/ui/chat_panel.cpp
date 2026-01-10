#include "chat_panel.h"
#include <QDateTime>
#include <QScrollBar>

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();
}

void ChatPanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    // Messages view
    messagesView_ = new QTextEdit(this);
    messagesView_->setReadOnly(true);
    messagesView_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    messagesView_->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(messagesView_, 1);
    
    // Input area container
    auto* inputContainer = new QWidget(this);
    inputContainer->setObjectName("inputContainer");
    auto* inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(8);
    
    messageInput_ = new QLineEdit(this);
    messageInput_->setPlaceholderText("Type a message...");
    messageInput_->setMinimumHeight(40);
    inputLayout->addWidget(messageInput_, 1);
    
    sendButton_ = new QPushButton(this);
    sendButton_->setText("➤");
    sendButton_->setFixedSize(40, 40);
    sendButton_->setCursor(Qt::PointingHandCursor);
    inputLayout->addWidget(sendButton_);
    
    layout->addWidget(inputContainer);
    
    // Connections
    connect(sendButton_, &QPushButton::clicked, this, &ChatPanel::onSendClicked);
    connect(messageInput_, &QLineEdit::returnPressed, this, &ChatPanel::onSendClicked);
}

void ChatPanel::applyStyles()
{
    QString styleSheet = R"(
        QTextEdit {
            background-color: transparent;
            color: #ffffff;
            border: none;
            font-size: 14px;
            selection-background-color: #5865f2;
        }
        
        QLineEdit {
            background-color: #2a2a35;
            color: #ffffff;
            border: 1px solid #3a3a4e;
            border-radius: 20px;
            padding: 8px 16px;
            font-size: 14px;
        }
        
        QLineEdit:focus {
            border: 1px solid #5865f2;
            background-color: #32323e;
        }
        
        QPushButton {
            background-color: #5865f2;
            color: #ffffff;
            border: none;
            border-radius: 20px;
            font-size: 16px;
            font-weight: bold;
            padding-bottom: 2px; /* Center the arrow slightly */
        }
        
        QPushButton:hover {
            background-color: #4752c4;
        }
        
        QPushButton:pressed {
            background-color: #3c45a5;
        }
        
        QScrollBar:vertical {
            border: none;
            background: transparent;
            width: 8px;
            margin: 0px 0px 0px 0px;
        }
        
        QScrollBar::handle:vertical {
            background: #3a3a4e;
            min-height: 20px;
            border-radius: 4px;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )";
    
    setStyleSheet(styleSheet);
}

void ChatPanel::addMessage(const ChatMessage& message)
{
    QString timestamp = QDateTime::fromMSecsSinceEpoch(message.timestamp)
                        .toString("hh:mm");
    
    QString headerColor = message.isLocal ? "#5865f2" : "#4caf50"; // Blue for local, green for remote
    
    // Modern chat bubble style
    // Use HTML for rich text in QTextEdit
    QString html;
    
    // We can't do full CSS bubbles easily in QTextEdit standard HTML engine, 
    // but we can make it look decent.
    
    html = QString(R"(
        <div style="margin-bottom: 16px;">
            <div style="color: %1; font-weight: 600; font-size: 12px; margin-bottom: 4px;">
                %2 <span style="color: #6a6a7e; font-weight: normal; margin-left: 8px;">%3</span>
            </div>
            <div style="color: #e0e0e0; font-size: 14px; line-height: 1.4;">
                %4
            </div>
        </div>
    )").arg(headerColor, message.sender, timestamp, message.message.toHtmlEscaped());
    
    messagesView_->append(html);
    
    // Auto-scroll to bottom
    QScrollBar* scrollBar = messagesView_->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ChatPanel::clear()
{
    messagesView_->clear();
}

void ChatPanel::onSendClicked()
{
    QString message = messageInput_->text().trimmed();
    if (message.isEmpty()) {
        return;
    }
    
    emit sendMessageRequested(message);
    messageInput_->clear();
}