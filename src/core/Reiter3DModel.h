#pragma once

#include <vector>

#include "Crystal3DModel.h"

// 3 次元 Reiter 型局所モデル(六角プリズム格子)。
// 近傍は面内 6 + 上下 2 の計 8。垂直成長を vbias で抑制し板状↔ブロック状を変える。
// 板状結晶向き(枝分かれは出ない)。
class Reiter3DModel : public HexPrismLattice {
public:
    Reiter3DModel(int radius = 48, int layers = 49);

    const char *name() const override { return "Reiter 3D (plate)"; }
    void reset() override;
    void step() override;

    bool frozen(int qi, int ri, int z) const override {
        if (z < 0 || z >= Hz_) return false;
        return s_[index(qi, ri, z)] >= 1.0;
    }
    void grow(int newRadius) override;

    std::vector<ParamSpec> params() const override;
    void setParam(int index, double v) override;
    std::vector<std::string> presetNames() const override;
    void applyPreset(int i) override;

    double alpha = 1.0;
    double beta = 0.4;
    double gamma = 0.001;
    double vbias = 0.08;

private:
    std::vector<double> s_, u_, v_;
    std::vector<char> fast_;

    bool frozenAt(int qi, int ri, int z) const {
        return (z >= 0 && z < Hz_) && s_[index(qi, ri, z)] >= 1.0;
    }
};
