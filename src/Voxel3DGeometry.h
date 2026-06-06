#pragma once

#include <QQuick3DGeometry>

class Crystal3DModel;

// 3D 結晶(付着セル)を六角プリズムのボクセルとして描く QQuick3DGeometry。
// 露出面(隣が空)だけを描く面カリング + インデックス化でファセットを保つ。
class Voxel3DGeometry : public QQuick3DGeometry {
    Q_OBJECT
public:
    explicit Voxel3DGeometry(QQuick3DGeometry *parent = nullptr);

    void rebuild(const Crystal3DModel &model);

    double cellSize = 1.0;   // 六角の外接半径
    double layerH = 1.5;     // c 軸方向のセル高さ
};
