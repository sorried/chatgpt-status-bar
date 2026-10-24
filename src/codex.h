#pragma once
#include <QJsonObject>
#include <QProcess>
#include <QTimer>

class CodexClient : public QObject {
    Q_OBJECT
  public:
    explicit CodexClient(QObject *parent = nullptr);
    ~CodexClient() override;
    void start(const QString &executable, const QString &home);
    void stop();
    bool connected() const { return ready; }
    void startTask(const QString &cwd, const QString &prompt, const QString &model);
    void interrupt(const QString &threadId, const QString &turnId);
    void answer(const QJsonValue &id, const QJsonObject &result);
    void rejectUnsupported(const QJsonValue &id);
    void refreshLimits();
    void receive(const QJsonObject &message);
  signals:
    void event(QJsonObject message);
    void request(QJsonObject message);
    void connectionChanged(bool ready);
    void error(QString message);
    void limits(QJsonObject limits);

  private:
    QProcess process;
    QByteArray buffer;
    QTimer timeout;
    int nextId = 1;
    bool ready = false;
    struct Pending {
        QString method, prompt;
    };
    QMap<int, Pending> pending;
    int call(const QString &method, const QJsonObject &params = {}, const QString &prompt = {});
    void send(const QJsonObject &message);
};
