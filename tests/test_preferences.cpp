#include "app.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QtTest>

class PreferencesTests : public QObject {
    Q_OBJECT
    QTemporaryDir directory;

    QDialog *open(App &window) {
        window.show();
        window.openSettings();
        QApplication::processEvents();
        return window.findChild<QDialog *>("preferencesDialog");
    }

  private slots:
    void initTestCase() {
        QVERIFY(directory.isValid());
        QCoreApplication::setOrganizationName("status-bar-tests");
        QCoreApplication::setApplicationName("preferences");
        QCoreApplication::setApplicationVersion("0.3.0");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        qApp->setQuitOnLastWindowClosed(false);
    }
    void init() {
        QSettings settings;
        settings.clear();
        settings.setValue("codexHome", directory.filePath("codex"));
        settings.setValue("language", "en");
        settings.setValue("theme", "dark");
        settings.setValue("motion", false);
        settings.sync();
    }
    void activityIsCompactAndApprovalsAreInline() {
        App window(true);
        window.show();
        QApplication::processEvents();
        QVERIFY(window.width() <= 480);
        QVERIFY(window.height() < 700);
        auto *client = window.findChild<CodexClient *>("codexClient");
        QVERIFY(client);
        client->receive({{"method", "item/commandExecution/requestApproval"},
                         {"id", "approval-test"},
                         {"params", QJsonObject{{"threadId", "example"},
                                                {"turnId", "t"},
                                                {"command", "echo safe"},
                                                {"reason", "test request"}}}});
        auto *approval = window.findChild<QWidget *>("approvalSection");
        QTRY_VERIFY(approval->isVisible());
        QVERIFY(window.findChild<QTextEdit *>("approvalDetail")->toPlainText().contains("echo safe"));
        QVERIFY(window.findChild<QPushButton *>("denyButton"));
        QVERIFY(!window.findChild<QPushButton *>("allowButton")->isEnabled());
        client->receive({{"method", "serverRequest/resolved"},
                         {"params", QJsonObject{{"threadId", "example"}, {"requestId", "approval-test"}}}});
        QTRY_VERIFY(!approval->isVisible());
        QTest::mouseClick(window.findChild<QPushButton *>("closeCardButton"), Qt::LeftButton);
        QVERIFY(!window.isVisible());
    }
    void navigationAndLayout_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("theme");
        QTest::newRow("English dark") << QString("en") << QString("dark");
        QTest::newRow("English light") << QString("en") << QString("light");
        QTest::newRow("Russian dark") << QString("ru") << QString("dark");
        QTest::newRow("Russian light") << QString("ru") << QString("light");
    }
    void navigationAndLayout() {
        QFETCH(QString, language);
        QFETCH(QString, theme);
        QSettings settings;
        settings.setValue("language", language);
        settings.setValue("theme", theme);
        App window(true);
        auto *dialog = open(window);
        QVERIFY(dialog);
        dialog->resize(dialog->minimumSize());
        auto *navigation = dialog->findChild<QListWidget *>("navigation");
        auto *pages = dialog->findChild<QStackedWidget *>("settingsPages");
        QVERIFY(navigation);
        QVERIFY(pages);
        QCOMPARE(navigation->count(), 3);
        for (int i = 0; i < navigation->count(); ++i) {
            navigation->setCurrentRow(i);
            QApplication::processEvents();
            QCOMPARE(pages->currentIndex(), i);
            QVERIFY(pages->currentWidget()->isVisible());
        }
        for (const auto *name : {"saveButton", "cancelButton"}) {
            auto *button = dialog->findChild<QPushButton *>(name);
            QVERIFY(button);
            QVERIFY(button->isVisible());
            QVERIFY(dialog->rect().contains(QRect(button->mapTo(dialog, QPoint()), button->size())));
        }
        dialog->reject();
    }
    void cancelDoesNotPersist() {
        App window(true);
        QPointer<QDialog> dialog = open(window);
        QVERIFY(dialog);
        dialog->findChild<QComboBox *>("themeControl")->setCurrentIndex(2);
        dialog->findChild<QSpinBox *>("pollControl")->setValue(17);
        dialog->findChild<QCheckBox *>("notificationsControl")->setChecked(false);
        QTest::mouseClick(dialog->findChild<QPushButton *>("cancelButton"), Qt::LeftButton);
        QTRY_VERIFY(dialog.isNull());
        QSettings settings;
        QCOMPARE(settings.value("theme").toString(), "dark");
        QCOMPARE(settings.value("pollSeconds", 3).toInt(), 3);
        QCOMPARE(settings.value("notifications", true).toBool(), true);
    }
    void savePersistsAndReloads() {
        App window(true);
        QPointer<QDialog> dialog = open(window);
        QVERIFY(dialog);
        auto *language = dialog->findChild<QComboBox *>("languageControl");
        language->setCurrentIndex(language->findData("ru"));
        auto *theme = dialog->findChild<QComboBox *>("themeControl");
        theme->setCurrentIndex(theme->findData("light"));
        dialog->findChild<QSpinBox *>("pollControl")->setValue(12);
        dialog->findChild<QCheckBox *>("notificationsControl")->setChecked(false);
        dialog->findChild<QCheckBox *>("transparencyControl")->setChecked(true);
        QTest::mouseClick(dialog->findChild<QPushButton *>("saveButton"), Qt::LeftButton);
        QTRY_VERIFY(dialog.isNull());
        QSettings settings;
        QCOMPARE(settings.value("language").toString(), "ru");
        QCOMPARE(settings.value("theme").toString(), "light");
        QCOMPARE(settings.value("pollSeconds").toInt(), 12);
        QVERIFY(settings.value("reduceTransparency").toBool());
        QVERIFY(!settings.value("notifications").toBool());
        dialog = open(window);
        QVERIFY(dialog);
        QCOMPARE(dialog->findChild<QComboBox *>("languageControl")->currentData().toString(), "ru");
        QCOMPARE(dialog->findChild<QComboBox *>("themeControl")->currentData().toString(), "light");
        QVERIFY(dialog->findChild<QCheckBox *>("transparencyControl")->isChecked());
        dialog->reject();
    }
    void switchesSupportKeyboard() {
        App window(true);
        auto *dialog = open(window);
        QVERIFY(dialog);
        auto *notifications = dialog->findChild<QCheckBox *>("notificationsControl");
        QVERIFY(notifications);
        QVERIFY(notifications->isChecked());
        notifications->setFocus();
        QTest::keyClick(notifications, Qt::Key_Space);
        QVERIFY(!notifications->isChecked());
        QVERIFY(!dialog->findChild<QCheckBox *>("soundControl")->isEnabled());
        QTest::keyClick(notifications, Qt::Key_Space);
        QVERIFY(notifications->isChecked());
        QVERIFY(dialog->findChild<QCheckBox *>("soundControl")->isEnabled());
        dialog->reject();
    }
    void previewIsReversible() {
        App window(true);
        auto *dialog = open(window);
        QVERIFY(dialog);
        auto *theme = dialog->findChild<QComboBox *>("themeControl");
        theme->setCurrentIndex(theme->findData("light"));
        QCOMPARE(dialog->property("glassDark").toBool(), false);
        dialog->findChild<QCheckBox *>("transparencyControl")->setChecked(true);
        QCOMPARE(dialog->property("reducedTransparency").toBool(), true);
        dialog->reject();
        QCOMPARE(QSettings().value("theme").toString(), "dark");
        QVERIFY(!QSettings().value("reduceTransparency", false).toBool());
    }
    void invalidHomeCannotBeSaved() {
        App window(true);
        auto *dialog = open(window);
        QVERIFY(dialog);
        dialog->findChild<QLineEdit *>("homeControl")->setText("relative/path");
        QTimer::singleShot(0, [] {
            for (auto *widget : QApplication::topLevelWidgets())
                if (auto *message = qobject_cast<QMessageBox *>(widget))
                    message->accept();
        });
        QTest::mouseClick(dialog->findChild<QPushButton *>("saveButton"), Qt::LeftButton);
        QVERIFY(dialog->isVisible());
        QCOMPARE(dialog->findChild<QListWidget *>("navigation")->currentRow(), 2);
        QCOMPARE(QSettings().value("codexHome").toString(), directory.filePath("codex"));
        dialog->reject();
    }
};
QTEST_MAIN(PreferencesTests)
#include "test_preferences.moc"
