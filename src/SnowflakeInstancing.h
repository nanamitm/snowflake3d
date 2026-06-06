#pragma once

#include <QQuick3DInstancing>

class CrystalModel;

// 2.5D 結晶: 各凍結セルを六角プリズムのインスタンスとして配置する。
// 位置=セル中心、z スケール=セルの厚み。毎フレーム refresh() で更新。
class SnowflakeInstancing : public QQuick3DInstancing {
    Q_OBJECT
public:
    explicit SnowflakeInstancing(QQuick3DInstancing *parent = nullptr);

    void setModel(const CrystalModel *m) { model_ = m; }
    void refresh() { markDirty(); }

    double cellSize = 1.0;
    double heightScale = 6.0;
    int colorMode = 0; // 0:Ice 1:Spectrum 2:Thickness

protected:
    QByteArray getInstanceBuffer(int *instanceCount) override;

private:
    const CrystalModel *model_ = nullptr;
};
