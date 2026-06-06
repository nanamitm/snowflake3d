#pragma once

#include <QObject>
#include <QQuick3DGeometry>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <memory>
#include <vector>

class Crystal3DModel;
class Voxel3DGeometry;

// 3D 結晶(Reiter3D / GG3D)を駆動し QML へ公開する。
class Sim3DController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuick3DGeometry *geometry READ geometryObject CONSTANT)
    Q_PROPERTY(QStringList modelNames READ modelNames CONSTANT)
    Q_PROPERTY(int modelIndex READ modelIndex WRITE setModelIndex NOTIFY modelChanged)
    Q_PROPERTY(QVariantList params READ params NOTIFY modelChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY modelChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int stepCount READ stepCount NOTIFY stepped)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)

public:
    explicit Sim3DController(QObject *parent = nullptr);
    ~Sim3DController() override;

    QQuick3DGeometry *geometryObject() const;
    QStringList modelNames() const;
    int modelIndex() const { return modelIndex_; }
    QVariantList params() const;
    QStringList presetNames() const;
    bool running() const { return timer_.isActive(); }
    int stepCount() const;
    int speed() const { return speed_; }

    void setModelIndex(int i);
    void setSpeed(int v);

public slots:
    void start();
    void stop();
    void stepOnce();
    void reset();
    void setParam(int index, double v);
    void applyPreset(int index);

signals:
    void modelChanged();
    void runningChanged();
    void stepped();
    void speedChanged();

private:
    void advance();
    void refreshMesh();
    Crystal3DModel *model() const;

    std::vector<std::unique_ptr<Crystal3DModel>> models_;
    int modelIndex_ = 0;
    std::unique_ptr<Voxel3DGeometry> geometry_;
    QTimer timer_;
    int speed_ = 2;
};
