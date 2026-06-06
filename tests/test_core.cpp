// Qt 非依存コアの簡易検証。両モデルが中心から六角対称に成長することを確認。
#include "GravnerGriffeathModel.h"
#include "ReiterModel.h"

#include <cmath>
#include <cstdio>

static void report(CrystalModel &m, const char *label) {
    std::printf("=== %s ===\n", label);
    auto stats = [&](long long step) {
        const int D = m.diameter();
        const int c = m.center();
        long long frozen = 0;
        int maxR = 0;
        for (int ri = 0; ri < D; ++ri)
            for (int qi = 0; qi < D; ++qi)
                if (m.frozen(qi, ri)) {
                    ++frozen;
                    const int dq = qi - c, dr = ri - c;
                    const int hd =
                        (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
                    if (hd > maxR) maxR = hd;
                }
        std::printf("  step %4lld : frozen=%-6lld maxRadius=%d\n", step, frozen,
                    maxR);
    };
    stats(0);
    for (int run = 1; run <= 5; ++run) {
        for (int i = 0; i < 100; ++i) m.step();
        stats(m.stepCount());
    }
    // 6 回対称チェック
    const int c = m.center();
    long long mism = 0, checked = 0;
    for (int dq = -100; dq <= 100; ++dq)
        for (int dr = -100; dr <= 100; ++dr) {
            const int hd = (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
            if (hd > 100) continue;
            const bool a = m.frozen(c + dq, c + dr);
            const bool b = m.frozen(c - dr, c + dq + dr); // 60°回転
            ++checked;
            if (a != b) ++mism;
        }
    std::printf("  6-fold symmetry: %lld/%lld mismatches\n\n", mism, checked);
}

int main() {
    ReiterModel r(120);
    r.applyPreset(0);
    r.reset();
    report(r, "Reiter (Dendrite)");

    GravnerGriffeathModel g(120);
    g.applyPreset(0);
    g.reset();
    report(g, "Gravner-Griffeath (Stellar dendrite)"); // 500 step まで進む

    // 側面カリングの削減効果(辺i→隣接の対応は MeshBuilder と同じ)
    static const int edgeNq[6] = {+1, 0, -1, -1, 0, +1};
    static const int edgeNr[6] = {0, +1, +1, 0, -1, -1};
    const int D = g.diameter();
    long long frozen = 0, triNaive = 0, triCulled = 0;
    long long vSoup = 0, vIndexed = 0;
    for (int ri = 0; ri < D; ++ri)
        for (int qi = 0; qi < D; ++qi) {
            if (!g.frozen(qi, ri)) continue;
            ++frozen;
            triNaive += 24; // 上6+下6+側12
            int sides = 0;
            for (int e = 0; e < 6; ++e)
                if (!g.frozen(qi + edgeNq[e], ri + edgeNr[e])) ++sides;
            const long long tri = 12 + 2 * sides; // カリング後の三角形
            triCulled += tri;
            vSoup += 3 * tri;            // 三角形スープ(頂点複製)
            vIndexed += 14 + 4 * sides;  // インデックス化(上7+下7+側4)
        }
    std::printf("=== Mesh reduction (GG @500) ===\n");
    std::printf("  frozen cells     : %lld\n", frozen);
    std::printf("  triangles naive  : %lld\n", triNaive);
    std::printf("  triangles culled : %lld  (%.1f%% reduction)\n", triCulled,
                100.0 * (triNaive - triCulled) / triNaive);
    std::printf("  vertices soup    : %lld\n", vSoup);
    std::printf("  vertices indexed : %lld  (%.1f%% reduction)\n", vIndexed,
                100.0 * (vSoup - vIndexed) / vSoup);
    return 0;
}
