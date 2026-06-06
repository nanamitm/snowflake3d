#pragma once

#include <QQuick3DInstancing>

class Crystal3DModel;

// 完全 3D 結晶: 各付着セルを六角プリズムのインスタンスとして配置する。
class Voxel3DInstancing : public QQuick3DInstancing {
    Q_OBJECT
public:
    explicit Voxel3DInstancing(QQuick3DInstancing *parent = nullptr);

    void setModel(const Crystal3DModel *m) { model_ = m; }
    void refresh() { markDirty(); }

    // 2D の見かけと揃えるため拡大率を内包(旧 Model scale 3 相当)
    double cellSize = 3.0;
    double layerH = 4.5;
    int colorMode = 0; // 0:Ice 1:Spectrum 2:Thickness(層)

protected:
    QByteArray getInstanceBuffer(int *instanceCount) override;

private:
    const Crystal3DModel *model_ = nullptr;
};
