#include "app.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyleOptionComboBox>
#include <QStyleOptionSpinBox>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <functional>

namespace {
class GlassDialog : public QDialog {
  public:
    explicit GlassDialog(QWidget *parent) : QDialog(parent) {
        setObjectName("preferencesDialog");
        auto font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        font.setCapitalization(QFont::MixedCase);
        setFont(font);
#ifdef Q_OS_LINUX
        setAttribute(Qt::WA_TranslucentBackground);
#endif
    }
    void setReducedTransparency(bool value) {
        reducedTransparency = value;
        setProperty("reducedTransparency", value);
        update();
    }
    void setDark(bool value) {
        dark = value;
        setProperty("glassDark", dark);
        const QString text = dark ? "#f3f6fc" : "#202736";
        const QString secondary = dark ? "#b8c4d7" : "#586477";
        const QString line = dark ? "rgba(230,240,255,30)" : "rgba(60,85,120,28)";
        const QString field = dark ? "rgba(14,23,39,155)" : "rgba(255,255,255,205)";
        const QString popup = dark ? "#253044" : "#f7f9fc";
        setStyleSheet(
            QString(
                "QWidget { background: transparent; color: %1; font-size: 13px; }"
                "QLabel { border: none; }"
                "QLabel[secondary=\"true\"] { color: %2; font-size: 11px; }"
                "QLabel[section=\"true\"] { color: %2; font-size: 11px; font-weight: 600; }"
                "QLabel[heading=\"true\"] { font-size: 27px; font-weight: 600; letter-spacing: -0.7px; }"
                "QFrame#settingsGroup { background: %3; border: 1px solid %4; border-radius: 17px; }"
                "QFrame#settingDivider { background: %4; border: none; max-height: 1px; }"
                "QListWidget#navigation { border: none; outline: none; padding: 0; }"
                "QListWidget#navigation::item { padding: 0 12px; border-radius: 10px; color: %1; }"
                "QListWidget#navigation::item:hover { background: %5; }"
                "QListWidget#navigation::item:selected { background: #1478ef; color: white; }"
                "QListWidget#navigation::item:focus { border: 1px solid #72b4ff; }"
                "QLineEdit, QComboBox, QSpinBox { background: %6; border: 1px solid %4; border-radius: 9px;"
                " padding: 8px 11px; min-height: 19px; selection-background-color: #1478ef; }"
                "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border: 2px solid #459bff; padding: 7px "
                "10px; }"
                "QComboBox { padding-right: 29px; }"
                "QComboBox::drop-down { border: none; width: 27px; }"
                "QComboBox::down-arrow { image: none; width: 0; height: 0; }"
                "QComboBox QAbstractItemView { background: %7; color: %1; selection-background-color: "
                "#1478ef;"
                " selection-color: white; border: 1px solid %4; padding: 5px; }"
                "QSpinBox { padding-right: 29px; }"
                "QSpinBox QLineEdit { background: transparent; border: none; padding: 0; min-height: 0; }"
                "QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right;"
                " border: none; width: 27px; height: 19px; }"
                "QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right;"
                " border: none; width: 27px; height: 19px; }"
                "QSpinBox::up-arrow, QSpinBox::down-arrow { image: none; width: 0; height: 0; }"
                "QPushButton { border: 1px solid %4; background: %6; border-radius: 11px; padding: 9px 17px;"
                " min-height: 19px; font-weight: 500; }"
                "QPushButton:hover { background: %5; border-color: #86a8d3; }"
                "QPushButton:focus { border: 2px solid #459bff; padding: 8px 16px; }"
                "QPushButton#saveButton { background: #1478ef; color: white; border-color: #1478ef; }"
                "QPushButton#saveButton:hover { background: #0868dc; }"
                "QPushButton#saveButton:focus { border-color: #9ccaff; }"
                "QWidget:disabled { color: %2; }"
                "QScrollArea { border: none; background: transparent; }"
                "QScrollBar:vertical { background: transparent; width: 7px; margin: 3px 0; }"
                "QScrollBar::handle:vertical { background: %8; border-radius: 3px; min-height: 28px; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
                "QToolTip { background: %7; color: %1; border: 1px solid %4; padding: 7px; }")
                .arg(text, secondary, dark ? "rgba(255,255,255,18)" : "rgba(255,255,255,190)", line,
                     dark ? "rgba(255,255,255,22)" : "rgba(255,255,255,235)", field, popup,
                     dark ? "rgba(195,215,240,85)" : "rgba(79,101,131,85)"));
        update();
        for (auto *child : findChildren<QWidget *>())
            child->update();
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(rect(), Qt::transparent);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        auto body = dark ? QColor("#182132") : QColor("#e9eef6");
        auto sidebar = dark ? QColor("#141d2c") : QColor("#edf2f9");
        body.setAlpha(reducedTransparency ? 255 : 245);
        sidebar.setAlpha(reducedTransparency ? 255 : 215);
        p.fillRect(QRect(214, 0, width() - 214, height()), body);
        p.fillRect(QRect(0, 0, 214, height()), sidebar);
        QRadialGradient light(QPointF(width() * .77, height() * -.10), width() * .76);
        light.setColorAt(0, dark ? QColor(93, 126, 185, 76) : QColor(255, 255, 255, 232));
        light.setColorAt(1, Qt::transparent);
        p.fillRect(rect(), light);
        QRadialGradient tint(QPointF(width() * .04, height() * .96), width() * .69);
        tint.setColorAt(0, dark ? QColor(84, 108, 164, 45) : QColor(180, 205, 243, 63));
        tint.setColorAt(1, Qt::transparent);
        p.fillRect(rect(), tint);
        p.fillRect(QRect(0, 0, 214, height()), dark ? QColor(8, 16, 29, 66) : QColor(255, 255, 255, 62));
        p.setPen(dark ? QColor(235, 245, 255, 24) : QColor(255, 255, 255, 170));
        p.drawLine(0, 0, width(), 0);
        p.setPen(dark ? QColor(235, 245, 255, 16) : QColor(87, 113, 152, 27));
        p.drawLine(213, 0, 213, height());
    }

  private:
    bool dark = false;
    bool reducedTransparency = false;
};

void drawChevron(QPainter &p, const QPointF &center, bool up, const QColor &color) {
    p.setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const qreal direction = up ? -1 : 1;
    p.drawLine(center + QPointF(-3.5, -direction * 1.7), center + QPointF(0, direction * 1.7));
    p.drawLine(center + QPointF(0, direction * 1.7), center + QPointF(3.5, -direction * 1.7));
}
class GlassCombo : public QComboBox {
  public:
    GlassCombo() { setFixedHeight(38); }

  protected:
    void paintEvent(QPaintEvent *event) override {
        QComboBox::paintEvent(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QStyleOptionComboBox option;
        initStyleOption(&option);
        const auto area =
            style()->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, this);
        const QColor ink = window()->property("glassDark").toBool() ? QColor("#d5dfed") : QColor("#526277");
        if (!isEnabled())
            p.setOpacity(.45);
        drawChevron(p, area.center(), false, ink);
    }
};
class GlassSpin : public QSpinBox {
  public:
    GlassSpin() { setFixedHeight(38); }

  protected:
    void paintEvent(QPaintEvent *event) override {
        QSpinBox::paintEvent(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QStyleOptionSpinBox option;
        initStyleOption(&option);
        const auto up = style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, this);
        const auto down = style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, this);
        const QColor ink = window()->property("glassDark").toBool() ? QColor("#d5dfed") : QColor("#526277");
        p.setOpacity(isEnabled() && option.stepEnabled.testFlag(QAbstractSpinBox::StepUpEnabled) ? 1 : .35);
        drawChevron(p, up.center(), true, ink);
        p.setOpacity(isEnabled() && option.stepEnabled.testFlag(QAbstractSpinBox::StepDownEnabled) ? 1 : .35);
        drawChevron(p, down.center(), false, ink);
    }
};

class PillSwitch : public QCheckBox {
  public:
    PillSwitch(const QString &label, bool checked, bool animate) : animate(animate) {
        setAccessibleName(label);
        setToolTip(label);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(50, 30);
        setChecked(checked);
        position = checked ? 1.0 : 0.0;
        animation.setDuration(150);
        animation.setEasingCurve(QEasingCurve::InOutCubic);
        connect(&animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            position = value.toReal();
            update();
        });
        connect(this, &QCheckBox::toggled, this, [this](bool checked) {
            animation.stop();
            if (this->animate) {
                animation.setStartValue(position);
                animation.setEndValue(checked ? 1.0 : 0.0);
                animation.start();
            } else {
                position = checked ? 1.0 : 0.0;
                update();
            }
        });
    }
    void setAnimated(bool value) {
        animate = value;
        if (!animate) {
            animation.stop();
            position = isChecked() ? 1.0 : 0.0;
            update();
        }
    }

  protected:
    bool hitButton(const QPoint &point) const override { return rect().contains(point); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const bool dark = window()->property("glassDark").toBool();
        const QRectF track(3, 3, 44, 24);
        if (!isEnabled())
            p.setOpacity(.45);
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked() ? QColor("#1478ef") : dark ? QColor("#5a6577") : QColor("#a6afbd"));
        p.drawRoundedRect(track, 12, 12);
        p.setPen(QPen(isChecked() ? QColor(255, 255, 255, 57) : QColor(255, 255, 255, 35), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(track.adjusted(.5, .5, -.5, -.5), 11.5, 11.5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 35));
        p.drawEllipse(QRectF(5 + position * 20, 5.5, 20, 20));
        p.setBrush(Qt::white);
        p.drawEllipse(QRectF(5 + position * 20, 5, 20, 20));
        if (hasFocus()) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor("#459bff"), 2));
            p.drawRoundedRect(QRectF(1, 1, 48, 28), 14, 14);
        }
    }

  private:
    QVariantAnimation animation;
    qreal position = 0;
    bool animate;
};

QLabel *textLabel(const QString &text, bool secondary = false) {
    auto *label = new QLabel(text);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setProperty("secondary", secondary);
    return label;
}
QWidget *makePage(QStackedWidget *pages, const QString &title, const QString &subtitle, QVBoxLayout **body) {
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->viewport()->setAutoFillBackground(false);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(32, 27, 32, 22);
    layout->setSpacing(22);
    auto *heading = textLabel(title);
    heading->setProperty("heading", true);
    auto *top = new QVBoxLayout;
    top->setSpacing(8);
    top->addWidget(heading);
    top->addWidget(textLabel(subtitle, true));
    layout->addLayout(top);
    scroll->setWidget(content);
    pages->addWidget(scroll);
    *body = layout;
    return content;
}
QFrame *group(QVBoxLayout *page, const QString &caption = {}) {
    auto *section = new QWidget;
    auto *column = new QVBoxLayout(section);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(8);
    if (!caption.isEmpty()) {
        auto *label = textLabel(caption);
        label->setProperty("section", true);
        column->addWidget(label);
    }
    auto *frame = new QFrame;
    frame->setObjectName("settingsGroup");
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    column->addWidget(frame);
    page->addWidget(section);
    return frame;
}
void separator(QVBoxLayout *layout) {
    auto *divider = new QFrame;
    divider->setObjectName("settingDivider");
    divider->setFixedHeight(1);
    auto *row = new QHBoxLayout;
    row->setContentsMargins(18, 0, 18, 0);
    row->addWidget(divider);
    layout->addLayout(row);
}
QWidget *settingRow(QFrame *frame, const QString &title, const QString &description, QWidget *control) {
    auto *layout = qobject_cast<QVBoxLayout *>(frame->layout());
    if (layout->count())
        separator(layout);
    auto *row = new QWidget;
    auto *line = new QHBoxLayout(row);
    line->setContentsMargins(18, 13, 18, 13);
    line->setSpacing(18);
    auto *labels = new QVBoxLayout;
    labels->setSpacing(5);
    auto *label = textLabel(title);
    label->setBuddy(control);
    labels->addWidget(label);
    if (!description.isEmpty())
        labels->addWidget(textLabel(description, true));
    line->addLayout(labels, 1);
    line->addWidget(control, 0, Qt::AlignVCenter);
    layout->addWidget(row);
    return row;
}
QWidget *inputRow(QFrame *frame, const QString &title, const QString &description, QLineEdit *field,
                  const std::function<void()> &browse = {}) {
    auto *layout = qobject_cast<QVBoxLayout *>(frame->layout());
    if (layout->count())
        separator(layout);
    auto *row = new QWidget;
    auto *column = new QVBoxLayout(row);
    column->setContentsMargins(18, 15, 18, 16);
    column->setSpacing(7);
    auto *label = textLabel(title);
    label->setBuddy(field);
    column->addWidget(label);
    if (!description.isEmpty())
        column->addWidget(textLabel(description, true));
    auto *line = new QHBoxLayout;
    line->setSpacing(8);
    line->addWidget(field);
    if (browse) {
        auto *button = new QPushButton("…");
        button->setAccessibleName(title);
        button->setFixedWidth(40);
        button->setToolTip(title);
        QObject::connect(button, &QPushButton::clicked, row, browse);
        line->addWidget(button);
    }
    column->addLayout(line);
    layout->addWidget(row);
    return row;
}
QIcon navigationIcon(int index) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#7894b8"), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (index == 0) {
        for (int y : {6, 12, 18})
            p.drawLine(4, y, 20, y);
        p.setBrush(QColor("#7894b8"));
        for (const QPoint &point : {QPoint(9, 6), QPoint(16, 12), QPoint(8, 18)})
            p.drawEllipse(QPointF(point), 2.5, 2.5);
    } else if (index == 1) {
        p.drawRoundedRect(QRectF(4, 4, 16, 16), 5, 5);
        p.drawLine(12, 4, 12, 20);
        p.setBrush(QColor("#7894b8"));
        p.drawRoundedRect(QRectF(5.5, 5.5, 5, 13), 2, 2);
    } else {
        p.drawRoundedRect(QRectF(3, 5, 18, 14), 4, 4);
        p.drawLine(7, 9, 10, 12);
        p.drawLine(10, 12, 7, 15);
        p.drawLine(13, 15, 17, 15);
    }
    return QIcon(pixmap);
}
} // namespace

QDialog *App::settingsDialog() {
    auto *dialog = new GlassDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(l("settings", "настройки"));
    dialog->resize(930, 690);
    dialog->setMinimumSize(800, 590);
    const auto initialTheme = settings.value("theme", "system").toString();
    auto isDark = [](const QString &theme) {
        return theme == "dark" ||
               (theme == "system" && qApp->palette().color(QPalette::Window).lightness() < 128);
    };
    dialog->setDark(isDark(initialTheme));
    dialog->setReducedTransparency(settings.value("reduceTransparency", false).toBool());
    auto *outer = new QHBoxLayout(dialog);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    auto *sidebar = new QWidget;
    sidebar->setFixedWidth(214);
    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(16, 28, 16, 20);
    side->setSpacing(22);
    auto *brand = new QVBoxLayout;
    brand->setSpacing(10);
    auto *logo = new QLabel;
    logo->setPixmap(Mascot::icon().pixmap(44, 44));
    logo->setAccessibleName("ChatGPT");
    brand->addWidget(logo);
    auto *name = textLabel("GPT status bar");
    auto font = name->font();
    font.setPointSize(13);
    font.setWeight(QFont::DemiBold);
    name->setFont(font);
    brand->addWidget(name);
    brand->addWidget(textLabel(l("Codex companion", "компаньон Codex"), true));
    side->addLayout(brand);
    auto *navigation = new QListWidget;
    navigation->setObjectName("navigation");
    navigation->setAccessibleName(l("settings sections", "разделы настроек"));
    navigation->setIconSize(QSize(22, 22));
    navigation->setSpacing(4);
    navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    const QStringList names{l("general", "основное"), l("appearance", "внешний вид"), "Codex"};
    for (int i = 0; i < names.size(); ++i) {
        auto *item = new QListWidgetItem(navigationIcon(i), names[i], navigation);
        item->setSizeHint(QSize(180, 44));
    }
    side->addWidget(navigation);
    auto *privacy =
        textLabel(l("Local activity, at a glance.", "локальная активность одним взглядом."), true);
    side->addWidget(privacy);
    side->addWidget(textLabel("v" + QCoreApplication::applicationVersion(), true));
    outer->addWidget(sidebar);
    auto *main = new QVBoxLayout;
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(0);
    auto *pages = new QStackedWidget;
    pages->setObjectName("settingsPages");
    main->addWidget(pages, 1);
    const bool animate = settings.value("motion", true).toBool();
    auto makeSwitch = [&](const QString &title, const char *key, bool fallback, const QString &name) {
        auto *control = new PillSwitch(title, settings.value(key, fallback).toBool(), animate);
        control->setObjectName(name);
        return control;
    };
    auto makeCombo = [](std::initializer_list<QPair<QString, QString>> values, const QString &current,
                        const QString &name) {
        auto *box = new GlassCombo;
        box->setObjectName(name);
        for (const auto &value : values)
            box->addItem(value.first, value.second);
        box->setCurrentIndex(qMax(0, box->findData(current)));
        box->setMinimumWidth(146);
        return box;
    };

    QVBoxLayout *general;
    makePage(pages, l("general", "основное"),
             l("Choose when the app starts and how it gets your attention.",
               "когда запускать приложение и как сообщать о событиях."),
             &general);
    auto *behavior = group(general, l("startup", "запуск"));
    auto *startup =
        makeSwitch(l("start at login", "запускать при входе"), "autostart", false, "startupControl");
    settingRow(behavior, l("start at login", "запускать при входе"),
               l("Keep the companion ready in the background.", "открывать компаньона в фоновом режиме."),
               startup);
    auto *alerts = group(general, l("notifications", "уведомления"));
    auto *notifications =
        makeSwitch(l("notifications", "уведомления"), "notifications", true, "notificationsControl");
    settingRow(alerts, l("notifications", "уведомления"),
               l("When a task finishes, fails or needs an answer.",
                 "когда задача завершилась, сломалась или ждёт ответа."),
               notifications);
    auto *sound = makeSwitch(l("system sound", "системный звук"), "sound", false, "soundControl");
    settingRow(alerts, l("system sound", "системный звук"),
               l("A short beep with the notification.", "короткий сигнал вместе с уведомлением."), sound);
    sound->setEnabled(notifications->isChecked());
    connect(notifications, &QCheckBox::toggled, sound, &QWidget::setEnabled);
    auto *regional = group(general, l("language & refresh", "язык и обновление"));
    auto *language = makeCombo({{"English", "en"}, {"Русский", "ru"}}, l("en", "ru"), "languageControl");
    language->setAccessibleName(l("language", "язык"));
    settingRow(
        regional, l("language", "язык"),
        l("Applies throughout the app after saving.", "применяется ко всему приложению после сохранения."),
        language);
    auto *poll = new GlassSpin;
    poll->setObjectName("pollControl");
    poll->setAccessibleName(l("session refresh interval", "период обновления сессий"));
    poll->setRange(1, 30);
    poll->setSuffix(l(" sec", " сек"));
    poll->setValue(settings.value("pollSeconds", 3).toInt());
    poll->setFixedWidth(112);
    settingRow(regional, l("refresh sessions", "обновлять сессии"),
               l("How often local activity files are checked.", "как часто проверять локальные журналы."),
               poll);
    general->addStretch();

    QVBoxLayout *appearance;
    makePage(pages, l("appearance", "внешний вид"),
             l("A quiet place for your agent, exactly where you want it.",
               "оформление и положение панели на рабочем столе."),
             &appearance);
    auto *previewFrame = group(appearance);
    auto *previewLayout = qobject_cast<QVBoxLayout *>(previewFrame->layout());
    auto *previewRow = new QWidget;
    auto *previewLine = new QHBoxLayout(previewRow);
    previewLine->setContentsMargins(20, 18, 20, 18);
    auto *previewCaption = new QVBoxLayout;
    previewCaption->setSpacing(5);
    previewCaption->addWidget(textLabel(l("your status bar", "твоя панель")));
    previewCaption->addWidget(
        textLabel(l("Preview only. No task is running.", "предпросмотр. задача не запущена."), true));
    previewLine->addLayout(previewCaption, 1);
    auto *sample = new Mascot;
    sample->setFixedSize(64, 64);
    sample->configure(State::Working, animate);
    previewLine->addWidget(sample);
    previewLayout->addWidget(previewRow);
    auto *look = group(appearance, l("style", "оформление"));
    auto *theme = makeCombo({{l("system", "системная"), "system"},
                             {l("dark", "тёмная"), "dark"},
                             {l("light", "светлая"), "light"}},
                            initialTheme, "themeControl");
    theme->setAccessibleName(l("theme", "тема"));
    settingRow(look, l("theme", "тема"),
               l("Follow the system or choose a look.", "системная тема или собственный выбор."), theme);
    auto *motion = makeSwitch(l("animation", "анимация"), "motion", true, "motionControl");
    settingRow(
        look, l("animation", "анимация"),
        l("Animate the activity indicator and switches.", "движение индикатора работы и переключателей."),
        motion);
    auto *transparency = makeSwitch(l("reduce transparency", "уменьшить прозрачность"), "reduceTransparency",
                                    false, "transparencyControl");
    settingRow(
        look, l("reduce transparency", "уменьшить прозрачность"),
        l("Use an opaque material for more contrast.", "непрозрачный материал для большей контрастности."),
        transparency);
    connect(transparency, &QCheckBox::toggled, dialog,
            [dialog](bool checked) { dialog->setReducedTransparency(checked); });
    connect(theme, &QComboBox::currentIndexChanged, dialog,
            [=] { dialog->setDark(isDark(theme->currentData().toString())); });
    connect(motion, &QCheckBox::toggled, sample,
            [sample](bool checked) { sample->configure(State::Working, checked); });
    for (auto *control : {startup, notifications, sound, motion, transparency})
        connect(motion, &QCheckBox::toggled, control,
                [control](bool checked) { control->setAnimated(checked); });
    auto *bar = group(appearance, l("floating panel", "плавающая панель"));
    auto *visible =
        makeSwitch(l("show floating panel", "показывать панель"), "companion", true, "companionControl");
    settingRow(bar, l("show floating panel", "показывать панель"),
               l("The compact bar stays above other windows.", "компактная панель поверх остальных окон."),
               visible);
    auto *hover =
        makeSwitch(l("details on hover", "подробности при наведении"), "hover", true, "hoverControl");
    settingRow(bar, l("details on hover", "подробности при наведении"),
               l("Reveal the task and its latest activity.", "показывать задачу и последнее действие."),
               hover);
    auto *panelStyle = makeCombo({{l("pill", "пилюля"), "pill"}, {l("notch", "чёлка"), "notch"}},
                                 settings.value("panelStyle", "pill").toString(), "panelStyleControl");
    panelStyle->setAccessibleName(l("panel style", "форма панели"));
    settingRow(bar, l("shape", "форма"),
               l("Choose a rounded pill or a top-edge notch.", "пилюля или вырез у верхнего края."),
               panelStyle);
    auto *panelWidth = new QSpinBox;
    panelWidth->setObjectName("panelWidthControl");
    panelWidth->setRange(120, 480);
    panelWidth->setSuffix(" px");
    panelWidth->setValue(settings.value("panelWidth", 224).toInt());
    settingRow(bar, l("width", "ширина"), {}, panelWidth);
    auto *panelHeight = new QSpinBox;
    panelHeight->setObjectName("panelHeightControl");
    panelHeight->setRange(32, 100);
    panelHeight->setSuffix(" px");
    panelHeight->setValue(settings.value("panelHeight", 40).toInt());
    settingRow(bar, l("height", "высота"), {}, panelHeight);
    for (auto *control : {visible, hover})
        connect(motion, &QCheckBox::toggled, control,
                [control](bool checked) { control->setAnimated(checked); });
    auto *position = makeCombo({{l("top center", "сверху по центру"), "top"},
                                {l("top right", "справа сверху"), "top-right"},
                                {l("bottom center", "снизу по центру"), "bottom"}},
                               settings.value("position", "top").toString(), "positionControl");
    position->setAccessibleName(l("panel position", "положение панели"));
    settingRow(bar, l("position", "расположение"), {}, position);
    auto *screen = new GlassCombo;
    screen->setObjectName("screenControl");
    screen->setAccessibleName(l("display", "экран"));
    screen->setMinimumWidth(146);
    for (auto *display : QGuiApplication::screens())
        screen->addItem(display->name().isEmpty() ? l("display %1", "экран %1").arg(screen->count() + 1)
                                                  : display->name());
    screen->setCurrentIndex(qBound(0, settings.value("screen", 0).toInt(), qMax(0, screen->count() - 1)));
    settingRow(bar, l("display", "экран"), {}, screen);
    appearance->addStretch();

    QVBoxLayout *integration;
    makePage(pages, l("Codex connection", "подключение Codex"),
             l("Use the Codex installation and account already on your computer.",
               "существующая установка и вход Codex на этом компьютере."),
             &integration);
    auto *paths = group(integration, l("local installation", "локальная установка"));
    auto *executable = new QLineEdit(settings.value("executable", "codex").toString());
    executable->setObjectName("executableControl");
    executable->setAccessibleName(l("Codex executable", "исполняемый файл Codex"));
    inputRow(paths, l("Codex executable", "исполняемый файл Codex"),
             l("Use an absolute path if the app cannot find codex.",
               "укажи полный путь, если приложение не находит codex."),
             executable, [=] {
                 const auto path = QFileDialog::getOpenFileName(
                     dialog, l("choose Codex executable", "выбери исполняемый файл Codex"),
                     executable->text());
                 if (!path.isEmpty())
                     executable->setText(path);
             });
    auto *codexHome = new QLineEdit(home());
    codexHome->setObjectName("homeControl");
    codexHome->setAccessibleName("CODEX_HOME");
    inputRow(
        paths, "CODEX_HOME",
        l("The folder containing your Codex settings and sessions.", "папка с настройками и сессиями Codex."),
        codexHome, [=] {
            const auto path = QFileDialog::getExistingDirectory(dialog, "CODEX_HOME", codexHome->text());
            if (!path.isEmpty())
                codexHome->setText(path);
        });
    auto *modelGroup = group(integration, l("new tasks", "новые задачи"));
    auto *model = new QLineEdit(settings.value("model").toString());
    model->setObjectName("modelControl");
    model->setAccessibleName(l("model override", "выбор модели"));
    model->setPlaceholderText(l("use Codex settings", "из настроек Codex"));
    inputRow(modelGroup, l("model", "модель"),
             l("Leave empty to use the model configured in Codex.",
               "оставь пустым, чтобы использовать модель из Codex."),
             model);
    auto *help =
        textLabel(l("Sign in with codex login in a terminal. This app stores no credentials. Tasks started "
                    "here use workspace-write isolation and request permission for untrusted "
                    "commands.\n\nChanging these paths disconnects the app. Finish running tasks first.",
                    "для входа выполни codex login в терминале. приложение не хранит учётные данные. задачи "
                    "отсюда используют workspace-write и запрашивают разрешение для недоверенных "
                    "команд.\n\nсмена путей отключает соединение. сначала заверши работающие задачи."),
                  true);
    integration->addWidget(help);
    integration->addStretch();

    auto *footer = new QWidget;
    auto *footerLine = new QHBoxLayout(footer);
    footerLine->setContentsMargins(32, 17, 32, 21);
    footerLine->setSpacing(10);
    auto *hint = textLabel(l("Changes apply when saved.", "изменения применятся после сохранения."), true);
    footerLine->addWidget(hint, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    auto *save = buttons->button(QDialogButtonBox::Save);
    auto *cancel = buttons->button(QDialogButtonBox::Cancel);
    save->setText(l("save", "сохранить"));
    save->setObjectName("saveButton");
    save->setIcon({});
    save->setMinimumWidth(82);
    cancel->setText(l("cancel", "отмена"));
    cancel->setObjectName("cancelButton");
    cancel->setIcon({});
    cancel->setMinimumWidth(70);
    footerLine->addWidget(buttons);
    main->addWidget(footer);
    outer->addLayout(main, 1);
    connect(navigation, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    navigation->setCurrentRow(0);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, dialog, [=] {
        const QString nextHome = QDir::cleanPath(codexHome->text().trimmed());
        if (!QDir::isAbsolutePath(nextHome) || executable->text().trimmed().isEmpty()) {
            QMessageBox::warning(dialog, l("invalid path", "неверный путь"),
                                 l("Enter an absolute CODEX_HOME and a Codex executable.",
                                   "укажи абсолютный CODEX_HOME и исполняемый файл Codex."));
            navigation->setCurrentRow(2);
            return;
        }
        const bool reconnect = nextHome != home() || executable->text().trimmed() !=
                                                         settings.value("executable", "codex").toString();
        if (reconnect && hasActiveTasks()) {
            QMessageBox::warning(dialog, l("tasks are running", "задачи ещё работают"),
                                 l("Finish or stop tasks before changing Codex paths.",
                                   "заверши или останови задачи перед сменой путей Codex."));
            return;
        }
        QString failure;
        if (!preview && startup->isChecked() != settings.value("autostart", false).toBool() &&
            !setAutostart(startup->isChecked(), &failure)) {
            QMessageBox::warning(dialog, l("could not change startup", "не удалось изменить автозапуск"),
                                 failure);
            return;
        }
        const QString previousHome = home();
        settings.setValue("language", language->currentData());
        settings.setValue("theme", theme->currentData());
        settings.setValue("motion", motion->isChecked());
        settings.setValue("reduceTransparency", transparency->isChecked());
        settings.setValue("companion", visible->isChecked());
        settings.setValue("panelStyle", panelStyle->currentData());
        settings.setValue("panelWidth", panelWidth->value());
        settings.setValue("panelHeight", panelHeight->value());
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
