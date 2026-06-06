#pragma once

#include <QVector3D>
#include <array>
#include <cmath>

#include "core/CrystalModel.h"

// 1 つの結晶セル(六角柱)の幾何データ。
struct SnowflakeCell {
    QVector3D centerTop, centerBot;       // 上面/下面の中心
    std::array<QVector3D, 6> top, bot;    // 上面/下面の 6 頂点(絶対座標)
    std::array<bool, 6> sideVisible;      // 辺 i の側面を描くか(隣接が未凍結)
};

// 凍結セルを六角柱として走査し、各セルを fn(const SnowflakeCell&) に渡す。
// メッシュ生成(インデックス化)と STL エクスポート(三角形スープ)で共用する。
template <class CellFn>
inline void forEachSnowflakeCell(const CrystalModel &m, double cellSize,
                                 double heightScale, CellFn &&fn) {
    const int D = m.diameter();
    const int c = m.center();
    const double size = cellSize;
    const double sqrt3 = std::sqrt(3.0);

    // pointy-top 六角形の 6 頂点(中心相対 xy)
    std::array<QVector3D, 6> corner;
    for (int i = 0; i < 6; ++i) {
        const double ang = M_PI / 180.0 * (60.0 * i - 30.0);
        corner[i] = QVector3D(static_cast<float>(size * std::cos(ang)),
                              static_cast<float>(size * std::sin(ang)), 0.0f);
    }

    // 辺 i (corner[i]→corner[i+1]) が面する隣接セルの軸オフセット。
    static constexpr int edgeNq[6] = {+1, 0, -1, -1, 0, +1};
    static constexpr int edgeNr[6] = {0, +1, +1, 0, -1, -1};

    SnowflakeCell cell;
    for (int ri = 0; ri < D; ++ri) {
        for (int qi = 0; qi < D; ++qi) {
            if (!m.frozen(qi, ri)) continue;
            const int dq = qi - c;
            const int dr = ri - c;
            if (!m.inHex(dq, dr)) continue;

            const float cx = static_cast<float>(size * sqrt3 * (dq + dr / 2.0));
            const float cy = static_cast<float>(size * 1.5 * dr);

            const double hf = m.heightAt(qi, ri); // 0..1
            const float h = static_cast<float>(heightScale * (0.35 + 0.65 * hf));
            const float zt = h * 0.5f, zb = -h * 0.5f;

            for (int i = 0; i < 6; ++i) {
                cell.top[i] = QVector3D(cx + corner[i].x(), cy + corner[i].y(), zt);
                cell.bot[i] = QVector3D(cx + corner[i].x(), cy + corner[i].y(), zb);
                cell.sideVisible[i] =
                    !m.frozen(qi + edgeNq[i], ri + edgeNr[i]);
            }
            cell.centerTop = QVector3D(cx, cy, zt);
            cell.centerBot = QVector3D(cx, cy, zb);
            fn(cell);
        }
    }
}

// 三角形スープ生成(STL エクスポート用)。
// sink(const QVector3D& a, const QVector3D& b, const QVector3D& c)
// 注意: 引数名に emit は使えない(Qt のマクロと衝突する)。
template <class Sink>
inline void buildSnowflakeMesh(const CrystalModel &m, double cellSize,
                               double heightScale, Sink &&sink) {
    forEachSnowflakeCell(m, cellSize, heightScale, [&](const SnowflakeCell &cl) {
        for (int i = 0; i < 6; ++i) {
            const int j = (i + 1) % 6;
            sink(cl.centerTop, cl.top[i], cl.top[j]);   // 上面
            sink(cl.centerBot, cl.bot[j], cl.bot[i]);   // 下面
            if (cl.sideVisible[i]) {
                sink(cl.top[i], cl.bot[i], cl.bot[j]);  // 側面
                sink(cl.top[i], cl.bot[j], cl.top[j]);
            }
        }
    });
}
