//
// Created by Nicky Gerritsen on 08/11/2021.
//

#include "ccs_contest_watcher.h"
#include "settings.h"

CcsContestWatcher::CcsContestWatcher(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &CcsContestWatcher::replyFinished);
    connect(manager, &QNetworkAccessManager::sslErrors, this, &CcsContestWatcher::allowSslErrors);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CcsContestWatcher::checkCcsUrl);
}

void CcsContestWatcher::startWatching() {
    timer->start(3000);
    checkCcsUrl();
}

void CcsContestWatcher::checkCcsUrl() {
    manager->get(QNetworkRequest(QUrl(Settings().ccsContestApiUrl())));
}

void CcsContestWatcher::replyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error()) {
        qDebug() << "ERROR checking CCS contest!";
        qDebug() << reply->errorString();
        emit errorLoadingContest(reply->errorString());
    } else {
        auto response = reply->readAll();
        auto data = QJsonDocument::fromJson(response);
        if (data.isNull()) {
            emit errorLoadingContest("Data is null");
            return;
        }

        if (!data.isObject()) {
            emit errorLoadingContest("Data is not an object");
            return;
        }

        auto contest = data.object();

        if (!contest.contains("start_time") || !contest.value("start_time").isString()) {
            emit errorLoadingContest("Contest has no start time");
            return;
        }

        auto contestStartTime = contest.value("start_time").toString();
        auto start = QDateTime::fromString(contestStartTime, Qt::ISODate);

        // If the contest is less than 15 seconds away, emit a signal that it is about to start and prepare to emit a
        // signal that it has started
        auto diff = QDateTime::currentDateTime().msecsTo(start);

        qDebug() << "Contest will start at" << start << ", i.e in" << diff << "ms";

        if (diff <= ABOUT_TO_START_MSEC) {
            emit contestAboutToStart();

            // Start a single shot timer to actually start the contest, but never start it within 500ms of the previous signal
            diff = std::max(diff, (long long) Settings().ccsStartMinimumMsec());

            QTimer::singleShot(diff, Qt::PreciseTimer, [this]() {
                emit contestStarted();
            });

            timer->stop();
        }
    }
}

void CcsContestWatcher::allowSslErrors(QNetworkReply *reply, const QList<QSslError> &errors) {
    reply->ignoreSslErrors();
}
