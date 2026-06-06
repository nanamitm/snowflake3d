#pragma once

#include <algorithm>
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
    virtual int grownRadius() const = 0;                 // 現在の最大六角半径
    virtual void grow(int newRadius) = 0;                // 格子半径を拡張(状態保持)

    // 初期条件(シード)
    virtual int seedType() const = 0;                    // 0:point 1:hex 2:ring 3:star
    virtual int seedSize() const = 0;
    virtual void setSeed(int type, int size) = 0;

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
        : R_(radius), D_(2 * (radius + 1) + 1), c_(radius + 1) {
        buildCanonMap();
    }

    int radius() const override { return R_; }
    int diameter() const override { return D_; }
    int center() const override { return c_; }
    long long stepCount() const override { return steps_; }
    bool inHex(int dq, int dr) const override {
        return hexDistance(dq, dr) <= R_;
    }

    int index(int qi, int ri) const { return ri * D_ + qi; }

    // --- シード(初期条件) ---
    int seedType() const override { return seedType_; }
    int seedSize() const override { return seedSize_; }
    void setSeed(int type, int size) override {
        seedType_ = type;
        seedSize_ = std::max(1, size);
    }
    // 各シードセルの軸オフセット(dq,dr)を fn に渡す。
    template <class Fn>
    void forEachSeed(Fn &&fn) const {
        if (seedType_ == 0) { fn(0, 0); return; }       // 点
        if (seedType_ == 3) {                            // 星(6 本腕)
            fn(0, 0);
            for (int k = 0; k < 6; ++k)
                fn(dqN_[k] * seedSize_, drN_[k] * seedSize_);
            return;
        }
        for (int dr = -seedSize_; dr <= seedSize_; ++dr)
            for (int dq = -seedSize_; dq <= seedSize_; ++dq) {
                const int hd = hexDistance(dq, dr);
                if (seedType_ == 1 && hd <= seedSize_) fn(dq, dr); // 六角形
                if (seedType_ == 2 && hd == seedSize_) fn(dq, dr); // 環
            }
    }

    // 旧状態を新半径(呼び出し前に R_,D_,c_ を更新済み)の中心へ移送する。
    template <class T>
    std::vector<T> regrid(const std::vector<T> &old, int oldD, int oldC,
                          T bg) const {
        std::vector<T> nw(static_cast<size_t>(D_) * D_, bg);
        const int off = c_ - oldC;
        for (int ri = 0; ri < oldD; ++ri)
            for (int qi = 0; qi < oldD; ++qi)
                nw[static_cast<size_t>(ri + off) * D_ + (qi + off)] =
                    old[static_cast<size_t>(ri) * oldD + qi];
        return nw;
    }

    // 現在の結晶の最大六角半径(frozen セルの中心からの最大距離)。stepCount でキャッシュ。
    int grownRadius() const override {
        if (radiusCacheStep_ == steps_) return radiusCache_;
        int mx = 0;
        for (int ri = 0; ri < D_; ++ri)
            for (int qi = 0; qi < D_; ++qi)
                if (frozen(qi, ri)) {
                    const int d = hexDistance(qi - c_, ri - c_);
                    if (d > mx) mx = d;
                }
        radiusCache_ = mx;
        radiusCacheStep_ = steps_;
        return mx;
    }

protected:
    int R_, D_, c_;
    long long steps_ = 0;
    int seedType_ = 0;
    int seedSize_ = 3;
    mutable int radiusCache_ = 0;
    mutable long long radiusCacheStep_ = -1;

    static int hexDistance(int dq, int dr) {
        return (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
    }

    // 軸座標での 6 近傍オフセット
    static constexpr int dqN_[6] = {+1, -1, 0, 0, +1, -1};
    static constexpr int drN_[6] = {0, 0, +1, -1, -1, +1};

    // ---- D6 対称化(2D) ----
    // 各セルの正準代表(12 個の回転・鏡映像の最小 index)を事前計算し、
    // 正準値をブロードキャストして樹枝成長の微小な非対称(FP 順序差)を打ち消す。
    std::vector<int> canon_; // size D_*D_

    void buildCanonMap() {
        canon_.assign(static_cast<size_t>(D_) * D_, 0);
        for (int ri = 0; ri < D_; ++ri)
            for (int qi = 0; qi < D_; ++qi) {
                const int dq0 = qi - c_, dr0 = ri - c_;
                int best = ri * D_ + qi;
                for (int refl = 0; refl < 2; ++refl) {
                    int dq = refl ? dr0 : dq0;
                    int dr = refl ? dq0 : dr0; // 鏡映 (dq,dr)->(dr,dq)
                    for (int rot = 0; rot < 6; ++rot) {
                        const int aq = dq + c_, ar = dr + c_;
                        if (aq >= 0 && aq < D_ && ar >= 0 && ar < D_)
                            best = std::min(best, ar * D_ + aq);
                        const int ndq = -dr, ndr = dq + dr; // 60° 回転
                        dq = ndq; dr = ndr;
                    }
                }
                canon_[ri * D_ + qi] = best;
            }
    }

    // フィールドを D6 対称に強制する。
    template <class T>
    void symmetrizeField(std::vector<T> &f) const {
        const int n = D_ * D_;
        for (int p = 0; p < n; ++p)
            f[p] = f[canon_[p]];
    }
};
