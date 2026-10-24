#include "codex.h"
#include "session.h"
#include "watcher.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <iostream>

static QJsonObject logEvent(const QString &type, const QJsonObject &payload,
                            QDateTime time = QDateTime::currentDateTimeUtc()) {
    return {{"type", type}, {"timestamp", time.toString(Qt::ISODateWithMs)}, {"payload", payload}};
}
static QJsonObject rpc(const QString &method, const QJsonObject &params) {
    return {{"method", method}, {"params", params}};
}
class CoreTests : public QObject {
    Q_OBJECT
  private slots:
    void logLifecycle() {
        SessionStore store;
        QSignalSpy changes(&store, &SessionStore::transitioned);
        store.ingestLog("a", logEvent("session_meta", {{"cwd", "/project"}}));
        store.ingestLog("a", logEvent("event_msg", {{"type", "user_message"}, {"message", "fix tests"}}));
        QCOMPARE(store.sessions()["a"].state, State::Working);
        QCOMPARE(store.sessions()["a"].title, "fix tests");
        store.ingestLog("a", logEvent("event_msg", {{"type", "agent_reasoning"}}));
        QCOMPARE(store.sessions()["a"].state, State::Thinking);
        store.ingestLog("a",
                        logEvent("response_item", {{"type", "function_call"}, {"name", "exec_command"}}));
        QCOMPARE(store.sessions()["a"].state, State::Tool);
        store.ingestLog(
            "a", logEvent("event_msg", {{"type", "task_complete"}, {"last_agent_message", "finished"}}));
        QCOMPARE(store.sessions()["a"].state, State::Done);
        QCOMPARE(store.sessions()["a"].detail, "finished");
        QCOMPARE(changes.size(), 4);
    }
    void currentRolloutMessages() {
        SessionStore store;
        store.ingestLog("a", logEvent("response_item",
                                      {{"type", "message"},
                                       {"role", "user"},
                                       {"content", QJsonArray{QJsonObject{{"type", "input_text"},
                                                                          {"text", "fix the build"}}}}}));
        QCOMPARE(store.sessions()["a"].title, "fix the build");
        store.ingestLog(
            "a", logEvent("response_item",
                          {{"type", "message"},
                           {"role", "user"},
                           {"content", QJsonArray{QJsonObject{
                                           {"type", "input_text"},
                                           {"text", "<environment_context>data</environment_context>"}}}}}));
        QCOMPARE(store.sessions()["a"].title, "fix the build");
        store.ingestLog("a", logEvent("response_item", {{"type", "reasoning"}}));
        QCOMPARE(store.sessions()["a"].state, State::Thinking);
    }
    void batchedHistoryHasOneTransition() {
        SessionStore store;
        QSignalSpy changed(&store, &SessionStore::changed);
        QSignalSpy states(&store, &SessionStore::transitioned);
        store.beginBatch();
        store.ingestLog("a", logEvent("event_msg", {{"type", "task_started"}}));
        store.ingestLog("a", logEvent("event_msg", {{"type", "agent_reasoning"}}));
        store.ingestLog("a", logEvent("event_msg", {{"type", "task_complete"}}));
        QVERIFY(changed.isEmpty());
        store.endBatch();
        QCOMPARE(changed.size(), 1);
        QCOMPARE(states.size(), 1);
        QCOMPARE(store.sessions()["a"].state, State::Done);
        store.retainObserved({});
        QVERIFY(store.sessions().isEmpty());
    }
    void fileApprovalPreservesDiff() {
        SessionStore store;
        store.ingestRpc(rpc(
            "item/started",
            {{"threadId", "a"},
             {"item", QJsonObject{{"type", "fileChange"},
                                  {"changes",
                                   QJsonArray{QJsonObject{{"path", "app.cpp"}, {"diff", "+new line"}}}}}}}));
        store.ingestRpc(
            rpc("item/fileChange/requestApproval", {{"threadId", "a"}, {"reason", "write files"}}));
        QCOMPARE(store.sessions()["a"].state, State::Waiting);
        QVERIFY(store.sessions()["a"].detail.contains("+new line"));
    }
    void staleIsUnknown() {
        SessionStore store;
        store.ingestLog("a", logEvent("event_msg", {{"type", "task_started"}},
                                      QDateTime::currentDateTimeUtc().addSecs(-301)));
        store.expire(300);
        QCOMPARE(store.sessions()["a"].state, State::Unknown);
    }
    void totalTokensNotContextUsage() {
        SessionStore store;
        store.ingestLog(
            "a", logEvent("event_msg",
                          {{"type", "token_count"},
                           {"info", QJsonObject{{"total_token_usage", QJsonObject{{"total_tokens", 900000}}},
                                                {"model_context_window", 128000}}}}));
        QCOMPARE(store.sessions()["a"].tokens, 900000);
        QCOMPARE(store.sessions()["a"].context, 128000);
    }
    void rpcLifecycleAndIsolation() {
        SessionStore store;
        store.ingestRpc(rpc("thread/started", {{"thread", QJsonObject{{"id", "a"}, {"cwd", "/project"}}}}));
        store.ingestRpc(rpc("turn/started", {{"threadId", "a"}, {"turn", QJsonObject{{"id", "turn1"}}}}));
        store.ingestRpc(
            rpc("item/commandExecution/requestApproval", {{"threadId", "a"}, {"command", "echo test"}}));
        QCOMPARE(store.sessions()["a"].state, State::Waiting);
        store.ingestLog("a", logEvent("event_msg", {{"type", "task_complete"}}));
        QCOMPARE(store.sessions()["a"].state, State::Waiting);
        store.ingestRpc(rpc("serverRequest/resolved", {{"threadId", "a"}}));
        QCOMPARE(store.sessions()["a"].state, State::Working);
        store.ingestRpc(
            rpc("turn/completed",
                {{"threadId", "a"},
                 {"turn", QJsonObject{{"status", "failed"}, {"error", QJsonObject{{"message", "quota"}}}}}}));
        QCOMPARE(store.sessions()["a"].state, State::Error);
        QCOMPARE(store.sessions()["a"].detail, "quota");
    }
    void incrementalLogAndPartialLine() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir(dir.path()).mkpath("sessions/2026/10/24"));
        QFile file(dir.filePath("sessions/2026/10/24/rollout.jsonl"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        auto append = [&](QJsonObject event) {
            file.write(QJsonDocument(event).toJson(QJsonDocument::Compact) + '\n');
            file.flush();
        };
        append(logEvent("session_meta", {{"id", "a"}, {"cwd", "/project"}}));
        append(logEvent("event_msg", {{"type", "task_started"}}));
        SessionStore store;
        SessionWatcher watcher(&store);
        watcher.setHome(dir.path());
        QCOMPARE(watcher.status(), "ok");
        QCOMPARE(store.sessions()["a"].state, State::Working);
        auto complete =
            QJsonDocument(logEvent("event_msg", {{"type", "task_complete"}})).toJson(QJsonDocument::Compact);
        file.write(complete.left(20));
        file.flush();
        watcher.scan();
        QCOMPARE(store.sessions()["a"].state, State::Working);
        file.write(complete.mid(20) + '\n');
        file.flush();
        watcher.scan();
        QCOMPARE(store.sessions()["a"].state, State::Done);
        QSignalSpy spy(&store, &SessionStore::changed);
        watcher.scan();
        QCOMPARE(spy.size(), 0);
        file.close();
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        append(logEvent("session_meta", {{"id", "b"}, {"cwd", "/other"}}));
        append(logEvent("event_msg", {{"type", "turn_aborted"}}));
        watcher.scan();
        QVERIFY(store.sessions().contains("b"));
        QCOMPARE(store.sessions()["b"].state, State::Idle);
    }
    void missingHomeAndMalformedLog() {
        QTemporaryDir dir;
        SessionStore store;
        SessionWatcher watcher(&store);
        watcher.setHome(dir.path());
        QCOMPARE(watcher.status(), "missing");
        QDir(dir.path()).mkdir("sessions");
        QFile f(dir.filePath("sessions/bad.jsonl"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("not json\n");
        f.close();
        watcher.scan();
        QVERIFY(store.sessions().isEmpty());
    }
    void fakeServerRoundTrip() {
        QTemporaryDir dir;
        CodexClient client;
        QSignalSpy errors(&client, &CodexClient::error);
        QSignalSpy limits(&client, &CodexClient::limits);
        QSignalSpy requests(&client, &CodexClient::request);
        QSignalSpy events(&client, &CodexClient::event);
        client.start(QCoreApplication::applicationFilePath(), dir.path());
        QTRY_VERIFY_WITH_TIMEOUT(client.connected(), 5000);
        QTRY_COMPARE(limits.size(), 1);
        client.startTask(dir.path(), "test prompt", "");
        QTRY_COMPARE(requests.size(), 1);
        auto request = requests[0][0].toJsonObject();
        QCOMPARE(request["method"].toString(), "item/commandExecution/requestApproval");
        client.answer(request["id"], {{"decision", "decline"}});
        QTRY_VERIFY(!events.isEmpty() && events.last()[0].toJsonObject()["method"] == "turn/completed");
        QVERIFY(errors.isEmpty());
        client.stop();
        QVERIFY(!client.connected());
    }
    void failedExecutable() {
        CodexClient client;
        QSignalSpy errors(&client, &CodexClient::error);
        client.start("/does/not/exist/codex", QDir::tempPath());
        QTRY_VERIFY(!errors.isEmpty());
        QVERIFY(!client.connected());
    }
};
static int fakeServer() {
    std::string line;
    bool initialized = false;
    auto send = [](const QJsonObject &message) {
        std::cout << QJsonDocument(message).toJson(QJsonDocument::Compact).constData() << std::endl;
    };
    while (std::getline(std::cin, line)) {
        const auto msg = QJsonDocument::fromJson(QByteArray::fromStdString(line)).object();
        const auto method = msg["method"].toString();
        const auto id = msg["id"];
        const auto params = msg["params"].toObject();
        if (method == "initialize") {
            send({{"id", id}, {"result", QJsonObject{{"userAgent", "test"}}}});
        } else if (method == "initialized")
            initialized = true;
        else if (!initialized)
            return 3;
        else if (method == "account/rateLimits/read") {
            send({{"id", id},
                  {"result",
                   QJsonObject{{"rateLimits", QJsonObject{{"primary", QJsonObject{{"usedPercent", 25}}}}}}}});
        } else if (method == "thread/start") {
            if (params["approvalPolicy"] != "untrusted" || params["sandbox"] != "workspace-write")
                return 4;
            send({{"id", id},
                  {"result",
                   QJsonObject{{"thread", QJsonObject{{"id", "test-thread"}, {"cwd", params["cwd"]}}}}}});
        } else if (method == "turn/start") {
            if (params["input"].toArray().first().toObject()["text"] != "test prompt")
                return 5;
            send({{"id", id}, {"result", QJsonObject{}}});
            send(rpc("turn/started", {{"threadId", "test-thread"}, {"turn", QJsonObject{{"id", "turn1"}}}}));
            auto request = rpc("item/commandExecution/requestApproval",
                               {{"threadId", "test-thread"}, {"turnId", "turn1"}, {"command", "echo test"}});
            request.insert("id", "approval-1");
            send(request);
        } else if (msg["id"] == "approval-1") {
            if (msg["result"].toObject()["decision"] != "decline")
                return 6;
            send(rpc("serverRequest/resolved", {{"threadId", "test-thread"}, {"requestId", "approval-1"}}));
            send(rpc("turn/completed",
                     {{"threadId", "test-thread"}, {"turn", QJsonObject{{"status", "completed"}}}}));
        }
    }
    return 0;
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc > 1 && QByteArray(argv[1]) == "app-server")
        return fakeServer();
    CoreTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "test_core.moc"
