#include "app.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QSaveFile>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <algorithm>

Companion::Companion(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle("GPT status bar");
    setAccessibleName("GPT status companion");
    setCursor(Qt::PointingHandCursor);
    setFixedSize(224, 40);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 3, 14, 3);
    mascot = new Mascot;
    mascot->setFixedSize(32, 32);
    label = new QLabel;
    label->setStyleSheet("color: #f1f5f4; font-weight: 600; font-size: 12px; background: transparent;");
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    mascot->setAttribute(Qt::WA_TransparentForMouseEvents);
    status = new QLabel;
    status->setAttribute(Qt::WA_TransparentForMouseEvents);
    status->setStyleSheet("color: #c9c9c9; font-size: 11px; background: transparent;");
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(mascot);
    layout->addWidget(status, 1);
    layout->addWidget(label);
}
void Companion::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#292929"), 1));
    p.setBrush(QColor("#050505"));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 18, 18);
}
void Companion::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton)
        emit activated();
}
void Companion::enterEvent(QEnterEvent *e) {
    QWidget::enterEvent(e);
    if (hoverEnabled)
        showDetails();
}
void Companion::setCard(QWidget *widget) {
    card = widget;
    card->installEventFilter(this);
}
void Companion::leaveEvent(QEvent *e) {
    QWidget::leaveEvent(e);
    QTimer::singleShot(230, this, [this] {
        if (card && !pinned && !QApplication::activeModalWidget() && !underMouse() && !card->underMouse())
            card->hide();
    });
}
bool Companion::eventFilter(QObject *object, QEvent *event) {
    if (object == card && event->type() == QEvent::Leave)
        QTimer::singleShot(230, this, [this] {
            if (card && !pinned && !QApplication::activeModalWidget() && !underMouse() && !card->underMouse())
                card->hide();
        });
    return QWidget::eventFilter(object, event);
}
void Companion::showDetails() {
    if (!card)
        return;
    if (!pinned)
        emit inspecting();
    card->adjustSize();
    auto *screen = QGuiApplication::screenAt(geometry().center());
    if (!screen)
        return;
    const auto area = screen->availableGeometry();
    const int x = qBound(area.left(), geometry().center().x() - card->width() / 2,
                         qMax(area.left(), area.right() - card->width() + 1));
    const int y = geometry().bottom() + card->height() + 8 < area.bottom()
                      ? geometry().bottom() + 5
                      : qMax(area.top(), geometry().top() - card->height() - 5);
    card->move(x, y);
    card->show();
}
void Companion::place(int index, const QString &position) {
    auto screens = QGuiApplication::screens();
    if (screens.isEmpty())
        return;
    const auto area = screens[qBound(0, index, int(screens.size()) - 1)]->availableGeometry();
    const int x = position == "top-right" ? area.right() - width() - 16 : area.center().x() - width() / 2;
    const int y = position == "bottom" ? area.bottom() - height() - 16 : area.top() + 10;
    move(x, y);
}
QString App::l(const char *en, const char *ru) const {
    return QString::fromUtf8(
        settings.value("language", QLocale::system().language() == QLocale::Russian ? "ru" : "en")
                    .toString() == "ru"
            ? ru
            : en);
}
QString App::home() const {
    return settings.value("codexHome", qEnvironmentVariable("CODEX_HOME", QDir::home().filePath(".codex")))
        .toString();
}
QString App::stateName(State state) const {
    switch (state) {
    case State::Idle:
        return l("resting", "отдыхает");
    case State::Working:
        return l("working", "работает");
    case State::Thinking:
        return l("thinking", "думает");
    case State::Tool:
        return l("using a tool", "выполняет действие");
    case State::Waiting:
        return l("needs you", "ждёт тебя");
    case State::Done:
        return l("done", "готово");
    case State::Error:
        return l("error", "ошибка");
    case State::Unknown:
        return l("no recent events", "нет свежих событий");
    }
    return {};
}
App::App(bool preview) : watcher(&store), preview(preview) {
    setObjectName("mainWindow");
    setWindowIcon(Mascot::icon());
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedWidth(440);
    activity = createActivityUi(this);
    setCentralWidget(activity.body);
    avatar = activity.avatar;
    summary = activity.summary;
    connection = activity.connection;
    meta = activity.meta;
    usage = activity.usage;
    list = activity.list;
    detail = activity.detail;
    connectButton = activity.connectButton;
    taskButton = activity.taskButton;
    stopButton = activity.stopButton;
    folderButton = activity.folderButton;
    connect(activity.settingsButton, &QPushButton::clicked, this, &App::openSettings);
    connect(activity.quitButton, &QPushButton::clicked, this, &App::requestQuit);
    connect(activity.closeButton, &QPushButton::clicked, this, [this] {
        companion.pinned = false;
        hide();
    });
    connect(activity.allowButton, &QPushButton::clicked, this, [this] { finishApproval(true); });
    connect(activity.denyButton, &QPushButton::clicked, this, [this] { finishApproval(false); });
    companion.setCard(this);
    client.setObjectName("codexClient");
    client.setParent(this);
    connect(list, &QListWidget::currentRowChanged, this, &App::refresh);
    connect(&store, &SessionStore::changed, this, &App::refresh);
    connect(&store, &SessionStore::transitioned, this, &App::notify);
    connect(&watcher, &SessionWatcher::scanned, this, &App::refresh);
    connect(&client, &CodexClient::event, &store, &SessionStore::ingestRpc);
    connect(&client, &CodexClient::event, this, [this](const QJsonObject &e) {
        const auto p = e["params"].toObject();
        if (!activeRequest.isEmpty()) {
            const auto params = activeRequest["params"].toObject();
            if ((e["method"] == "serverRequest/resolved" && p["requestId"] == activeRequest["id"]) ||
                (e["method"] == "turn/completed" && p["threadId"] == params["threadId"]))
                clearApproval();
        }
        for (auto it = requests.begin(); it != requests.end();) {
            const auto params = (*it)["params"].toObject();
            if ((e["method"] == "serverRequest/resolved" && p["requestId"] == (*it)["id"]) ||
                (e["method"] == "turn/completed" && p["threadId"] == params["threadId"]))
                it = requests.erase(it);
            else
                ++it;
        }
    });
    connect(&client, &CodexClient::connectionChanged, this, [this](bool ready) {
        if (!ready) {
            requests.clear();
            clearApproval();
            for (const auto &s : store.sessions()) {
                if (s.managed && s.state != State::Done && s.state != State::Idle)
                    store.ingestRpc(
                        {{"method", "error"},
                         {"params",
                          QJsonObject{
                              {"threadId", s.id},
                              {"error", QJsonObject{{"message",
                                                     l("Connection lost. Check the task in Codex.",
                                                       "соединение потеряно. проверь задачу в Codex.")}}}}}});
            }
        }
        refresh();
    });
    connect(&client, &CodexClient::error, this, [this](const QString &message) {
        errorText = message;
        refresh();
    });
    connect(&client, &CodexClient::limits, this, [this](const auto &limits) {
        rateLimits = limits;
        refresh();
    });
    connect(&client, &CodexClient::request, this, [this](const auto &request) {
        auto queued = request;
        const auto id = request["params"].toObject()["threadId"].toString();
        queued.insert("_detail", store.sessions().value(id).detail);
        requests.append(queued);
        QTimer::singleShot(0, this, &App::handleRequests);
    });
    connect(connectButton, &QPushButton::clicked, this, [this] {
        if (client.connected())
            client.refreshLimits();
        else {
            errorText.clear();
            client.start(settings.value("executable", "codex").toString(), home());
        }
    });
    connect(taskButton, &QPushButton::clicked, this, &App::newTask);
    connect(stopButton, &QPushButton::clicked, this, [this] {
        const auto s = store.sessions().value(selected());
        client.interrupt(s.id, s.turnId);
    });
    connect(folderButton, &QPushButton::clicked, this, [this] {
        const auto cwd = store.sessions().value(selected()).cwd;
        if (QFileInfo(cwd).isDir())
            QDesktopServices::openUrl(QUrl::fromLocalFile(cwd));
    });
    connect(&companion, &Companion::inspecting, this, [this] {
        if (list->count())
            list->setCurrentRow(0);
    });
    connect(&companion, &Companion::activated, this, [this] {
        if (list->count())
            list->setCurrentRow(0);
        showWindow();
    });
    connect(&tray, &QSystemTrayIcon::activated, this, [this](auto reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            showWindow();
    });
    clock.setInterval(1000);
    connect(&clock, &QTimer::timeout, this, &App::refresh);
    clock.start();
    limitTimer.setInterval(5 * 60 * 1000);
    connect(&limitTimer, &QTimer::timeout, &client, &CodexClient::refreshLimits);
    limitTimer.start();
    auto *shortcut = new QShortcut(QKeySequence::Preferences, this);
    connect(shortcut, &QShortcut::activated, this, &App::openSettings);
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this] {
        companion.pinned = false;
        hide();
    });
    applySettings();
    if (!preview)
        watcher.setHome(home());
    else {
        const auto timestamp = QDateTime::currentDateTimeUtc().addSecs(-94).toString(Qt::ISODateWithMs);
        store.ingestLog("preview-session", {{"type", "session_meta"},
                                            {"timestamp", timestamp},
                                            {"payload", QJsonObject{{"cwd", "/projects/observatory"}}}});
        store.ingestLog(
            "preview-session",
            {{"type", "event_msg"},
             {"timestamp", timestamp},
             {"payload", QJsonObject{{"type", "user_message"},
                                     {"message", l("Build the settings window", "собери окно настроек")}}}});
        store.ingestLog("preview-session", {{"type", "response_item"},
                                            {"payload", QJsonObject{{"type", "function_call"},
                                                                    {"name", "cmake --build build"}}}});
    }
    if (preview) {
        store.ingestRpc(
            {{"method", "turn/plan/updated"},
             {"params",
              QJsonObject{
                  {"threadId", "preview-session"},
                  {"plan",
                   QJsonArray{QJsonObject{{"step", l("Prepare the settings", "подготовить настройки")},
                                          {"status", "completed"}},
                              QJsonObject{{"step", l("Build and check", "собрать и проверить")},
                                          {"status", "inProgress"}}}}}}});
    }
    booting = false;
    refresh();
    connect(qApp, &QGuiApplication::screenAdded, this, [this] { applySettings(); });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this] { applySettings(); });
}
QString App::selected() const {
    return list->currentItem() ? list->currentItem()->data(Qt::UserRole).toString() : QString();
}
void App::refresh() {
    const QString previous = selected();
    auto sessions = store.sessions().values();
    std::sort(sessions.begin(), sessions.end(), [](const auto &a, const auto &b) {
        auto rank = [](State state) {
            if (state == State::Waiting)
                return 3;
            if (state == State::Working || state == State::Thinking || state == State::Tool)
                return 2;
            if (state == State::Error)
                return 1;
            return 0;
        };
        if (rank(a.state) != rank(b.state))
            return rank(a.state) > rank(b.state);
        return a.updated > b.updated;
    });
    const int scroll = list->verticalScrollBar()->value();
    list->blockSignals(true);
    list->clear();
    int active = 0, selectedRow = -1;
    for (const auto &s : sessions) {
        if (s.state == State::Working || s.state == State::Tool || s.state == State::Thinking ||
            s.state == State::Waiting)
            ++active;
        auto *item = new QListWidgetItem;
        const QString project = QFileInfo(s.cwd).fileName();
        item->setText((project.isEmpty() ? l("Codex session", "сессия Codex") : project) + "\n" +
                      stateName(s.state));
        item->setData(Qt::UserRole, s.id);
        item->setSizeHint(QSize(380, 44));
        list->addItem(item);
        if (s.id == previous)
            selectedRow = list->count() - 1;
    }
    if (list->count())
        list->setCurrentRow(selectedRow >= 0 ? selectedRow : 0);
    list->verticalScrollBar()->setValue(scroll);
    list->blockSignals(false);
    const bool approving = !activeRequest.isEmpty();
    activity.taskLabel->setVisible(!approving);
    meta->setVisible(!approving);
    activity.stateLabel->setVisible(!approving);
    detail->setVisible(!approving);
    list->setVisible(sessions.size() > 1 && !approving);
    list->setFixedHeight(qMin(3, int(sessions.size())) * 44 + 6);
    const auto s = store.sessions().value(selected());
    QString duration;
    if (s.started.isValid()) {
        const auto end = s.state == State::Done || s.state == State::Idle || s.state == State::Unknown ||
                                 s.state == State::Error
                             ? s.updated
                             : QDateTime::currentDateTimeUtc();
        const auto seconds = qMax<qint64>(0, s.started.secsTo(end));
        duration = QString("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
    }
    summary->setText((active > 1 ? QString("%1 · ").arg(active) : QString()) +
                     (duration.isEmpty() ? stateName(s.state) : duration));
    activity.taskLabel->setText(l("task", "задача"));
    activity.stateLabel->setText(stateName(s.state));
    meta->setText(sessions.isEmpty()  ? l("No active task", "нет активной задачи")
                  : s.title.isEmpty() ? QFileInfo(s.cwd).fileName()
                                      : s.title);
    const auto text = sessions.isEmpty()
                          ? l("Open Codex in a project. Its activity appears here automatically.",
                              "открой Codex в проекте. его действия появятся здесь автоматически.")
                          : s.detail;
    if (detail->toPlainText() != text)
        detail->setPlainText(text);
    detail->document()->setTextWidth(390);
    detail->setFixedHeight(qBound(32, int(detail->document()->size().height()) + 8, 92));
    QStringList plan;
    for (const auto &step : s.plan.mid(0, 3)) {
        const auto marker = step.second == "completed"    ? QString("✓ ")
                            : step.second == "inProgress" ? QString("› ")
                                                          : QString("· ");
        plan.append(marker + step.first.left(110));
    }
    activity.planLabel->setText(plan.isEmpty() ? QString() : l("plan\n", "план\n") + plan.join('\n'));
    activity.planLabel->setVisible(!plan.isEmpty() && !approving);
    QStringList recent;
    int agents = 0;
    for (auto it = s.agents.begin(); it != s.agents.end(); ++it)
        if (it.value() == "running" || it.value() == "pendingInit")
            ++agents;
    if (agents)
        recent.append(l("%1 agents working", "работают агентов: %1").arg(agents));
    for (const auto &entry : s.history.mid(0, 2))
        recent.append(entry.left(120));
    activity.historyLabel->setText(recent.isEmpty() ? QString()
                                                    : l("recently\n", "до этого\n") + recent.join('\n'));
    activity.historyLabel->setVisible(!recent.isEmpty() && !approving);
    const bool motion = settings.value("motion", true).toBool();
    const auto state = sessions.isEmpty() ? State::Idle : s.state;
    avatar->configure(state, motion);
    const auto primary = sessions.isEmpty() ? Session{} : sessions.first();
    companion.mascot->configure(primary.state, motion);
    const auto seconds =
        primary.started.isValid()
            ? qMax<qint64>(0, primary.started.secsTo(
                                  primary.state == State::Done || primary.state == State::Idle ||
                                          primary.state == State::Unknown || primary.state == State::Error
                                      ? primary.updated
                                      : QDateTime::currentDateTimeUtc()))
            : 0;
    const auto elapsed = primary.started.isValid()
                             ? QString("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'))
                             : "GPT";
    companion.label->setText(elapsed);
    companion.status->setText(stateName(primary.state) + (active > 1 ? QString(" · %1").arg(active) : ""));
    tray.setIcon(Mascot::icon(primary.state));
    tray.setToolTip("GPT · " + stateName(primary.state));
    QString status = client.connected() ? l("Codex connected", "Codex подключён")
                                        : l("Local observation", "локальное наблюдение");
    if (preview)
        status = l("PREVIEW · sample session, no agent is running",
                   "ПРЕДПРОСМОТР · пример сессии, агент не запущен");
    else if (!errorText.isEmpty())
        status += " · " + errorText;
    else if (watcher.status() == "missing")
        status += l(" · sessions folder not found. Check settings.",
                    " · папка сессий не найдена. проверь настройки.");
    else if (watcher.status() == "unreadable")
        status += l(" · a session file could not be read", " · не удалось прочитать файл сессии");
    if (!preview && !s.id.isEmpty() && !s.managed)
        status += l(" · approvals in Codex", " · разрешения в Codex");
    connection->setText(status);
    connectButton->setText(client.connected() ? l("Refresh limits", "обновить лимиты")
                                              : l("Connect Codex", "подключить Codex"));
    taskButton->setText(l("New task", "новая задача"));
    taskButton->setEnabled(client.connected() && !preview);
    taskButton->setVisible(client.connected() && !preview);
    connectButton->setEnabled(!preview);
    connectButton->setVisible(!preview);
    stopButton->setText(l("Stop task", "остановить"));
    stopButton->setEnabled(s.managed && !s.turnId.isEmpty() && client.connected() &&
                           (s.state == State::Working || s.state == State::Thinking ||
                            s.state == State::Tool || s.state == State::Waiting));
    folderButton->setText(l("Open folder", "открыть папку"));
    folderButton->setEnabled(!s.cwd.isEmpty() && QFileInfo(s.cwd).isDir());
    folderButton->setVisible(folderButton->isEnabled());
    stopButton->setVisible(stopButton->isEnabled());
    QStringList windows;
    auto limits = rateLimits["rateLimitsByLimitId"].toObject();
    if (limits.isEmpty() && rateLimits["rateLimits"].isObject())
        limits.insert("codex", rateLimits["rateLimits"]);
    for (auto it = limits.begin(); it != limits.end(); ++it) {
        const auto bucket = it.value().toObject();
        for (const auto &key : {"primary", "secondary"}) {
            const auto window = bucket[key].toObject();
            if (!window["usedPercent"].isDouble())
                continue;
            QString text =
                it.key() +
                QString(" · %1% ").arg(qBound(0.0, 100.0 - window["usedPercent"].toDouble(), 100.0)) +
                l("remaining", "осталось");
            if (window["windowDurationMins"].isDouble())
                text +=
                    QString(" · %1 ").arg(window["windowDurationMins"].toInt()) + l("min window", "мин окно");
            if (window["resetsAt"].isDouble())
                text += " · " + l("resets ", "сброс ") +
                        QDateTime::fromSecsSinceEpoch(window["resetsAt"].toInteger())
                            .toLocalTime()
                            .toString("ddd HH:mm");
            windows.append(text);
        }
    }
    QStringList footer;
    if (!s.cwd.isEmpty())
        footer.append(QFileInfo(s.cwd).fileName());
    if (!s.model.isEmpty())
        footer.append(s.model);
    if (s.tokens)
        footer.append(l("%1 tokens", "токенов: %1").arg(s.tokens));
    if (s.context)
        footer.append(l("capacity %1", "ёмкость %1").arg(s.context));
    if (!windows.isEmpty())
        footer += windows;
    usage->setText(footer.join(" · "));
    usage->setVisible(!footer.isEmpty());
    adjustSize();
}
void App::showWindow() {
    companion.pinned = true;
    companion.showDetails();
    showNormal();
    raise();
    activateWindow();
}
void App::applySettings() {
    setWindowTitle(l("GPT status bar", "GPT status bar"));
    activity.settingsButton->setToolTip(l("settings", "настройки"));
    activity.settingsButton->setAccessibleName(l("settings", "настройки"));
    activity.closeButton->setToolTip(l("hide card", "скрыть карточку"));
    activity.closeButton->setAccessibleName(l("hide card", "скрыть карточку"));
    activity.quitButton->setText(l("quit", "выйти"));
    activity.allowButton->setText(l("allow once", "разрешить один раз"));
    activity.denyButton->setText(l("deny", "отклонить"));
    activity.quitButton->setVisible(preview || !QSystemTrayIcon::isSystemTrayAvailable());
    companion.hoverEnabled = settings.value("hover", true).toBool();
    if (!companion.hoverEnabled && !companion.pinned)
        companion.card->hide();
    companion.place(settings.value("screen", 0).toInt(), settings.value("position", "top").toString());
    companion.setVisible(settings.value("companion", true).toBool() ||
                         !QSystemTrayIcon::isSystemTrayAvailable());
    watcher.setInterval(settings.value("pollSeconds", 3).toInt() * 1000);
    auto *old = tray.contextMenu();
    auto *menu = new QMenu(this);
    menu->addAction(l("Open", "открыть"), this, &App::showWindow);
    menu->addAction(l("Settings", "настройки"), this, &App::openSettings);
    menu->addSeparator();
    menu->addAction(l("Quit", "выйти"), this, &App::requestQuit);
    tray.setContextMenu(menu);
    if (old)
        old->deleteLater();
    if (!preview)
        tray.show();
    refresh();
}
void App::openSettings() { settingsDialog()->open(); }
void App::newTask() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(l("New Codex task", "новая задача Codex"));
    dialog->resize(560, 360);
    auto *form = new QFormLayout(dialog);
    auto *row = new QHBoxLayout;
    auto *cwd = new QLineEdit(settings.value("lastProject", QDir::homePath()).toString());
    auto *browse = new QPushButton(l("Browse", "выбрать"));
    row->addWidget(cwd, 1);
    row->addWidget(browse);
    connect(browse, &QPushButton::clicked, dialog, [=] {
        const auto path = QFileDialog::getExistingDirectory(dialog, l("Project", "проект"), cwd->text());
        if (!path.isEmpty())
            cwd->setText(path);
    });
    auto *prompt = new QTextEdit;
    prompt->setAcceptRichText(false);
    form->addRow(l("Project", "проект"), row);
    form->addRow(l("Task", "задача"), prompt);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(l("Start task", "запустить"));
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, dialog, [=] {
        if (!QFileInfo(cwd->text()).isDir() || !QDir::isAbsolutePath(cwd->text()) ||
            prompt->toPlainText().trimmed().isEmpty()) {
            QMessageBox::warning(dialog, l("Check task", "проверь задачу"),
                                 l("Choose an existing absolute project path and enter a task.",
                                   "выбери существующий абсолютный путь проекта и введи задачу."));
            return;
        }
        settings.setValue("lastProject", cwd->text());
        client.startTask(cwd->text(), prompt->toPlainText(), settings.value("model").toString());
        dialog->accept();
    });
    dialog->open();
}
void App::handleRequests() {
    if (handlingRequest || requests.isEmpty())
        return;
    handlingRequest = true;
    const auto message = requests.takeFirst();
    const auto method = message["method"].toString();
    const auto params = message["params"].toObject();
    const auto id = message["id"];
    if (method == "item/commandExecution/requestApproval" || method == "item/fileChange/requestApproval") {
        activeRequest = message;
        for (int i = 0; i < list->count(); ++i)
            if (list->item(i)->data(Qt::UserRole).toString() == params["threadId"].toString()) {
                list->setCurrentRow(i);
                break;
            }
        const auto session = store.sessions().value(params["threadId"].toString());
        QString text = params["cwd"].toString(session.cwd) + "\n" + params["reason"].toString() + "\n" +
                       params["command"].toString();
        if (method == "item/fileChange/requestApproval")
            text += "\n" + message["_detail"].toString() + "\n" + params["grantRoot"].toString();
        if (params["networkApprovalContext"].isObject())
            text += "\n" + params["networkApprovalContext"].toObject()["host"].toString();
        activity.approvalTitle->setText(l("allow this action?", "разрешить это действие?"));
        activity.approvalDetail->setPlainText(text.trimmed());
        const auto decisions = params["availableDecisions"].toArray();
        activity.allowButton->setEnabled(client.connected() &&
                                         (decisions.isEmpty() || decisions.contains("accept")));
        activity.denyButton->setEnabled(client.connected());
        activity.approvalSection->show();
        refresh();
        showWindow();
        activity.denyButton->setFocus();
        return;
    } else if (method == "item/tool/requestUserInput") {
        QDialog dialog(this);
        dialog.setWindowTitle(l("Codex has a question", "вопрос от Codex"));
        dialog.resize(540, 340);
        auto *layout = new QVBoxLayout(&dialog);
        auto *area = new QScrollArea;
        area->setWidgetResizable(true);
        auto *content = new QWidget;
        auto *fields = new QFormLayout(content);
        QMap<QString, QLineEdit *> answers;
        for (const auto &value : params["questions"].toArray()) {
            const auto question = value.toObject();
            QString text = question["question"].toString();
            for (const auto &option : question["options"].toArray()) {
                const auto o = option.toObject();
                text += "\n" + o["label"].toString() + ": " + o["description"].toString();
            }
            auto *label = new QLabel(text);
            label->setTextFormat(Qt::PlainText);
            label->setWordWrap(true);
            auto *field = new QLineEdit;
            if (question["isSecret"].toBool())
                field->setEchoMode(QLineEdit::Password);
            fields->addRow(label);
            fields->addRow(field);
            answers.insert(question["id"].toString(), field);
        }
        area->setWidget(content);
        layout->addWidget(area);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(&client, &CodexClient::connectionChanged, &dialog, [&](bool ready) {
            if (!ready)
                dialog.reject();
        });
        connect(&client, &CodexClient::event, &dialog, [&](const QJsonObject &e) {
            const auto p = e["params"].toObject();
            if ((e["method"] == "serverRequest/resolved" && p["requestId"] == id) ||
                (e["method"] == "turn/completed" && p["threadId"] == params["threadId"]))
                dialog.reject();
        });
        QJsonObject result;
        if (dialog.exec() == QDialog::Accepted) {
            for (auto it = answers.begin(); it != answers.end(); ++it)
                result.insert(it.key(), QJsonObject{{"answers", QJsonArray{it.value()->text()}}});
        }
        if (client.connected())
            client.answer(id, {{"answers", result}});
    } else
        client.rejectUnsupported(id);
    handlingRequest = false;
    QTimer::singleShot(0, this, &App::handleRequests);
}
void App::notify(const QString &id, State, State after) {
    if (booting || preview || !settings.value("notifications", true).toBool())
        return;
    const auto s = store.sessions().value(id);
    if (s.updated.secsTo(QDateTime::currentDateTimeUtc()) > 30)
        return;
    if (after != State::Done && after != State::Waiting && after != State::Error)
        return;
    tray.showMessage("GPT · " + stateName(after), QFileInfo(s.cwd).fileName(), QSystemTrayIcon::Information,
                     5000);
    if (settings.value("sound", false).toBool())
        QApplication::beep();
}
bool App::setAutostart(bool enabled, QString *error) {
    const auto executable = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
#ifdef Q_OS_WIN
    QSettings registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);
    if (enabled)
        registry.setValue("GptStatusBar", "\"" + executable + "\" --background");
    else
        registry.remove("GptStatusBar");
    registry.sync();
    if (registry.status() != QSettings::NoError) {
        *error = "Could not update the startup registry entry.";
        return false;
    }
    return true;
#else
#ifdef Q_OS_MACOS
    const auto directory = QDir::home().filePath("Library/LaunchAgents");
    const auto path = directory + "/io.github.sorried.gpt-status-bar.plist";
    const auto content =
        QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST "
                "1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"><plist "
                "version=\"1.0\"><dict><key>Label</key><string>io.github.sorried.gpt-status-bar</"
                "string><key>ProgramArguments</key><array><string>%1</string><string>--background</string></"
                "array><key>RunAtLoad</key><true/></dict></plist>")
            .arg(executable.toHtmlEscaped());
#else
    const auto directory =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/autostart";
    const auto path = directory + "/io.github.sorried.gpt-status-bar.desktop";
    auto escaped = executable;
    escaped.replace("\\", "\\\\")
        .replace("\"", "\\\"")
        .replace("`", "\\`")
        .replace("$", "\\$")
        .replace("%", "%%");
    const auto content = QString("[Desktop Entry]\nType=Application\nName=GPT status bar\nExec=\"%1\" "
                                 "--background\nTerminal=false\nX-GNOME-Autostart-enabled=true\n")
                             .arg(escaped);
#endif
    if (!enabled) {
        if (!QFile::exists(path) || QFile::remove(path))
            return true;
        *error = "Could not remove " + path;
        return false;
    }
    if (!QDir().mkpath(directory)) {
        *error = "Could not create " + directory;
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(content.toUtf8()) < 0 || !file.commit()) {
        *error = file.errorString();
        return false;
    }
    return true;
#endif
}
bool App::hasActiveTasks() const {
    for (const auto &s : store.sessions())
        if (s.managed && (s.state == State::Working || s.state == State::Thinking || s.state == State::Tool ||
                          s.state == State::Waiting))
            return true;
    return false;
}
void App::closeEvent(QCloseEvent *e) {
    if (!quitting) {
        companion.pinned = false;
        hide();
        e->ignore();
    } else
        e->accept();
}
void App::capture(const QString &directory) {
    QDir().mkpath(directory);
    showWindow();
    qApp->processEvents();
    grab().save(QDir(directory).filePath("monitor.png"));
    companion.grab().save(QDir(directory).filePath("companion.png"));
    auto *dialog = settingsDialog();
    dialog->show();
    qApp->processEvents();
    dialog->grab().save(QDir(directory).filePath("settings.png"));
    dialog->findChild<QListWidget *>("navigation")->setCurrentRow(1);
    qApp->processEvents();
    dialog->grab().save(QDir(directory).filePath("appearance.png"));
    dialog->findChild<QListWidget *>("navigation")->setCurrentRow(2);
    qApp->processEvents();
    dialog->grab().save(QDir(directory).filePath("connection.png"));
    dialog->close();
    companion.showDetails();
    qApp->processEvents();
    companion.card->grab().save(QDir(directory).filePath("hover.png"));
    companion.card->hide();
}

void App::shutdown() {
    quitting = true;
    qApp->quit();
}

App::~App() {
    disconnect(&client, nullptr, this, nullptr);
    disconnect(&store, nullptr, this, nullptr);
    client.stop();
}

void App::requestQuit() {
    if (hasActiveTasks() &&
        QMessageBox::question(this, l("Quit?", "выйти?"),
                              l("Running tasks started here will be stopped. Quit?",
                                "запущенные здесь задачи будут остановлены. выйти?")) != QMessageBox::Yes)
        return;
    shutdown();
}

void App::clearApproval() {
    activeRequest = {};
    activity.approvalSection->hide();
    handlingRequest = false;
    refresh();
    QTimer::singleShot(0, this, &App::handleRequests);
}
void App::finishApproval(bool allow) {
    if (activeRequest.isEmpty() || !client.connected())
        return;
    if (allow && !activity.allowButton->isEnabled())
        return;
    client.answer(activeRequest["id"], {{"decision", allow ? "accept" : "decline"}});
    clearApproval();
}
