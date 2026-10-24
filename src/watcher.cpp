#include "watcher.h"
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <algorithm>

SessionWatcher::SessionWatcher(SessionStore *store, QObject *parent) : QObject(parent), store(store) {
    timer.setInterval(3000);
    connect(&timer, &QTimer::timeout, this, &SessionWatcher::scan);
}
void SessionWatcher::setHome(const QString &value) {
    home = value;
    cursors.clear();
    store->clear();
    scan();
    timer.start();
}
void SessionWatcher::setInterval(int milliseconds) { timer.setInterval(milliseconds); }
void SessionWatcher::scan() {
    const QString root = QDir(home).filePath("sessions");
    if (!QFileInfo(root).isDir()) {
        diagnostic = "missing";
        store->retainObserved({});
        emit scanned();
        return;
    }
    QList<QFileInfo> files;
    QDirIterator it(root, {"*.jsonl"}, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    const auto cutoff = QDateTime::currentDateTimeUtc().addDays(-7);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        if (info.lastModified() >= cutoff)
            files.append(info);
    }
    std::sort(files.begin(), files.end(),
              [](const auto &a, const auto &b) { return a.lastModified() > b.lastModified(); });
    diagnostic = "ok";
    store->beginBatch();
    QSet<QString> retained;
    for (int i = 0; i < qMin(64, files.size()); ++i) {
        const auto path = files[i].absoluteFilePath();
        retained.insert(path);
        read(path);
    }
    for (auto cur = cursors.begin(); cur != cursors.end();) {
        if (!retained.contains(cur.key()))
            cur = cursors.erase(cur);
        else
            ++cur;
    }
    QSet<QString> ids;
    for (const auto &cursor : cursors)
        ids.insert(cursor.id);
    store->retainObserved(ids);
    store->expire(300);
    store->endBatch();
    emit scanned();
}
void SessionWatcher::read(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        diagnostic = "unreadable";
        return;
    }
    auto &cursor = cursors[path];
    if (file.size() < cursor.offset)
        cursor = {};
    if (cursor.offset == 0) {
        const auto first = file.readLine(1024 * 1024);
        const auto meta = QJsonDocument::fromJson(first).object();
        if (meta["type"] != "session_meta")
            return;
        cursor.id = meta["payload"].toObject()["id"].toString();
        if (cursor.id.isEmpty())
            return;
        store->ingestLog(cursor.id, meta);
        cursor.offset = file.pos();
    }
    file.seek(cursor.offset);
    if (file.size() - cursor.offset > 512 * 1024) {
        file.seek(file.size() - 512 * 1024);
        file.readLine(1024 * 1024);
        cursor.offset = file.pos();
    }
    while (!file.atEnd()) {
        const auto start = file.pos();
        const auto line = file.readLine(1024 * 1024);
        if (!line.endsWith('\n')) {
            if (line.size() >= 1024 * 1024) {
                cursor.offset = file.pos();
                continue;
            }
            cursor.offset = start;
            break;
        }
        cursor.offset = file.pos();
        const auto event = QJsonDocument::fromJson(line);
        if (event.isObject())
            store->ingestLog(cursor.id, event.object());
    }
}
