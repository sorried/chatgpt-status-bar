#include "mascot.h"
#include <QPainter>

static QColor stateColor(State state) {
    if (state == State::Error)
        return QColor("#ff8989");
    if (state == State::Waiting)
        return QColor("#ffcc70");
    if (state == State::Done)
        return QColor("#96dfb7");
    return QColor("#f2f2f2");
}
Mascot::Mascot(QWidget *parent) : QWidget(parent) {
    setMinimumSize(24, 24);
    setAccessibleName("ChatGPT");
    timer.setInterval(80);
    connect(&timer, &QTimer::timeout, this, [this] {
        ++frame;
        update();
    });
}
void Mascot::configure(State value, bool animate) {
    state = value;
    motion = animate;
    syncTimer();
    update();
}
void Mascot::syncTimer() {
    if (motion && isVisible() &&
        (state == State::Working || state == State::Thinking || state == State::Tool))
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
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const qreal side = qMin(width(), height());
    p.translate((width() - side) / 2, (height() - side) / 2);
    static const QPixmap logo(":/chatgpt.png");
    p.drawPixmap(QRectF(0, 0, side, side), logo, logo.rect());
    if (state == State::Idle)
        return;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#090909"));
    p.drawEllipse(QPointF(side * .84, side * .84), side * .14, side * .14);
    p.setPen(QPen(stateColor(state), qMax(1.5, side * .045), Qt::SolidLine, Qt::RoundCap));
    const QRectF indicator(side * .75, side * .75, side * .18, side * .18);
    if (state == State::Working || state == State::Thinking || state == State::Tool) {
        p.drawArc(indicator, (motion ? frame * -24 : 40) * 16, 250 * 16);
    } else if (state == State::Done) {
        p.drawLine(QPointF(side * .77, side * .84), QPointF(side * .82, side * .89));
        p.drawLine(QPointF(side * .82, side * .89), QPointF(side * .91, side * .78));
    } else {
        p.setBrush(stateColor(state));
        p.setPen(Qt::NoPen);
        p.drawEllipse(indicator);
    }
}
QIcon Mascot::icon(State) { return QIcon(":/chatgpt.png"); }
