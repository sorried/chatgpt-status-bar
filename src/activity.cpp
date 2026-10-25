#include "activity.h"
#include "mascot.h"
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>
#include <cmath>

namespace {
class ActivityCard : public QWidget {
  public:
    explicit ActivityCard(QWidget *parent) : QWidget(parent) {
        setObjectName("activityCard");
        setAttribute(Qt::WA_TranslucentBackground);
        setMinimumWidth(360);
        setMaximumWidth(480);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF material = QRectF(rect()).adjusted(3.5, 3.5, -3.5, -3.5);
        p.setBrush(QColor("#050505"));
        p.setPen(QPen(QColor(142, 227, 187, 64), 1));
        p.drawRoundedRect(material, 21, 21);
    }
};

QLabel *label(QWidget *parent, const QString &name, const char *kind = nullptr) {
    auto *text = new QLabel(parent);
    text->setObjectName(name);
    text->setTextFormat(Qt::PlainText);
    text->setWordWrap(true);
    if (kind)
        text->setProperty(kind, true);
    return text;
}
QIcon cardIcon(QWidget *widget, QStyle::StandardPixmap type, int size) {
    auto pixmap = widget->style()->standardIcon(type).pixmap(size, size);
    QPainter p(&pixmap);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pixmap.rect(), QColor("#b9b9b9"));
    return QIcon(pixmap);
}
QPushButton *button(QWidget *parent, const QString &name) {
    auto *control = new QPushButton(parent);
    control->setObjectName(name);
    control->setCursor(Qt::PointingHandCursor);
    control->setAutoDefault(false);
    control->setFocusPolicy(Qt::StrongFocus);
    return control;
}
QTextEdit *activityText(QWidget *parent, const QString &name, int maximumHeight) {
    auto *text = new QTextEdit(parent);
    text->setObjectName(name);
    text->setReadOnly(true);
    text->setAcceptRichText(false);
    text->setFrameShape(QFrame::NoFrame);
    text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    text->setMinimumHeight(35);
    text->setMaximumHeight(maximumHeight);
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    text->setFont(font);
    return text;
}
} // namespace

ActivityUi createActivityUi(QWidget *parent) {
    ActivityUi ui{};
    ui.body = new ActivityCard(parent);
    ui.body->setStyleSheet(
        "QWidget { background: transparent; color: #e7e7e7; font-size: 12px; }"
        "QLabel { border: none; padding: 0; }"
        "QLabel[caption=\"true\"] { color: #969696; font-size: 10px; font-weight: 600; }"
        "QLabel[secondary=\"true\"] { color: #aaaaaa; font-size: 11px; }"
        "QLabel#sessionMeta { font-size: 14px; font-weight: 500; }"
        "QLabel#activitySummary { color: #cccccc; font-size: 12px; font-weight: 600; }"
        "QLabel#approvalTitle { color: #ffffff; font-size: 15px; font-weight: 600; }"
        "QTextEdit { background: transparent; color: #b9b9b9; border: none; padding: 0;"
        " selection-background-color: #385449; selection-color: white; }"
        "QListWidget { background: transparent; border: none; outline: none; padding: 0; }"
        "QListWidget::item { color: #b5b5b5; border-radius: 7px; padding: 6px 8px; }"
        "QListWidget::item:hover { background: #171717; }"
        "QListWidget::item:selected { background: #242424; color: #ffffff; }"
        "QListWidget::item:focus { border: 1px solid #8eae9f; }"
        "QPushButton { background: #202020; color: #dedede; border: 1px solid #303030;"
        " border-radius: 13px; min-height: 18px; padding: 5px 11px; font-size: 11px; }"
        "QPushButton:hover { background: #303030; border-color: #595959; }"
        "QPushButton:pressed { background: #3b3b3b; }"
        "QPushButton:focus { border: 1px solid #a4d5bd; }"
        "QPushButton:disabled { color: #777777; border-color: #242424; background: #151515; }"
        "QPushButton#settingsButton, QPushButton#closeCardButton { padding: 0; border: none;"
        " background: transparent; border-radius: 8px; }"
        "QPushButton#settingsButton:hover, QPushButton#closeCardButton:hover { background: #272727; }"
        "QPushButton#settingsButton:focus, QPushButton#closeCardButton:focus { border: 1px solid #a4d5bd; }"
        "QPushButton#allowButton { color: #111111; background: #f3f3f3; border-color: #f3f3f3;"
        " font-weight: 600; padding: 7px 19px; border-radius: 17px; }"
        "QPushButton#allowButton:hover { background: #dcebe2; border-color: #dcebe2; }"
        "QPushButton#denyButton { padding: 7px 19px; border-radius: 17px; font-weight: 600; }"
        "QFrame#activitySeparator { background: #252525; border: none; max-height: 1px; }"
        "QScrollBar:vertical { background: transparent; width: 5px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #4d4d4d; border-radius: 2px; min-height: 18px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QToolTip { background: #242424; color: #ffffff; border: 1px solid #555555; padding: 6px; }");
    auto *layout = new QVBoxLayout(ui.body);
    layout->setContentsMargins(20, 15, 20, 17);
    layout->setSpacing(11);
    layout->setAlignment(Qt::AlignTop);
    auto *header = new QHBoxLayout;
    header->setSpacing(6);
    ui.avatar = new Mascot(ui.body);
    ui.avatar->setFixedSize(32, 32);
    ui.summary = label(ui.body, "activitySummary");
    ui.summary->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui.summary->setWordWrap(false);
    ui.settingsButton = button(ui.body, "settingsButton");
    ui.settingsButton->setFixedSize(26, 26);
    QPixmap gear(16, 16);
    gear.fill(Qt::transparent);
    {
        QPainter painter(&gear);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor("#b9b9b9"), 1.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawEllipse(QPointF(8, 8), 4.3, 4.3);
        painter.drawEllipse(QPointF(8, 8), 1.5, 1.5);
        for (int i = 0; i < 8; ++i) {
            const double angle = i * 3.141592653589793 / 4;
            painter.drawLine(QPointF(8 + std::cos(angle) * 4.5, 8 + std::sin(angle) * 4.5),
                             QPointF(8 + std::cos(angle) * 6.7, 8 + std::sin(angle) * 6.7));
        }
    }
    ui.settingsButton->setIcon(QIcon(gear));
    ui.settingsButton->setIconSize(QSize(16, 16));
    ui.closeButton = button(ui.body, "closeCardButton");
    ui.closeButton->setFixedSize(26, 26);
    ui.closeButton->setIcon(cardIcon(ui.body, QStyle::SP_TitleBarCloseButton, 14));
    ui.closeButton->setIconSize(QSize(14, 14));
    header->addWidget(ui.avatar);
    header->addStretch(1);
    header->addWidget(ui.summary);
    header->addSpacing(4);
    header->addWidget(ui.settingsButton);
    header->addWidget(ui.closeButton);
    layout->addLayout(header);

    ui.connection = label(ui.body, "connectionStatus", "secondary");
    layout->addWidget(ui.connection);
    auto *task = new QVBoxLayout;
    task->setSpacing(6);
    ui.taskLabel = label(ui.body, "taskCaption", "caption");
    ui.meta = label(ui.body, "sessionMeta");
    ui.meta->setTextInteractionFlags(Qt::TextSelectableByMouse);
    task->addWidget(ui.taskLabel);
    task->addWidget(ui.meta);
    layout->addLayout(task);
    auto *current = new QVBoxLayout;
    current->setSpacing(5);
    ui.stateLabel = label(ui.body, "stateCaption", "caption");
    ui.detail = activityText(ui.body, "activityDetail", 92);
    current->addWidget(ui.stateLabel);
    current->addWidget(ui.detail);
    layout->addLayout(current);

    ui.planLabel = label(ui.body, "activityPlan", "secondary");
    ui.historyLabel = label(ui.body, "activityHistory", "secondary");
    ui.planLabel->hide();
    ui.historyLabel->hide();
    layout->addWidget(ui.planLabel);
    layout->addWidget(ui.historyLabel);
    ui.list = new QListWidget(ui.body);
    ui.list->setObjectName("sessionList");
    ui.list->setMinimumHeight(0);
    ui.list->setMaximumHeight(96);
    ui.list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.list->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui.list->hide();
    layout->addWidget(ui.list);

    ui.approvalSection = new QWidget(ui.body);
    ui.approvalSection->setObjectName("approvalSection");
    auto *approval = new QVBoxLayout(ui.approvalSection);
    approval->setContentsMargins(0, 3, 0, 3);
    approval->setSpacing(8);
    ui.approvalTitle = label(ui.approvalSection, "approvalTitle");
    ui.approvalDetail = activityText(ui.approvalSection, "approvalDetail", 150);
    ui.denyButton = button(ui.approvalSection, "denyButton");
    ui.allowButton = button(ui.approvalSection, "allowButton");
    auto *decisions = new QHBoxLayout;
    decisions->setSpacing(8);
    decisions->addStretch();
    decisions->addWidget(ui.denyButton);
    decisions->addWidget(ui.allowButton);
    approval->addWidget(ui.approvalTitle);
    approval->addWidget(ui.approvalDetail);
    approval->addLayout(decisions);
    ui.approvalSection->hide();
    layout->addWidget(ui.approvalSection);

    ui.usage = label(ui.body, "usageSummary", "secondary");
    layout->addWidget(ui.usage);
    auto *divider = new QFrame(ui.body);
    divider->setObjectName("activitySeparator");
    divider->setFixedHeight(1);
    layout->addWidget(divider);
    ui.connectButton = button(ui.body, "connectButton");
    ui.taskButton = button(ui.body, "taskButton");
    ui.stopButton = button(ui.body, "stopButton");
    ui.folderButton = button(ui.body, "folderButton");
    ui.quitButton = button(ui.body, "quitButton");
    auto *actions = new QHBoxLayout;
    actions->setSpacing(6);
    actions->addWidget(ui.connectButton);
    actions->addWidget(ui.taskButton);
    actions->addStretch(1);
    actions->addWidget(ui.stopButton);
    layout->addLayout(actions);
    auto *secondaryActions = new QHBoxLayout;
    secondaryActions->setSpacing(6);
    secondaryActions->addWidget(ui.folderButton);
    secondaryActions->addStretch(1);
    secondaryActions->addWidget(ui.quitButton);
    layout->addLayout(secondaryActions);
    return ui;
}
