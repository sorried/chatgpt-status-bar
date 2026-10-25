#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>

enum class State { Idle, Working, Thinking, Tool, Waiting, Done, Error, Unknown };
struct Session {
    QString id, cwd, title, detail, model, turnId;
    QStringList history;
    QList<QPair<QString, QString>> plan;
    QMap<QString, QString> agents;
    State state = State::Idle;
    QDateTime updated, started;
    qint64 tokens = 0;
    qint64 context = 0;
    bool managed = false;
};
class SessionStore : public QObject {
    Q_OBJECT
  public:
    using QObject::QObject;
    const QMap<QString, Session> &sessions() const { return data; }
    void ingestLog(const QString &key, const QJsonObject &event);
    void ingestRpc(const QJsonObject &event);
    void expire(int seconds);
    void clear();
    void retainObserved(const QSet<QString> &ids);
    void beginBatch();
    void endBatch();
  signals:
    void changed();
    void transitioned(QString id, State before, State after);

  private:
    QMap<QString, Session> data;
    QMap<QString, State> beforeBatch;
    bool batching = false, batchDirty = false;
    void finish(Session &session, State before);
};
