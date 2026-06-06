#include "Voxel3DInstancing.h"

#include "core/Crystal3DModel.h"

#include <QColor>
#include <QVector3D>
#include <cmath>

Voxel3DInstancing::Voxel3DInstancing(QQuick3DInstancing *parent)
    : QQuick3DInstancing(parent) {}

QByteArray Voxel3DInstancing::getInstanceBuffer(int *instanceCount) {
    QByteArray buf;
    int count = 0;
    if (!model_) {
        if (instanceCount) *instanceCount = 0;
        return buf;
    }

    const int D = model_->diameter();
    const int c = model_->center();
    const int zc = model_->centerZ();
    const double size = cellSize;
    const double sqrt3 = std::sqrt(3.0);

    for (int z = 0; z < model_->layers(); ++z) {
        for (int ri = 0; ri < D; ++ri) {
            for (int qi = 0; qi < D; ++qi) {
                if (!model_->frozen(qi, ri, z)) continue;
                const int dq = qi - c, dr = ri - c;
                if (!model_->inHex(dq, dr)) continue;

                const float cx = static_cast<float>(size * sqrt3 * (dq + dr / 2.0));
                const float cy = static_cast<float>(size * 1.5 * dr);
                const float cz = static_cast<float>((z - zc) * layerH);

                const auto e = calculateTableEntry(
                    QVector3D(cx, cy, cz),
                    QVector3D(static_cast<float>(size), static_cast<float>(size),
                              static_cast<float>(layerH)),
                    QVector3D(0, 0, 0), Qt::white);
                buf.append(reinterpret_cast<const char *>(&e), sizeof(e));
                ++count;
            }
        }
    }

    if (instanceCount) *instanceCount = count;
    return buf;
}
