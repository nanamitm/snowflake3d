#pragma once

#include <random>
#include <vector>

#include "CrystalModel.h"

// Gravner & Griffeath (2008)
// "Modeling snow crystal growth II: A mesoscopic lattice map with plausible dynamics".
//
// 各セルは 4 つの量を持つ:
//   a : 付着フラグ(結晶か)        b : 境界質量(準液体)
//   c : 結晶質量(氷)             d : 拡散質量(蒸気)
//
// 1 ステップ = 拡散 → 凍結 → 付着 → 融解 → ノイズ の合成。
class GravnerGriffeathModel : public HexLattice {
public:
    explicit GravnerGriffeathModel(int radius = 160);

    const char *name() const override { return "Gravner-Griffeath (2008)"; }
    void reset() override;
    void step() override;

    bool frozen(int qi, int ri) const override {
        return a_[index(qi, ri)] != 0;
    }
    double heightAt(int qi, int ri) const override;

    std::vector<ParamSpec> params() const override;
    void setParam(int index, double v) override;
    std::vector<std::string> presetNames() const override;
    void applyPreset(int i) override;

    // --- パラメータ ---
    double rho = 0.635;    // 初期蒸気密度
    double kappa = 0.005;  // 凍結: 拡散質量が結晶質量へ回る割合
    double beta = 1.6;     // 付着閾値(隣接 1-2)
    double alpha = 0.4;    // 付着閾値(隣接 3)
    double theta = 0.025;  // 付着の蒸気しきい値(隣接 3)
    double mu = 0.015;     // 融解: 境界質量
    double gam = 0.0001;   // 融解: 結晶質量
    double sigma = 0.0;    // 拡散質量ノイズ
    bool enforceSymmetry = true; // D6 対称を各ステップで強制(大サイズの非対称を防ぐ)

private:
    std::vector<char> a_;        // 付着フラグ
    std::vector<double> b_;      // 境界質量
    std::vector<double> cm_;     // 結晶質量 (HexLattice::c_ と衝突回避のため cm_)
    std::vector<double> d_;      // 拡散質量
    std::vector<double> dn_;     // 拡散の次状態
    std::vector<char> boundary_;   // このステップの境界マスク
    std::vector<char> attachMask_; // このステップで付着するセル
    std::mt19937 rng_{12345};

    bool hasAttachedNeighbor(int qi, int ri) const;
    int attachedNeighbors(int qi, int ri) const;
};
