#pragma once
#include "codex.h"
#include "mascot.h"
#include "watcher.h"
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTextEdit>

class Companion : public QWidget {
    Q_OBJECT
  public:
    explicit Companion(QWidget *parent = nullptr);
    Mascot *mascot;
    QLabel *label;
    QLabel *status;
    QWidget *card;
    QLabel *cardTitle, *cardTask, *cardDetail;
    bool hoverEnabled = true;
    void showDetails();
    void place(int screen, const QString &position);
  signals:
    void activated();

  protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    bool eventFilter(QObject *object, QEvent *event) override;
};
class App : public QMainWindow {
    Q_OBJECT
  public:
    explicit App(bool preview = false);
    ~App() override;
    void capture(const QString &directory);
    void shutdown();
    void openSettings();

  protected:
    void closeEvent(QCloseEvent *) override;

  private:
    QSettings settings;
    SessionStore store;
    SessionWatcher watcher;
    CodexClient client;
    QSystemTrayIcon tray;
    Companion companion;
    QTimer clock, limitTimer;
    QListWidget *list;
    QLabel *summary, *connection, *meta, *usage;
    QTextEdit *detail;
    QPushButton *connectButton, *taskButton, *stopButton, *folderButton;
    Mascot *avatar;
    QJsonObject rateLimits;
    QList<QJsonObject> requests;
    bool preview, booting = true, handlingRequest = false, quitting = false;
    QString errorText;
    QString l(const char *en, const char *ru) const;
    QString home() const;
    QString stateName(State state) const;
    QString selected() const;
    void refresh();
    void showWindow();
    void requestQuit();
    void applySettings();
    QDialog *settingsDialog();
    void newTask();
    void handleRequests();
    void notify(const QString &id, State before, State after);
    bool setAutostart(bool enabled, QString *error);
    bool hasActiveTasks() const;
};
