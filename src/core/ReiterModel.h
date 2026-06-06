#pragma once

#include <vector>

#include "CrystalModel.h"

// Reiter (2005) "A local cellular model for snow crystal growth".
// 六角格子上の実数値セルオートマトン。
//
// 各セルは「水の量」s を持つ。s >= 1 で凍結(結晶)。
// 受容セル(receptive) = 凍結セル または 凍結セルに隣接するセル。
//   受容セル   : 受容部 v = s + gamma, 拡散部 u = 0
//   非受容セル : 拡散部 u = s,          受容部 v = 0
// 拡散部のみ拡散させ、s_next = u' + v とする。
class ReiterModel : public HexLattice {
public:
    explicit ReiterModel(int radius = 160);

    const char *name() const override { return "Reiter (2005)"; }
    void reset() override;
    void step() override;

    bool frozen(int qi, int ri) const override {
        return s_[index(qi, ri)] >= 1.0;
    }
    double heightAt(int qi, int ri) const override;
    void grow(int newRadius) override;

    std::vector<ParamSpec> params() const override;
    void setParam(int index, double v) override;
    std::vector<std::string> presetNames() const override;
    void applyPreset(int i) override;

    // --- パラメータ ---
    double alpha = 1.0;   // 拡散定数
    double beta = 0.4;    // 背景蒸気密度(初期値)
    double gamma = 0.001; // 受容セルへの蒸気付加量

private:
    std::vector<double> s_; // 現在の水量
    std::vector<double> u_; // 拡散部
    std::vector<double> v_; // 受容部
};
