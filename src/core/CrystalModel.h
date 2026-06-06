#pragma once

#include <cstdlib>
#include <string>
#include <vector>

// UI に動的提示するパラメータ定義
struct ParamSpec {
    std::string name;     // 表示名
    double minV;
    double maxV;
    double value;
    int decimals;
    bool needsReset;      // 変更を反映するには reset が必要か
};

// 雪結晶成長モデルの共通インターフェース。
// Reiter / Gravner-Griffeath など複数モデルを差し替え可能にする。
class CrystalModel {
public:
    virtual ~CrystalModel() = default;

    virtual const char *name() const = 0;
    virtual void reset() = 0;
    virtual void step() = 0;

    // 格子参照系
    virtual int radius() const = 0;
    virtual int diameter() const = 0;
    virtual int center() const = 0;
    virtual long long stepCount() const = 0;
    virtual bool inHex(int dq, int dr) const = 0;

    // セル状態
    virtual bool frozen(int qi, int ri) const = 0;       // 結晶(付着)か
    virtual double heightAt(int qi, int ri) const = 0;   // 厚み係数 0..1

    // パラメータ
    virtual std::vector<ParamSpec> params() const = 0;
    virtual void setParam(int index, double v) = 0;

    // プリセット
    virtual std::vector<std::string> presetNames() const { return {}; }
    virtual void applyPreset(int) {}
};

// 六角格子(axial 座標)の共通土台。配列は (2R+3)^2、中心 c=R+1。
class HexLattice : public CrystalModel {
public:
    explicit HexLattice(int radius)
        : R_(radius), D_(2 * (radius + 1) + 1), c_(radius + 1) {}

    int radius() const override { return R_; }
    int diameter() const override { return D_; }
    int center() const override { return c_; }
    long long stepCount() const override { return steps_; }
    bool inHex(int dq, int dr) const override {
        return hexDistance(dq, dr) <= R_;
    }

    int index(int qi, int ri) const { return ri * D_ + qi; }

protected:
    int R_, D_, c_;
    long long steps_ = 0;

    static int hexDistance(int dq, int dr) {
        return (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
    }

    // 軸座標での 6 近傍オフセット
    static constexpr int dqN_[6] = {+1, -1, 0, 0, +1, -1};
    static constexpr int drN_[6] = {0, 0, +1, -1, -1, +1};
};
