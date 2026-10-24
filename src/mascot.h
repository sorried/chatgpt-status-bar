#pragma once
#include "session.h"
#include <QIcon>
#include <QTimer>
#include <QWidget>

class Mascot : public QWidget {
    Q_OBJECT
  public:
    explicit Mascot(QWidget *parent = nullptr);
    void configure(State state, bool motion);
    static QIcon icon(State state = State::Idle);

  protected:
    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;

  private:
    State state = State::Idle;
    bool motion = true;
    int frame = 0;
    QTimer timer;
    void syncTimer();
};
