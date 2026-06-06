#include "GravnerGriffeathModel.h"

#include <algorithm>
#include <cmath>

GravnerGriffeathModel::GravnerGriffeathModel(int radius) : HexLattice(radius) {
    const size_t n = static_cast<size_t>(D_) * D_;
    a_.assign(n, 0);
    b_.assign(n, 0.0);
    cm_.assign(n, 0.0);
    d_.assign(n, 0.0);
    dn_.assign(n, 0.0);
    boundary_.assign(n, 0);
    attachMask_.assign(n, 0);
    reset();
}

double GravnerGriffeathModel::heightAt(int qi, int ri) const {
    // 結晶質量を 0..1 に正規化
    return std::clamp((cm_[index(qi, ri)] - 0.5) / 1.5, 0.0, 1.0);
}

void GravnerGriffeathModel::reset() {
    std::fill(a_.begin(), a_.end(), 0);
    std::fill(b_.begin(), b_.end(), 0.0);
    std::fill(cm_.begin(), cm_.end(), 0.0);
    std::fill(d_.begin(), d_.end(), rho);
    const int o = index(c_, c_);
    a_[o] = 1;
    cm_[o] = 1.0;
    d_[o] = 0.0;
    steps_ = 0;
}

int GravnerGriffeathModel::attachedNeighbors(int qi, int ri) const {
    int n = 0;
    for (int k = 0; k < 6; ++k)
        if (a_[index(qi + dqN_[k], ri + drN_[k])]) ++n;
    return n;
}

bool GravnerGriffeathModel::hasAttachedNeighbor(int qi, int ri) const {
    for (int k = 0; k < 6; ++k)
        if (a_[index(qi + dqN_[k], ri + drN_[k])]) return true;
    return false;
}

void GravnerGriffeathModel::step() {
    std::uniform_int_distribution<int> coin(0, 1);

    // ===== (i) 拡散: 非付着セルの拡散質量を 7 点平均。 =====
    //          付着セルは反射境界(隣の値を自セル値で置換)。
#pragma omp parallel for schedule(static)
    for (int ri = 1; ri < D_ - 1; ++ri) {
        for (int qi = 1; qi < D_ - 1; ++qi) {
            const int idx = index(qi, ri);
            if (!inHex(qi - c_, ri - c_)) { dn_[idx] = rho; continue; }
            if (a_[idx]) { dn_[idx] = 0.0; continue; }
            double sum = d_[idx];
            for (int k = 0; k < 6; ++k) {
                const int nidx = index(qi + dqN_[k], ri + drN_[k]);
                sum += a_[nidx] ? d_[idx] : d_[nidx];
            }
            dn_[idx] = sum / 7.0;
        }
    }
    d_.swap(dn_);

    // 境界マスク(このステップで処理する非付着セル)
#pragma omp parallel for schedule(static)
    for (int ri = 1; ri < D_ - 1; ++ri)
        for (int qi = 1; qi < D_ - 1; ++qi) {
            const int idx = index(qi, ri);
            boundary_[idx] = (inHex(qi - c_, ri - c_) && !a_[idx] &&
                              hasAttachedNeighbor(qi, ri))
                                 ? 1 : 0;
        }

    // ===== (ii) 凍結 =====
#pragma omp parallel for schedule(static)
    for (int ri = 1; ri < D_ - 1; ++ri)
        for (int qi = 1; qi < D_ - 1; ++qi) {
            const int idx = index(qi, ri);
            if (!boundary_[idx]) continue;
            b_[idx] += (1.0 - kappa) * d_[idx];
            cm_[idx] += kappa * d_[idx];
            d_[idx] = 0.0;
        }

    // ===== (iii) 付着 =====（隣接数判定は同時更新のためマスクに記録し後で反映）
#pragma omp parallel for schedule(static)
    for (int ri = 1; ri < D_ - 1; ++ri)
        for (int qi = 1; qi < D_ - 1; ++qi) {
            const int idx = index(qi, ri);
            attachMask_[idx] = 0;
            if (!boundary_[idx]) continue;
            const int n = attachedNeighbors(qi, ri);
            bool att = false;
            if (n <= 2) {
                att = b_[idx] >= beta;
            } else if (n == 3) {
                if (b_[idx] >= 1.0) {
                    att = true;
                } else {
                    double dsum = 0.0;
                    for (int k = 0; k < 6; ++k)
                        dsum += d_[index(qi + dqN_[k], ri + drN_[k])];
                    att = (dsum < theta) && (b_[idx] >= alpha);
                }
            } else {
                att = true; // n >= 4
            }
            attachMask_[idx] = att ? 1 : 0;
        }
    for (size_t idx = 0; idx < attachMask_.size(); ++idx) {
        if (!attachMask_[idx]) continue;
        a_[idx] = 1;
        cm_[idx] += b_[idx];
        b_[idx] = 0.0;
    }

    // ===== (iv) 融解 =====（このステップの境界セルに作用）
#pragma omp parallel for schedule(static)
    for (int ri = 1; ri < D_ - 1; ++ri)
        for (int qi = 1; qi < D_ - 1; ++qi) {
            const int idx = index(qi, ri);
            if (!boundary_[idx] || a_[idx]) continue;
            d_[idx] += mu * b_[idx] + gam * cm_[idx];
            b_[idx] *= (1.0 - mu);
            cm_[idx] *= (1.0 - gam);
        }

    // ===== (v) ノイズ =====
    if (sigma > 0.0) {
        for (int ri = 1; ri < D_ - 1; ++ri)
            for (int qi = 1; qi < D_ - 1; ++qi) {
                const int idx = index(qi, ri);
                if (!inHex(qi - c_, ri - c_) || a_[idx]) continue;
                d_[idx] *= (coin(rng_) ? (1.0 + sigma) : (1.0 - sigma));
            }
    }

    ++steps_;
}

std::vector<ParamSpec> GravnerGriffeathModel::params() const {
    return {
        {"ρ (vapor density)", 0.30, 0.90, rho, 3, true},
        {"κ (freezing)", 0.0, 0.10, kappa, 4, false},
        {"β (attach 1-2)", 1.0, 3.0, beta, 2, false},
        {"α (attach 3)", 0.0, 1.0, alpha, 3, false},
        {"θ (vapor thresh)", 0.0, 0.10, theta, 3, false},
        {"μ (melt boundary)", 0.0, 0.20, mu, 3, false},
        {"γ (melt crystal)", 0.0, 0.01, gam, 4, false},
        {"σ (noise)", 0.0, 0.10, sigma, 3, false},
    };
}

void GravnerGriffeathModel::setParam(int index, double v) {
    switch (index) {
    case 0: rho = v; break;
    case 1: kappa = v; break;
    case 2: beta = v; break;
    case 3: alpha = v; break;
    case 4: theta = v; break;
    case 5: mu = v; break;
    case 6: gam = v; break;
    case 7: sigma = v; break;
    }
}

std::vector<std::string> GravnerGriffeathModel::presetNames() const {
    return {"Simple plate", "Sectored plate", "Stellar dendrite", "Fernlike"};
}

void GravnerGriffeathModel::applyPreset(int i) {
    switch (i) {
    case 0: // 単純六角板(中身の詰まった板, fill≈1.0)
        rho = 0.48; kappa = 0.001; beta = 1.5; alpha = 0.08;
        theta = 0.008; mu = 0.10; gam = 0.00005; sigma = 0.0;
        break;
    case 1: // 扇形板(部分的に枝あり, fill≈0.73)
        rho = 0.50; kappa = 0.005; beta = 1.6; alpha = 0.35;
        theta = 0.030; mu = 0.020; gam = 0.0001; sigma = 0.0;
        break;
    case 2: // 星形樹枝(広めの枝, fill≈0.58)
        rho = 0.635; kappa = 0.005; beta = 1.6; alpha = 0.4;
        theta = 0.025; mu = 0.015; gam = 0.0001; sigma = 0.0;
        break;
    case 3: // 羊歯状(細かい枝, fill≈0.34)
        rho = 0.75; kappa = 0.05; beta = 1.3; alpha = 0.08;
        theta = 0.01; mu = 0.12; gam = 0.0001; sigma = 0.0;
        break;
    }
}
