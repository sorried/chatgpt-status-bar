#pragma once

class QWidget;
class QLabel;
class QListWidget;
class QTextEdit;
class QPushButton;
class Mascot;

struct ActivityUi {
    QWidget *body;
    Mascot *avatar;
    QLabel *summary, *connection, *meta, *usage, *taskLabel, *stateLabel, *planLabel, *historyLabel;
    QListWidget *list;
    QTextEdit *detail;
    QPushButton *settingsButton, *connectButton, *taskButton, *stopButton, *folderButton, *quitButton,
        *closeButton;
    QLabel *approvalTitle;
    QTextEdit *approvalDetail;
    QPushButton *allowButton, *denyButton;
    QWidget *approvalSection;
};

ActivityUi createActivityUi(QWidget *parent);
