#include "SimController.h"

#include "MeshBuilder.h"
#include "SnowflakeGeometry.h"
#include "core/GravnerGriffeathModel.h"
#include "core/ReiterModel.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QVector3D>
#include <algorithm>
#include <vector>

SimController::SimController(QObject *parent)
    : QObject(parent), geometry_(std::make_unique<SnowflakeGeometry>()) {
    constexpr int kRadius = 160;
    models_.push_back(std::make_unique<ReiterModel>(kRadius));
    models_.push_back(std::make_unique<GravnerGriffeathModel>(kRadius));

    timer_.setInterval(33); // ~30 fps
    connect(&timer_, &QTimer::timeout, this, &SimController::advance);
    refreshMesh();
}

SimController::~SimController() = default;

CrystalModel *SimController::model() const {
    return models_[modelIndex_].get();
}

QQuick3DGeometry *SimController::geometryObject() const {
    return geometry_.get();
}

QStringList SimController::modelNames() const {
    QStringList names;
    for (const auto &m : models_)
        names << QString::fromUtf8(m->name());
    return names;
}

QVariantList SimController::params() const {
    QVariantList list;
    const auto specs = model()->params();
    for (int i = 0; i < static_cast<int>(specs.size()); ++i) {
        const auto &p = specs[i];
        QVariantMap m;
        m["index"] = i;
        m["name"] = QString::fromUtf8(p.name.c_str());
        m["min"] = p.minV;
        m["max"] = p.maxV;
        m["value"] = p.value;
        m["decimals"] = p.decimals;
        m["needsReset"] = p.needsReset;
        list << m;
    }
    return list;
}

QStringList SimController::presetNames() const {
    QStringList names;
    for (const auto &s : model()->presetNames())
        names << QString::fromUtf8(s.c_str());
    return names;
}

double SimController::thickness() const { return geometry_->heightScale; }

int SimController::stepCount() const {
    return static_cast<int>(model()->stepCount());
}

void SimController::setModelIndex(int i) {
    if (i < 0 || i >= static_cast<int>(models_.size()) || i == modelIndex_)
        return;
    const bool wasRunning = timer_.isActive();
    timer_.stop();
    modelIndex_ = i;
    model()->reset();
    refreshMesh();
    emit modelChanged();
    emit seedChanged();
    emit stepped();
    if (wasRunning) emit runningChanged();
}

void SimController::setThickness(double v) {
    if (geometry_->heightScale == v) return;
    geometry_->heightScale = v;
    emit thicknessChanged();
    refreshMesh();
}

void SimController::setSpeed(int stepsPerTick) {
    if (speed_ == stepsPerTick) return;
    speed_ = std::max(1, stepsPerTick);
    emit speedChanged();
}

void SimController::setParam(int index, double v) {
    model()->setParam(index, v);
}

void SimController::applyPreset(int index) {
    const bool wasRunning = timer_.isActive();
    timer_.stop();
    model()->applyPreset(index);
    model()->reset();
    refreshMesh();
    emit modelChanged(); // パラメータ値の再読込
    emit stepped();
    if (wasRunning) emit runningChanged();
}

void SimController::start() {
    if (!timer_.isActive()) { timer_.start(); emit runningChanged(); }
}

void SimController::stop() {
    if (timer_.isActive()) { timer_.stop(); emit runningChanged(); }
}

void SimController::stepOnce() {
    model()->step();
    refreshMesh();
    emit stepped();
}

void SimController::reset() {
    const bool wasRunning = timer_.isActive();
    timer_.stop();
    model()->reset();
    refreshMesh();
    emit stepped();
    if (wasRunning) emit runningChanged();
}

void SimController::advance() {
    for (int i = 0; i < speed_; ++i)
        model()->step();
    // 格子端に達したら自動拡張(または上限で自動停止)
    bool stopped = false;
    if (atBoundary()) {
        const int R = model()->radius();
        if (autoExpand_ && R < capRadius_)
            model()->grow(std::min(capRadius_, R + expandStep_));
        else if (timer_.isActive()) {
            timer_.stop();
            stopped = true;
        }
    }
    refreshMesh();
    emit stepped();
    if (stopped) emit runningChanged();
}

void SimController::setAutoExpand(bool v) {
    if (autoExpand_ == v) return;
    autoExpand_ = v;
    emit autoExpandChanged();
}

int SimController::seedType() const { return model()->seedType(); }
int SimController::seedSize() const { return model()->seedSize(); }

void SimController::setSeedType(int v) {
    if (model()->seedType() == v) return;
    timer_.stop();
    model()->setSeed(v, model()->seedSize());
    model()->reset();
    refreshMesh();
    emit seedChanged();
    emit stepped();
    emit runningChanged();
}

void SimController::setSeedSize(int v) {
    if (model()->seedSize() == v) return;
    timer_.stop();
    model()->setSeed(model()->seedType(), v);
    model()->reset();
    refreshMesh();
    emit seedChanged();
    emit stepped();
    emit runningChanged();
}

int SimController::growthPercent() const {
    const int R = model()->radius();
    if (R <= 0) return 0;
    return std::min(100, model()->grownRadius() * 100 / R);
}

bool SimController::atBoundary() const {
    return model()->grownRadius() >= static_cast<int>(model()->radius() * 0.94);
}

void SimController::refreshMesh() {
    geometry_->rebuild(*model());
}

bool SimController::saveConfig(const QUrl &fileUrl) {
    const QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) return false;

    QJsonObject root;
    root["model"] = QString::fromUtf8(model()->name());
    root["modelIndex"] = modelIndex_;
    root["thickness"] = geometry_->heightScale;
    root["speed"] = speed_;

    QJsonObject params;
    for (const auto &p : model()->params())
        params[QString::fromUtf8(p.name.c_str())] = p.value;
    root["params"] = params;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool SimController::loadConfig(const QUrl &fileUrl) {
    const QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) return false;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return false;
    const QJsonObject root = doc.object();

    timer_.stop();

    int mi = root.value("modelIndex").toInt(modelIndex_);
    if (mi < 0 || mi >= static_cast<int>(models_.size())) mi = modelIndex_;
    modelIndex_ = mi;

    // パラメータを名前で突合して適用
    const QJsonObject params = root.value("params").toObject();
    const auto specs = model()->params();
    for (int i = 0; i < static_cast<int>(specs.size()); ++i) {
        const QString key = QString::fromUtf8(specs[i].name.c_str());
        if (params.contains(key))
            model()->setParam(i, params.value(key).toDouble(specs[i].value));
    }

    if (root.contains("thickness"))
        geometry_->heightScale = root.value("thickness").toDouble(geometry_->heightScale);
    if (root.contains("speed"))
        speed_ = std::max(1, root.value("speed").toInt(speed_));

    model()->reset();
    refreshMesh();
    emit modelChanged();
    emit thicknessChanged();
    emit speedChanged();
    emit stepped();
    emit runningChanged();
    return true;
}

bool SimController::exportStl(const QUrl &fileUrl) {
    const QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) return false;

    struct Tri { QVector3D a, b, c; };
    std::vector<Tri> tris;
    buildSnowflakeMesh(*model(), geometry_->cellSize, geometry_->heightScale,
                       [&](const QVector3D &a, const QVector3D &b,
                           const QVector3D &c) { tris.push_back({a, b, c}); });

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;

    char header[80] = {0};
    f.write(header, 80);
    quint32 count = static_cast<quint32>(tris.size());
    f.write(reinterpret_cast<const char *>(&count), 4);
    for (const auto &t : tris) {
        const QVector3D n =
            QVector3D::crossProduct(t.b - t.a, t.c - t.a).normalized();
        float buf[12] = {n.x(),   n.y(),   n.z(),   t.a.x(), t.a.y(), t.a.z(),
                         t.b.x(), t.b.y(), t.b.z(), t.c.x(), t.c.y(), t.c.z()};
        f.write(reinterpret_cast<const char *>(buf), sizeof(buf));
        quint16 attr = 0;
        f.write(reinterpret_cast<const char *>(&attr), 2);
    }
    f.close();
    return true;
}
