#include "client.h"
#include "storage/storagepaths.h"
#include "log/logger.h"
#include "qmlmodule.h"

#include "qtquick2applicationviewer.h"

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlFileSelector>
#include <QTimer>

#ifdef Q_OS_ANDROID
#include "statusbarhelper.h"
#elif !defined(Q_OS_IOS)
#include <QApplication>
#include "desktopstatusbarhelper.h"
#endif // Q_OS_ANDROID

#define HAVE_BREAKPAD // We always have breakpad at the moment
#ifdef HAVE_BREAKPAD
#include "crashhandler.h"
#endif

#ifdef Q_OS_IOS
#include <QtPlugin>
Q_IMPORT_PLUGIN(QtQuick2Plugin)
Q_IMPORT_PLUGIN(QtQuick2WindowPlugin)
Q_IMPORT_PLUGIN(QtQuickControlsPlugin)
#endif

LOGGER(main)

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationDomain("de.hsaugsburg.informatik");
    QCoreApplication::setOrganizationName("HS-Augsburg");
    QCoreApplication::setApplicationName("mPlaneClient");

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QGuiApplication app(argc, argv);
#else
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);
#endif

#ifdef HAVE_BREAKPAD
    QDir crashdumpDir = StoragePaths().crashDumpDirectory();
    if (!crashdumpDir.exists()) {
        if (!QDir::root().mkpath(crashdumpDir.absolutePath())) {
            LOG_ERROR(QString("Failed to create crashdump directory: %1").arg(crashdumpDir.absolutePath()));
        } else {
            LOG_INFO("Crashdump directory created");
        }
    }

    CrashHandler crashHandler;
    crashHandler.init(crashdumpDir.absolutePath());
#endif // HAVE_BREAKPAD

    QmlModule::registerTypes();

    QtQuick2ApplicationViewer view;

    QQmlEngine* engine = view.engine();
    QmlModule::initializeEngine(engine);

    // Allow QFileSelector to be automatically applied on qml scripting
    QQmlFileSelector selector;
    engine->setUrlInterceptor(&selector);

    QQmlContext* rootContext = view.rootContext();
    rootContext->setContextProperty("client", Client::instance());
#ifdef Q_OS_ANDROID
    rootContext->setContextProperty("statusBar", new StatusBarHelper(&view));
#elif defined(Q_OS_IOS)
    // TODO: iOS Code
#else
    rootContext->setContextProperty("statusBar", new DesktopStatusBarHelper(&view));
#endif // Q_OS_ANDROID

#ifdef Q_OS_IOS
    view.addImportPath(QStringLiteral("imports/qml"));
#endif

    view.setMainQmlFile(QStringLiteral("qml/main.qml"));
    view.showExpanded();

    int returnCode = app.exec();

    // Cleanly shutdown
    Client::instance()->deleteLater();
    QTimer::singleShot(1, &app, SLOT(quit()));
    app.exec();

    return returnCode;
}
