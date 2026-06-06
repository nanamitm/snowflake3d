#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "CrystalModel.h" // ParamSpec を再利用

// 3 次元雪結晶モデルの共通インターフェース(六角プリズム格子)。
class Crystal3DModel {
public:
    virtual ~Crystal3DModel() = default;

    virtual const char *name() const = 0;
    virtual void reset() = 0;
    virtual void step() = 0;

    virtual int radius() const = 0;
    virtual int diameter() const = 0;
    virtual int center() const = 0;
    virtual int centerZ() const = 0;
    virtual int layers() const = 0;
    virtual long long stepCount() const = 0;
    virtual bool inHex(int dq, int dr) const = 0;
    virtual bool frozen(int qi, int ri, int z) const = 0;
    virtual int grownRadius() const = 0; // 現在の面内最大六角半径
    virtual void grow(int newRadius) = 0; // 面内半径を拡張(状態保持)

    // 初期条件(シード)
    virtual int seedType() const = 0;
    virtual int seedSize() const = 0;
    virtual void setSeed(int type, int size) = 0;

    virtual std::vector<ParamSpec> params() const = 0;
    virtual void setParam(int index, double v) = 0;
    virtual std::vector<std::string> presetNames() const { return {}; }
    virtual void applyPreset(int) {}
};

// 六角プリズム格子の土台。面内は axial 六角、c 軸方向に Hz 層。
class HexPrismLattice : public Crystal3DModel {
public:
    HexPrismLattice(int radius, int layers)
        : R_(radius), D_(2 * (radius + 1) + 1), c_(radius + 1), Hz_(layers),
          zc_(layers / 2) {
        buildCanonMap();
    }

    int radius() const override { return R_; }
    int diameter() const override { return D_; }
    int center() const override { return c_; }
    int centerZ() const override { return zc_; }
    int layers() const override { return Hz_; }
    long long stepCount() const override { return steps_; }
    bool inHex(int dq, int dr) const override {
        return hexDistance(dq, dr) <= R_;
    }
    int index(int qi, int ri, int z) const { return (z * D_ + ri) * D_ + qi; }

    // --- シード(初期条件、面内パターンを中心層に配置) ---
    int seedType() const override { return seedType_; }
    int seedSize() const override { return seedSize_; }
    void setSeed(int type, int size) override {
        seedType_ = type;
        seedSize_ = std::max(1, size);
    }
    template <class Fn>
    void forEachSeed(Fn &&fn) const {
        if (seedType_ == 0) { fn(0, 0); return; }
        if (seedType_ == 3) {
            fn(0, 0);
            for (int k = 0; k < 6; ++k)
                fn(dqN_[k] * seedSize_, drN_[k] * seedSize_);
            return;
        }
        for (int dr = -seedSize_; dr <= seedSize_; ++dr)
            for (int dq = -seedSize_; dq <= seedSize_; ++dq) {
                const int hd = hexDistance(dq, dr);
                if (seedType_ == 1 && hd <= seedSize_) fn(dq, dr);
                if (seedType_ == 2 && hd == seedSize_) fn(dq, dr);
            }
    }

    // 旧状態を新面内半径(R_,D_,c_ 更新済み)の中心へ移送(z はそのまま)。
    template <class T>
    std::vector<T> regrid(const std::vector<T> &old, int oldD, int oldC,
                          T bg) const {
        std::vector<T> nw(static_cast<size_t>(D_) * D_ * Hz_, bg);
        const int off = c_ - oldC;
        for (int z = 0; z < Hz_; ++z)
            for (int ri = 0; ri < oldD; ++ri)
                for (int qi = 0; qi < oldD; ++qi)
                    nw[(static_cast<size_t>(z) * D_ + (ri + off)) * D_ +
                       (qi + off)] =
                        old[(static_cast<size_t>(z) * oldD + ri) * oldD + qi];
        return nw;
    }

    // 現在の結晶の面内最大六角半径。stepCount でキャッシュ。
    int grownRadius() const override {
        if (radiusCacheStep_ == steps_) return radiusCache_;
        int mx = 0;
        for (int z = 0; z < Hz_; ++z)
            for (int ri = 0; ri < D_; ++ri)
                for (int qi = 0; qi < D_; ++qi)
                    if (frozen(qi, ri, z)) {
                        const int d = hexDistance(qi - c_, ri - c_);
                        if (d > mx) mx = d;
                    }
        radiusCache_ = mx;
        radiusCacheStep_ = steps_;
        return mx;
    }

protected:
    int R_, D_, c_, Hz_, zc_;
    long long steps_ = 0;
    int seedType_ = 0;
    int seedSize_ = 3;
    mutable int radiusCache_ = 0;
    mutable long long radiusCacheStep_ = -1;

    static int hexDistance(int dq, int dr) {
        return (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
    }
    static constexpr int dqN_[6] = {+1, -1, 0, 0, +1, -1};
    static constexpr int drN_[6] = {0, 0, +1, -1, -1, +1};

    // ---- D6h 対称化 ----
    // 各面内セルの「正準代表」(12 個の回転・鏡映像のうち最小 index)を事前計算。
    // 樹枝成長の不安定性が浮動小数点和の順序差を増幅して生じる微小な非対称を、
    // 正準値のブロードキャストで厳密に打ち消す。
    std::vector<int> inplaneCanon_; // size D_*D_、面内 2D index を返す

    void buildCanonMap() {
        inplaneCanon_.assign(static_cast<size_t>(D_) * D_, 0);
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
                inplaneCanon_[ri * D_ + qi] = best;
            }
    }

    // フィールドを D6h 対称に強制する(正準像の値を全像へブロードキャスト)。
    template <class T>
    void symmetrizeField(std::vector<T> &f) const {
        const int layerN = D_ * D_;
        for (int z = 0; z < Hz_; ++z) {
            const int zz = std::min(z, 2 * zc_ - z); // 垂直ミラー
            const int zBase = z * layerN;
            const int czBase = zz * layerN;
            for (int p = 0; p < layerN; ++p)
                f[zBase + p] = f[czBase + inplaneCanon_[p]];
        }
    }
};
