/*
* Copyright (c) 2012-2015 Christian Surlykke, Petr Vanek
*
* This file is part of lightdm-qt5-greeter 
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/
#include <QApplication>
#include <QDebug>
#include <QPalette>
#include <QRect>
#include <QScreen>
#include <QString>

#include "mainwindow.h"
#include "loginform.h"
#include "settings.h"
#include "downloader.h"

MainWindow::MainWindow(int screen, QWidget *parent) :
        QWidget(parent),
        m_Screen(screen) {
    setObjectName(QString("MainWindow_%1").arg(screen));


    QRect screenRect = QGuiApplication::screens().at(screen)->availableGeometry();
    setGeometry(screenRect);

    setBackground();

    // display login dialog only in the main screen

    if (showLoginForm()) {
        m_LoginForm = new LoginForm(this);

        int maxX = screenRect.width() - m_LoginForm->width();
        int maxY = screenRect.height() - m_LoginForm->height();
        int defaultX = 10 * maxX / 100;
        int defaultY = 50 * maxY / 100;
        int offsetX = getOffset(Settings().offsetX(), maxX, defaultX);
        int offsetY = getOffset(Settings().offsetY(), maxY, defaultY);

        m_LoginForm->move(offsetX, offsetY);
        m_LoginForm->show();

        connect(m_LoginForm, SIGNAL(passwordWasInvalid(bool)), this, SLOT(showWrongPasswordLabel(bool)));
        connect(m_LoginForm, SIGNAL(userIsEnteringData()), this, SLOT(hideWrongPasswordLabel()));
        connect(m_LoginForm, SIGNAL(contestCantBeLoaded(QString)), this, SLOT(showContestcantBeLoaded(QString)));

        // This hack ensures that the primary screen will have focus
        // if there are more screens (move the mouse cursor in the center
        // of primary screen - not in the center of all X area). It
        // won't affect single-screen environments.
        int centerX = screenRect.width() / 2 + screenRect.x();
        int centerY = screenRect.height() / 2 + screenRect.y();
        QCursor::setPos(centerX, centerY);

        m_InfoLabel = new QLabel(this);
        m_InfoLabel->setWordWrap(true);
        m_InfoLabel->setAlignment(Qt::AlignCenter);
        m_InfoLabel->setVisible(false);

        QPalette pal = QPalette();
        pal.setColor(QPalette::Window, Qt::red);
        pal.setColor(QPalette::WindowText, Qt::white);

        QFont font = QFont();
        font.setPointSize(16);

        m_InfoLabel->setAutoFillBackground(true);
        m_InfoLabel->setPalette(pal);
        m_InfoLabel->setFont(font);

        m_InfoLabel->move(0, this->height() - 100);
        m_InfoLabel->resize(this->width(), 100);
    }
}

MainWindow::~MainWindow() {
}

bool MainWindow::showLoginForm() {
    QScreen *m_Screen = QGuiApplication::primaryScreen();
    return m_Screen;
}

void MainWindow::setFocus(Qt::FocusReason reason) {
    if (m_LoginForm) {
        m_LoginForm->setFocus(reason);
    } else {
        QWidget::setFocus(reason);
    }
}

int MainWindow::getOffset(QString settingsOffset, int maxVal, int defaultVal) {

    int offset = defaultVal > maxVal ? maxVal : defaultVal;

    if (!settingsOffset.isEmpty()) {
        if (QRegExp("^\\d+px$", Qt::CaseInsensitive).exactMatch(settingsOffset)) {
            offset = settingsOffset.left(settingsOffset.size() - 2).toInt();
            if (offset > maxVal) offset = maxVal;
        } else if (QRegExp("^\\d+%$", Qt::CaseInsensitive).exactMatch(settingsOffset)) {
            int offsetPct = settingsOffset.left(settingsOffset.size() - 1).toInt();
            if (offsetPct > 100) offsetPct = 100;
            offset = (maxVal * offsetPct) / 100;
        } else {
            qWarning() << "Could not understand" << settingsOffset
                       << "- must be of form <positivenumber>px or <positivenumber>%, e.g. 35px or 25%";
        }
    }

    return offset;
}

void MainWindow::setBackground() {
    QImage backgroundImage;

    auto backgroundImagePath = Settings().backgrundImagePath();

    if (!backgroundImagePath.isEmpty()) {
        // First, check if the background image is an URL, since we then need to download it
        if (backgroundImagePath.startsWith("http")) {
            auto fileName = backgroundImagePath.split("/").last();
            auto downloadTarget = Cache::GREETER_DATA_DIR_PATH + "/" + fileName;
            d = new Downloader;
            connect(d, SIGNAL(imageDownloaded(QString)), this, SLOT(backgroundDownloaded(QString)));
            d->doDownload(backgroundImagePath, downloadTarget);
        } else {
            backgroundImage = QImage(backgroundImagePath);
            if (backgroundImage.isNull()) {
                qWarning() << "Not able to read" << backgroundImagePath << "as image";
            }
        }
    }

    QPalette palette;
    QRect rect = QGuiApplication::screens().at(m_Screen)->availableGeometry();
    if (backgroundImage.isNull()) {
        palette.setColor(QPalette::Window, Qt::black);
    } else {
        QBrush brush(backgroundImage.scaled(rect.width(), rect.height()));
        palette.setBrush(this->backgroundRole(), brush);
    }
    this->setPalette(palette);
}

void MainWindow::backgroundDownloaded(const QString& path) {
    auto backgroundImage = QImage(path);
    if (backgroundImage.isNull()) {
        qWarning() << "Not able to read" << path << "as image";
    }

    QPalette palette;
    QRect rect = QGuiApplication::screens().at(m_Screen)->availableGeometry();
    QBrush brush(backgroundImage.scaled(rect.width(), rect.height()));
    palette.setBrush(this->backgroundRole(), brush);
    this->setPalette(palette);
}

void MainWindow::showWrongPasswordLabel(bool forAutoLogin) {
    if (forAutoLogin) {
        m_InfoLabel->setText("Auto login failed");
    } else {
        m_InfoLabel->setText("Wrong username or password");
    }
    m_InfoLabel->setVisible(true);
}

void MainWindow::hideWrongPasswordLabel() {
    m_InfoLabel->setVisible(false);
}

void MainWindow::showContestcantBeLoaded(const QString& message) {
    m_InfoLabel->setText("Contest can't be loaded: " + message);
    m_InfoLabel->setVisible(true);
}
