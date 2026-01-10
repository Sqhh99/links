#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QScrollArea>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    QString apiUrl() const;
    void setApiUrl(const QString& url);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onAccept();
    void onReject();
    void populateDevices();

private:
    void setupUI();
    void applyStyles();
    void loadSettings();
    void saveSettings();

    QWidget* titleBar_{nullptr};
    QLabel* titleLabel_{nullptr};
    QPushButton* closeButton_{nullptr};
    QPushButton* audioBtn_{nullptr};
    QPushButton* videoBtn_{nullptr};
    QPushButton* networkBtn_{nullptr};
    QStackedWidget* stack_{nullptr};
    QWidget* navContainer_{nullptr};
    QComboBox* micCombo_{nullptr};
    QComboBox* speakerCombo_{nullptr};
    QComboBox* cameraCombo_{nullptr};
    QComboBox* resolutionCombo_{nullptr};
    QCheckBox* echoCancel_{nullptr};
    QCheckBox* noiseSuppression_{nullptr};
    QCheckBox* hardwareAccel_{nullptr};
    QLineEdit* apiUrlInput_{nullptr};
    QPushButton* saveButton_{nullptr};
    QPushButton* cancelButton_{nullptr};

    bool dragging_{false};
    QPoint dragStartPos_;
};

#endif // SETTINGS_DIALOG_H
