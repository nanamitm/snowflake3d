#include "GravnerGriffeath3DModel.h"

#include <algorithm>
#include <cmath>

GravnerGriffeath3DModel::GravnerGriffeath3DModel(int radius, int layers)
    : HexPrismLattice(radius, layers) {
    const size_t n = static_cast<size_t>(D_) * D_ * Hz_;
    a_.assign(n, 0);
    b_.assign(n, 0.0);
    cm_.assign(n, 0.0);
    d_.assign(n, 0.0);
    dn_.assign(n, 0.0);
    boundary_.assign(n, 0);
    attachMask_.assign(n, 0);
    reset();
}

void GravnerGriffeath3DModel::reset() {
    std::fill(a_.begin(), a_.end(), 0);
    std::fill(b_.begin(), b_.end(), 0.0);
    std::fill(cm_.begin(), cm_.end(), 0.0);
    std::fill(d_.begin(), d_.end(), rho);
    forEachSeed([&](int dq, int dr) {
        const int o = index(c_ + dq, c_ + dr, zc_);
        a_[o] = 1; cm_[o] = 1.0; d_[o] = 0.0;
    });
    steps_ = 0;
    radiusCacheStep_ = -1;
}

void GravnerGriffeath3DModel::grow(int newRadius) {
    const int oldD = D_, oldC = c_;
    R_ = newRadius; D_ = 2 * (R_ + 1) + 1; c_ = R_ + 1;
    buildCanonMap();
    a_ = regrid(a_, oldD, oldC, static_cast<char>(0));
    b_ = regrid(b_, oldD, oldC, 0.0);
    cm_ = regrid(cm_, oldD, oldC, 0.0);
    d_ = regrid(d_, oldD, oldC, rho);
    const size_t n = static_cast<size_t>(D_) * D_ * Hz_;
    dn_.assign(n, 0.0);
    boundary_.assign(n, 0);
    attachMask_.assign(n, 0);
    radiusCacheStep_ = -1;
}

void GravnerGriffeath3DModel::step() {
    const double wv = 0.15;           // 垂直拡散重み(小さめ→枝分かれ促進)
    const double norm = 1.0 + 6.0 + 2.0 * wv;
    const double halfT = thickness * 0.5; // 垂直核形成を許す中心からの層数

    // ===== (i) 拡散(反射境界、垂直は重み wv) =====
#pragma omp parallel for schedule(static)
    for (int z = 0; z < Hz_; ++z)
        for (int ri = 1; ri < D_ - 1; ++ri)
            for (int qi = 1; qi < D_ - 1; ++qi) {
                const int idx = index(qi, ri, z);
                if (!inHex(qi - c_, ri - c_)) { dn_[idx] = rho; continue; }
                if (a_[idx]) { dn_[idx] = 0.0; continue; }
                double sum = d_[idx];
                for (int k = 0; k < 6; ++k) {
                    const int nidx = index(qi + dqN_[k], ri + drN_[k], z);
                    sum += a_[nidx] ? d_[idx] : d_[nidx];
                }
                // 上下(範囲外は rho リザーバ、付着は反射)
                const bool upF = frozenAt(qi, ri, z + 1);
                const bool dnF = frozenAt(qi, ri, z - 1);
                const double up =
                    upF ? d_[idx] : ((z + 1 < Hz_) ? d_[index(qi, ri, z + 1)] : rho);
                const double dw =
                    dnF ? d_[idx] : ((z - 1 >= 0) ? d_[index(qi, ri, z - 1)] : rho);
                sum += wv * (up + dw);
                dn_[idx] = sum / norm;
            }
    d_.swap(dn_);

    // 境界マスク(非付着 + 8 近傍に付着あり)
#pragma omp parallel for schedule(static)
    for (int z = 0; z < Hz_; ++z)
        for (int ri = 1; ri < D_ - 1; ++ri)
            for (int qi = 1; qi < D_ - 1; ++qi) {
                const int idx = index(qi, ri, z);
                bool bnd = inHex(qi - c_, ri - c_) && !a_[idx];
                if (bnd) {
                    bool nb = frozenAt(qi, ri, z + 1) || frozenAt(qi, ri, z - 1);
                    for (int k = 0; k < 6 && !nb; ++k)
                        if (frozenAt(qi + dqN_[k], ri + drN_[k], z)) nb = true;
                    bnd = nb;
                }
                boundary_[idx] = bnd ? 1 : 0;
            }

    // ===== (ii) 凍結 =====
#pragma omp parallel for schedule(static)
    for (int z = 0; z < Hz_; ++z)
        for (int ri = 1; ri < D_ - 1; ++ri)
            for (int qi = 1; qi < D_ - 1; ++qi) {
                const int idx = index(qi, ri, z);
                if (!boundary_[idx]) continue;
                b_[idx] += (1.0 - kappa) * d_[idx];
                cm_[idx] += kappa * d_[idx];
                d_[idx] = 0.0;
            }

    // ===== (iii) 付着(面内 GG 規則 + 垂直抑制) =====
#pragma omp parallel for schedule(static)
    for (int z = 0; z < Hz_; ++z)
        for (int ri = 1; ri < D_ - 1; ++ri)
            for (int qi = 1; qi < D_ - 1; ++qi) {
                const int idx = index(qi, ri, z);
                attachMask_[idx] = 0;
                if (!boundary_[idx]) continue;

                int nH = 0;
                for (int k = 0; k < 6; ++k)
                    if (frozenAt(qi + dqN_[k], ri + drN_[k], z)) ++nH;

                bool att = false;
                if (nH >= 1) {
                    if (nH <= 2) {
                        att = b_[idx] >= beta;
                    } else if (nH == 3) {
                        if (b_[idx] >= 1.0) {
                            att = true;
                        } else {
                            double dsum = 0.0;
                            for (int k = 0; k < 6; ++k)
                                dsum += d_[index(qi + dqN_[k], ri + drN_[k], z)];
                            att = (dsum < theta) && (b_[idx] >= alpha);
                        }
                    } else {
                        att = true; // nH >= 4
                    }
                } else {
                    // 面内支持なし(垂直のみ核形成): 中心±halfT の窓内だけ許可
                    // → 厚み thickness 層の平板に制限しつつ枝分かれは保つ
                    att = (b_[idx] >= beta) && (std::abs(z - zc_) <= halfT);
                }
                attachMask_[idx] = att ? 1 : 0;
            }
    for (size_t idx = 0; idx < attachMask_.size(); ++idx) {
        if (!attachMask_[idx]) continue;
        a_[idx] = 1;
        cm_[idx] += b_[idx];
        b_[idx] = 0.0;
    }

    // ===== (iv) 融解 =====
#pragma omp parallel for schedule(static)
    for (int z = 0; z < Hz_; ++z)
        for (int ri = 1; ri < D_ - 1; ++ri)
            for (int qi = 1; qi < D_ - 1; ++qi) {
                const int idx = index(qi, ri, z);
                if (!boundary_[idx] || a_[idx]) continue;
                d_[idx] += mu * b_[idx] + gam * cm_[idx];
                b_[idx] *= (1.0 - mu);
                cm_[idx] *= (1.0 - gam);
            }

    // ===== D6h 対称化 =====
    if (enforceSymmetry) {
        symmetrizeField(a_);
        symmetrizeField(b_);
        symmetrizeField(cm_);
        symmetrizeField(d_);
    }

    ++steps_;
}

std::vector<ParamSpec> GravnerGriffeath3DModel::params() const {
    return {
        {"thickness (layers)", 1.0, 15.0, thickness, 1, true},
        {"ρ (vapor density)", 0.30, 0.90, rho, 3, true},
        {"κ (freezing)", 0.0, 0.10, kappa, 4, false},
        {"β (attach 1-2)", 1.0, 3.0, beta, 2, false},
        {"α (attach 3)", 0.0, 1.0, alpha, 3, false},
        {"θ (vapor thresh)", 0.0, 0.10, theta, 3, false},
        {"μ (melt boundary)", 0.0, 0.20, mu, 3, false},
        {"γ (melt crystal)", 0.0, 0.01, gam, 4, false},
    };
}

void GravnerGriffeath3DModel::setParam(int index, double v) {
    switch (index) {
    case 0: thickness = v; break;
    case 1: rho = v; break;
    case 2: kappa = v; break;
    case 3: beta = v; break;
    case 4: alpha = v; break;
    case 5: theta = v; break;
    case 6: mu = v; break;
    case 7: gam = v; break;
    }
}

std::vector<std::string> GravnerGriffeath3DModel::presetNames() const {
    return {"Stellar dendrite", "Fernlike", "Broad branch", "Thick dendrite"};
}

void GravnerGriffeath3DModel::applyPreset(int i) {
    switch (i) {
    case 0: // 星形樹枝(枝あり, 対称)
        rho = 0.50; kappa = 0.005; beta = 1.6; alpha = 0.40;
        theta = 0.025; mu = 0.015; gam = 0.0001; thickness = 3.0;
        break;
    case 1: // 羊歯状(細かい枝、薄い)
        rho = 0.70; kappa = 0.05; beta = 1.4; alpha = 0.08;
        theta = 0.008; mu = 0.14; gam = 0.0001; thickness = 1.0;
        break;
    case 2: // 広幅の枝
        rho = 0.48; kappa = 0.005; beta = 1.7; alpha = 0.35;
        theta = 0.020; mu = 0.015; gam = 0.0001; thickness = 3.0;
        break;
    case 3: // 厚めの樹枝
        rho = 0.50; kappa = 0.005; beta = 1.6; alpha = 0.40;
        theta = 0.025; mu = 0.015; gam = 0.0001; thickness = 7.0;
        break;
    }
}
