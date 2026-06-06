#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

#include "EnvProvider.h"
#include "Sim3DController.h"
#include "SimController.h"

static QString argVal(const QStringList &a, const QString &key,
                      const QString &def) {
    const int i = a.indexOf(key);
    return (i >= 0 && i + 1 < a.size()) ? a[i + 1] : def;
}

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

    // --- スクリーンショット用キャプチャモード(README/docs 生成) ---
    //   --shot <out.png> [--mode 2d|3d] [--model N] [--preset N]
    //                    [--steps K] [--tilt deg] [--cam dist]
    const QStringList args = app.arguments();
    const int shotIdx = args.indexOf("--shot");
    if (shotIdx >= 0 && shotIdx + 1 < args.size()) {
        const QString out = args[shotIdx + 1];
        const QString mode = argVal(args, "--mode", "2d");
        const int model = argVal(args, "--model", "1").toInt();
        const int preset = argVal(args, "--preset", "2").toInt();
        const int steps = argVal(args, "--steps", "600").toInt();
        const qreal tilt = argVal(args, "--tilt", "0").toDouble();
        const qreal cam = argVal(args, "--cam", "650").toDouble();

        QObject *root = engine.rootObjects().first();
        QQuickWindow *w = qobject_cast<QQuickWindow *>(root);

        QTimer::singleShot(700, [=, &sim, &sim3d, &app]() {
            root->setProperty("hidePanel", true);
            root->setProperty("sceneTilt", tilt);
            root->setProperty("camDistance", cam);
            if (mode == "3d") {
                root->setProperty("mode3d", true);
                sim.stop();
                sim3d.setModelIndex(model);
                sim3d.applyPreset(preset);
                for (int i = 0; i < steps; ++i) sim3d.stepOnce();
            } else {
                root->setProperty("mode3d", false);
                sim3d.stop();
                sim.setModelIndex(model);
                sim.applyPreset(preset);
                for (int i = 0; i < steps; ++i) sim.stepOnce();
            }
            // 数フレーム描画させてからグラブ
            QTimer::singleShot(400, [=, &app]() {
                const QImage img = w->grabWindow();
                if (!img.isNull() && img.save(out))
                    qInfo("saved %s", qPrintable(out));
                else
                    qWarning("failed to save %s", qPrintable(out));
                app.quit();
            });
        });
    }

    return app.exec();
}
