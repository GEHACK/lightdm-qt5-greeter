//
// Created by Nicky Gerritsen on 08/11/2021.
//

#ifndef CCS_CONTEST_WATCHER_H
#define CCS_CONTEST_WATCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QList>
#include <QSslError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

class CcsContestWatcher : public QObject {
Q_OBJECT
public:
    explicit CcsContestWatcher(QObject *parent = nullptr);

    void startWatching();

signals:

    void contestAboutToStart();
    void contestStarted();
    void errorLoadingContest(QString message);

private slots:

    void checkCcsUrl();

    void allowSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void replyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
    QTimer *timer;
    static const int ABOUT_TO_START_MSEC = 15000;
    static const int START_MINIMUM_MSEC = 500;
};

#endif // CCS_CONTEST_WATCHER_H
