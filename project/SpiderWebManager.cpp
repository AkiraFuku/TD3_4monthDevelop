#include "SpiderWebManager.h"
#include <cassert>

void SpiderWebManager::Initialize(int maxWebs, Camera* camera) {
    assert(camera);
    camera_ = camera;
    maxWebs_ = maxWebs;

    webs_.reserve(maxWebs_);

    renderer_ = std::make_unique<SpiderWebRenderer>();
    renderer_->Initialize(maxWebs_);
}

void SpiderWebManager::Update() {
    // データリストをそのままRendererに渡し、頂点を構築させる
    renderer_->Update(webs_, camera_);
}

void SpiderWebManager::Draw() {
    renderer_->Draw();
}

void SpiderWebManager::AddWeb(const Vector3& pos, float scale) {
    if (webs_.size() >= maxWebs_) return;
    webs_.push_back({pos, scale});
}

void SpiderWebManager::ClearWebs() {
    webs_.clear();
}