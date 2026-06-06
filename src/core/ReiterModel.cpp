#include "ReiterModel.h"

#include <algorithm>
#include <cmath>

ReiterModel::ReiterModel(int radius) : HexLattice(radius) {
    s_.assign(static_cast<size_t>(D_) * D_, beta);
    u_.assign(s_.size(), 0.0);
    v_.assign(s_.size(), 0.0);
    reset();
}

double ReiterModel::heightAt(int qi, int ri) const {
    const double s = s_[index(qi, ri)];
    return std::clamp((s - 1.0) / 2.0, 0.0, 1.0);
}

void ReiterModel::reset() {
    std::fill(s_.begin(), s_.end(), beta);
    s_[index(c_, c_)] = 1.0; // 種結晶
    steps_ = 0;
}

void ReiterModel::step() {
    // 1) 受容性判定 + u/v 分離
#pragma omp parallel for schedule(static)
    for (int ri = 0; ri < D_; ++ri) {
        for (int qi = 0; qi < D_; ++qi) {
            const int dq = qi - c_;
            const int dr = ri - c_;
            const int idx = index(qi, ri);

            if (!inHex(dq, dr)) {
                s_[idx] = beta; // リザーバ
                u_[idx] = beta;
                v_[idx] = 0.0;
                continue;
            }

            bool receptive = s_[idx] >= 1.0;
            if (!receptive) {
                for (int k = 0; k < 6; ++k) {
                    const int nidx = index(qi + dqN_[k], ri + drN_[k]);
                    if (s_[nidx] >= 1.0) { receptive = true; break; }
                }
            }

            if (receptive) {
                v_[idx] = s_[idx] + gamma;
                u_[idx] = 0.0;
            } else {
                u_[idx] = s_[idx];
                v_[idx] = 0.0;
            }
        }
    }

    // 2) 拡散(拡散部 u のみ) + 3) 合成
    const double half = alpha * 0.5;
#pragma omp parallel for schedule(static)
    for (int ri = 1; ri < D_ - 1; ++ri) {
        for (int qi = 1; qi < D_ - 1; ++qi) {
            if (!inHex(qi - c_, ri - c_)) continue;
            const int idx = index(qi, ri);
            double sum = 0.0;
            for (int k = 0; k < 6; ++k)
                sum += u_[index(qi + dqN_[k], ri + drN_[k])];
            const double mean = sum / 6.0;
            s_[idx] = (u_[idx] + half * (mean - u_[idx])) + v_[idx];
        }
    }

    ++steps_;
}

std::vector<ParamSpec> ReiterModel::params() const {
    return {
        {"α (diffusion)", 0.5, 2.0, alpha, 2, false},
        {"β (vapor density)", 0.3, 0.8, beta, 3, true},
        {"γ (vapor addition)", 0.0, 0.01, gamma, 4, false},
    };
}

void ReiterModel::setParam(int index, double v) {
    switch (index) {
    case 0: alpha = v; break;
    case 1: beta = v; break;
    case 2: gamma = v; break;
    }
}

std::vector<std::string> ReiterModel::presetNames() const {
    return {"Dendrite", "Stellar plate", "Sectored"};
}

void ReiterModel::applyPreset(int i) {
    switch (i) {
    case 0: alpha = 1.0; beta = 0.40; gamma = 0.0010; break; // 樹枝状
    case 1: alpha = 1.0; beta = 0.60; gamma = 0.0005; break; // 板状/星形
    case 2: alpha = 2.0; beta = 0.50; gamma = 0.0020; break; // 扇形
    }
}
