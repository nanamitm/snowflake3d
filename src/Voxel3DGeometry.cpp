#include "Voxel3DGeometry.h"

#include "core/Crystal3DModel.h"

#include <QByteArray>
#include <QVector3D>
#include <algorithm>
#include <array>
#include <cmath>

namespace {
struct Vtx { float px, py, pz, nx, ny, nz; };
} // namespace

Voxel3DGeometry::Voxel3DGeometry(QQuick3DGeometry *parent)
    : QQuick3DGeometry(parent) {}

void Voxel3DGeometry::rebuild(const Crystal3DModel &m) {
    QByteArray vbuf, ibuf;
    quint32 vcount = 0;

    float mnx = 0, mny = 0, mnz = 0, mxx = 0, mxy = 0, mxz = 0;
    auto track = [&](const QVector3D &p) {
        mnx = std::min(mnx, p.x()); mxx = std::max(mxx, p.x());
        mny = std::min(mny, p.y()); mxy = std::max(mxy, p.y());
        mnz = std::min(mnz, p.z()); mxz = std::max(mxz, p.z());
    };
    auto addVert = [&](const QVector3D &p, const QVector3D &n) -> quint32 {
        const Vtx v{p.x(), p.y(), p.z(), n.x(), n.y(), n.z()};
        vbuf.append(reinterpret_cast<const char *>(&v), sizeof(Vtx));
        track(p);
        return vcount++;
    };
    auto addTri = [&](quint32 a, quint32 b, quint32 c) {
        const quint32 idx[3] = {a, b, c};
        ibuf.append(reinterpret_cast<const char *>(idx), sizeof(idx));
    };

    const double size = cellSize;
    const double sqrt3 = std::sqrt(3.0);
    std::array<QVector3D, 6> corner;
    for (int i = 0; i < 6; ++i) {
        const double ang = M_PI / 180.0 * (60.0 * i - 30.0);
        corner[i] = QVector3D(static_cast<float>(size * std::cos(ang)),
                              static_cast<float>(size * std::sin(ang)), 0.0f);
    }
    // 辺 i が面する面内隣接の軸オフセット(2D と同じ)
    static constexpr int edgeNq[6] = {+1, 0, -1, -1, 0, +1};
    static constexpr int edgeNr[6] = {0, +1, +1, 0, -1, -1};

    const int D = m.diameter();
    const int c = m.center();
    const int zc = m.centerZ();
    const QVector3D up(0, 0, 1), down(0, 0, -1);

    for (int z = 0; z < m.layers(); ++z) {
        for (int ri = 0; ri < D; ++ri) {
            for (int qi = 0; qi < D; ++qi) {
                if (!m.frozen(qi, ri, z)) continue;
                const int dq = qi - c, dr = ri - c;
                if (!m.inHex(dq, dr)) continue;

                const float cx = static_cast<float>(size * sqrt3 * (dq + dr / 2.0));
                const float cy = static_cast<float>(size * 1.5 * dr);
                const float cz = static_cast<float>((z - zc) * layerH);
                const float zt = cz + float(layerH) * 0.5f;
                const float zb = cz - float(layerH) * 0.5f;

                std::array<QVector3D, 6> top, bot;
                for (int i = 0; i < 6; ++i) {
                    top[i] = QVector3D(cx + corner[i].x(), cy + corner[i].y(), zt);
                    bot[i] = QVector3D(cx + corner[i].x(), cy + corner[i].y(), zb);
                }

                // 上面(隣が空のときのみ)
                if (!m.frozen(qi, ri, z + 1)) {
                    const quint32 ct = addVert(QVector3D(cx, cy, zt), up);
                    quint32 ti[6];
                    for (int i = 0; i < 6; ++i) ti[i] = addVert(top[i], up);
                    for (int i = 0; i < 6; ++i) addTri(ct, ti[i], ti[(i + 1) % 6]);
                }
                // 下面
                if (!m.frozen(qi, ri, z - 1)) {
                    const quint32 cb = addVert(QVector3D(cx, cy, zb), down);
                    quint32 bi[6];
                    for (int i = 0; i < 6; ++i) bi[i] = addVert(bot[i], down);
                    for (int i = 0; i < 6; ++i) addTri(cb, bi[(i + 1) % 6], bi[i]);
                }
                // 側面(面内隣接が空の辺のみ)
                for (int i = 0; i < 6; ++i) {
                    if (m.frozen(qi + edgeNq[i], ri + edgeNr[i], z)) continue;
                    const int j = (i + 1) % 6;
                    const QVector3D n =
                        QVector3D::crossProduct(bot[i] - top[i],
                                                bot[j] - top[i]).normalized();
                    const quint32 v0 = addVert(top[i], n);
                    const quint32 v1 = addVert(bot[i], n);
                    const quint32 v2 = addVert(bot[j], n);
                    const quint32 v3 = addVert(top[j], n);
                    addTri(v0, v1, v2);
                    addTri(v0, v2, v3);
                }
            }
        }
    }

    clear();
    setStride(sizeof(Vtx));
    setVertexData(vbuf);
    setIndexData(ibuf);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::U32Type);
    setBounds(QVector3D(mnx, mny, mnz), QVector3D(mxx, mxy, mxz));

    update();
}
