#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QTimer>
#include "../core/network_client.h"
#include "WindowEffect.h"
#include <QMouseEvent>
#include "settings_dialog.h"

class LoginWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override;
    
private slots:
    void onJoinClicked();
    void onQuickJoinClicked();
    void onCreateRoomClicked();
    void onTokenReceived(const TokenResponse& response);
    void onNetworkError(const QString& error);
    void showSettingsDialog();
    void switchTab(int index);
    
private:
    void setupUI();
    void setupConnections();
    void applyStyles();
    void showError(const QString& message);
    void showLoading(bool show);
    void joinConference(const TokenResponse& response);
    bool eventFilter(QObject* watched, QEvent* event) override;
    
    // UI Components
    QWidget* centralWidget_;
    QWidget* titleBar_;
    QLabel* titleLabel_;
    QPushButton* minimizeButton_;
    QPushButton* settingsButton_;
    QPushButton* closeButton_;
    QWidget* heroPanel_;
    QLabel* timeLabel_;
    QLabel* dateLabel_;
    QLabel* highlightLabel_;
    QStackedWidget* formStack_;
    QPushButton* joinTabButton_;
    QPushButton* startTabButton_;
    QPushButton* scheduleTabButton_;
    QLineEdit* userNameInput_;
    QLineEdit* roomNameInput_;
    QLineEdit* scheduledTimeInput_;
    QPushButton* joinButton_;
    QPushButton* quickJoinButton_;
    QPushButton* createRoomButton_;
    QPushButton* micToggleButton_;
    QPushButton* camToggleButton_;
    QLabel* statusLabel_;
    QWidget* loadingWidget_;
    
    // Network
    NetworkClient* networkClient_;
    
    // State
    bool isLoading_;
    bool dragging_{false};
    QPoint dragPos_;
    WindowEffect windowEffect_;
    SettingsDialog* settingsDialog_{nullptr};
    int cornerRadius_{14};
    QTimer* clockTimer_{nullptr};
};

#endif // LOGIN_WINDOW_H
