#ifndef INSTALLER_WINDOW_H
#define INSTALLER_WINDOW_H

#include <QListWidget>
#include <QMainWindow>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>

class InstallerWindow : public QMainWindow {
    Q_OBJECT

  public:
    InstallerWindow(QWidget *parent = nullptr);

  private slots:
    void onInstallClicked();
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

  private:
    QListWidget *driveList;
    QPushButton *installButton;
    QProgressBar *progressBar;
    QProcess *buildProcess;

    void setupUI();
    void loadDrives();
    void startInstallation(const QString &drive);
};

#endif
