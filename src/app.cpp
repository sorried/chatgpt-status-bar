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
#include <QStandardPaths>
#include <QTabWidget>
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
    card = new QWidget(this, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    card->setWindowTitle("Codex activity");
    card->setCursor(Qt::PointingHandCursor);
    card->installEventFilter(this);
    card->setFixedWidth(360);
    card->setStyleSheet(
        "QWidget { background: #090909; color: #f1f1f1; } QLabel { background: transparent; padding: 0; }");
    auto *details = new QVBoxLayout(card);
    details->setContentsMargins(20, 18, 20, 18);
    details->setSpacing(10);
    cardTitle = new QLabel;
    cardTask = new QLabel;
    cardDetail = new QLabel;
    cardTitle->setStyleSheet("font-weight: 600; font-size: 14px;");
    cardDetail->setStyleSheet("color: #b8b8b8; font-size: 12px;");
    for (auto *text : {cardTitle, cardTask, cardDetail}) {
        text->setTextFormat(Qt::PlainText);
        text->setWordWrap(true);
        details->addWidget(text);
    }
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
void Companion::leaveEvent(QEvent *e) {
    QWidget::leaveEvent(e);
    QTimer::singleShot(180, card, [this] {
        if (!underMouse() && !card->underMouse())
            card->hide();
    });
}
bool Companion::eventFilter(QObject *object, QEvent *event) {
    if (object == card && event->type() == QEvent::Leave)
        QTimer::singleShot(180, card, [this] {
            if (!underMouse() && !card->underMouse())
                card->hide();
        });
    if (object == card && event->type() == QEvent::MouseButtonRelease) {
        if (static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
            card->hide();
            emit activated();
        }
    }
    return QWidget::eventFilter(object, event);
}
void Companion::showDetails() {
    card->adjustSize();
    auto *screen = QGuiApplication::screenAt(geometry().center());
    if (!screen)
        return;
    const auto area = screen->availableGeometry();
    const int x =
        qBound(area.left(), geometry().center().x() - card->width() / 2, area.right() - card->width() + 1);
    const int y = geometry().bottom() + card->height() + 8 < area.bottom()
                      ? geometry().bottom() + 5
                      : geometry().top() - card->height() - 5;
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
    resize(840, 620);
    setMinimumSize(680, 480);
    auto *body = new QWidget;
    auto *layout = new QVBoxLayout(body);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);
    auto *head = new QHBoxLayout;
    avatar = new Mascot;
    avatar->setFixedSize(72, 72);
    auto *titles = new QVBoxLayout;
    auto *name = new QLabel("GPT status bar");
    name->setStyleSheet("font-size: 23px; font-weight: 600;");
    summary = new QLabel;
    titles->addWidget(name);
    titles->addWidget(summary);
    auto *prefs = new QPushButton;
    prefs->setObjectName("settingsButton");
    connect(prefs, &QPushButton::clicked, this, &App::openSettings);
    head->addWidget(avatar);
    head->addLayout(titles, 1);
    head->addWidget(prefs);
    layout->addLayout(head);
    connection = new QLabel;
    connection->setTextFormat(Qt::PlainText);
    connection->setWordWrap(true);
    layout->addWidget(connection);
    auto *split = new QSplitter;
    list = new QListWidget;
    list->setMinimumWidth(220);
    list->setAccessibleName(l("Codex sessions", "сессии Codex"));
    split->addWidget(list);
    auto *right = new QWidget;
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(16, 0, 0, 0);
    meta = new QLabel;
    meta->setTextFormat(Qt::PlainText);
    meta->setWordWrap(true);
    meta->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detail = new QTextEdit;
    detail->setReadOnly(true);
    detail->setAcceptRichText(false);
    detail->setAccessibleName(l("Session activity", "активность сессии"));
    rightLayout->addWidget(meta);
    rightLayout->addWidget(detail, 1);
    split->addWidget(right);
    split->setStretchFactor(1, 2);
    layout->addWidget(split, 1);
    usage = new QLabel;
    usage->setWordWrap(true);
    layout->addWidget(usage);
    auto *actions = new QHBoxLayout;
    connectButton = new QPushButton;
    taskButton = new QPushButton;
    stopButton = new QPushButton;
    folderButton = new QPushButton;
    actions->addWidget(connectButton);
    actions->addWidget(taskButton);
    actions->addStretch();
    actions->addWidget(folderButton);
    actions->addWidget(stopButton);
    auto *quitButton = new QPushButton;
    quitButton->setObjectName("quitButton");
    connect(quitButton, &QPushButton::clicked, this, &App::requestQuit);
    actions->addWidget(quitButton);
    layout->addLayout(actions);
    setCentralWidget(body);
    connect(list, &QListWidget::currentRowChanged, this, &App::refresh);
    connect(&store, &SessionStore::changed, this, &App::refresh);
    connect(&store, &SessionStore::transitioned, this, &App::notify);
    connect(&watcher, &SessionWatcher::scanned, this, &App::refresh);
    connect(&client, &CodexClient::event, &store, &SessionStore::ingestRpc);
    connect(&client, &CodexClient::event, this, [this](const QJsonObject &e) {
        const auto p = e["params"].toObject();
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
        item->setSizeHint(QSize(210, 62));
        list->addItem(item);
        if (s.id == previous)
            selectedRow = list->count() - 1;
    }
    if (list->count())
        list->setCurrentRow(selectedRow >= 0 ? selectedRow : 0);
    list->verticalScrollBar()->setValue(scroll);
    list->blockSignals(false);
    summary->setText(
        sessions.isEmpty()
            ? l("Your agent, at a glance.", "твой агент одним взглядом.")
            : l("%1 active · %2 sessions", "активных: %1 · сессий: %2").arg(active).arg(sessions.size()));
    const auto s = store.sessions().value(selected());
    QString duration;
    if (s.started.isValid()) {
        const qint64 seconds =
            qMax<qint64>(0, s.started.secsTo(s.state == State::Done || s.state == State::Idle ||
                                                     s.state == State::Unknown || s.state == State::Error
                                                 ? s.updated
                                                 : QDateTime::currentDateTimeUtc()));
        duration = QString("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QChar('0'));
    }
    if (sessions.isEmpty()) {
        meta->setText(l("No sessions yet", "пока нет сессий"));
        detail->setPlainText(
            l("Open Codex in a project. Recent local sessions will appear here automatically.\n\nConnect to "
              "Codex to launch a task and answer its approval requests here.",
              "открой Codex в проекте. свежие локальные сессии появятся здесь автоматически.\n\nподключись к "
              "Codex, чтобы запустить задачу и отвечать на её запросы здесь."));
    } else {
        meta->setText(
            s.title + "\n" + s.cwd + "\n" + stateName(s.state) + "  " + duration +
            (s.model.isEmpty() ? "" : "  ·  " + s.model) +
            (s.tokens ? l("\n%1 total tokens", "\nвсего токенов: %1").arg(s.tokens) : "") +
            (s.context ? l(" · context capacity %1", " · ёмкость контекста %1").arg(s.context) : "") + "\n" +
            (s.managed ? l("controlled here", "управляется здесь")
                       : l("observed session · approvals stay in Codex",
                           "наблюдаемая сессия · разрешения в Codex")));
        if (detail->toPlainText() != s.detail)
            detail->setPlainText(s.detail);
    }
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
    companion.cardTitle->setText("Codex · " + stateName(primary.state));
    companion.cardTask->setText(primary.title.isEmpty() ? l("No active task", "нет активной задачи")
                                                        : primary.title.left(220));
    companion.cardDetail->setText(QFileInfo(primary.cwd).fileName() + "\n" + primary.detail.left(280));
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
    connection->setText(status);
    connectButton->setText(client.connected() ? l("Refresh limits", "обновить лимиты")
                                              : l("Connect Codex", "подключить Codex"));
    taskButton->setText(l("New task", "новая задача"));
    taskButton->setEnabled(client.connected() && !preview);
    connectButton->setEnabled(!preview);
    stopButton->setText(l("Stop task", "остановить"));
    stopButton->setEnabled(s.managed && !s.turnId.isEmpty() && client.connected() &&
                           (s.state == State::Working || s.state == State::Thinking ||
                            s.state == State::Tool || s.state == State::Waiting));
    folderButton->setText(l("Open folder", "открыть папку"));
    folderButton->setEnabled(!s.cwd.isEmpty() && QFileInfo(s.cwd).isDir());
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
    usage->setText(windows.isEmpty()
                       ? l("Limits unavailable. Connect with your existing Codex login to check.",
                           "лимиты недоступны. подключись с существующим входом Codex для проверки.")
                       : windows.join('\n'));
}
void App::showWindow() {
    showNormal();
    raise();
    activateWindow();
}
void App::applySettings() {
    setWindowTitle(l("GPT status bar", "GPT status bar"));
    findChild<QPushButton *>("settingsButton")->setText(l("Settings", "настройки"));
    findChild<QPushButton *>("quitButton")->setText(l("Quit", "выйти"));
    const QString theme = settings.value("theme", "system").toString();
    const bool dark =
        theme == "dark" || (theme == "system" && qApp->palette().color(QPalette::Window).lightness() < 128);
    if (theme == "system")
        setStyleSheet({});
    else {
        const QString bg = dark ? "#111111" : "#f7f7f7";
        const QString fg = dark ? "#eeeeee" : "#171717";
        const QString field = dark ? "#191919" : "#ffffff";
        const QString border = dark ? "#444444" : "#aaaaaa";
        setStyleSheet(
            QString("QWidget { background: %1; color: %2; } QListWidget, QTextEdit, QLineEdit, QComboBox, "
                    "QSpinBox { background: %3; border: 1px solid %4; border-radius: 6px; padding: 7px; } "
                    "QPushButton { background: %3; border: 1px solid %4; border-radius: 6px; padding: 8px "
                    "12px; } QPushButton:hover { border-color: #8e8e8e; } QPushButton:focus { border: 2px "
                    "solid #8e8e8e; } QPushButton:disabled { color: %4; } QListWidget::item { padding: 5px; "
                    "} QListWidget::item:selected { background: %4; color: %2; } QToolTip { background: %3; "
                    "color: %2; border: 1px solid %4; }")
                .arg(bg, fg, field, border));
    }
    companion.hoverEnabled = settings.value("hover", true).toBool();
    if (!companion.hoverEnabled)
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
QDialog *App::settingsDialog() {
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(l("Settings", "настройки"));
    dialog->resize(580, 470);
    auto *outer = new QVBoxLayout(dialog);
    auto *tabs = new QTabWidget;
    auto *general = new QWidget;
    auto *form = new QFormLayout(general);
    auto *language = new QComboBox;
    language->addItem("English", "en");
    language->addItem("Русский", "ru");
    language->setCurrentIndex(l("en", "ru") == "ru" ? 1 : 0);
    form->addRow(l("Language", "язык"), language);
    auto *notifications = new QCheckBox(
        l("Notify when done, waiting or failed", "уведомлять о завершении, ожидании и ошибках"));
    notifications->setChecked(settings.value("notifications", true).toBool());
    form->addRow(notifications);
    auto *sound = new QCheckBox(l("Play a system beep", "системный звук"));
    sound->setChecked(settings.value("sound", false).toBool());
    form->addRow(sound);
    auto *startup = new QCheckBox(l("Start at login", "запускать при входе"));
    startup->setChecked(settings.value("autostart", false).toBool());
    form->addRow(startup);
    auto *hover = new QCheckBox(l("Show details on hover", "подсказка при наведении"));
    hover->setChecked(settings.value("hover", true).toBool());
    form->addRow(hover);
    auto *poll = new QSpinBox;
    poll->setRange(1, 30);
    poll->setSuffix(l(" seconds", " сек"));
    poll->setValue(settings.value("pollSeconds", 3).toInt());
    form->addRow(l("Read sessions every", "читать сессии каждые"), poll);
    tabs->addTab(general, l("General", "основное"));
    auto *appearance = new QWidget;
    auto *visual = new QFormLayout(appearance);
    auto combo = [](std::initializer_list<QPair<QString, QString>> values, const QString &current) {
        auto *box = new QComboBox;
        for (const auto &v : values)
            box->addItem(v.first, v.second);
        box->setCurrentIndex(qMax(0, box->findData(current)));
        return box;
    };
    auto *theme = combo({{l("System", "системная"), "system"},
                         {l("Dark", "тёмная"), "dark"},
                         {l("Light", "светлая"), "light"}},
                        settings.value("theme", "system").toString());
    visual->addRow(l("Theme", "тема"), theme);
    auto *motion = new QCheckBox(l("Animate companion", "анимация персонажа"));
    motion->setChecked(settings.value("motion", true).toBool());
    visual->addRow(motion);
    auto *visible = new QCheckBox(l("Show floating status bar", "показывать плавающую панель"));
    visible->setChecked(settings.value("companion", true).toBool());
    visual->addRow(visible);
    auto *position = combo({{l("Top center", "сверху по центру"), "top"},
                            {l("Top right", "справа сверху"), "top-right"},
                            {l("Bottom center", "снизу по центру"), "bottom"}},
                           settings.value("position", "top").toString());
    visual->addRow(l("Position", "расположение"), position);
    auto *screen = new QComboBox;
    for (auto *s : QGuiApplication::screens())
        screen->addItem(s->name().isEmpty() ? l("Display %1", "экран %1").arg(screen->count() + 1)
                                            : s->name());
    screen->setCurrentIndex(qBound(0, settings.value("screen", 0).toInt(), qMax(0, screen->count() - 1)));
    visual->addRow(l("Display", "экран"), screen);
    auto *sample = new Mascot;
    sample->setFixedSize(100, 100);
    sample->configure(State::Working, motion->isChecked());
    visual->addRow(sample);
    auto updateSample = [=] { sample->configure(State::Working, motion->isChecked()); };
    connect(motion, &QCheckBox::toggled, sample, updateSample);
    tabs->addTab(appearance, l("Appearance", "внешний вид"));
    auto *integration = new QWidget;
    auto *integrationForm = new QFormLayout(integration);
    auto *executable = new QLineEdit(settings.value("executable", "codex").toString());
    auto *codexHome = new QLineEdit(home());
    auto *model = new QLineEdit(settings.value("model").toString());
    model->setPlaceholderText(l("Use the model from Codex settings", "модель из настроек Codex"));
    integrationForm->addRow(l("Codex executable", "исполняемый файл Codex"), executable);
    integrationForm->addRow("CODEX_HOME", codexHome);
    integrationForm->addRow(l("Model override", "модель"), model);
    auto *help =
        new QLabel(l("Uses your existing Codex login. Run codex login in a terminal first. No credentials "
                     "are stored by this app.\n\nLocal sessions are observed only. Tasks launched here use "
                     "workspace-write isolation and ask for untrusted commands.\n\nChanging the executable "
                     "or home requires reconnecting. Running tasks must finish first.",
                     "используется существующий вход Codex. сначала выполни codex login в терминале. "
                     "приложение не хранит учётные данные.\n\nлокальные сессии доступны для наблюдения. "
                     "задачи отсюда используют workspace-write и запрашивают разрешения для недоверенных "
                     "команд.\n\nсмена пути требует переподключения. сначала заверши запущенные задачи."));
    help->setWordWrap(true);
    integrationForm->addRow(help);
    tabs->addTab(integration, l("Codex", "Codex"));
    outer->addWidget(tabs);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText(l("Save", "сохранить"));
    buttons->button(QDialogButtonBox::Cancel)->setText(l("Cancel", "отмена"));
    outer->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, dialog, [=] {
        const QString nextHome = QDir::cleanPath(codexHome->text().trimmed());
        if (!QDir::isAbsolutePath(nextHome) || executable->text().trimmed().isEmpty()) {
            QMessageBox::warning(dialog, l("Invalid path", "неверный путь"),
                                 l("Enter an absolute CODEX_HOME and a Codex executable.",
                                   "укажи абсолютный CODEX_HOME и исполняемый файл Codex."));
            return;
        }
        const bool reconnect = nextHome != home() || executable->text().trimmed() !=
                                                         settings.value("executable", "codex").toString();
        if (reconnect && hasActiveTasks()) {
            QMessageBox::warning(dialog, l("Tasks are running", "задачи ещё работают"),
                                 l("Finish or stop tasks before changing Codex paths.",
                                   "заверши или останови задачи перед сменой путей Codex."));
            return;
        }
        QString failure;
        if (!preview && startup->isChecked() != settings.value("autostart", false).toBool() &&
            !setAutostart(startup->isChecked(), &failure)) {
            QMessageBox::warning(dialog, l("Could not change startup", "не удалось изменить автозапуск"),
                                 failure);
            return;
        }
        const QString previousHome = home();
        settings.setValue("language", language->currentData());
        settings.setValue("theme", theme->currentData());
        settings.setValue("motion", motion->isChecked());
        settings.setValue("companion", visible->isChecked());
        settings.setValue("position", position->currentData());
        settings.setValue("screen", screen->currentIndex());
        settings.setValue("notifications", notifications->isChecked());
        settings.setValue("sound", sound->isChecked());
        settings.setValue("hover", hover->isChecked());
        settings.setValue("autostart", startup->isChecked());
        settings.setValue("pollSeconds", poll->value());
        settings.setValue("executable", executable->text().trimmed());
        settings.setValue("codexHome", nextHome);
        settings.setValue("model", model->text().trimmed());
        settings.sync();
        if (reconnect) {
            client.stop();
            rateLimits = {};
            errorText.clear();
        }
        if (!preview && previousHome != nextHome) {
            booting = true;
            watcher.setHome(nextHome);
            booting = false;
        }
        applySettings();
        dialog->accept();
    });
    return dialog;
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
        showWindow();
        QMessageBox box(QMessageBox::Question, l("Codex needs permission", "Codex запрашивает разрешение"),
                        QString(), QMessageBox::NoButton, this);
        box.setTextFormat(Qt::PlainText);
        box.setText(params["reason"].toString(
            l("Review this action before allowing it.", "проверь действие перед разрешением.")));
        const auto session = store.sessions().value(params["threadId"].toString());
        box.setInformativeText(params["cwd"].toString(session.cwd) + "\n" + params["command"].toString() +
                               (params["networkApprovalContext"].isObject()
                                    ? "\n" + params["networkApprovalContext"].toObject()["host"].toString()
                                    : ""));
        if (method == "item/fileChange/requestApproval") {
            box.setDetailedText(message["_detail"].toString() + "\n" + params["grantRoot"].toString());
            box.setInformativeText(
                session.cwd + "\n" +
                l("Review the file changes in Show Details.", "проверь изменения файлов в подробностях."));
        }
        auto *allow = box.addButton(l("Allow once", "разрешить один раз"), QMessageBox::AcceptRole);
        const auto decisions = params["availableDecisions"].toArray();
        if (!decisions.isEmpty() && !decisions.contains("accept"))
            allow->setEnabled(false);
        auto *deny = box.addButton(l("Deny", "отклонить"), QMessageBox::RejectRole);
        box.setDefaultButton(deny);
        box.setEscapeButton(deny);
        connect(&client, &CodexClient::event, &box, [&](const QJsonObject &e) {
            const auto p = e["params"].toObject();
            if ((e["method"] == "serverRequest/resolved" && p["requestId"] == id) ||
                (e["method"] == "turn/completed" && p["threadId"] == params["threadId"]))
                box.reject();
        });
        connect(&client, &CodexClient::connectionChanged, &box, [&](bool ready) {
            if (!ready)
                box.reject();
        });
        box.exec();
        if (client.connected())
            client.answer(id, {{"decision", box.clickedButton() == allow ? "accept" : "decline"}});
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
    dialog->findChild<QTabWidget *>()->setCurrentIndex(1);
    qApp->processEvents();
    dialog->grab().save(QDir(directory).filePath("appearance.png"));
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
