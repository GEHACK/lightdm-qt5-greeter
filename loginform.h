/*
* Copyright (c) 2012-2015 Christian Surlykke
*
* This file is part of lightdm-qt5-greeter 
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/
#ifndef LOGINFORM_H
#define LOGINFORM_H

#include <QDialog>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QMap>
#include <QProcess>
#include <QWidget>

#include <QLightDM/Power>
#include <QLightDM/Greeter>
#include <QLightDM/SessionsModel>
#include "ccs_contest_watcher.h"

namespace Ui
{
class LoginForm;
}

class LoginForm : public QWidget
{
    Q_OBJECT

friend class DecoratedUsersModel;

public:
    explicit LoginForm(QWidget *parent = 0);
    ~LoginForm();
    virtual void setFocus(Qt::FocusReason reason);

private slots:
    void userChanged();
    void leaveDropDownActivated(int index);
    void respond();
    void onPrompt(QString prompt, QLightDM::Greeter::PromptType promptType);
    void authenticationComplete();
    void contestAboutToStart();
    void contestStarted();
    void textChanged(QString text);
    void errorLoadingContest(const QString& message);

protected:
    virtual void keyPressEvent(QKeyEvent *event);

private:
    void initialize();
    void addLeaveEntry(bool canDo, QString iconName, QString text, QString actionName);
    QString currentSession();
    void setCurrentSession(QString session);

    Ui::LoginForm *ui;

    QLightDM::Greeter m_Greeter;
    QLightDM::PowerInterface power;
    QLightDM::SessionsModel sessionsModel;

    QMap<int, void (QLightDM::PowerInterface::*)()> powerSlots;

    int positionInChain = 0;

    bool isPerformingAutoLogin = false;
    bool chainDone;

    CcsContestWatcher *contestWatcher;

signals:
    void passwordWasInvalid(bool wasAutoLogin);
    void userIsEnteringData();
    void contestCantBeLoaded(const QString& message);
};

#endif // LOGINFORM_H
