#include "installer_window.h"
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>

InstallerWindow::InstallerWindow(QWidget *parent)
    : QMainWindow(parent), buildProcess(nullptr) {
    setupUI();
    loadDrives();
}

void InstallerWindow::setupUI() {
    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    auto *title = new QLabel("T2 SDE Installer");
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    layout->addWidget(title);

    auto *instructions = new QLabel("Select a drive to install T2 SDE:");
    layout->addWidget(instructions);

    driveList = new QListWidget();
    layout->addWidget(driveList);

    progressBar = new QProgressBar();
    progressBar->setVisible(true); // intially hidden
    layout->addWidget(progressBar);

    // Install button
    installButton = new QPushButton("Install");
    layout->addWidget(installButton);

    setCentralWidget(centralWidget);
    setWindowTitle("T2 SDE Installer");
    resize(600, 400);

    connect(installButton, &QPushButton::clicked, this,
            &InstallerWindow::onInstallClicked);
}

void InstallerWindow::loadDrives() {
    // Read block devices from /proc/partitions or use lsblk
    // TODO: Fix this hack with actual cpp
    QProcess lsblk;
    lsblk.start("lsblk", QStringList()
                             << "-d" << "-n" << "-o" << "NAME,SIZE,TYPE");
    lsblk.waitForFinished();

    QString output = lsblk.readAllStandardOutput();
    QStringList lines = output.split("\n");

    for (const QString &line : lines) {
        if (line.trimmed().isEmpty())
            continue;
        if (line.contains("disk")) {
            QStringList parts =
                line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString device = "/dev/" + parts[0];
                QString size = parts[1];
                driveList->addItem(QString("%1 (%2)").arg(device, size));
            }
        }
    }
}

void InstallerWindow::onInstallClicked() {
    if (!driveList->currentItem()) {
        QMessageBox::warning(this, "No Selection",
                             "Please select a drive first.");
        return;
    }

    QString selectedDrive = driveList->currentItem()->text();
    // Extract just the device name (/dev/sda)
    QString drive = selectedDrive.split(" ").first();

    auto reply = QMessageBox::question(
        this, "Confirm Installation",
        QString("Install T2 SDE to %1?\n\n"
                "⚠️ This will ERASE ALL DATA on this drive!")
            .arg(drive),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        startInstallation(drive);
    }
}

void InstallerWindow::startInstallation(const QString &drive) {
    installButton->setEnabled(false);
    driveList->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);

    // Create QProcess to run the T2 build scripts
    buildProcess = new QProcess(this);

    connect(buildProcess, &QProcess::readyReadStandardOutput, this,
            &InstallerWindow::onProcessOutput);
    connect(buildProcess, &QProcess::finished, this,
            &InstallerWindow::onProcessFinished);
    // TODO: find actual script that installs this
    buildProcess->start("/path/to/t2sde/scripts/Build-Target",
                        QStringList() << "desktop");
}

void InstallerWindow::onProcessOutput() {
    QString output = buildProcess->readAllStandardOutput();
    // TODO: Create nice GUI progress bar in a widget....
    qDebug() << output;
}

void InstallerWindow::onProcessFinished(int exitCode,
                                        QProcess::ExitStatus exitStatus) {
    progressBar->setVisible(false);

    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        QMessageBox::information(this, "Success",
                                 "Installation completed successfully!");
    } else {
        QMessageBox::critical(
            this, "Error",
            QString("Installation failed with exit code %1").arg(exitCode));
    }

    installButton->setEnabled(true);
    driveList->setEnabled(true);
}
