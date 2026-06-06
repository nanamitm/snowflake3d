#pragma once

#include <QQuick3DGeometry>

// インスタンシングの基本メッシュ: 単位六角プリズム。
// pointy-top 六角(外接半径 1)、高さ 1(z = -0.5..0.5)。一度だけ構築する。
class HexPrismGeometry : public QQuick3DGeometry {
    Q_OBJECT
public:
    explicit HexPrismGeometry(QQuick3DGeometry *parent = nullptr);
};
