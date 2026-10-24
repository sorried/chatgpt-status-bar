#include "mascot.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

static QColor stateColor(State state) {
    if (state == State::Error)
        return QColor("#fb8b8b");
    if (state == State::Waiting)
        return QColor("#ffd37a");
    if (state == State::Done)
        return QColor("#8ee3bb");
    if (state == State::Unknown)
        return QColor("#b6bdc8");
    return QColor("#9ee8d2");
}
Mascot::Mascot(QWidget *parent) : QWidget(parent) {
    setMinimumSize(48, 48);
    setAccessibleName("GPT companion");
    timer.setInterval(80);
    connect(&timer, &QTimer::timeout, this, [this] {
        ++frame;
        update();
    });
}
void Mascot::configure(State value, const QString &style, bool animate) {
    state = value;
    skin = style;
    motion = animate;
    syncTimer();
    update();
}
void Mascot::syncTimer() {
    if (motion && isVisible())
        timer.start();
    else
        timer.stop();
}
void Mascot::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    syncTimer();
}
void Mascot::hideEvent(QHideEvent *e) {
    QWidget::hideEvent(e);
    syncTimer();
}
void Mascot::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal side = qMin(width(), height());
    p.translate((width() - side) / 2, (height() - side) / 2);
    p.scale(side / 80, side / 80);
    const qreal phase = motion ? frame * 0.18 : 0;
    const qreal bob = motion ? std::sin(phase) * (state == State::Done ? 4 : 1.4) : 0;
    p.translate(0, bob);
    const auto color = stateColor(state);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#182b2a"));
    p.drawEllipse(QRectF(11, 62 - bob, 58, 7));
    p.setBrush(color);
    if (skin == "comet") {
        p.drawEllipse(QRectF(12, 13, 56, 51));
        p.setBrush(color.darker(130));
        p.drawEllipse(QRectF(6, 36, 12, 16));
        p.drawEllipse(QRectF(62, 36, 12, 16));
    } else if (skin == "pixel") {
        p.setRenderHint(QPainter::Antialiasing, false);
        p.drawRect(QRectF(16, 17, 48, 42));
        p.drawRect(QRectF(9, 30, 7, 19));
        p.drawRect(QRectF(64, 30, 7, 19));
        p.drawRect(QRectF(25, 59, 9, 7));
        p.drawRect(QRectF(46, 59, 9, 7));
        p.drawRect(QRectF(37, 7, 6, 10));
    } else {
        p.drawRoundedRect(QRectF(13, 18, 54, 43), 15, 15);
        p.drawRoundedRect(QRectF(5, 31, 10, 18), 5, 5);
        p.drawRoundedRect(QRectF(65, 31, 10, 18), 5, 5);
        p.setPen(QPen(color, 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(40, 18), QPointF(40, 9));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(40, 7), 4, 4);
    }
    p.setBrush(QColor("#14312e"));
    p.drawRoundedRect(QRectF(21, 27, 38, 23), skin == "pixel" ? 0 : 9, skin == "pixel" ? 0 : 9);
    p.setPen(QPen(color, 3, Qt::SolidLine, Qt::RoundCap));
    const bool blink = motion && frame % 65 > 61;
    if (state == State::Idle || state == State::Unknown || blink) {
        p.drawLine(QPointF(28, 39), QPointF(33, 39));
        p.drawLine(QPointF(47, 39), QPointF(52, 39));
    } else if (state == State::Error) {
        for (int x : {29, 49}) {
            p.drawLine(QPointF(x - 2, 35), QPointF(x + 2, 41));
            p.drawLine(QPointF(x + 2, 35), QPointF(x - 2, 41));
        }
    } else if (state == State::Done) {
        p.drawArc(QRectF(27, 34, 8, 8), 0, 180 * 16);
        p.drawArc(QRectF(45, 34, 8, 8), 0, 180 * 16);
    } else {
        const int look = state == State::Thinking ? 2 : 0;
        p.drawLine(QPointF(30 + look, 35), QPointF(30 + look, 41));
        p.drawLine(QPointF(49 + look, 35), QPointF(49 + look, 41));
    }
    if (state == State::Working || state == State::Tool) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#45635e"));
        p.drawRoundedRect(QRectF(17, 54, 46, 14), 3, 3);
        p.setPen(QPen(color, 2));
        p.drawLine(QPointF(26, 59), QPointF(35 + std::sin(phase) * 3, 59));
        p.drawLine(QPointF(43, 63), QPointF(54, 63));
    } else if (state == State::Waiting) {
        p.setPen(QPen(color, 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(70, 24), QPointF(74, 17 + std::sin(phase) * 3));
    } else if (state == State::Thinking) {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        for (int i = 0; i < 3; ++i)
            p.drawEllipse(QPointF(58 + i * 6, 12 - std::sin(phase + i) * 2), 2, 2);
    }
}
QIcon Mascot::icon(State state) {
    QPixmap image(64, 64);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(stateColor(state));
    p.drawRoundedRect(QRectF(4, 10, 56, 46), 16, 16);
    p.setBrush(QColor("#14312e"));
    p.drawRoundedRect(QRectF(13, 22, 38, 23), 8, 8);
    p.setPen(QPen(stateColor(state), 4, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(24, 30, 24, 37);
    p.drawLine(41, 30, 41, 37);
    return QIcon(image);
}
