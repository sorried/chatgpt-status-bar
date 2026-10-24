#include "session.h"
#include <QJsonArray>

void SessionStore::finish(Session &s, State before) {
    if (batching) {
        if (!beforeBatch.contains(s.id))
            beforeBatch.insert(s.id, before);
        batchDirty = true;
        return;
    }
    emit changed();
    if (before != s.state)
        emit transitioned(s.id, before, s.state);
}
void SessionStore::clear() {
    data.clear();
    emit changed();
}
void SessionStore::ingestLog(const QString &key, const QJsonObject &e) {
    const QString type = e["type"].toString();
    const auto p = e["payload"].toObject();
    if (type != "session_meta" && type != "turn_context" && type != "event_msg" && type != "response_item")
        return;
    auto &s = data[key];
    if (s.managed)
        return;
    s.id = key;
    const State before = s.state;
    auto time = QDateTime::fromString(e["timestamp"].toString(), Qt::ISODateWithMs);
    if (!time.isValid())
        time = QDateTime::currentDateTimeUtc();
    s.updated = time;
    if (type == "session_meta") {
        s.cwd = p["cwd"].toString();
    } else if (type == "turn_context") {
        s.cwd = p["cwd"].toString(s.cwd);
        s.model = p["model"].toString(s.model);
    } else if (type == "event_msg") {
        const QString kind = p["type"].toString();
        if (kind == "task_started" || kind == "turn_started" || kind == "user_message") {
            s.state = State::Working;
            s.started = time;
            if (kind == "user_message")
                s.title = p["message"].toString().simplified().left(180);
        } else if (kind == "agent_reasoning") {
            s.state = State::Thinking;
        } else if (kind == "agent_message") {
            s.detail = p["message"].toString().left(4000);
        } else if (kind == "task_complete" || kind == "turn_complete" || kind == "turn_completed") {
            s.state = State::Done;
            s.detail = p["last_agent_message"].toString(s.detail).left(4000);
        } else if (kind == "turn_aborted") {
            s.state = State::Idle;
        } else if (kind == "error") {
            s.state = State::Error;
            s.detail = p["message"].toString().left(4000);
        } else if (kind == "token_count") {
            const auto info = p["info"].toObject();
            s.tokens = info["total_token_usage"].toObject()["total_tokens"].toInteger(s.tokens);
            s.context = info["model_context_window"].toInteger(s.context);
        }
    } else if (type == "response_item") {
        const QString kind = p["type"].toString();
        if (kind == "message") {
            QString text;
            for (const auto &value : p["content"].toArray()) {
                const auto part = value.toObject();
                if (part["text"].isString())
                    text += part["text"].toString() + "\n";
            }
            text = text.trimmed();
            if (p["role"] == "user" && !text.startsWith("<") && !text.isEmpty())
                s.title = text.simplified().left(180);
            else if (p["role"] == "assistant" && !text.isEmpty())
                s.detail = text.left(4000);
        } else if (kind == "reasoning") {
            s.state = State::Thinking;
        } else if (kind == "function_call" || kind == "custom_tool_call") {
            s.state = State::Tool;
            s.detail = p["name"].toString();
        } else if (kind == "function_call_output" || kind == "custom_tool_call_output") {
            s.state = State::Working;
        }
    }
    finish(s, before);
}
void SessionStore::ingestRpc(const QJsonObject &e) {
    const QString method = e["method"].toString();
    const auto p = e["params"].toObject();
    const auto thread = p["thread"].toObject();
    const QString id = p["threadId"].toString(thread["id"].toString());
    if (id.isEmpty())
        return;
    auto &s = data[id];
    s.id = id;
    s.managed = true;
    s.updated = QDateTime::currentDateTimeUtc();
    const auto before = s.state;
    const auto turn = p["turn"].toObject();
    const auto item = p["item"].toObject();
    if (method == "thread/started") {
        s.cwd = thread["cwd"].toString();
        s.title = thread["preview"].toString().left(180);
    } else if (method == "turn/started") {
        s.state = State::Working;
        s.started = s.updated;
        s.turnId = turn["id"].toString();
    } else if (method == "turn/completed") {
        const auto status = turn["status"].toString();
        s.state = status == "failed" ? State::Error : status == "interrupted" ? State::Idle : State::Done;
        const auto error = turn["error"].toObject()["message"].toString();
        if (!error.isEmpty())
            s.detail = error.left(4000);
    } else if (method == "item/started") {
        const auto type = item["type"].toString();
        s.state = type == "reasoning"                                  ? State::Thinking
                  : type == "commandExecution" || type == "fileChange" ? State::Tool
                                                                       : State::Working;
        if (type == "agentMessage")
            s.detail.clear();
        else if (type == "fileChange") {
            QString changes;
            for (const auto &value : item["changes"].toArray()) {
                const auto change = value.toObject();
                changes += change["path"].toString() + "\n" + change["diff"].toString() + "\n";
            }
            s.detail = changes.left(24000);
            if (changes.size() > 24000)
                s.detail += "\n[diff truncated]";
        } else
            s.detail = item["command"].toString(type).left(4000);
    } else if (method == "item/completed") {
        if (item["type"] == "agentMessage")
            s.detail = item["text"].toString().left(4000);
        s.state = State::Working;
    } else if (method == "item/agentMessage/delta") {
        s.detail = (s.detail + p["delta"].toString()).right(4000);
    } else if (method == "thread/tokenUsage/updated") {
        const auto usage = p["tokenUsage"].toObject();
        s.tokens = usage["total"].toObject()["totalTokens"].toInteger(s.tokens);
        s.context = usage["modelContextWindow"].toInteger(s.context);
    } else if (method.endsWith("requestApproval") || method == "item/tool/requestUserInput") {
        s.state = State::Waiting;
        s.turnId = p["turnId"].toString(s.turnId);
        if (method != "item/fileChange/requestApproval")
            s.detail = p["command"].toString(p["reason"].toString()).left(4000);
    } else if (method == "serverRequest/resolved") {
        s.state = State::Working;
    } else if (method == "error") {
        s.state = State::Error;
        s.detail = p["error"].toObject()["message"].toString().left(4000);
    }
    finish(s, before);
}
void SessionStore::expire(int seconds) {
    const auto now = QDateTime::currentDateTimeUtc();
    bool dirty = false;
    for (auto &s : data) {
        if (!s.managed &&
            (s.state == State::Working || s.state == State::Thinking || s.state == State::Tool) &&
            s.updated.secsTo(now) > seconds) {
            s.state = State::Unknown;
            dirty = true;
        }
    }
    if (dirty) {
        if (batching)
            batchDirty = true;
        else
            emit changed();
    }
}

void SessionStore::retainObserved(const QSet<QString> &ids) {
    bool dirty = false;
    for (auto it = data.begin(); it != data.end();) {
        if (!it->managed && !ids.contains(it.key())) {
            it = data.erase(it);
            dirty = true;
        } else
            ++it;
    }
    if (dirty) {
        if (batching)
            batchDirty = true;
        else
            emit changed();
    }
}

void SessionStore::beginBatch() {
    batching = true;
    batchDirty = false;
    beforeBatch.clear();
}
void SessionStore::endBatch() {
    batching = false;
    if (batchDirty)
        emit changed();
    for (auto it = beforeBatch.begin(); it != beforeBatch.end(); ++it) {
        if (data.contains(it.key()) && it.value() != data[it.key()].state)
            emit transitioned(it.key(), it.value(), data[it.key()].state);
    }
    beforeBatch.clear();
}
