#pragma once

#include <QObject>
#include <QQuick3DGeometry>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <memory>
#include <vector>

class CrystalModel;
class SnowflakeGeometry;

// シミュレーションを駆動し、QML へモデル/パラメータ/状態を公開する。
// 複数のモデル(Reiter / Gravner-Griffeath)を切り替えられる。
class SimController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuick3DGeometry *geometry READ geometryObject CONSTANT)
    Q_PROPERTY(QStringList modelNames READ modelNames CONSTANT)
    Q_PROPERTY(int modelIndex READ modelIndex WRITE setModelIndex NOTIFY modelChanged)
    Q_PROPERTY(QVariantList params READ params NOTIFY modelChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY modelChanged)
    Q_PROPERTY(double thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
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
    explicit SimController(QObject *parent = nullptr);
    ~SimController() override;

    QQuick3DGeometry *geometryObject() const;
    QStringList modelNames() const;
    int modelIndex() const { return modelIndex_; }
    QVariantList params() const;
    QStringList presetNames() const;
    double thickness() const;
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
    void setThickness(double v);
    void setSpeed(int stepsPerTick);
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
    bool exportStl(const QUrl &fileUrl);
    bool saveConfig(const QUrl &fileUrl);
    bool loadConfig(const QUrl &fileUrl);

signals:
    void modelChanged();
    void runningChanged();
    void stepped();
    void speedChanged();
    void autoExpandChanged();
    void seedChanged();
    void thicknessChanged();

private:
    void advance();
    void refreshMesh();
    CrystalModel *model() const;

    std::vector<std::unique_ptr<CrystalModel>> models_;
    int modelIndex_ = 0;
    std::unique_ptr<SnowflakeGeometry> geometry_;
    QTimer timer_;
    int speed_ = 4;
    bool autoExpand_ = true;
    int capRadius_ = 360;   // 自動拡張の上限半径
    int expandStep_ = 60;   // 1 回の拡張量
};
