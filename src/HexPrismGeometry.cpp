#include "HexPrismGeometry.h"

#include <QByteArray>
#include <QVector3D>
#include <array>
#include <cmath>

namespace {
struct Vtx { float px, py, pz, nx, ny, nz; };

void pushTri(QByteArray &buf, const QVector3D &a, const QVector3D &b,
             const QVector3D &c) {
    const QVector3D n = QVector3D::crossProduct(b - a, c - a).normalized();
    const Vtx vs[3] = {{a.x(), a.y(), a.z(), n.x(), n.y(), n.z()},
                       {b.x(), b.y(), b.z(), n.x(), n.y(), n.z()},
                       {c.x(), c.y(), c.z(), n.x(), n.y(), n.z()}};
    buf.append(reinterpret_cast<const char *>(vs), sizeof(vs));
}
} // namespace

HexPrismGeometry::HexPrismGeometry(QQuick3DGeometry *parent)
    : QQuick3DGeometry(parent) {
    QByteArray buf;

    std::array<QVector3D, 6> top, bot;
    for (int i = 0; i < 6; ++i) {
        const double ang = M_PI / 180.0 * (60.0 * i - 30.0);
        const float x = static_cast<float>(std::cos(ang));
        const float y = static_cast<float>(std::sin(ang));
        top[i] = QVector3D(x, y, 0.5f);
        bot[i] = QVector3D(x, y, -0.5f);
    }
    const QVector3D ctop(0, 0, 0.5f), cbot(0, 0, -0.5f);

    for (int i = 0; i < 6; ++i) {
        const int j = (i + 1) % 6;
        pushTri(buf, ctop, top[i], top[j]);   // 上面
        pushTri(buf, cbot, bot[j], bot[i]);   // 下面
        pushTri(buf, top[i], bot[i], bot[j]); // 側面
        pushTri(buf, top[i], bot[j], top[j]);
    }

    setStride(sizeof(Vtx));
    setVertexData(buf);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 3 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);
    setBounds(QVector3D(-1, -1, -0.5f), QVector3D(1, 1, 0.5f));
}
