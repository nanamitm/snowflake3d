#include "SnowflakeGeometry.h"

#include "MeshBuilder.h"
#include "core/CrystalModel.h"

#include <QByteArray>
#include <QVector3D>
#include <algorithm>

namespace {
struct Vtx { float px, py, pz, nx, ny, nz; };
} // namespace

SnowflakeGeometry::SnowflakeGeometry(QQuick3DGeometry *parent)
    : QQuick3DGeometry(parent) {}

void SnowflakeGeometry::rebuild(const CrystalModel &model) {
    // インデックス化メッシュ。セル内で頂点を共有しつつ、面ごとに法線を分けて
    // 六角ファセットのエッジを保つ(上面/下面/各側面で別頂点)。
    QByteArray vbuf;
    QByteArray ibuf;
    quint32 vcount = 0;

    float minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
    auto track = [&](const QVector3D &p) {
        minX = std::min(minX, p.x()); maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y()); maxY = std::max(maxY, p.y());
        minZ = std::min(minZ, p.z()); maxZ = std::max(maxZ, p.z());
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

    const QVector3D up(0, 0, 1), down(0, 0, -1);

    forEachSnowflakeCell(model, cellSize, heightScale, [&](const SnowflakeCell &cl) {
        // 上面(法線 +z, 中心+6頂点を共有)
        const quint32 cT = addVert(cl.centerTop, up);
        quint32 tIdx[6];
        for (int i = 0; i < 6; ++i) tIdx[i] = addVert(cl.top[i], up);
        for (int i = 0; i < 6; ++i) addTri(cT, tIdx[i], tIdx[(i + 1) % 6]);

        // 下面(法線 -z, 巻き順反転)
        const quint32 cB = addVert(cl.centerBot, down);
        quint32 bIdx[6];
        for (int i = 0; i < 6; ++i) bIdx[i] = addVert(cl.bot[i], down);
        for (int i = 0; i < 6; ++i) addTri(cB, bIdx[(i + 1) % 6], bIdx[i]);

        // 側面(露出している辺のみ、面ごとに別頂点+面法線)
        for (int i = 0; i < 6; ++i) {
            if (!cl.sideVisible[i]) continue;
            const int j = (i + 1) % 6;
            const QVector3D n =
                QVector3D::crossProduct(cl.bot[i] - cl.top[i],
                                        cl.bot[j] - cl.top[i]).normalized();
            const quint32 v0 = addVert(cl.top[i], n);
            const quint32 v1 = addVert(cl.bot[i], n);
            const quint32 v2 = addVert(cl.bot[j], n);
            const quint32 v3 = addVert(cl.top[j], n);
            addTri(v0, v1, v2);
            addTri(v0, v2, v3);
        }
    });

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
    setBounds(QVector3D(minX, minY, minZ), QVector3D(maxX, maxY, maxZ));

    update();
}
