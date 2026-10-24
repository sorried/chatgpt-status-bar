#include "app.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTemporaryDir>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("sorried");
    QCoreApplication::setApplicationName("gpt-status-bar");
    QCoreApplication::setApplicationVersion("0.2.0");
    app.setQuitOnLastWindowClosed(false);
    QCommandLineParser parser;
    parser.setApplicationDescription("Native desktop companion for OpenAI Codex");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"background", "Start with the monitor window hidden"});
    parser.addOption({"preview", "Show a clearly labelled sample session without connecting to Codex"});
    parser.addOption({"capture", "Save preview screenshots and exit", "directory"});
    parser.addOption({"smoke-test", "Start the application and exit after one second"});
    parser.addOption({"language", "Preview language: en or ru", "language", "en"});
    parser.addOption({"settings", "Open settings on startup"});
    parser.addOption({"theme", "Preview theme: light or dark", "theme", "dark"});
    parser.process(app);
    const bool preview = parser.isSet("preview") || parser.isSet("capture");
    QTemporaryDir previewSettings;
    if (preview) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, previewSettings.path());
        QSettings settings;
        settings.setValue("language", parser.value("language") == "ru" ? "ru" : "en");
        settings.setValue("theme", parser.value("theme") == "light" ? "light" : "dark");
        settings.setValue("motion", false);
        settings.setValue("codexHome", QDir::cleanPath(QDir::home().filePath("../demo/.codex")));
    }
    const auto runtime = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(runtime);
    QLockFile lock(runtime + "/instance.lock");
    if (!preview && !parser.isSet("smoke-test") && !lock.tryLock(100)) {
        QMessageBox::information(nullptr, "GPT status bar",
                                 "Already running. Open it from the tray or floating companion.");
        return 0;
    }
    App window(preview);
    if (!parser.isSet("background") || preview)
        window.show();
    if (parser.isSet("settings"))
        QTimer::singleShot(0, &window, [&] { window.openSettings(); });
    if (parser.isSet("capture"))
        QTimer::singleShot(100, &app, [&] {
            window.capture(parser.value("capture"));
            window.shutdown();
        });
    if (parser.isSet("smoke-test"))
        QTimer::singleShot(1000, &app, [&] { window.shutdown(); });
    return app.exec();
}
