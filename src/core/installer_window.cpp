#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

#include "installer_window.h"

#define DEFAULT_DRIVES_PATH "/sys/block"

class DriveItemWidget : public QWidget {
      public:
        DriveItemWidget(const QString &name, const QString &size,
                        QWidget *parent = nullptr)
            : QWidget(parent), driveName(name), driveSize(size),
              isHovered(false), isSelected(false) {
                setMinimumSize(450, 250);
                setCursor(Qt::PointingHandCursor);

                drivePixmap = QPixmap("../assets/drive.png");
        }

        QSize sizeHint() const override { return QSize(450, 250); }

        void setSelected(bool selected) {
                isSelected = selected;
                update();
        }

      protected:
        void enterEvent(QEnterEvent *) override { isHovered = true;  update(); }
        void leaveEvent(QEvent *)       override { isHovered = false; update(); }

        void paintEvent(QPaintEvent *) override {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing);

		QColor bg = isSelected ? QColor(45, 48, 90)
                          : isHovered  ? QColor(38, 42, 78)
                                       : QColor(30, 33, 65);
                p.setBrush(bg);
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(2, 2, width() - 4, height() - 4, 10, 10);

                if (isSelected) {
                        p.setPen(QPen(QColor(234, 179, 8), 1));
                        p.setBrush(Qt::NoBrush);
                        p.drawRoundedRect(2, 2, width() - 4, height() - 4, 10, 10);

                        // Yellow left accent bar
                        p.setBrush(QColor(234, 179, 8));
                        p.setPen(Qt::NoPen);
                        p.drawRoundedRect(2, 2, 4, height() - 4, 2, 2);
                } else {
                        p.setPen(QPen(QColor(45, 52, 85), 1));
                        p.setBrush(Qt::NoBrush);
                        p.drawRoundedRect(2, 2, width() - 4, height() - 4, 10, 10);
                }

                // Drive icon or fallback
                if (!drivePixmap.isNull()) {
                        QPixmap scaled = drivePixmap.scaled(
                            36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        int x = (width() - scaled.width()) / 2;
                        p.drawPixmap(x, 14, scaled);
                } else {
                        int cx = width() / 2;
                        p.setBrush(isSelected ? QColor(211, 47, 47) : QColor(190, 190, 198));
                        p.setPen(Qt::NoPen);
                        p.drawRoundedRect(cx - 18, 14, 36, 28, 4, 4);
                        p.setBrush(QColor(255, 255, 255, 160));
                        p.drawRect(cx - 13, 20, 26, 3);
                        p.drawRect(cx - 13, 27, 26, 3);
                        p.drawRect(cx - 13, 34, 26, 3);
                        // Yellow LED dot
                        p.setBrush(QColor(234, 179, 8));
                        p.drawEllipse(cx + 8, 17, 5, 5);
                }

                // Device name
                p.setPen(QColor(220, 225, 255));
                QFont nameFont = p.font();
                nameFont.setPointSize(11);
                nameFont.setBold(true);
                p.setFont(nameFont);
                p.drawText(QRect(8, 58, width() - 16, 22), Qt::AlignCenter, driveName);

                // Size 
                QFont sizeFont = p.font();
                sizeFont.setPointSize(9);
                sizeFont.setBold(false);
                p.setFont(sizeFont);
                p.setPen(QColor(234, 179, 8));
                p.drawText(QRect(8, 78, width() - 16, 18), Qt::AlignCenter, driveSize);
        }

      private:
        QString driveName;
        QString driveSize;
        QPixmap drivePixmap;
        bool isHovered;
        bool isSelected;
};

void InstallerWindow::setupUI() {
        auto *centralWidget = new QWidget(this);
	centralWidget->setStyleSheet("background-color: #131324;");

        auto *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        auto *topBar = new QWidget();
        topBar->setFixedHeight(48);
	topBar->setContentsMargins(0, 0, 0, 0);
	mainLayout->setContentsMargins(0, 0, 0, 0); //HACK
        topBar->setStyleSheet(
	    "background-color: #12122a;"
            "border-bottom: 1px solid #eab30830;");

        auto *topBarLayout = new QHBoxLayout(topBar);
        topBarLayout->setContentsMargins(16, 0, 16, 0);
        topBarLayout->setSpacing(10);

        auto *logoLabel = new QLabel();
        QPixmap logo("../assets/t2_logo.png");
        if (!logo.isNull()) {
                logoLabel->setPixmap(
                    logo.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
                logoLabel->setText("T2");
                logoLabel->setStyleSheet(
                    "font-size: 13px; font-weight: bold; color: #d32f2f;"
                    "background: transparent;");
        }
        topBarLayout->addWidget(logoLabel);

        auto *appTitle = new QLabel("T2 SDE Installer");
        appTitle->setStyleSheet(
            //"font-size: 14px; font-weight: 600; color: #111118;"
            "font-size: 17px; font-weight: 600; color: #dde2ff;"
            "background: transparent;");
        topBarLayout->addWidget(appTitle);
        topBarLayout->addStretch();

        mainLayout->addWidget(topBar);

        auto *contentWidget = new QWidget();
        auto *contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(24, 20, 24, 24);
        contentLayout->setSpacing(12);

        auto *sectionLabel = new QLabel("Select a drive to install T2 SDE");
        sectionLabel->setStyleSheet(
	    "font-size: 12px; font-weight: 600; color: #a8a8cc;"
            "background: transparent;");
        contentLayout->addWidget(sectionLabel);

        driveList = new QListWidget();
        driveList->setFlow(QListView::LeftToRight);
        driveList->setWrapping(true);
        driveList->setResizeMode(QListView::Adjust);
        driveList->setSpacing(8);
	driveList->setStyleSheet(
            "QListWidget {"
	    "   border: 1px solid #32325a;"
            "   border-radius: 12px;"
	    "   background-color: #1c1c38;"
            "   padding: 12px;"
	    "QListWidget::item { border: none; background-color: #1c1c38; }"
            "QListWidget::item:selected { background-color: #1c1c38; }"
            "QListWidget::item:hover { background-color: #1c1c38; }"
            "}");


        contentLayout->addWidget(driveList, 1);

        progressBar = new QProgressBar();
        progressBar->setVisible(false);
        progressBar->setFixedHeight(6);
        progressBar->setTextVisible(false);
	progressBar->setStyleSheet(
            "QProgressBar {"
            "   border: none; border-radius: 3px;"
            "   background-color: #e5e5ea;"
            "}"
            "QProgressBar::chunk {"
            "   background-color: #2563c8; border-radius: 3px;"
            "}");
	
        contentLayout->addWidget(progressBar);

        auto *buttonRow = new QHBoxLayout();
        buttonRow->addStretch();

        installButton = new QPushButton("Install");
        installButton->setFixedSize(220, 46);
	 installButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #eab308;"
            "   color: #0f1228;;"
            "   font-size: 16px;"
            "   font-weight: 600;"
            "   border: none;"
            "   border-radius: 23px;"
            "}"
            "QPushButton:hover   { background-color: #ca9a06; }"
            "QPushButton:pressed { background-color: #a87d05; }"
            "QPushButton:disabled {"
            "   background-color: #1e2448; color: #3a4070;"
            "}");
        


        buttonRow->addWidget(installButton);
        buttonRow->addStretch();
        contentLayout->addLayout(buttonRow);

        mainLayout->addWidget(contentWidget, 1);

        setCentralWidget(centralWidget);
        setWindowTitle("T2 SDE Installer");
        resize(780, 560);

        connect(installButton, &QPushButton::clicked, this,
                &InstallerWindow::onInstallClicked);

        connect(driveList, &QListWidget::currentItemChanged, this,
                [this](QListWidgetItem *current, QListWidgetItem *previous) {
                        if (previous) {
                                auto *w = static_cast<DriveItemWidget *>(
                                    driveList->itemWidget(previous));
                                if (w) w->setSelected(false);
                        }
                        if (current) {
                                auto *w = static_cast<DriveItemWidget *>(
                                    driveList->itemWidget(current));
                                if (w) w->setSelected(true);
                        }
                });
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

		// Omit loop devices and partitions
		if (std::regex_match(san_device, std::regex("loop.*"))  ||
                    std::regex_match(san_device, std::regex("zram.*")) 	||
                    std::regex_match(san_device, std::regex("sr.*")) 	||
                    std::regex_match(san_device, std::regex("fd.*")))
                        continue;

                std::filesystem::path size_device_path = device.path() / "size";
                std::ifstream size_device(size_device_path);
                uint64_t sectors = 0;

                if (size_device >> sectors) {
                        double size_device_gb =
                            (sectors * 512.0) / (1024.0 * 1024.0 * 1024.0);

                        QString device_name = QString::fromStdString(san_device);
                        QString size_str =
                            QString("%1 GB").arg(size_device_gb, 0, 'f', 1);

                	std::filesystem::path model_path = device.path() / "device/model";
			std::ifstream model_file(model_path);
			std::string model_name;
			std::getline(model_file, model_name);

			// Some drive manufacturers like to put the size of the drive in the name
			// We will trim it as to not duplicate size
			if (std::regex_search(model_name, std::regex("TB|GB|MB"))) {
				model_name = std::regex_replace( 
					model_name,
					std::regex("\\s*\\d+(\\.\\d+)?\\s*(TB|GB|MB)"),	
					""
					);
			}

			QString device_display_str = QString::fromStdString(model_name) + 
				" (" + device_name + ")";


                        auto *item = new QListWidgetItem(driveList);
                        auto *widget = new DriveItemWidget(device_display_str, size_str);

                        item->setSizeHint(widget->sizeHint());
                        driveList->addItem(item);
                        driveList->setItemWidget(item, widget);
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

        QString drive =
            "/dev/" + driveList->currentItem()->data(Qt::UserRole).toString();

        auto reply = QMessageBox::question(
            this, "Confirm Installation",
            QString("Install T2 SDE to %1?\n\nThis will ERASE ALL DATA on this drive.")
                .arg(drive),
            QMessageBox::Yes | QMessageBox::No);

        std::cout << "Going to start installation on drive: "
                  << drive.toStdString() << std::endl;

        if (reply == QMessageBox::Yes) {
                startInstallation(drive);
        }
}

void InstallerWindow::startInstallation(const QString &drive) {
        std::cout << "Starting installation on drive: "
                  << drive.toStdString() << std::endl;

        installButton->setEnabled(false);
        driveList->setEnabled(false);
        progressBar->setVisible(true);
        progressBar->setRange(0, 0);

        buildProcess = new QProcess(this);

        connect(buildProcess, &QProcess::readyReadStandardOutput, this,
                &InstallerWindow::onProcessOutput);
        connect(buildProcess, &QProcess::readyReadStandardError, this,
                &InstallerWindow::onProcessOutput);
        connect(buildProcess, &QProcess::finished, this,
                &InstallerWindow::onProcessFinished);

        std::cout << "Starting stone wrapper...\n";
        buildProcess->start("../scripts/stone_wrapper.sh",
                            QStringList() << drive);

        connect(buildProcess, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError error) {
                        qDebug() << "Process error:" << error
                                 << buildProcess->errorString();
                });
}

void InstallerWindow::onProcessOutput() {
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
