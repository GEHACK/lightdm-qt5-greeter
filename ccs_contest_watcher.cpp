//
// Created by Nicky Gerritsen on 08/11/2021.
//

#include "ccs_contest_watcher.h"
#include "settings.h"

CcsContestWatcher::CcsContestWatcher(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &CcsContestWatcher::replyFinished);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CcsContestWatcher::checkCcsUrl);
}

void CcsContestWatcher::startWatching(QString u) {
    url = std::move(u);
    timer->start(3000);
    checkCcsUrl();
}

void CcsContestWatcher::checkCcsUrl() {
    manager->get(QNetworkRequest(QUrl(url)));
}

void CcsContestWatcher::replyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error()) {
        qDebug() << "ERROR checking CCS contest!";
        qDebug() << reply->errorString();
    } else {
        auto response = reply->readAll();
        auto data = QJsonDocument::fromJson(response);
        if (data.isNull()) {
            return;
        }

        if (!data.isObject()) {
            return;
        }

        auto contest = data.object();

        if (!contest.contains("start_time") || !contest.value("start_time").isString()) {
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
