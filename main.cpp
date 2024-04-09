#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include "MyVideo.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#else
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QGuiApplication app(argc, argv);
    app.setOrganizationName("tttttt");
    qmlRegisterType<MyVideo>("SceneGraphRender", 1, 0, "MyVideoItem");
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
