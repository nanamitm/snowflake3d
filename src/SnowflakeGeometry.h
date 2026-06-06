#pragma once

#include <QQuick3DGeometry>

class CrystalModel;

// 結晶セル(凍結セル)を六角柱としてメッシュ化する QQuick3DGeometry。
// 各セルの水量 s を柱の高さ(厚み)に写像し、薄い六角板状の 2.5D 形状を作る。
// 上下対称(z = -h/2 .. +h/2)に押し出すので、横/斜めから板の厚みが見える。
class SnowflakeGeometry : public QQuick3DGeometry {
    Q_OBJECT
public:
    explicit SnowflakeGeometry(QQuick3DGeometry *parent = nullptr);

    // モデルの現状からメッシュ全体を再構築する。
    void rebuild(const CrystalModel &model);

    // 厚みの誇張係数(横から見やすくするため、実物より厚めにできる)
    double heightScale = 6.0;
    double cellSize    = 1.0;   // 六角セルの外接半径
};
