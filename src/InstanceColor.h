#pragma once

#include <QColor>
#include <algorithm>

// インスタンス(セル)ごとの色を決める。
//   mode 0: Ice       … 氷の単色
//   mode 1: Spectrum  … 中心(青)→外周(赤)の放射状レインボー
//   mode 2: Thickness … 薄い(青)→厚い(白)の氷グラデーション
// radial01 = 中心からの正規化距離、value01 = 厚み/層などの正規化量。
inline QColor crystalColor(int mode, double radial01, double value01) {
    switch (mode) {
    case 1: { // Spectrum: 中心 0.66(青) → 外周 0.0(赤)
        const double t = std::clamp(radial01, 0.0, 1.0);
        const double h = 0.66 * (1.0 - t);
        return QColor::fromHsvF(static_cast<float>(h), 0.55f, 1.0f);
    }
    case 2: { // Thickness: 青 → 白
        const double t = std::clamp(value01, 0.0, 1.0);
        return QColor::fromRgbF(static_cast<float>(0.50 + 0.50 * t),
                                static_cast<float>(0.72 + 0.28 * t), 1.0f);
    }
    default: // Ice
        return QColor::fromRgbF(0.86f, 0.95f, 1.0f);
    }
}
