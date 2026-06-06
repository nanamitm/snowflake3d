#pragma once

#include <QColor>
#include <QImage>
#include <QQuickImageProvider>
#include <algorithm>
#include <cmath>
#include <vector>

// IBL 用のスタジオ風 equirectangular 環境マップを手続き生成する。
// HDR ファイルを同梱せずに、氷の屈折・反射が拾える環境を用意する。
// QML から  Texture { source: "image://env/studio" }  として参照する。
class EnvProvider : public QQuickImageProvider {
public:
    EnvProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    QImage requestImage(const QString &, QSize *size,
                        const QSize &requested) override {
        int W = requested.width() > 0 ? requested.width() : 1024;
        int H = requested.height() > 0 ? requested.height() : 512;
        if (W < 2 * H) W = 2 * H; // equirectangular は 2:1

        QImage img(W, H, QImage::Format_RGBA8888);

        // 縦グラデーション色(天頂 / 地平 / 地底)
        const QColor zenith(150, 178, 224);
        const QColor horizon(238, 244, 255);
        const QColor nadir(20, 28, 46);

        // ソフトボックス(氷面に鋭いハイライトを作る明点)
        struct Blob { double theta, phi, radius, gain; };
        const std::vector<Blob> blobs = {
            {0.6, 0.55, 0.28, 1.6},
            {-1.7, 0.35, 0.20, 1.3},
            {2.5, 0.20, 0.16, 1.0},
        };

        auto lerp = [](const QColor &a, const QColor &b, double t) {
            return QColor(int(a.red() + (b.red() - a.red()) * t),
                          int(a.green() + (b.green() - a.green()) * t),
                          int(a.blue() + (b.blue() - a.blue()) * t));
        };

        for (int y = 0; y < H; ++y) {
            const double phi = (0.5 - double(y) / H) * M_PI; // +π/2..-π/2
            const double t = phi / (M_PI / 2.0);             // -1..1
            const QColor base =
                t >= 0 ? lerp(horizon, zenith, t) : lerp(horizon, nadir, -t);

            for (int x = 0; x < W; ++x) {
                const double theta = (double(x) / W) * 2.0 * M_PI - M_PI;
                double r = base.red(), g = base.green(), b = base.blue();

                // 視線方向ベクトル
                const double dx = std::cos(phi) * std::cos(theta);
                const double dy = std::sin(phi);
                const double dz = std::cos(phi) * std::sin(theta);

                for (const auto &bl : blobs) {
                    const double lx = std::cos(bl.phi) * std::cos(bl.theta);
                    const double ly = std::sin(bl.phi);
                    const double lz = std::cos(bl.phi) * std::sin(bl.theta);
                    const double d = dx * lx + dy * ly + dz * lz;
                    const double cosR = std::cos(bl.radius);
                    if (d > cosR) {
                        const double k = (d - cosR) / (1.0 - cosR); // 0..1
                        const double add = bl.gain * k * k * 255.0;
                        r += add; g += add; b += add;
                    }
                }

                img.setPixelColor(x, y,
                                  QColor(std::min(255, int(r)),
                                         std::min(255, int(g)),
                                         std::min(255, int(b))));
            }
        }

        if (size) *size = img.size();
        return img;
    }
};
