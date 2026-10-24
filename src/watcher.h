#pragma once
#include "session.h"
#include <QTimer>

class SessionWatcher : public QObject {
    Q_OBJECT
  public:
    explicit SessionWatcher(SessionStore *store, QObject *parent = nullptr);
    void setHome(const QString &home);
    void setInterval(int milliseconds);
    void scan();
    QString status() const { return diagnostic; }
  signals:
    void scanned();

  private:
    struct Cursor {
        qint64 offset = 0;
        QString id;
    };
    QMap<QString, Cursor> cursors;
    SessionStore *store;
    QTimer timer;
    QString home, diagnostic;
    void read(const QString &path);
};
