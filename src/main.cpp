#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "EnvProvider.h"
#include "Sim3DController.h"
#include "SimController.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Snowflake3D");
    QQuickStyle::setStyle("Basic"); // カスタマイズ可能なスタイル

    SimController sim;
    Sim3DController sim3d;

    QQmlApplicationEngine engine;
    engine.addImageProvider("env", new EnvProvider); // IBL 環境マップ(所有権は engine)
    engine.rootContext()->setContextProperty("sim", &sim);
    engine.rootContext()->setContextProperty("sim3d", &sim3d);
    engine.loadFromModule("Snowflake3D", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
