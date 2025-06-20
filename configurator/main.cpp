#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include "ConfiguratorController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<ConfiguratorController>("Config", 1, 0, "ConfiguratorController");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/ConfiguratorMain.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
} 
