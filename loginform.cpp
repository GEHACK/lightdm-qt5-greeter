/*
* Copyright (c) 2012-2015 Christian Surlykke
*
* This file is part of lightdm-qt5-greeter 
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/
#include <QAbstractListModel>
#include <QCompleter>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QStringList>

#include <QLightDM/UsersModel>

#include "loginform.h"
#include "ui_loginform.h"
#include "settings.h"

const int KeyRole = QLightDM::SessionsModel::KeyRole;

int rows(QAbstractItemModel& model) {
    return model.rowCount(QModelIndex());
}

QString displayData(QAbstractItemModel& model, int row, int role)
{
    QModelIndex modelIndex = model.index(row, 0);
    return model.data(modelIndex, role).toString();
}

LoginForm::LoginForm(QWidget *parent) :
    QWidget(parent), 
    ui(new Ui::LoginForm),
    m_Greeter(),
    power(this),
    sessionsModel()
{
    if (!m_Greeter.connectSync()) {
        close();
    }

    contestWatcher = new CcsContestWatcher(this);
    ui->setupUi(this);
    initialize();
}

LoginForm::~LoginForm()
{
    delete ui;
}

void LoginForm::setFocus(Qt::FocusReason reason)
{
    auto inputChain = Settings().loginformShowInputChain();
    if (!inputChain.isEmpty() && positionInChain < inputChain.size()) {
        QWidget::setFocus(reason);
    } else if (ui->userInput->text().isEmpty()) {
        ui->userInput->setFocus(reason);
    } else {
        ui->passwordInput->setFocus(reason);
    }
}

void LoginForm::initialize()
{
    QPixmap icon(":/resources/rqt-2.png"); // This project came from Razor-qt
    ui->iconLabel->setPixmap(icon.scaled(ui->iconLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->hostnameLabel->setText(m_Greeter.hostname());

    ui->sessionCombo->setModel(&sessionsModel);

    addLeaveEntry(power.canShutdown(), "system-shutdown", tr("Shutdown"), "shutdown");
    addLeaveEntry(power.canRestart(), "system-reboot", tr("Restart"), "restart");
    addLeaveEntry(power.canHibernate(), "system-suspend-hibernate", tr("Hibernate"), "hibernate");
    addLeaveEntry(power.canSuspend(), "system-suspend", tr("Suspend"), "suspend");
    ui->leaveComboBox->setDisabled(ui->leaveComboBox->count() <= 1);

    ui->sessionCombo->setCurrentIndex(0);
    setCurrentSession(m_Greeter.defaultSessionHint());

    connect(ui->userInput, SIGNAL(editingFinished()), this, SLOT(userChanged()));
    connect(ui->leaveComboBox, SIGNAL(activated(int)), this, SLOT(leaveDropDownActivated(int)));
    connect(&m_Greeter, SIGNAL(showPrompt(QString, QLightDM::Greeter::PromptType)), this, SLOT(onPrompt(QString, QLightDM::Greeter::PromptType)));
    connect(&m_Greeter, SIGNAL(authenticationComplete()), this, SLOT(authenticationComplete()));
    connect(ui->userInput, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    connect(ui->passwordInput, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    connect(contestWatcher, &CcsContestWatcher::contestAboutToStart, this, &LoginForm::contestAboutToStart);
    connect(contestWatcher, &CcsContestWatcher::contestStarted, this, &LoginForm::contestStarted);
    connect(contestWatcher, &CcsContestWatcher::errorLoadingContest, this, &LoginForm::errorLoadingContest);
    connect(contestWatcher, &CcsContestWatcher::clearErrors, this, &LoginForm::clearErrors);

    ui->passwordInput->setEnabled(false);
    ui->passwordInput->clear();

    if (! m_Greeter.hideUsersHint()) {
        QStringList knownUsers;
        QLightDM::UsersModel usersModel;
        for (int i = 0; i < usersModel.rowCount(QModelIndex()); i++) {
            knownUsers << usersModel.data(usersModel.index(i, 0), QLightDM::UsersModel::NameRole).toString();
        }
        ui->userInput->setCompleter(new QCompleter(knownUsers));
        ui->userInput->completer()->setCompletionMode(QCompleter::InlineCompletion);
    }

    QString user = Cache().getLastUser();
    if (user.isEmpty()) {
        user = m_Greeter.selectUserHint();
    }
    ui->userInput->setText(user);

    auto inputChain = Settings().loginformShowInputChain();
    if (inputChain.isEmpty()) {
        userChanged();
    } else {
        // Validate input chain
        auto meta = QMetaEnum::fromType<Qt::Key>();
        QSet<QString> allKeysLower;
        for (int i = 0; i < meta.keyCount(); i++) {
            allKeysLower.insert(QString(meta.key(i)).toLower());
            allKeysLower.insert(QString(meta.key(i)).toLower().replace("key_", ""));
        }

        QStringList wrongKeys;
        for (const auto& item : inputChain) {
            if (!allKeysLower.contains(item.toLower())) {
                wrongKeys.append(item);
            }
        }

        if (!wrongKeys.isEmpty()) {
            QString message = "The following keys in the input chain are not valid: " + wrongKeys.join(", ");
            QMessageBox::critical(this, "Wrong input chain", message);
        }

        ui->formFrame->hide();
    }

    if (!Settings().ccsContestApiUrl().isEmpty()) {
        contestWatcher->startWatching();

        if (Settings().ccsAutologinUsername().isEmpty()) {
            QMessageBox::critical(this, "No autologin user", "CCS contest API URL provided, but no autologin username");
        }

        if (Settings().ccsAutologinPassword().isEmpty()) {
            QMessageBox::critical(this, "No autologin user", "CCS contest API URL provided, but no autologin password");
        }
    }

    this->setFocusPolicy(Qt::StrongFocus);
    chainDone = Settings().loginformShowInputChain().isEmpty();
}

void LoginForm::userChanged()
{
    setCurrentSession(Cache().getLastSession(ui->userInput->text()));

    if (m_Greeter.inAuthentication()) {
        m_Greeter.cancelAuthentication();
    }
    if (! ui->userInput->text().isEmpty()) {
        m_Greeter.authenticate(ui->userInput->text());
//        ui->passwordInput->setFocus();
    }
    else {
//        ui->userInput->setFocus();
    }
}

void LoginForm::leaveDropDownActivated(int index)
{
    QString actionName = ui->leaveComboBox->itemData(index).toString();
    if      (actionName == "shutdown") power.shutdown();
    else if (actionName == "restart") power.restart();
    else if (actionName == "hibernate") power.hibernate();
    else if (actionName == "suspend") power.suspend();
}

void LoginForm::respond()
{
    m_Greeter.respond(ui->passwordInput->text().trimmed());
    ui->passwordInput->clear();
    ui->passwordInput->setEnabled(false);
}

void LoginForm::onPrompt(QString prompt, QLightDM::Greeter::PromptType promptType)
{
    ui->passwordInput->setEnabled(true);
    ui->passwordInput->setFocus();
}


void LoginForm::addLeaveEntry(bool canDo, QString iconName, QString text, QString actionName)
{
    if (canDo) {
        ui->leaveComboBox->addItem(QIcon::fromTheme(iconName), text, actionName);
    }
}

QString LoginForm::currentSession()
{
    QModelIndex index = sessionsModel.index(ui->sessionCombo->currentIndex(), 0, QModelIndex());
    return sessionsModel.data(index, QLightDM::SessionsModel::KeyRole).toString();
}

void LoginForm::setCurrentSession(QString session)
{
    for (int i = 0; i < ui->sessionCombo->count(); i++) {
        if (session == sessionsModel.data(sessionsModel.index(i, 0), KeyRole).toString()) {
            ui->sessionCombo->setCurrentIndex(i);
            return;
        }
    }
}

void LoginForm::authenticationComplete()
{
    if (m_Greeter.isAuthenticated()) {
        Cache().setLastUser(ui->userInput->text());
        Cache().setLastSession(ui->userInput->text(), currentSession());
        Cache().sync();
        m_Greeter.startSessionSync(currentSession());
    }
    else  {
        ui->passwordInput->clear();
        if (!isPerformingAutoLogin) {
            userChanged();
        }
        emit passwordWasInvalid(isPerformingAutoLogin);
        isPerformingAutoLogin = false;
        this->setFocus(Qt::MouseFocusReason);
    }
}

void LoginForm::contestAboutToStart() {
    if (m_Greeter.inAuthentication()) {
        m_Greeter.cancelAuthentication();
    }
    m_Greeter.authenticate(Settings().ccsAutologinUsername());
}

void LoginForm::contestStarted() {
    qDebug() << QDateTime::currentDateTime();
    m_Greeter.respond(Settings().ccsAutologinPassword());
    isPerformingAutoLogin = true;
}

void LoginForm::keyPressEvent(QKeyEvent *event)
{
    auto inputChain = Settings().loginformShowInputChain();
    if (!inputChain.isEmpty() && ui->formFrame->isHidden() && positionInChain < inputChain.size()) {
        emit userPressedKey();
        // Check if the current key press is the next in line
        auto meta = QMetaEnum::fromType<Qt::Key>();
        auto key = QString(meta.valueToKey(event->key()));
        // Allow both `key_x` and `x`
        if (key.toLower() == "key_" + inputChain[positionInChain].toLower() || key.toLower() == inputChain[positionInChain].toLower()) {
            qDebug() << "Chain advance";
            positionInChain++;
            if (positionInChain == inputChain.size()) {
                emit userIsEnteringData();
                qDebug() << "Chain done";
                ui->formFrame->show();
                userChanged();
                chainDone = true;
            }
        } else {
            qDebug() << "Chain reset";
            positionInChain = 0;
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        respond();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void LoginForm::textChanged(QString text) {
    emit userIsEnteringData();
}

void LoginForm::errorLoadingContest(const QString& message) {
    emit contestCantBeLoaded(message);
}

void LoginForm::clearErrors() {
    emit contestCanBeLoaded();
}