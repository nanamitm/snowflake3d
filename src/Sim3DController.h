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
    Q_PROPERTY(int growthPercent READ growthPercent NOTIFY stepped)
    Q_PROPERTY(bool atBoundary READ atBoundary NOTIFY stepped)
    Q_PROPERTY(bool autoExpand READ autoExpand WRITE setAutoExpand NOTIFY autoExpandChanged)
    Q_PROPERTY(int seedType READ seedType WRITE setSeedType NOTIFY seedChanged)
    Q_PROPERTY(int seedSize READ seedSize WRITE setSeedSize NOTIFY seedChanged)
    Q_PROPERTY(QStringList seedNames READ seedNames CONSTANT)

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
    int growthPercent() const;
    bool atBoundary() const;
    bool autoExpand() const { return autoExpand_; }
    int seedType() const;
    int seedSize() const;
    QStringList seedNames() const { return {"Point", "Hexagon", "Ring", "Star (6)"}; }

    void setModelIndex(int i);
    void setSpeed(int v);
    void setAutoExpand(bool v);
    void setSeedType(int v);
    void setSeedSize(int v);

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
    void autoExpandChanged();
    void seedChanged();

private:
    void advance();
    void refreshMesh();
    Crystal3DModel *model() const;

    std::vector<std::unique_ptr<Crystal3DModel>> models_;
    int modelIndex_ = 0;
    std::unique_ptr<Voxel3DGeometry> geometry_;
    QTimer timer_;
    int speed_ = 2;
    bool autoExpand_ = true;
    int capRadius_ = 100;   // 3D は重いので控えめな上限
    int expandStep_ = 16;
};
