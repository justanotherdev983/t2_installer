#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>

#include <filesystem>
#include <fstream>
#include <iostream>

#include "installer_window.h"

#define DEFAULT_DRIVES_PATH "/sys/block"

class DriveItemWidget : public QWidget {
      public:
        DriveItemWidget(const QString &name, const QString &size,
                        QWidget *parent = nullptr)
            : QWidget(parent), driveName(name), driveSize(size) {
                setMinimumSize(200, 180);

                drivePixmap = QPixmap("../assets/drive.png");
                if (drivePixmap.isNull()) {
                        drivePixmap =
                            QPixmap(); // Will draw a fallback icon instead
                }
        }

	QSize sizeHint() const override {
    		return QSize(200, 180);
	}

      protected:
        void paintEvent(QPaintEvent *) override {
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing);

                // Background card
                painter.setBrush(QColor(45, 45, 48));
                painter.setPen(QPen(QColor(70, 70, 75), 2));
                painter.drawRoundedRect(5, 5, width() - 10, height() - 10, 12,
                                        12);

                // Draw drive image or fallback icon
                if (!drivePixmap.isNull()) {
                        // Center the image at the top
                        int imgWidth = 80;
                        int imgHeight = 80;
                        QPixmap scaled = drivePixmap.scaled(
                            imgWidth, imgHeight, Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
                        int x = (width() - scaled.width()) / 2;
                        painter.drawPixmap(x, 25, scaled);
                } else {
                        // Fallback: draw simple drive icon
                        int centerX = width() / 2;
                        painter.setBrush(QColor(100, 100, 105));
                        painter.setPen(Qt::NoPen);
                        painter.drawRoundedRect(centerX - 30, 25, 60, 50, 5, 5);

                        painter.setBrush(QColor(180, 180, 190));
                        painter.drawRect(centerX - 22, 35, 44, 4);
                        painter.drawRect(centerX - 22, 45, 44, 4);
                        painter.drawRect(centerX - 22, 55, 44, 4);
                        painter.drawRect(centerX - 22, 65, 44, 4);
                }

                // Draw device name
                painter.setPen(QColor(220, 220, 225));
                QFont nameFont = painter.font();
                nameFont.setPointSize(14);
                nameFont.setBold(true);
                painter.setFont(nameFont);

                QRect nameRect(10, 115, width() - 20, 25);
                painter.drawText(nameRect, Qt::AlignCenter, driveName);

                // Draw size
                QFont sizeFont = painter.font();
                sizeFont.setPointSize(11);
                sizeFont.setBold(false);
                painter.setFont(sizeFont);
                painter.setPen(QColor(150, 150, 160));

                QRect sizeRect(10, 140, width() - 20, 20);
                painter.drawText(sizeRect, Qt::AlignCenter, driveSize);
        }

      private:
        QString driveName;
        QString driveSize;
        QPixmap drivePixmap;
};

void InstallerWindow::setupUI() {
        auto *centralWidget = new QWidget(this);
        auto *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(40, 40, 40, 40);
        mainLayout->setSpacing(25);

        auto *title = new QLabel("T2 SDE Installer");
        title->setStyleSheet(
            "font-size: 32px; font-weight: bold; color: #e0e0e5;");
        title->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(title);

        auto *instructions = new QLabel("Select a drive to install T2 SDE");
        instructions->setStyleSheet("font-size: 15px; color: #a0a0a8;");
        instructions->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(instructions);

        mainLayout->addSpacing(15);

        driveList = new QListWidget();
        driveList->setFlow(QListView::LeftToRight); // Horizontal layout!
        driveList->setWrapping(true);
        driveList->setResizeMode(QListView::Adjust);
        driveList->setSpacing(15);
        driveList->setStyleSheet("QListWidget {"
                                 "   border: 2px solid #3a3a3f;"
                                 "   border-radius: 10px;"
                                 "   background-color: #1e1e20;"
                                 "   padding: 15px;"
                                 "}"
                                 "QListWidget::item {"
                                 "   border: none;"
                                 "   background-color: transparent;"
                                 "}"
                                 "QListWidget::item:selected {"
                                 "   background-color: transparent;"
                                 "}"
                                 "QListWidget::item:hover {"
                                 "   background-color: transparent;"
                                 "}");
        mainLayout->addWidget(driveList);

        progressBar = new QProgressBar();
        progressBar->setVisible(false);
        progressBar->setStyleSheet("QProgressBar {"
                                   "   border: 2px solid #3a3a3f;"
                                   "   border-radius: 10px;"
                                   "   text-align: center;"
                                   "   height: 30px;"
                                   "   background-color: #1e1e20;"
                                   "   color: #e0e0e5;"
                                   "}"
                                   "QProgressBar::chunk {"
                                   "   background-color: #7c3aed;"
                                   "   border-radius: 8px;"
                                   "}");
        mainLayout->addWidget(progressBar);

        installButton = new QPushButton("Install");
        installButton->setStyleSheet("QPushButton {"
                                     "   background-color: #7c3aed;"
                                     "   color: white;"
                                     "   font-size: 17px;"
                                     "   font-weight: bold;"
                                     "   padding: 15px;"
                                     "   border: none;"
                                     "   border-radius: 10px;"
                                     "}"
                                     "QPushButton:hover {"
                                     "   background-color: #8b5cf6;"
                                     "}"
                                     "QPushButton:pressed {"
                                     "   background-color: #6d28d9;"
                                     "}"
                                     "QPushButton:disabled {"
                                     "   background-color: #4a4a4f;"
                                     "   color: #808085;"
                                     "}");
        mainLayout->addWidget(installButton);

        setCentralWidget(centralWidget);
        setWindowTitle("T2 SDE Installer");
        resize(800, 600);

        // Dark mode background
        centralWidget->setStyleSheet("background-color: #18181b;");

        connect(installButton, &QPushButton::clicked, this,
                &InstallerWindow::onInstallClicked);
}

InstallerWindow::InstallerWindow(QWidget *parent)
    : QMainWindow(parent), buildProcess(nullptr) {
        setupUI();
        loadDrives();
}

void InstallerWindow::loadDrives() {
        std::string path = DEFAULT_DRIVES_PATH;

        for (const auto &device : std::filesystem::directory_iterator(path)) {
                std::string san_device = device.path().filename().string();
                std::cout << san_device << std::endl;

                std::filesystem::path size_device_path = device.path() / "size";
                std::ifstream size_device(size_device_path);
                uint64_t sectors = 0;

                if (size_device >> sectors) {
                        // Calculate GB: (sectors * 512 bytes) / 1024^3
                        double size_device_gb =
                            (sectors * 512.0) /
                            (1024.0 * 1024.0 *
                             1024.0); // TODO: support mg, gb and tb std::cout
                                      // << size_device_gb << std::endl;

			QString device_name =
                            QString::fromStdString(san_device);
                        QString size_str =
                            QString("%1 GB").arg(size_device_gb, 0, 'f', 1);


                        QString display_device =
                            QString("%1 (%2 GB)")
                                .arg(QString::fromStdString(san_device))
                                .arg(size_device_gb);
                        //driveList->addItem(display_device);
                        auto *item = new QListWidgetItem(driveList);
                        auto *widget =
                            new DriveItemWidget(device_name, size_str);

                        item->setSizeHint(widget->sizeHint());
                        driveList->addItem(item);
                        driveList->setItemWidget(item, widget);

                        // Store device name in item data for later retrieval
                        item->setData(Qt::UserRole, device_name);
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
        //QString drive = selectedDrive.split(" ").first();
	QString drive = "/dev/" + driveList->currentItem()->data(Qt::UserRole).toString();

        auto reply = QMessageBox::question(
            this, "Confirm Installation",
            QString("Install T2 SDE to %1?\n\n"
                    "⚠️ This will ERASE ALL DATA on this drive!")
                .arg(drive),
            QMessageBox::Yes | QMessageBox::No);
	
	std::cout << "Going to starting installation on drive: " << drive.toStdString() << std::endl;

        if (reply == QMessageBox::Yes) {
                startInstallation(drive);
        }
}

void InstallerWindow::startInstallation(const QString &drive) {
	std::cout << "Starting installation on drive: " << drive.toStdString() << std::endl;
        installButton->setEnabled(false);
        driveList->setEnabled(false);
        progressBar->setVisible(true);
        progressBar->setRange(0, 0);

        // Create QProcess to run the T2 build scripts
        buildProcess = new QProcess(this);

        connect(buildProcess, &QProcess::readyReadStandardOutput, this,
                &InstallerWindow::onProcessOutput);
	connect(buildProcess, &QProcess::readyReadStandardError, this,
        	&InstallerWindow::onProcessOutput);
        connect(buildProcess, &QProcess::finished, this,
                &InstallerWindow::onProcessFinished);
	std::cout << "startings stone wrapper....\n";
        buildProcess->start("../scripts/stone_wrapper.sh",
                            QStringList() << drive);

	connect(buildProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    		qDebug() << "Process error:" << error << buildProcess->errorString();
	});
}

void InstallerWindow::onProcessOutput() {
        // TODO: Create nice GUI progress bar in a widget....
    buildProcess->setReadChannel(QProcess::StandardOutput);
    while (buildProcess->canReadLine())
        qDebug() << "[stdout]" << buildProcess->readLine().trimmed();
    buildProcess->setReadChannel(QProcess::StandardError);
    while (buildProcess->canReadLine())
        qDebug() << "[stderr]" << buildProcess->readLine().trimmed();
}

void InstallerWindow::onProcessFinished(int exitCode,
                                        QProcess::ExitStatus exitStatus) {
        progressBar->setVisible(false);

        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                QMessageBox::information(
                    this, "Success", "Installation completed successfully!");
        } else {
                QMessageBox::critical(
                    this, "Error",
                    QString("Installation failed with exit code %1")
                        .arg(exitCode));
        }

        installButton->setEnabled(true);
        driveList->setEnabled(true);
}
