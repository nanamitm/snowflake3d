#include "Reiter3DModel.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

Reiter3DModel::Reiter3DModel(int radius, int layers)
    : HexPrismLattice(radius, layers) {
    s_.assign(static_cast<size_t>(D_) * D_ * Hz_, beta);
    u_.assign(s_.size(), 0.0);
    v_.assign(s_.size(), 0.0);
    fast_.assign(s_.size(), 1);
    reset();
}

void Reiter3DModel::reset() {
    std::fill(s_.begin(), s_.end(), beta);
    s_[index(c_, c_, zc_)] = 1.0; // 種結晶
    steps_ = 0;
}

void Reiter3DModel::step() {
    // 1) 受容性判定 + u/v 分離 + 成長方向フラグ
#pragma omp parallel for schedule(static)
    for (int z = 0; z < Hz_; ++z) {
        for (int ri = 0; ri < D_; ++ri) {
            for (int qi = 0; qi < D_; ++qi) {
                const int dq = qi - c_;
                const int dr = ri - c_;
                const int idx = index(qi, ri, z);

                if (!inHex(dq, dr)) {
                    s_[idx] = beta;
                    u_[idx] = beta;
                    v_[idx] = 0.0;
                    fast_[idx] = 1;
                    continue;
                }

                const bool self = s_[idx] >= 1.0;
                bool horiz = false;
                for (int k = 0; k < 6; ++k)
                    if (frozenAt(qi + dqN_[k], ri + drN_[k], z)) { horiz = true; break; }
                const bool vert =
                    frozenAt(qi, ri, z + 1) || frozenAt(qi, ri, z - 1);

                // 水平支持があれば高速、垂直のみは vbias で抑制(板状化)
                fast_[idx] = (self || horiz) ? 1 : 0;

                if (self || horiz || vert) {
                    v_[idx] = s_[idx] + gamma;
                    u_[idx] = 0.0;
                } else {
                    u_[idx] = s_[idx];
                    v_[idx] = 0.0;
                }
            }
        }
    }

    // 2) 拡散(非受容部 u を 8 近傍平均) + 3) 合成
    const double half = alpha * 0.5;
#pragma omp parallel for schedule(static)
    for (int z = 0; z < Hz_; ++z) {
        for (int ri = 1; ri < D_ - 1; ++ri) {
            for (int qi = 1; qi < D_ - 1; ++qi) {
                if (!inHex(qi - c_, ri - c_)) continue;
                const int idx = index(qi, ri, z);
                double sum = 0.0;
                for (int k = 0; k < 6; ++k)
                    sum += u_[index(qi + dqN_[k], ri + drN_[k], z)];
                sum += (z + 1 < Hz_) ? u_[index(qi, ri, z + 1)] : beta;
                sum += (z - 1 >= 0) ? u_[index(qi, ri, z - 1)] : beta;
                const double mean = sum / 8.0;
                double influx = half * (mean - u_[idx]); // 拡散流入
                // 垂直のみで成長する前線は influx を vbias 倍に抑制
                if (!fast_[idx]) influx *= vbias;
                s_[idx] = (u_[idx] + influx) + v_[idx];
            }
        }
    }

    ++steps_;
}

std::vector<ParamSpec> Reiter3DModel::params() const {
    return {
        {"vbias (plate↔block)", 0.02, 1.0, vbias, 3, false},
        {"α (diffusion)", 0.5, 2.0, alpha, 2, false},
        {"β (vapor density)", 0.3, 0.8, beta, 3, true},
        {"γ (vapor addition)", 0.0, 0.01, gamma, 4, false},
    };
}

void Reiter3DModel::setParam(int index, double v) {
    switch (index) {
    case 0: vbias = v; break;
    case 1: alpha = v; break;
    case 2: beta = v; break;
    case 3: gamma = v; break;
    }
}

std::vector<std::string> Reiter3DModel::presetNames() const {
    return {"Thin plate", "Plate", "Thick plate", "Blocky"};
}

void Reiter3DModel::applyPreset(int i) {
    switch (i) {
    case 0: vbias = 0.04; break;
    case 1: vbias = 0.10; break;
    case 2: vbias = 0.25; break;
    case 3: vbias = 0.60; break;
    }
}
