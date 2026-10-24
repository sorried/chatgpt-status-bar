#include "codex.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcessEnvironment>

CodexClient::CodexClient(QObject *parent) : QObject(parent) {
    timeout.setSingleShot(true);
    timeout.setInterval(15000);
    connect(&timeout, &QTimer::timeout, this, [this] {
        emit error("Codex initialization timed out.");
        stop();
    });
    connect(&process, &QProcess::started, this, [this] {
        call("initialize",
             {{"clientInfo", QJsonObject{{"name", "gpt_status_bar"},
                                         {"title", "GPT status bar"},
                                         {"version", QCoreApplication::applicationVersion()}}}});
        timeout.start();
    });
    connect(&process, &QProcess::readyReadStandardOutput, this, [this] {
        buffer += process.readAllStandardOutput();
        if (buffer.size() > 8 * 1024 * 1024) {
            emit error("Codex sent an oversized message.");
            stop();
            return;
        }
        int end;
        while ((end = buffer.indexOf('\n')) >= 0) {
            auto line = buffer.left(end);
            buffer.remove(0, end + 1);
            auto doc = QJsonDocument::fromJson(line);
            if (doc.isObject())
                receive(doc.object());
        }
    });
    connect(&process, &QProcess::readyReadStandardError, this, [this] { process.readAllStandardError(); });
    connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        timeout.stop();
        emit error(process.errorString());
    });
    connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus) {
                timeout.stop();
                ready = false;
                pending.clear();
                emit connectionChanged(false);
                if (code != 0)
                    emit error(QString("Codex exited with code %1.").arg(code));
            });
}
CodexClient::~CodexClient() { stop(); }
void CodexClient::start(const QString &executable, const QString &home) {
    if (process.state() != QProcess::NotRunning)
        return;
    buffer.clear();
    pending.clear();
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("CODEX_HOME", home);
    process.setProcessEnvironment(env);
    process.start(executable, {"app-server"});
}
void CodexClient::stop() {
    timeout.stop();
    ready = false;
    if (process.state() != QProcess::NotRunning) {
        process.closeWriteChannel();
        process.terminate();
        if (!process.waitForFinished(1500)) {
            process.kill();
            process.waitForFinished(1500);
        }
    }
    pending.clear();
}
void CodexClient::send(const QJsonObject &message) {
    if (process.state() == QProcess::Running)
        process.write(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
}
int CodexClient::call(const QString &method, const QJsonObject &params, const QString &prompt) {
    const int id = nextId++;
    pending.insert(id, {method, prompt});
    send({{"id", id}, {"method", method}, {"params", params}});
    return id;
}
void CodexClient::receive(const QJsonObject &message) {
    if (message.contains("method")) {
        emit event(message);
        if (message.contains("id"))
            emit request(message);
        if (message["method"] == "account/rateLimits/updated")
            emit limits(message["params"].toObject());
        return;
    }
    if (!message["id"].isDouble())
        return;
    const int id = message["id"].toInt();
    if (!pending.contains(id))
        return;
    const auto op = pending.take(id);
    if (message.contains("error")) {
        emit error(message["error"].toObject()["message"].toString());
        if (op.method == "initialize")
            stop();
        return;
    }
    const auto result = message["result"].toObject();
    if (op.method == "initialize") {
        send({{"method", "initialized"}, {"params", QJsonObject{}}});
        timeout.stop();
        ready = true;
        emit connectionChanged(true);
        refreshLimits();
    } else if (op.method == "thread/start") {
        auto thread = result["thread"].toObject();
        if (thread["preview"].toString().isEmpty())
            thread.insert("preview", op.prompt.simplified().left(180));
        thread.insert("model", result["model"]);
        const auto threadId = thread["id"].toString();
        if (threadId.isEmpty()) {
            emit error("Codex returned no thread id.");
            return;
        }
        emit event({{"method", "thread/started"}, {"params", QJsonObject{{"thread", thread}}}});
        call("turn/start", {{"threadId", threadId},
                            {"input", QJsonArray{QJsonObject{{"type", "text"}, {"text", op.prompt}}}}});
    } else if (op.method == "account/rateLimits/read") {
        emit limits(result);
    }
}
void CodexClient::startTask(const QString &cwd, const QString &prompt, const QString &model) {
    if (!ready) {
        emit error("Connect to Codex first.");
        return;
    }
    QJsonObject params{{"cwd", cwd}, {"approvalPolicy", "untrusted"}, {"sandbox", "workspace-write"}};
    if (!model.trimmed().isEmpty())
        params.insert("model", model.trimmed());
    call("thread/start", params, prompt);
}
void CodexClient::interrupt(const QString &threadId, const QString &turnId) {
    if (ready && !turnId.isEmpty())
        call("turn/interrupt", {{"threadId", threadId}, {"turnId", turnId}});
}
void CodexClient::answer(const QJsonValue &id, const QJsonObject &result) {
    send({{"id", id}, {"result", result}});
}
void CodexClient::rejectUnsupported(const QJsonValue &id) {
    send({{"id", id}, {"error", QJsonObject{{"code", -32601}, {"message", "Unsupported client request"}}}});
}
void CodexClient::refreshLimits() {
    if (ready)
        call("account/rateLimits/read");
}
