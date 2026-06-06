#include "Sim3DController.h"

#include "Voxel3DGeometry.h"
#include "core/GravnerGriffeath3DModel.h"
#include "core/Reiter3DModel.h"

#include <QVariantMap>

Sim3DController::Sim3DController(QObject *parent)
    : QObject(parent), geometry_(std::make_unique<Voxel3DGeometry>()) {
    models_.push_back(std::make_unique<Reiter3DModel>(48, 49));
    models_.push_back(std::make_unique<GravnerGriffeath3DModel>(60, 41));

    timer_.setInterval(40);
    connect(&timer_, &QTimer::timeout, this, &Sim3DController::advance);
    refreshMesh();
}

Sim3DController::~Sim3DController() = default;

Crystal3DModel *Sim3DController::model() const {
    return models_[modelIndex_].get();
}

QQuick3DGeometry *Sim3DController::geometryObject() const {
    return geometry_.get();
}

QStringList Sim3DController::modelNames() const {
    QStringList names;
    for (const auto &m : models_)
        names << QString::fromUtf8(m->name());
    return names;
}

QVariantList Sim3DController::params() const {
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

QStringList Sim3DController::presetNames() const {
    QStringList names;
    for (const auto &s : model()->presetNames())
        names << QString::fromUtf8(s.c_str());
    return names;
}

int Sim3DController::stepCount() const {
    return static_cast<int>(model()->stepCount());
}

void Sim3DController::setModelIndex(int i) {
    if (i < 0 || i >= static_cast<int>(models_.size()) || i == modelIndex_)
        return;
    const bool wasRunning = timer_.isActive();
    timer_.stop();
    modelIndex_ = i;
    model()->reset();
    refreshMesh();
    emit modelChanged();
    emit stepped();
    if (wasRunning) emit runningChanged();
}

void Sim3DController::setSpeed(int v) {
    if (speed_ == v) return;
    speed_ = std::max(1, v);
    emit speedChanged();
}

void Sim3DController::setParam(int index, double v) {
    model()->setParam(index, v);
}

void Sim3DController::applyPreset(int index) {
    const bool wasRunning = timer_.isActive();
    timer_.stop();
    model()->applyPreset(index);
    model()->reset();
    refreshMesh();
    emit modelChanged();
    emit stepped();
    if (wasRunning) emit runningChanged();
}

void Sim3DController::start() {
    if (!timer_.isActive()) { timer_.start(); emit runningChanged(); }
}
void Sim3DController::stop() {
    if (timer_.isActive()) { timer_.stop(); emit runningChanged(); }
}
void Sim3DController::stepOnce() {
    model()->step(); refreshMesh(); emit stepped();
}
void Sim3DController::reset() {
    const bool wasRunning = timer_.isActive();
    timer_.stop();
    model()->reset(); refreshMesh(); emit stepped();
    if (wasRunning) emit runningChanged();
}
void Sim3DController::advance() {
    for (int i = 0; i < speed_; ++i) model()->step();
    refreshMesh();
    emit stepped();
    if (atBoundary() && timer_.isActive()) {
        timer_.stop();
        emit runningChanged();
    }
}

int Sim3DController::growthPercent() const {
    const int R = model()->radius();
    if (R <= 0) return 0;
    return std::min(100, model()->grownRadius() * 100 / R);
}

bool Sim3DController::atBoundary() const {
    return model()->grownRadius() >= static_cast<int>(model()->radius() * 0.94);
}
void Sim3DController::refreshMesh() {
    geometry_->rebuild(*model());
}
