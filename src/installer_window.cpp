#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>

#include <iostream>
#include <fstream>
#include <filesystem>

#include "installer_window.h"

#define DEFAULT_DRIVES_PATH "/sys/block"

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

    installButton = new QPushButton("Install");
    layout->addWidget(installButton);

    setCentralWidget(centralWidget);
    setWindowTitle("T2 SDE Installer");
    resize(600, 400);

    connect(installButton, &QPushButton::clicked, this,
            &InstallerWindow::onInstallClicked);
}

void InstallerWindow::loadDrives() {
    std::string path = DEFAULT_DRIVES_PATH;

    for (const auto& device : std::filesystem::directory_iterator(path)) {
	std::string san_device = device.path().filename().string();
	std::cout << san_device << std::endl;
	//driveList->addItem(san_device);
	
	std::filesystem::path size_device_path = device.path() / "size";
        std::ifstream size_device(size_device_path);
        uint64_t sectors = 0;

        if (size_device >> sectors) {
            // Calculate GB: (sectors * 512 bytes) / 1024^3
            double size_device_gb = (sectors * 512.0) / (1024.0 * 1024.0 * 1024.0); // TODO: support mg, gb and tb
	    std::cout << size_device_gb << std::endl;

	    QString display_device = QString("%1 (%2 GB)")
		    .arg(QString::fromStdString(san_device))
		    .arg(size_device_gb);
	    driveList->addItem(display_device);

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
