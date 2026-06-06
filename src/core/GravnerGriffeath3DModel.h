#pragma once

#include <random>
#include <vector>

#include "Crystal3DModel.h"

// 3 次元 Gravner-Griffeath 型モデル(六角プリズム格子)。
// 面内 6 近傍に GG の付着規則を効かせて樹枝状の枝分かれを生み、
// 垂直方向は付着閾値を上げて抑制することで「薄い板の上に伸びる星形樹枝」を作る。
//
// 各セル: a(付着) / b(境界質量) / cm(結晶質量) / d(拡散質量)。
class GravnerGriffeath3DModel : public HexPrismLattice {
public:
    GravnerGriffeath3DModel(int radius = 48, int layers = 41);

    const char *name() const override { return "Gravner-Griffeath 3D (dendrite)"; }
    void reset() override;
    void step() override;

    bool frozen(int qi, int ri, int z) const override {
        if (z < 0 || z >= Hz_) return false;
        return a_[index(qi, ri, z)] != 0;
    }

    std::vector<ParamSpec> params() const override;
    void setParam(int index, double v) override;
    std::vector<std::string> presetNames() const override;
    void applyPreset(int i) override;

    // --- パラメータ ---
    double rho = 0.50;     // 初期蒸気密度
    double kappa = 0.005;  // 凍結
    double beta = 1.6;     // 付着閾値(面内 1-2)
    double alpha = 0.40;   // 付着閾値(面内 3)
    double theta = 0.025;  // 付着の蒸気しきい値(面内 3)
    double mu = 0.015;     // 融解(境界質量)
    double gam = 0.0001;   // 融解(結晶質量)
    double thickness = 3.0; // 板厚(層数)。垂直核形成を中心±thickness/2 に制限
    bool enforceSymmetry = true; // D6h 対称を各ステップで強制(大サイズの非対称を防ぐ)

private:
    std::vector<char> a_;
    std::vector<double> b_, cm_, d_, dn_;
    std::vector<char> boundary_, attachMask_;
    std::mt19937 rng_{12345};

    bool frozenAt(int qi, int ri, int z) const {
        return (z >= 0 && z < Hz_) && a_[index(qi, ri, z)] != 0;
    }
};
