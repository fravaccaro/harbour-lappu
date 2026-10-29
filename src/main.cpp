#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QGuiApplication>
#include <QLocale>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QScopedPointer>
#include <QTranslator>

#include <sailfishapp.h>

#include "muistio/config.h"
#include "muistio/notesmodel.h"
#include "muistio/session.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QStringLiteral("org.frapps"));
    app->setOrganizationDomain(QStringLiteral("frapps.org"));
    app->setApplicationName(QStringLiteral("harbour-muistio"));
    app->setApplicationVersion(QStringLiteral(APP_VERSION));

    QScopedPointer<QTranslator> translator(new QTranslator);
    if (translator->load(QLocale(), QStringLiteral("harbour-muistio"), QStringLiteral("-"),
                         SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile())) {
        app->installTranslator(translator.data());
    }

    Config config;
    NotesModel notes;
    Session session(&config, &notes);

    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // The Opal modules are shipped with the application rather than installed
    // system-wide, so the engine has to be told where to look for them.
    view->engine()->addImportPath(SailfishApp::pathTo(QStringLiteral("qml/modules")).toString());

    QQmlContext *context = view->rootContext();
    context->setContextProperty(QStringLiteral("Muistio"), &session);
    context->setContextProperty(QStringLiteral("Notes"), &notes);

    view->setSource(SailfishApp::pathTo(QStringLiteral("qml/harbour-muistio.qml")));
    view->show();
    return app->exec();
}
